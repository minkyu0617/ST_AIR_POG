// FPG — SimulateMove() 결정론 테스트.
//
// docs/16 §16.14: "SimulateMove() 결정론 테스트가 가장 중요합니다.
//  같은 입력 시퀀스를 두 번 돌려 결과가 다르면 예측·화해가 반드시 깨지며,
//  이 테스트 없이는 원인을 찾는 데 며칠이 걸립니다."
//
// 실행: UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests FPG.Flight" -unattended -nullrhi

#include "Misc/AutomationTest.h"
#include "Flight/FlightMovementComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace FPGFlightTest
{
	constexpr float FixedDt = 1.f / 60.f;

	FFPGMove MakeMove(int32 Index, uint8 Flags)
	{
		FFPGMove Move;
		Move.ClientTimestamp = Index * FixedDt;
		Move.DeltaTime = FixedDt;
		Move.InputFlags = Flags;
		return Move;
	}

	/**
	 * 결정론적으로 변화하는 입력 시퀀스.
	 *
	 * 난수를 쓰지 않습니다. 테스트 자체가 재현 가능해야 하고,
	 * 실패했을 때 같은 시퀀스로 다시 돌릴 수 있어야 합니다.
	 * 가속·제동·롤·벡터기동·부스트가 모두 한 번씩 관여하도록 짰습니다.
	 */
	TArray<FFPGMove> BuildSequence(int32 Count)
	{
		using EF = EFPGInputFlags;
		TArray<FFPGMove> Moves;
		Moves.Reserve(Count);

		for (int32 i = 0; i < Count; ++i)
		{
			uint8 Flags = 0;
			const int32 Phase = (i / 30) % 6;
			switch (Phase)
			{
			case 0: Flags = static_cast<uint8>(EF::Thrust); break;
			case 1: Flags = static_cast<uint8>(EF::Thrust) | static_cast<uint8>(EF::RollRight); break;
			case 2: Flags = static_cast<uint8>(EF::Vector) | static_cast<uint8>(EF::Thrust); break;
			case 3: Flags = static_cast<uint8>(EF::Vector) | static_cast<uint8>(EF::RollLeft); break;
			case 4: Flags = static_cast<uint8>(EF::Boost) | static_cast<uint8>(EF::Thrust); break;
			case 5: Flags = static_cast<uint8>(EF::Brake); break;
			}
			Moves.Add(MakeMove(i, Flags));
		}
		return Moves;
	}

	/** 결정론 검사이므로 허용 오차를 두지 않습니다. 비트 단위로 같아야 합니다. */
	bool ExactlyEqual(const FFPGMoveState& A, const FFPGMoveState& B)
	{
		return A.Location == B.Location
			&& A.Rotation == B.Rotation
			&& A.Velocity == B.Velocity
			&& A.Throttle == B.Throttle
			&& A.BoostRemaining == B.BoostRemaining
			&& A.BoostCooldownRemaining == B.BoostCooldownRemaining
			&& A.StallSeconds == B.StallSeconds
			&& A.State == B.State;
	}

	FFPGMoveState RunAll(const TArray<FFPGMove>& Moves, const FFPGFlightParams& P, FFPGMoveState S)
	{
		for (const FFPGMove& M : Moves)
		{
			S = UFlightMovementComponent::Step(S, P, M, M.DeltaTime);
		}
		return S;
	}

	FFPGMoveState MakeStartState(const FFPGFlightParams& P)
	{
		FFPGMoveState S;
		S.Rotation = FQuat::Identity;
		S.Throttle = (P.CruiseSpeed - P.MinSpeed) / (P.MaxSpeed - P.MinSpeed);
		S.Velocity = FVector::ForwardVector * P.CruiseSpeed;
		return S;
	}
}

// ─────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPGFlightDeterminismTest,
	"FPG.Flight.SimulateMove.Determinism",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFPGFlightDeterminismTest::RunTest(const FString&)
{
	using namespace FPGFlightTest;

	const FFPGFlightParams P;
	const TArray<FFPGMove> Moves = BuildSequence(360);   // 6초

	const FFPGMoveState A = RunAll(Moves, P, MakeStartState(P));
	const FFPGMoveState B = RunAll(Moves, P, MakeStartState(P));

	TestTrue(TEXT("같은 입력 시퀀스를 두 번 돌리면 결과가 비트 단위로 같아야 합니다"), ExactlyEqual(A, B));

	// 실제로 움직이긴 했는지 — 아무것도 안 하는 함수도 '결정론적'이므로
	TestTrue(TEXT("시뮬레이션이 실제로 기체를 이동시켜야 합니다"), !A.Location.IsNearlyZero());

	return true;
}

// ─────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPGFlightReplayTest,
	"FPG.Flight.SimulateMove.Replay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFPGFlightReplayTest::RunTest(const FString&)
{
	using namespace FPGFlightTest;

	// M3 화해가 실제로 하는 일을 그대로 재현합니다.
	// 중간 상태로 되돌린 뒤 남은 입력을 재실행했을 때 원본과 같아야 합니다.
	// 여기서 어긋나면 FFPGMoveState에 빠진 필드가 있다는 뜻입니다.
	const FFPGFlightParams P;
	const TArray<FFPGMove> Moves = BuildSequence(360);
	const int32 SplitAt = 137;   // 부스트·벡터기동 구간을 가로지르도록 일부러 어중간하게

	FFPGMoveState S = MakeStartState(P);
	FFPGMoveState Snapshot;
	for (int32 i = 0; i < Moves.Num(); ++i)
	{
		if (i == SplitAt) { Snapshot = S; }
		S = UFlightMovementComponent::Step(S, P, Moves[i], Moves[i].DeltaTime);
	}
	const FFPGMoveState Original = S;

	FFPGMoveState Replayed = Snapshot;
	for (int32 i = SplitAt; i < Moves.Num(); ++i)
	{
		Replayed = UFlightMovementComponent::Step(Replayed, P, Moves[i], Moves[i].DeltaTime);
	}

	TestTrue(
		TEXT("중간 상태로 되돌린 뒤 재실행하면 원본과 같아야 합니다 (FFPGMoveState에 누락 필드 없음)"),
		ExactlyEqual(Original, Replayed));

	return true;
}

// ─────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPGFlightRulesTest,
	"FPG.Flight.SimulateMove.Rules",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFPGFlightRulesTest::RunTest(const FString&)
{
	using namespace FPGFlightTest;
	using EF = EFPGInputFlags;

	const FFPGFlightParams P;

	// D-04 — Space 홀드 중에는 스로틀이 유지되어야 합니다.
	{
		FFPGMoveState S = MakeStartState(P);
		const float Before = S.Throttle;
		for (int32 i = 0; i < 60; ++i)
		{
			// Space + W: W는 피치로 가고 스로틀은 그대로여야 함
			const uint8 Flags = static_cast<uint8>(EF::Vector) | static_cast<uint8>(EF::Thrust);
			S = UFlightMovementComponent::Step(S, P, MakeMove(i, Flags), FixedDt);
		}
		TestEqual(TEXT("D-04: Space 홀드 중 스로틀이 변하지 않아야 합니다"), S.Throttle, Before);
		TestTrue(TEXT("D-04: Space+W는 기수를 올려야 합니다"), S.Rotation.Rotator().Pitch > 1.f);
	}

	// D-03 — A와 D는 좌우 대칭이어야 합니다.
	{
		FFPGMoveState L = MakeStartState(P);
		FFPGMoveState R = MakeStartState(P);
		for (int32 i = 0; i < 60; ++i)
		{
			L = UFlightMovementComponent::Step(L, P, MakeMove(i, static_cast<uint8>(EF::RollLeft)),  FixedDt);
			R = UFlightMovementComponent::Step(R, P, MakeMove(i, static_cast<uint8>(EF::RollRight)), FixedDt);
		}
		const float RollL = L.Rotation.Rotator().Roll;
		const float RollR = R.Rotation.Rotator().Roll;
		TestTrue(TEXT("D-03: A/D의 롤 크기가 같아야 합니다"), FMath::IsNearlyEqual(RollL, -RollR, 0.01f));
		TestTrue(TEXT("D-03: 롤을 유지하면 뱅크턴으로 방향이 바뀌어야 합니다"),
			!FMath::IsNearlyZero(L.Rotation.Rotator().Yaw, 0.1f));
	}

	// 부스트 — 3초 분사 후 쿨다운에 들어가고, 쿨다운 중에는 재발동되지 않아야 합니다.
	{
		FFPGMoveState S = MakeStartState(P);
		const uint8 BoostFlags = static_cast<uint8>(EF::Boost) | static_cast<uint8>(EF::Thrust);
		for (int32 i = 0; i < 300; ++i)   // 5초 — 분사 3초를 넘김
		{
			S = UFlightMovementComponent::Step(S, P, MakeMove(i, BoostFlags), FixedDt);
		}
		TestEqual(TEXT("부스트: 지속시간이 끝나면 분사가 종료되어야 합니다"), S.BoostRemaining, 0.f);
		TestTrue(TEXT("부스트: 종료 후 쿨다운이 걸려야 합니다"), S.BoostCooldownRemaining > 0.f);
	}

	// 스톨 상태 기계 — 임계 미만이 1.5초 지속되면 진입, 100 회복 시 해제.
	//
	// ⚠️ 여기서는 MinSpeed를 낮춘 **전용 파라미터**를 씁니다.
	//    DT_Flight의 실제 수치로는 스톨에 도달할 수 없기 때문입니다.
	//    감속 하한이 70으로 스톨 임계와 같고(docs/02 §2.2), 상승 페널티
	//    최대 -35 u/s²가 추력 +55 u/s²를 이기지 못합니다.
	//    → 밸런스 문제이며 M1 손맛 튜닝 대상입니다. docs/14 D-15 참조.
	//    이 테스트는 **상태 기계가 올바른지**만 봅니다.
	{
		FFPGFlightParams SP = P;
		SP.MinSpeed = 0.f;

		FFPGMoveState S = MakeStartState(SP);
		S.Velocity = FVector::ForwardVector * 10.f;
		S.Throttle = 0.f;

		for (int32 i = 0; i < 120; ++i)   // 2초 — 진입 조건 1.5초를 넘김
		{
			S = UFlightMovementComponent::Step(S, SP, MakeMove(i, 0), FixedDt);
		}
		TestTrue(TEXT("스톨: 저속이 1.5초 지속되면 진입해야 합니다"), S.State == EFlightState::Stall);

		// 회복: 속도가 StallRecoverSpeed를 넘으면 즉시 해제되어야 합니다.
		//
		// Throttle을 1로 함께 맞춰 목표 속도를 최고 속도로 둡니다. 그러지 않으면
		// (스톨 루프 동안 입력이 없어 Throttle이 0에 가까운 채로 남아 있어)
		// 이 한 프레임에서 감속(DecelBrake)이 걸려, 직접 세팅한 속도가 같은
		// 프레임 안에 회복 임계선 아래로 다시 떨어질 수 있습니다 — 시뮬레이션
		// 결함이 아니라 "속도만 세팅하고 조종사는 손을 놓고 있다"는 시나리오의
		// 산물이라 이 테스트의 의도(회복 판정 자체)와 무관합니다.
		S.Velocity = FVector::ForwardVector * (SP.StallRecoverSpeed + 20.f);
		S.Throttle = 1.f;
		S = UFlightMovementComponent::Step(S, SP, MakeMove(999, static_cast<uint8>(EF::Thrust)), FixedDt);
		TestTrue(TEXT("스톨: 속도를 회복하면 즉시 해제되어야 합니다"), S.State != EFlightState::Stall);
	}

	// 감속만으로는 스톨에 들어갈 수 없어야 합니다. (docs/02 §2.2 "최소 70에서 하한 클램프")
	// 이건 결함이 아니라 의도된 설계입니다 — 캐주얼 타깃이 브레이크만 밟다가
	// 스톨에 빠지면 좌절합니다.
	{
		FFPGMoveState S = MakeStartState(P);
		for (int32 i = 0; i < 600; ++i)   // 10초 내내 감속
		{
			S = UFlightMovementComponent::Step(S, P, MakeMove(i, static_cast<uint8>(EF::Brake)), FixedDt);
		}
		TestTrue(TEXT("감속만으로는 스톨에 빠지지 않아야 합니다"), S.State != EFlightState::Stall);
		TestTrue(TEXT("감속 하한은 MinSpeed 이상이어야 합니다"), S.Velocity.Size() >= P.MinSpeed - 1.f);
	}

	// 격추 상태는 시뮬레이션하지 않습니다.
	{
		FFPGMoveState S = MakeStartState(P);
		S.State = EFlightState::Destroyed;
		const FVector Before = S.Location;
		S = UFlightMovementComponent::Step(S, P, MakeMove(0, static_cast<uint8>(EF::Thrust)), FixedDt);
		TestEqual(TEXT("격추된 기체는 이동하지 않아야 합니다"), S.Location, Before);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
