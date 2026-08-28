#include "FlightMovementComponent.h"

namespace
{
	/**
	 * 롤 각도에서 뱅크턴 선회율을 구합니다. (docs/03 C-1 · D-03)
	 *
	 * 문서가 준 앵커는 두 개뿐입니다. 롤 30° → 12°/s, 롤 75° → 34°/s.
	 * 롤 0°에서 선회율 0을 더해 세 점을 지나는 구간 선형으로 잇습니다.
	 * 이렇게 하면 "A/D로도 결국 방향이 바뀐다"는 원안 의도가
	 * 물리적으로 자연스럽게 충족됩니다.
	 */
	float BankTurnRate(float AbsRollDeg, const FFPGFlightParams& P)
	{
		if (AbsRollDeg <= 30.f)
		{
			return FMath::GetMappedRangeValueClamped(
				FVector2f(0.f, 30.f),
				FVector2f(0.f, P.BankTurnRateAt30),
				AbsRollDeg);
		}
		return FMath::GetMappedRangeValueClamped(
			FVector2f(30.f, FMath::Max(P.RollMaxAngle, 31.f)),
			FVector2f(P.BankTurnRateAt30, P.BankTurnRateAt75),
			AbsRollDeg);
	}

	/** 목표 각도로 다가가되(관성) 프레임당 변화량을 최대 각속도로 제한합니다. */
	float ApproachAngle(float Current, float Target, float MaxRateDegPerSec, float InertiaSec, float Dt)
	{
		const float Smoothed = FMath::FInterpTo(Current, Target, Dt, 1.f / FMath::Max(InertiaSec, KINDA_SMALL_NUMBER));
		const float MaxDelta = MaxRateDegPerSec * Dt;
		return Current + FMath::Clamp(Smoothed - Current, -MaxDelta, MaxDelta);
	}
}

// ─────────────────────────────────────────────────────────────
//  🔴 순수 함수. 멤버 접근 없음(static). 난수·시각 조회 없음.
// ─────────────────────────────────────────────────────────────
FFPGMoveState UFlightMovementComponent::Step(
	const FFPGMoveState& In,
	const FFPGFlightParams& P,
	const FFPGMove& Move,
	float DeltaTime)
{
	FFPGMoveState Out = In;
	Out.Timestamp = Move.ClientTimestamp;

	if (DeltaTime <= 0.f)
	{
		return Out;
	}

	// 격추된 기체는 시뮬레이션하지 않습니다. 연출은 호출자가 담당합니다.
	if (In.State == EFlightState::Destroyed)
	{
		return Out;
	}

	const bool bStalled = (In.State == EFlightState::Stall);

	// 스톨 중에도 조작을 완전히 막지는 않습니다. 감도만 40%로 낮춥니다.
	// "완전한 조작 불능 구간은 만들지 않는다" (docs/02 §2.2 원칙1)
	const float InputScale = bStalled ? P.StallInputScale : 1.f;

	const bool bVectorMode = Move.Has(EFPGInputFlags::Vector);
	const float RollAxis   = (Move.Has(EFPGInputFlags::RollRight) ? 1.f : 0.f)
	                       - (Move.Has(EFPGInputFlags::RollLeft)  ? 1.f : 0.f);

	// docs/03 §3.1 — Space+W = 기수 상승, Space+S = 기수 하강.
	// 조종간을 당기면 올라가는 시뮬 관습(뒤로=위)이 아니라, W가 곧 위입니다.
	// 캐주얼 타깃에게는 이쪽이 직관적이고 원안 표기도 그렇습니다.
	const float PitchAxis  = (Move.Has(EFPGInputFlags::Thrust) ? 1.f : 0.f)
	                       - (Move.Has(EFPGInputFlags::Brake)  ? 1.f : 0.f);

	// ── 1. 부스트 상태 기계 ───────────────────────────────────
	// 3초 분사 → 8초 쿨다운. 스톨 중에는 발동하지 않습니다. (docs/03 §3.3 규칙3)
	Out.BoostCooldownRemaining = FMath::Max(0.f, In.BoostCooldownRemaining - DeltaTime);

	const bool bBoostHeld = Move.Has(EFPGInputFlags::Boost) && !bStalled;
	bool bBoosting = false;

	if (In.BoostRemaining > 0.f)
	{
		if (bBoostHeld)
		{
			Out.BoostRemaining = In.BoostRemaining - DeltaTime;
			bBoosting = true;
		}
		else
		{
			Out.BoostRemaining = 0.f;   // 손 떼면 즉시 종료
		}

		if (Out.BoostRemaining <= 0.f)
		{
			Out.BoostRemaining = 0.f;
			Out.BoostCooldownRemaining = P.BoostCooldown;
		}
	}
	else if (bBoostHeld && Out.BoostCooldownRemaining <= 0.f)
	{
		Out.BoostRemaining = P.BoostDuration;
		bBoosting = true;
	}

	// ── 2. 스로틀 ────────────────────────────────────────────
	// D-04 자동 스로틀: Space 홀드 중에는 W/S가 피치로 가므로 스로틀은
	// 건드리지 않고 **직전 값을 그대로 유지**합니다.
	// 진입 시점 값을 래치하는 방식이 아니라, 입력이 없을 때 감쇠시키지 않는
	// 방식입니다. 래치로 만들면 홀드 중 W를 눌렀다 뗄 때 값이 튑니다.
	if (!bVectorMode)
	{
		const float ThrottleAxis = (Move.Has(EFPGInputFlags::Thrust) ? 1.f : 0.f)
		                         - (Move.Has(EFPGInputFlags::Brake)  ? 1.f : 0.f);
		Out.Throttle = FMath::Clamp(In.Throttle + ThrottleAxis * P.ThrottleRate * DeltaTime, 0.f, 1.f);
	}

	// ── 3. 자세 ──────────────────────────────────────────────
	FRotator Attitude = In.Rotation.Rotator();

	if (bVectorMode)
	{
		// 벡터 기동: 롤 없이 기수를 즉각 꺾습니다. 롤은 수평으로 복귀.
		Attitude.Pitch += PitchAxis * P.PitchRate * InputScale * DeltaTime;
		Attitude.Yaw   += RollAxis  * P.YawRate   * InputScale * DeltaTime;
		Attitude.Roll   = ApproachAngle(Attitude.Roll, 0.f, P.RollRate, P.AttitudeInertiaSec, DeltaTime);
	}
	else
	{
		// D-03: A/D는 좌우 대칭으로 롤만 담당하고, 선회는 뱅크턴으로 자연 발생.
		const float TargetRoll = RollAxis * P.RollMaxAngle * InputScale;
		Attitude.Roll = ApproachAngle(Attitude.Roll, TargetRoll, P.RollRate, P.AttitudeInertiaSec, DeltaTime);
		Attitude.Yaw += FMath::Sign(Attitude.Roll) * BankTurnRate(FMath::Abs(Attitude.Roll), P) * DeltaTime;
	}

	// 스톨: 기수가 자동으로 내려가 속도를 되찾게 합니다.
	if (bStalled)
	{
		Attitude.Pitch -= P.StallNoseDownRate * DeltaTime;
	}

	Attitude.Pitch = FMath::Clamp(Attitude.Pitch, -89.f, 89.f);  // 짐벌락 회피
	Attitude.Roll  = FMath::Clamp(Attitude.Roll, -P.RollMaxAngle, P.RollMaxAngle);
	Attitude.Yaw   = FRotator::NormalizeAxis(Attitude.Yaw);
	Out.Rotation   = Attitude.Quaternion();

	// ── 4. 속도 ──────────────────────────────────────────────
	float Speed = In.Velocity.Size();

	// 고도 상한을 넘으면 출력이 떨어집니다. (docs/02 §2.2)
	const float AltitudeM = Out.Location.Z / 100.f;   // 1 UU = 1 cm
	const float PowerMult = (AltitudeM > P.AltitudeCeilingM) ? P.AltitudeCeilingPowerMult : 1.f;

	const float TargetSpeed = bBoosting
		? P.BoostSpeed * PowerMult
		: FMath::Lerp(P.MinSpeed, P.MaxSpeed, Out.Throttle) * PowerMult;

	const float AccelRate = (Speed < TargetSpeed) ? P.AccelThrust : P.DecelBrake;
	Speed = FMath::FInterpConstantTo(Speed, TargetSpeed, DeltaTime, AccelRate);

	// 강하하면 속도를 얻고 상승하면 잃습니다. "속도가 곧 자원" (docs/02 §2.2 원칙3)
	// 문서는 강하 보너스(+25 u/s² @ -45°)만 명시하지만, 원칙3이 교환을 전제하므로
	// 상승에도 대칭으로 적용합니다.
	const float RefSin = FMath::Sin(FMath::DegreesToRadians(FMath::Max(P.DiveAccelRefPitch, 1.f)));
	Speed += -FMath::Sin(FMath::DegreesToRadians(Attitude.Pitch)) / RefSin * P.DiveAccelBonus * DeltaTime;

	// 급기동하면 속도를 잃습니다. 선회각에 비례하고 상한이 있습니다.
	float LossPct = 0.f;
	if (!FMath::IsNearlyZero(RollAxis))
	{
		LossPct += bVectorMode ? P.SpeedLossVectorPct : P.SpeedLossRollPct;
	}
	if (bVectorMode && !FMath::IsNearlyZero(PitchAxis))
	{
		LossPct += P.SpeedLossVectorPct;
	}
	LossPct = FMath::Min(LossPct, P.SpeedLossMaxPct);
	Speed *= FMath::Max(0.f, 1.f - LossPct * 0.01f * DeltaTime);

	Speed = FMath::Clamp(Speed, 0.f, P.BoostSpeed);

	// ── 5. 스톨 판정 ─────────────────────────────────────────
	// 속도 < 70이 1.5초 지속되면 진입, 100 회복 시 즉시 해제. (docs/02 §2.2)
	if (Speed < P.StallSpeed)
	{
		Out.StallSeconds = In.StallSeconds + DeltaTime;
	}
	else
	{
		Out.StallSeconds = 0.f;
	}

	if (bStalled)
	{
		Out.State = (Speed > P.StallRecoverSpeed) ? EFlightState::Normal : EFlightState::Stall;
	}
	else if (Out.StallSeconds >= P.StallEnterSec)
	{
		Out.State = EFlightState::Stall;
	}
	else
	{
		Out.State = bVectorMode ? EFlightState::Vector : EFlightState::Normal;
	}

	if (Out.State == EFlightState::Normal || Out.State == EFlightState::Vector)
	{
		Out.StallSeconds = (Speed < P.StallSpeed) ? Out.StallSeconds : 0.f;
	}

	// ── 6. 적분 ──────────────────────────────────────────────
	Out.Velocity = Out.Rotation.GetForwardVector() * Speed;
	Out.Location = In.Location + Out.Velocity * DeltaTime;

	return Out;
}

// ─────────────────────────────────────────────────────────────

UFlightMovementComponent::UFlightMovementComponent()
{
	// 이동은 매 프레임 갱신이 필요한 몇 안 되는 컴포넌트입니다. (docs/16 §16.13)
	PrimaryComponentTick.bCanEverTick = false;   // 소유 Pawn이 명시적으로 호출합니다
}

void UFlightMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* Owner = GetOwner())
	{
		ResetToCruise(Owner->GetActorLocation(), Owner->GetActorQuat());
	}
}

FFPGFlightParams UFlightMovementComponent::GetEffectiveParams() const
{
	if (Modifiers.IsIdentity())
	{
		return Params;   // 손상도 버프도 없을 때가 대부분입니다. 곱셈을 건너뜁니다.
	}

	FFPGFlightParams Effective = Params;

	// MinSpeed는 깎지 않습니다. 손상으로 최저 속도까지 내려가면 스톨 임계에
	// 걸려 조작 불능에 가까워지는데, 그건 "추락하지 않는다"는 docs/02 §2.2
	// 원칙1에 정면으로 어긋납니다.
	Effective.MaxSpeed   = Params.MaxSpeed * Modifiers.MaxSpeedMult;
	Effective.BoostSpeed = Params.BoostSpeed * Modifiers.MaxSpeedMult;
	Effective.CruiseSpeed = FMath::Min(Params.CruiseSpeed, Effective.MaxSpeed);

	Effective.RollRate  = Params.RollRate * Modifiers.TurnRateMult;
	Effective.PitchRate = Params.PitchRate * Modifiers.TurnRateMult;
	Effective.YawRate   = Params.YawRate * Modifiers.TurnRateMult;
	Effective.BankTurnRateAt30 = Params.BankTurnRateAt30 * Modifiers.TurnRateMult;
	Effective.BankTurnRateAt75 = Params.BankTurnRateAt75 * Modifiers.TurnRateMult;

	Effective.AccelThrust = Params.AccelThrust * Modifiers.AccelMult;

	// 최대 속도가 최소 속도 아래로 내려가면 V9 전제(Min < Max < Boost)가
	// 깨져 시뮬레이션이 이상해집니다. 배율이 아무리 가혹해도 순서는 지킵니다.
	Effective.MaxSpeed   = FMath::Max(Effective.MaxSpeed, Params.MinSpeed * 1.05f);
	Effective.BoostSpeed = FMath::Max(Effective.BoostSpeed, Effective.MaxSpeed * 1.05f);

	return Effective;
}

void UFlightMovementComponent::SimulateMove(const FFPGMove& Move, float DeltaTime)
{
	// 🔴 배율을 미리 곱해 **완성된 Params**를 넘깁니다.
	//    Step() 안에서 배율을 처리하면 순수 함수의 입력이 하나 늘어나고,
	//    M3 재실행 때 그 값도 함께 되돌려야 하는 부담이 생깁니다.
	State = Step(State, GetEffectiveParams(), Move, DeltaTime);
}

void UFlightMovementComponent::ResetToCruise(const FVector& StartLocation, const FQuat& StartRotation)
{
	State = FFPGMoveState();
	State.Location = StartLocation;
	State.Rotation = StartRotation;

	// 순항 속도(180)에 대응하는 스로틀 값을 역산합니다.
	const float Range = FMath::Max(Params.MaxSpeed - Params.MinSpeed, KINDA_SMALL_NUMBER);
	State.Throttle = FMath::Clamp((Params.CruiseSpeed - Params.MinSpeed) / Range, 0.f, 1.f);
	State.Velocity = StartRotation.GetForwardVector() * Params.CruiseSpeed;
}
