// One pawn class for every aircraft. The 8 variants are DataTable rows, not subclasses (docs/16 §16.4).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Flight/FPGFlightTypes.h"
#include "FPGAircraftPawn.generated.h"

class UCameraComponent;
class USphereComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UFPGFlightMovementComponent;
class UFPGHealthComponent;
class UFPGWeaponSlotComponent;

UCLASS()
class FPG_API AFPGAircraftPawn : public APawn
{
	GENERATED_BODY()

public:
	AFPGAircraftPawn();

	virtual void BeginPlay() override;

	/** Applies a DataTable row to this craft. Called by the game mode on spawn. */
	void ApplyAircraftData(FName AircraftId);

	/** Driven by the player controller so input always precedes simulation. */
	void ApplyMove(const FFPGMove& Move);

	void SetInputLocked(bool bLocked) { bInputLocked = bLocked; }
	bool IsInputLocked() const { return bInputLocked; }

	void FireSelected();
	void CycleItemSlot();

	UFUNCTION(BlueprintPure, Category = "FPG") UFPGFlightMovementComponent* GetFlightMovement() const { return FlightMovement; }
	UFUNCTION(BlueprintPure, Category = "FPG") UFPGHealthComponent* GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "FPG") UFPGWeaponSlotComponent* GetWeapons() const { return Weapons; }
	UFUNCTION(BlueprintPure, Category = "FPG") FName GetAircraftId() const { return AircraftId; }

	/** Total ground distance flown this run, in metres. Drives the endurance score. */
	UFUNCTION(BlueprintPure, Category = "FPG") float GetDistanceTravelledMeters() const { return DistanceTravelledCm * 0.01f; }

protected:
	/** Root + collision. Kept separate from the mesh so the visual tilt never affects the transform. */
	UPROPERTY(VisibleAnywhere, Category = "FPG") TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, Category = "FPG") TObjectPtr<UStaticMeshComponent> Hull;
	UPROPERTY(VisibleAnywhere, Category = "FPG") TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere, Category = "FPG") TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, Category = "FPG") TObjectPtr<UFPGFlightMovementComponent> FlightMovement;
	UPROPERTY(VisibleAnywhere, Category = "FPG") TObjectPtr<UFPGHealthComponent> Health;
	UPROPERTY(VisibleAnywhere, Category = "FPG") TObjectPtr<UFPGWeaponSlotComponent> Weapons;

	/** Damage taken when the hull sweeps into geometry. 999 = instant kill on terrain (docs/02 §2.4). */
	UPROPERTY(EditDefaultsOnly, Category = "FPG") float CrashDamage = 999.f;

private:
	FName AircraftId;
	bool bInputLocked = true;
	float DistanceTravelledCm = 0.f;

	void HandleCrashed(const FHitResult& Hit);
	void HandleDied();
};
