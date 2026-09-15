// The flight sim must be deterministic or client prediction cannot work (docs/16 §16.14).
// Run from the editor: Tools > Session Frontend > Automation > FPG.Flight
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Flight/FPGFlightSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FFPGFlightConfig MakeTestConfig()
	{
		FFPGFlightConfig Config;
		Config.MaxSpeed = 26000.f;
		Config.MinSpeed = 7000.f;
		Config.BoostSpeed = 38000.f;
		Config.AccelRate = 5500.f;
		Config.DecelRate = 7500.f;
		Config.StallRecoverSpeed = 10000.f;
		return Config;
	}

	FFPGMoveState MakeStartState()
	{
		FFPGMoveState State;
		State.Location = FVector(0.f, 0.f, 300000.f);
		State.Rotation = FRotator::ZeroRotator;
		State.Speed = 18000.f;
		State.BoostRemaining = 3.f;
		return State;
	}

	/** Deterministic pseudo input pattern - no RNG, just an index-driven bit pattern. */
	uint8 ButtonsForStep(int32 Index)
	{
		uint32 Buttons = 0;
		if (Index % 3 == 0)  { Buttons |= EFPGInputFlag::Accelerate; }
		if (Index % 7 == 0)  { Buttons |= EFPGInputFlag::Decelerate; }
		if (Index % 4 == 0)  { Buttons |= EFPGInputFlag::RollLeft; }
		if (Index % 5 == 0)  { Buttons |= EFPGInputFlag::RollRight; }
		if (Index % 11 == 0) { Buttons |= EFPGInputFlag::VectorMod; }
		if (Index % 13 == 0) { Buttons |= EFPGInputFlag::Boost; }
		return static_cast<uint8>(Buttons);
	}

	FFPGMoveState RunSequence(int32 Steps)
	{
		const FFPGFlightConfig Config = MakeTestConfig();
		FFPGMoveState State = MakeStartState();

		for (int32 Index = 0; Index < Steps; ++Index)
		{
			FFPGMove Move;
			Move.DeltaTime = 1.f / 60.f;
			Move.ClientTimestamp = Index * Move.DeltaTime;
			Move.Buttons = ButtonsForStep(Index);
			State = FPGFlightSimulation::Step(State, Move, Config);
		}

		return State;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPGFlightDeterminismTest,
	"FPG.Flight.Determinism",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFPGFlightDeterminismTest::RunTest(const FString& /*Parameters*/)
{
	// Same inputs, twice, must land on bit-identical state. If this ever fails,
	// prediction and reconciliation in M3 will desync and be very hard to debug.
	const FFPGMoveState A = RunSequence(600);
	const FFPGMoveState B = RunSequence(600);

	TestEqual(TEXT("Location X"), A.Location.X, B.Location.X);
	TestEqual(TEXT("Location Y"), A.Location.Y, B.Location.Y);
	TestEqual(TEXT("Location Z"), A.Location.Z, B.Location.Z);
	TestEqual(TEXT("Pitch"), A.Rotation.Pitch, B.Rotation.Pitch);
	TestEqual(TEXT("Yaw"), A.Rotation.Yaw, B.Rotation.Yaw);
	TestEqual(TEXT("Roll"), A.Rotation.Roll, B.Rotation.Roll);
	TestEqual(TEXT("Speed"), A.Speed, B.Speed);
	TestEqual(TEXT("BoostRemaining"), A.BoostRemaining, B.BoostRemaining);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPGFlightBoundsTest,
	"FPG.Flight.Bounds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFPGFlightBoundsTest::RunTest(const FString& /*Parameters*/)
{
	const FFPGFlightConfig Config = MakeTestConfig();

	// Holding S must never reverse the aircraft - it clamps at MinSpeed (docs/02 §2.2).
	{
		FFPGMoveState State = MakeStartState();
		for (int32 Index = 0; Index < 600; ++Index)
		{
			FFPGMove Move;
			Move.DeltaTime = 1.f / 60.f;
			Move.Buttons = static_cast<uint8>(EFPGInputFlag::Decelerate);
			State = FPGFlightSimulation::Step(State, Move, Config);
		}
		TestTrue(TEXT("Throttling down never goes below MinSpeed"), State.Speed >= Config.MinSpeed - 1.f);
	}

	// Holding W must saturate at MaxSpeed, not run away.
	{
		FFPGMoveState State = MakeStartState();
		for (int32 Index = 0; Index < 600; ++Index)
		{
			FFPGMove Move;
			Move.DeltaTime = 1.f / 60.f;
			Move.Buttons = static_cast<uint8>(EFPGInputFlag::Accelerate);
			State = FPGFlightSimulation::Step(State, Move, Config);
		}
		TestTrue(TEXT("Throttle is capped at MaxSpeed"), State.Speed <= Config.MaxSpeed + 1.f);
	}

	// Roll is clamped, so A/D can never flip the craft past the bank limit.
	{
		FFPGMoveState State = MakeStartState();
		for (int32 Index = 0; Index < 600; ++Index)
		{
			FFPGMove Move;
			Move.DeltaTime = 1.f / 60.f;
			Move.Buttons = static_cast<uint8>(EFPGInputFlag::RollRight);
			State = FPGFlightSimulation::Step(State, Move, Config);
		}
		TestTrue(TEXT("Roll within limit"), FMath::Abs(State.Rotation.Roll) <= Config.MaxRollAngle + 0.01f);
		TestTrue(TEXT("Bank turn produced yaw"), FMath::Abs(State.Rotation.Yaw) > 1.f);
	}

	// A sustained climb bleeds speed until the stall latches - the only route into a stall.
	{
		FFPGMoveState State = MakeStartState();
		State.Speed = Config.MinSpeed + 200.f;
		bool bStalled = false;
		for (int32 Index = 0; Index < 900; ++Index)
		{
			FFPGMove Move;
			Move.DeltaTime = 1.f / 60.f;
			Move.Buttons = static_cast<uint8>(EFPGInputFlag::VectorMod | EFPGInputFlag::Accelerate); // nose up, throttle held
			State = FPGFlightSimulation::Step(State, Move, Config);
			if (State.State == EFPGFlightState::Stall)
			{
				bStalled = true;
				break;
			}
		}
		TestTrue(TEXT("A sustained climb eventually stalls"), bStalled);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
