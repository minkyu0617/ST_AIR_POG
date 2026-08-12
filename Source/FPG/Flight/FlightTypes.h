// FPG — 비행 이동의 자료형.
//
// docs/16 §16.5 · docs/17 §17.11 참조.
//
// 🔴 이 헤더의 모든 구조체는 SimulateMove()의 입출력입니다.
//    필드를 추가할 때는 "이 값이 없으면 재실행(Replay) 결과가 달라지는가"를
//    먼저 물으십시오. 달라진다면 FFPGMoveState에 넣어야 합니다.

#pragma once

#include "CoreMinimal.h"
#include "FlightTypes.generated.h"

/** docs/17 §17.2 */
UENUM(BlueprintType)
enum class EFlightState : uint8
{
	Normal,
	Vector,
	Stall,
	Destroyed
};

/**
 * 입력 비트 플래그.
 *
 * 60Hz로 전송되므로 bool 7개 대신 1바이트로 압축합니다 (docs/17 §17.11).
 * 키 배치는 docs/03 §3.1 — D-02(Q) · D-03(대칭 롤) · D-04(자동 스로틀) 반영.
 */
enum class EFPGInputFlags : uint8
{
	None       = 0,
	Thrust     = 1 << 0,  // W
	Brake      = 1 << 1,  // S
	RollLeft   = 1 << 2,  // A
	RollRight  = 1 << 3,  // D
	Vector     = 1 << 4,  // Space (홀드) — 벡터 기동 모디파이어
	Boost      = 1 << 5,  // Ctrl
	Fire       = 1 << 6,  // LMB
};
ENUM_CLASS_FLAGS(EFPGInputFlags)

/**
 * 클라이언트 → 서버로 보내는 한 프레임의 입력.
 *
 * 🔴 위치를 넣지 않습니다. 클라이언트가 위치를 보내면 텔레포트핵이 가능해집니다.
 *    입력만 보내고 위치는 서버가 계산합니다. (docs/17 §17.11)
 */
USTRUCT()
struct FFPGMove
{
	GENERATED_BODY()

	/** 클라이언트 기준 시각. 서버가 "어디까지 처리했는지" 회신할 때의 키. */
	UPROPERTY()
	float ClientTimestamp = 0.f;

	/**
	 * 이 입력이 적용된 시간 폭.
	 *
	 * docs/16의 시그니처는 SimulateMove(Move, DeltaTime)이라 인자로도 받지만,
	 * M3의 재실행에서는 PendingMoves가 각자의 DeltaTime을 기억하고 있어야
	 * 같은 결과가 나옵니다. 그래서 Move 자체에 실어 둡니다.
	 */
	UPROPERTY()
	float DeltaTime = 0.f;

	/** EFPGInputFlags 비트 조합. */
	UPROPERTY()
	uint8 InputFlags = 0;

	/** 마우스 델타 (조준·카메라). 이동 시뮬레이션에는 쓰지 않습니다. */
	UPROPERTY()
	FVector2D AimDelta = FVector2D::ZeroVector;

	UPROPERTY()
	uint8 SelectedSlot = 0;

	bool Has(EFPGInputFlags Flag) const
	{
		return (InputFlags & static_cast<uint8>(Flag)) != 0;
	}

	void Set(EFPGInputFlags Flag, bool bValue)
	{
		if (bValue) { InputFlags |= static_cast<uint8>(Flag); }
		else        { InputFlags &= ~static_cast<uint8>(Flag); }
	}
};

/**
 * 시뮬레이션의 전체 상태. 서버가 권위를 갖고 클라이언트로 회신합니다.
 *
 * 🔴 **여기에 없는 값을 시뮬레이션이 기억하면 안 됩니다.**
 *    화해 후 PendingMoves를 재실행할 때 상태를 이 구조체로만 되돌리므로,
 *    바깥에 남은 값이 있으면 재실행 결과가 원본과 달라집니다.
 *
 * docs/17 §17.11의 원안에서 StallSeconds·BoostCooldownRemaining이 빠져 있었습니다.
 * 둘 다 프레임을 넘어 누적되는 값이라 없으면 결정론이 깨집니다. (2026-08-12 추가)
 */
USTRUCT()
struct FFPGMoveState
{
	GENERATED_BODY()

	/** 이 상태가 반영한 마지막 Move의 ClientTimestamp. */
	UPROPERTY()
	float Timestamp = 0.f;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FQuat Rotation = FQuat::Identity;

	UPROPERTY()
	FVector Velocity = FVector::ZeroVector;

	/** 조종사가 명령한 출력 0~1. MinSpeed~MaxSpeed 구간에 대응합니다. */
	UPROPERTY()
	float Throttle = 0.f;

	/** 현재 부스트 분사에 남은 시간(초). 0이면 분사 중이 아님. */
	UPROPERTY()
	float BoostRemaining = 0.f;

	/** 다음 부스트까지 남은 쿨다운(초). ★결정론에 필요해 추가. */
	UPROPERTY()
	float BoostCooldownRemaining = 0.f;

	/** 스톨 임계 속도 미만으로 머문 누적 시간(초). ★결정론에 필요해 추가. */
	UPROPERTY()
	float StallSeconds = 0.f;

	UPROPERTY()
	EFlightState State = EFlightState::Normal;
};

/**
 * 시뮬레이션이 쓰는 모든 수치.
 *
 * 🔴 P5 — 밸런스 수치는 코드에 하드코딩하지 않습니다.
 *    기체별 값은 DT_Aircraft.csv, 기체 공통 값은 DT_Flight.csv에서 옵니다.
 *    아래 기본값은 **DT_Falcon 기준의 안전망**일 뿐이며, 실제 플레이에서는
 *    반드시 데이터로 덮어써야 합니다. → docs/17
 */
USTRUCT()
struct FFPGFlightParams
{
	GENERATED_BODY()

	// ── DT_Aircraft (기체별) ─────────────────────────────────
	UPROPERTY() float MinSpeed = 70.f;
	UPROPERTY() float MaxSpeed = 260.f;
	UPROPERTY() float BoostSpeed = 380.f;
	UPROPERTY() float RollRate = 140.f;          // °/s
	UPROPERTY() float PitchRate = 50.f;          // °/s
	UPROPERTY() float YawRate = 65.f;            // °/s
	UPROPERTY() float BoostDuration = 3.f;
	UPROPERTY() float BoostCooldown = 8.f;

	// ── DT_Flight (기체 공통) ────────────────────────────────
	UPROPERTY() float CruiseSpeed = 180.f;
	UPROPERTY() float AccelThrust = 55.f;        // u/s²
	UPROPERTY() float DecelBrake = 75.f;         // u/s²
	UPROPERTY() float ThrottleRate = 0.55f;      // 1/s
	UPROPERTY() float RollMaxAngle = 75.f;       // °
	UPROPERTY() float BankTurnRateAt30 = 12.f;   // °/s
	UPROPERTY() float BankTurnRateAt75 = 34.f;   // °/s
	UPROPERTY() float AttitudeInertiaSec = 0.3f;
	UPROPERTY() float SpeedLossRollPct = 3.f;    // %/s
	UPROPERTY() float SpeedLossVectorPct = 12.f; // %/s
	UPROPERTY() float SpeedLossMaxPct = 18.f;    // %/s
	UPROPERTY() float DiveAccelBonus = 25.f;     // u/s² @ 기준 피치
	UPROPERTY() float DiveAccelRefPitch = 45.f;  // °
	UPROPERTY() float StallSpeed = 70.f;
	UPROPERTY() float StallEnterSec = 1.5f;
	UPROPERTY() float StallRecoverSpeed = 100.f;
	UPROPERTY() float StallInputScale = 0.4f;
	UPROPERTY() float StallNoseDownRate = 25.f;  // °/s
	UPROPERTY() float AltitudeCeilingM = 12000.f;
	UPROPERTY() float AltitudeCeilingPowerMult = 0.6f;
};
