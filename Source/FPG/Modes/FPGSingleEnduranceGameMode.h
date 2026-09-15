// Single player distance run (docs/04 §4.1). One life; the score is how far you got.
#pragma once

#include "CoreMinimal.h"
#include "Modes/FPGGameModeBase.h"
#include "FPGSingleEnduranceGameMode.generated.h"

UCLASS()
class FPG_API AFPGSingleEnduranceGameMode : public AFPGGameModeBase
{
	GENERATED_BODY()

public:
	AFPGSingleEnduranceGameMode();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "FPG") float GetBestDistanceMeters() const { return BestDistanceMeters; }

protected:
	virtual void OnPlayerDied(AFPGAircraftPawn* Aircraft) override;

	/** Credits per kilometre flown (DT_Economy CREDIT_PER_KM). */
	UPROPERTY(EditDefaultsOnly, Category = "FPG|Economy") int32 CreditPerKm = 15;

private:
	float LastAwardedKm = 0.f;
	float BestDistanceMeters = 0.f;
};
