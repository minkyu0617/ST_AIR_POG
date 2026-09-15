// Owns input. Builds one FFPGMove per frame in PlayerTick, which runs *after* input
// processing - that ordering is what keeps input and simulation in lockstep.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPGPlayerController.generated.h"

class AFPGAircraftPawn;
class UInputAction;
class UInputMappingContext;

UCLASS()
class FPG_API AFPGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFPGPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	uint8 GetInputFlags() const { return InputFlags; }

private:
	// Input actions are built in C++ rather than as .uasset files so the whole input
	// setup lives in version-controlled text and needs no editor work to review.
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Accelerate;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Decelerate;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_RollLeft;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_RollRight;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_VectorMod;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Boost;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Fire;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_CycleItem;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Restart;

	uint8 InputFlags = 0;
	float ElapsedTime = 0.f;

	void BuildInputActions();

	void SetFlag(uint8 Flag);
	void ClearFlag(uint8 Flag);
	void OnCycleItem();
	void OnRestart();

	AFPGAircraftPawn* GetAircraft() const;
};
