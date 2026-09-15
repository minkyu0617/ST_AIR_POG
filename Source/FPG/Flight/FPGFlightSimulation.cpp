#include "Flight/FPGFlightSimulation.h"

namespace
{
	constexpr float MaxStepDelta = 0.1f;

	float ApplyThrottle(const FFPGMoveState& In, const FFPGMove& Move, const FFPGFlightConfig& Config,
		float Dt, float InputScale, bool bBoosting, float PitchDegrees)
	{
		float Speed = In.Speed;

		// Ceiling of the speed band for this tick.
		const float TopSpeed = bBoosting ? Config.BoostSpeed : Config.MaxSpeed;

		// D-04 option A: holding Space keeps the previous throttle, so the player can
		// climb or turn without losing the speed they built up. W/S are ignored while
		// VectorMod is held because those keys mean "pitch" and "yaw" in that mode.
		const bool bVector = Move.Has(EFPGInputFlag::VectorMod);
		if (!bVector)
		{
			if (Move.Has(EFPGInputFlag::Accelerate))
			{
				Speed += Config.AccelRate * InputScale * Dt;
			}
			if (Move.Has(EFPGInputFlag::Decelerate))
			{
				Speed -= Config.DecelRate * InputScale * Dt;
				// Throttling down never drops below MinSpeed - there is no reverse.
				Speed = FMath::Max(Speed, Config.MinSpeed);
			}
		}

		if (bBoosting)
		{
			// Boost pulls toward BoostSpeed rather than snapping, so it reads as thrust not teleport.
			Speed = FMath::FInterpConstantTo(Speed, Config.BoostSpeed, Dt, Config.AccelRate * 2.f);
		}

		// Diving trades altitude for speed; climbing does the reverse. This is the whole
		// energy game and the only reason a stall can ever happen.
		const float PitchRadians = FMath::DegreesToRadians(PitchDegrees);
		Speed += Config.DiveAccelBonus * -FMath::Sin(PitchRadians) * Dt;

		// Manoeuvring costs energy.
		const float RollFraction = Config.MaxRollAngle > KINDA_SMALL_NUMBER
			? FMath::Abs(In.Rotation.Roll) / Config.MaxRollAngle
			: 0.f;
		Speed -= Speed * Config.BankSpeedLossRate * FMath::Clamp(RollFraction, 0.f, 1.f) * Dt;

		const uint8 ManoeuvreBits = static_cast<uint8>(EFPGInputFlag::Accelerate | EFPGInputFlag::Decelerate
			| EFPGInputFlag::RollLeft | EFPGInputFlag::RollRight);
		if (bVector && (Move.Buttons & ManoeuvreBits) != 0)
		{
			Speed -= Speed * Config.VectorSpeedLossRate * Dt;
		}

		const float AbsoluteFloor = Config.MinSpeed * Config.AbsoluteSpeedFloorRatio;
		return FMath::Clamp(Speed, AbsoluteFloor, TopSpeed);
	}

	FRotator ApplyRotation(const FFPGMoveState& In, const FFPGMove& Move, const FFPGFlightConfig& Config,
		float Dt, float InputScale, bool bStalled)
	{
		FRotator Rotation = In.Rotation;
		const bool bVector = Move.Has(EFPGInputFlag::VectorMod);

		if (bVector)
		{
			// Space + WASD: direct vector manoeuvring. Sharp, immediate, expensive in speed.
			if (Move.Has(EFPGInputFlag::Accelerate)) { Rotation.Pitch += Config.PitchRate * InputScale * Dt; }
			if (Move.Has(EFPGInputFlag::Decelerate)) { Rotation.Pitch -= Config.PitchRate * InputScale * Dt; }
			if (Move.Has(EFPGInputFlag::RollLeft))   { Rotation.Yaw   -= Config.YawRate   * InputScale * Dt; }
			if (Move.Has(EFPGInputFlag::RollRight))  { Rotation.Yaw   += Config.YawRate   * InputScale * Dt; }
		}
		else
		{
			// D-03 option A: A/D are symmetric and only roll. Holding a roll produces a
			// bank turn below, so "A/D also change direction" is satisfied by physics
			// rather than by a special case on one key.
			const bool bLeft = Move.Has(EFPGInputFlag::RollLeft);
			const bool bRight = Move.Has(EFPGInputFlag::RollRight);

			if (bLeft && !bRight)  { Rotation.Roll -= Config.RollRate * InputScale * Dt; }
			if (bRight && !bLeft)  { Rotation.Roll += Config.RollRate * InputScale * Dt; }
			if (bLeft == bRight)
			{
				// Hands off: the airframe self-centres.
				Rotation.Roll = FMath::FInterpConstantTo(Rotation.Roll, 0.f, Dt, Config.RollAutoLevelRate);
			}
		}

		Rotation.Roll = FMath::Clamp(Rotation.Roll, -Config.MaxRollAngle, Config.MaxRollAngle);

		// Bank turn: yaw follows the roll angle, proportionally.
		if (Config.MaxRollAngle > KINDA_SMALL_NUMBER)
		{
			const float RollFraction = Rotation.Roll / Config.MaxRollAngle;
			Rotation.Yaw += RollFraction * Config.BankTurnRateAtMaxRoll * Dt;
		}

		if (bStalled)
		{
			// Stall drops the nose. Control is degraded, never removed.
			Rotation.Pitch -= Config.StallNoseDownRate * Dt;
		}

		Rotation.Pitch = FMath::Clamp(Rotation.Pitch, -Config.MaxPitchAngle, Config.MaxPitchAngle);
		Rotation.Yaw = FRotator::NormalizeAxis(Rotation.Yaw);
		return Rotation;
	}
}

FFPGFlightConfig FFPGFlightConfig::FromRow(const FFPGAircraftRow& Row)
{
	FFPGFlightConfig Config;
	Config.MaxSpeed = Row.MaxSpeed;
	Config.MinSpeed = Row.MinSpeed;
	Config.BoostSpeed = Row.BoostSpeed;
	Config.AccelRate = Row.AccelRate;
	Config.DecelRate = Row.DecelRate;
	Config.RollRate = Row.RollRate;
	Config.PitchRate = Row.PitchRate;
	Config.YawRate = Row.YawRate;
	Config.BoostDuration = Row.BoostDuration;
	Config.BoostCooldown = Row.BoostCooldown;
	Config.StallRecoverSpeed = FMath::Max(Row.MinSpeed * 1.4f, Row.MinSpeed + 1.f);
	return Config;
}

namespace FPGFlightSimulation
{
	float GetMaxStepDeltaTime()
	{
		return MaxStepDelta;
	}

	FFPGMoveState Step(const FFPGMoveState& In, const FFPGMove& Move, const FFPGFlightConfig& Config)
	{
		FFPGMoveState Out = In;

		const float Dt = FMath::Clamp(Move.DeltaTime, 0.f, MaxStepDelta);
		if (Dt <= 0.f)
		{
			return Out;
		}

		Out.Timestamp = Move.ClientTimestamp;

		if (In.State == EFPGFlightState::Destroyed)
		{
			// Destroyed craft keep drifting so the kill cam has something to follow.
			Out.Location = In.Location + In.Rotation.Vector() * In.Speed * Dt;
			return Out;
		}

		const bool bStalled = (In.State == EFPGFlightState::Stall);
		const float InputScale = bStalled ? Config.StallInputScale : 1.f;

		// --- boost timers -------------------------------------------------------
		Out.BoostCooldownRemaining = FMath::Max(0.f, In.BoostCooldownRemaining - Dt);

		const bool bWantsBoost = Move.Has(EFPGInputFlag::Boost);
		const bool bCanBoost = !bStalled && Out.BoostCooldownRemaining <= 0.f && In.BoostRemaining > 0.f;
		const bool bBoosting = bWantsBoost && bCanBoost;

		if (bBoosting)
		{
			Out.BoostRemaining = FMath::Max(0.f, In.BoostRemaining - Dt);
			if (Out.BoostRemaining <= 0.f)
			{
				Out.BoostCooldownRemaining = Config.BoostCooldown;
			}
		}
		else if (Out.BoostCooldownRemaining <= 0.f)
		{
			// Recharge only once the cooldown has fully elapsed.
			Out.BoostRemaining = FMath::Min(Config.BoostDuration, In.BoostRemaining + Dt);
		}

		// --- attitude -----------------------------------------------------------
		Out.Rotation = ApplyRotation(In, Move, Config, Dt, InputScale, bStalled);

		// --- speed --------------------------------------------------------------
		Out.Speed = ApplyThrottle(In, Move, Config, Dt, InputScale, bBoosting, Out.Rotation.Pitch);

		// Thin air above the service ceiling.
		if (In.Location.Z > Config.ThrustFalloffAltitude)
		{
			const float Range = FMath::Max(1.f, Config.CeilingAltitude - Config.ThrustFalloffAltitude);
			const float Over = FMath::Clamp((In.Location.Z - Config.ThrustFalloffAltitude) / Range, 0.f, 1.f);
			const float Scale = FMath::Lerp(1.f, Config.HighAltitudeThrustScale, Over);
			Out.Speed = FMath::Min(Out.Speed, Config.MaxSpeed * Scale);
		}

		// --- stall --------------------------------------------------------------
		if (Out.Speed < Config.MinSpeed)
		{
			Out.StallTimer = In.StallTimer + Dt;
			if (Out.StallTimer >= Config.StallEnterSeconds)
			{
				Out.State = EFPGFlightState::Stall;
			}
		}
		else
		{
			Out.StallTimer = 0.f;
		}

		if (bStalled && Out.Speed >= Config.StallRecoverSpeed)
		{
			Out.State = EFPGFlightState::Normal;
			Out.StallTimer = 0.f;
		}

		if (Out.State != EFPGFlightState::Stall)
		{
			Out.State = Move.Has(EFPGInputFlag::VectorMod) ? EFPGFlightState::Vector : EFPGFlightState::Normal;
		}

		// --- integrate ----------------------------------------------------------
		Out.Location = In.Location + Out.Rotation.Vector() * Out.Speed * Dt;

		// Hard ceiling. The floor is terrain, handled by the caller's sweep.
		if (Out.Location.Z > Config.CeilingAltitude)
		{
			Out.Location.Z = Config.CeilingAltitude;
		}

		return Out;
	}
}
