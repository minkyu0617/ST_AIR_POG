// Wire + simulation types for flight (docs/16 §16.5, docs/17 §17.11).
#pragma once

#include "CoreMinimal.h"
#include "Core/Types/FPGTypes.h"
#include "Core/Types/FPGDataRows.h"
#include "FPGFlightTypes.generated.h"

/**
 * Button bitmask. Packed so the 60 Hz input stream stays small (docs/17 §17.11).
 * The client sends buttons only - never a position - so speed and teleport hacks have nothing to forge.
 */
namespace EFPGInputFlag
{
	enum Type : uint8
	{
		None       = 0,
		Accelerate = 1 << 0, // W
		Decelerate = 1 << 1, // S
		RollLeft   = 1 << 2, // A
		RollRight  = 1 << 3, // D
		VectorMod  = 1 << 4, // Space - turns W/A/S/D into vector manoeuvring (docs/03 C-1)
		Boost      = 1 << 5, // Ctrl
		Fire       = 1 << 6  // LMB
	};
}

/** One tick of player intent. Client -> server. */
USTRUCT()
struct FPG_API FFPGMove
{
	GENERATED_BODY()

	UPROPERTY() float ClientTimestamp = 0.f;
	UPROPERTY() float DeltaTime = 0.f;
	UPROPERTY() uint8 Buttons = 0;

	bool Has(EFPGInputFlag::Type Flag) const { return (Buttons & static_cast<uint8>(Flag)) != 0; }
};

/** Authoritative flight state. Server -> client. */
USTRUCT()
struct FPG_API FFPGMoveState
{
	GENERATED_BODY()

	UPROPERTY() float Timestamp = 0.f;
	UPROPERTY() FVector Location = FVector::ZeroVector;
	UPROPERTY() FRotator Rotation = FRotator::ZeroRotator;
	/** Scalar speed along the forward axis, uu/s. Arcade model: velocity is always forward * Speed. */
	UPROPERTY() float Speed = 0.f;
	UPROPERTY() float BoostRemaining = 0.f;
	UPROPERTY() float BoostCooldownRemaining = 0.f;
	UPROPERTY() float StallTimer = 0.f;
	UPROPERTY() EFPGFlightState State = EFPGFlightState::Normal;

	FVector GetVelocity() const { return Rotation.Vector() * Speed; }
};

/** Tuning constants. Built from DT_Aircraft so no balance number lives in code (docs/16 A3). */
USTRUCT()
struct FPG_API FFPGFlightConfig
{
	GENERATED_BODY()

	UPROPERTY() float MaxSpeed = 26000.f;
	UPROPERTY() float MinSpeed = 7000.f;
	UPROPERTY() float BoostSpeed = 38000.f;
	UPROPERTY() float AccelRate = 5500.f;
	UPROPERTY() float DecelRate = 7500.f;

	UPROPERTY() float RollRate = 140.f;
	UPROPERTY() float PitchRate = 50.f;
	UPROPERTY() float YawRate = 65.f;

	/** Roll clamp, degrees (docs/02 §2.2). */
	UPROPERTY() float MaxRollAngle = 75.f;
	/** Yaw rate produced by holding full roll - this is what makes A/D steer (docs/03 C-1). */
	UPROPERTY() float BankTurnRateAtMaxRoll = 34.f;
	/** Roll self-centring rate when A/D are released, deg/s. */
	UPROPERTY() float RollAutoLevelRate = 60.f;
	UPROPERTY() float MaxPitchAngle = 85.f;

	UPROPERTY() float BoostDuration = 3.f;
	UPROPERTY() float BoostCooldown = 8.f;

	/** Seconds below MinSpeed before the stall state latches. */
	UPROPERTY() float StallEnterSeconds = 1.5f;
	/** Speed that clears a stall. Above MinSpeed so it cannot flicker. */
	UPROPERTY() float StallRecoverSpeed = 10000.f;
	/** Control authority retained while stalled - never zero, the player is never locked out (docs/02 §2.2). */
	UPROPERTY() float StallInputScale = 0.4f;
	/** Nose-down rate forced while stalled, deg/s. */
	UPROPERTY() float StallNoseDownRate = 35.f;
	/**
	 * Absolute speed floor as a fraction of MinSpeed. Throttle-down clamps at MinSpeed,
	 * but a steep climb can bleed below it - that is the only way to enter a stall.
	 */
	UPROPERTY() float AbsoluteSpeedFloorRatio = 0.35f;

	/** Extra acceleration at a 90 degree dive, uu/s^2. Scaled by sin(-pitch). */
	UPROPERTY() float DiveAccelBonus = 2500.f;
	/** Speed bled per second at full roll while banking. */
	UPROPERTY() float BankSpeedLossRate = 0.03f;
	/** Speed bled per second during vector manoeuvres - the cost of a hard turn. */
	UPROPERTY() float VectorSpeedLossRate = 0.12f;

	/** Above this altitude engine output falls off (docs/02 §2.2). */
	UPROPERTY() float ThrustFalloffAltitude = 900000.f;  // 9,000 m
	UPROPERTY() float CeilingAltitude = 1200000.f;       // 12,000 m
	UPROPERTY() float HighAltitudeThrustScale = 0.6f;

	static FFPGFlightConfig FromRow(const FFPGAircraftRow& Row);
};
