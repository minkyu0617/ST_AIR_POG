// Thin wrapper around the pure simulation. Owns state, applies the result to the actor.
// M3 adds prediction + reconciliation on top of this without touching FPGFlightSimulation.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Flight/FPGFlightTypes.h"
#include "FPGFlightMovementComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FFPGOnCrashed, const FHitResult& /*Hit*/);

UCLASS(ClassGroup = (FPG), meta = (BlueprintSpawnableComponent))
class FPG_API UFPGFlightMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPGFlightMovementComponent();

	void InitializeFromRow(const FFPGAircraftRow& Row);

	/** Runs one simulation step and sweeps the owner into place. */
	void ApplyMove(const FFPGMove& Move);

	/** Places the craft and clears all flight state. Used by the boarding sequence. */
	void ResetTo(const FTransform& Transform, float StartSpeed);

	const FFPGMoveState& GetState() const { return State; }
	const FFPGFlightConfig& GetConfig() const { return Config; }

	UFUNCTION(BlueprintPure, Category = "FPG|Flight") float GetSpeed() const { return State.Speed; }
	UFUNCTION(BlueprintPure, Category = "FPG|Flight") float GetSpeedKmh() const { return State.Speed * 0.036f; }
	UFUNCTION(BlueprintPure, Category = "FPG|Flight") float GetAltitudeMeters() const { return State.Location.Z * 0.01f; }
	UFUNCTION(BlueprintPure, Category = "FPG|Flight") float GetBoostPercent() const;
	UFUNCTION(BlueprintPure, Category = "FPG|Flight") EFPGFlightState GetFlightState() const { return State.State; }
	UFUNCTION(BlueprintPure, Category = "FPG|Flight") bool IsStalled() const { return State.State == EFPGFlightState::Stall; }

	void MarkDestroyed();

	FFPGOnCrashed OnCrashed;

private:
	FFPGMoveState State;
	FFPGFlightConfig Config;
};
