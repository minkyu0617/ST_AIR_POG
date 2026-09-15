#include "Flight/FPGFlightMovementComponent.h"

#include "FPG.h"
#include "Flight/FPGFlightSimulation.h"
#include "GameFramework/Actor.h"

UFPGFlightMovementComponent::UFPGFlightMovementComponent()
{
	// The pawn drives the step explicitly so input and simulation stay in a known order.
	PrimaryComponentTick.bCanEverTick = false;
}

void UFPGFlightMovementComponent::InitializeFromRow(const FFPGAircraftRow& Row)
{
	Config = FFPGFlightConfig::FromRow(Row);
	State.BoostRemaining = Config.BoostDuration;
}

void UFPGFlightMovementComponent::ResetTo(const FTransform& Transform, float StartSpeed)
{
	State = FFPGMoveState();
	State.Location = Transform.GetLocation();
	State.Rotation = Transform.Rotator();
	State.Speed = StartSpeed;
	State.BoostRemaining = Config.BoostDuration;
	State.State = EFPGFlightState::Normal;

	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocationAndRotation(State.Location, State.Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UFPGFlightMovementComponent::ApplyMove(const FFPGMove& Move)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// The one and only place the simulation runs. Server will call the identical function.
	State = FPGFlightSimulation::Step(State, Move, Config);

	FHitResult Hit;
	Owner->SetActorLocationAndRotation(State.Location, State.Rotation, /*bSweep=*/true, &Hit);

	if (Hit.bBlockingHit && State.State != EFPGFlightState::Destroyed)
	{
		// Keep the authoritative state in sync with where the sweep actually stopped,
		// otherwise the craft tunnels forward again on the next step.
		State.Location = Owner->GetActorLocation();
		OnCrashed.Broadcast(Hit);
	}
}

float UFPGFlightMovementComponent::GetBoostPercent() const
{
	return Config.BoostDuration > KINDA_SMALL_NUMBER
		? FMath::Clamp(State.BoostRemaining / Config.BoostDuration, 0.f, 1.f)
		: 0.f;
}

void UFPGFlightMovementComponent::MarkDestroyed()
{
	State.State = EFPGFlightState::Destroyed;
}
