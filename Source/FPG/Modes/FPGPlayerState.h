// Survives the pawn. Kills, rank and credits must NOT live on the aircraft, or they vanish
// the moment it is shot down (docs/16 §16.3).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FPGPlayerState.generated.h"

UCLASS()
class FPG_API AFPGPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Session credits are spent inside a run and destroyed at the end (docs/07 §7.1). */
	UFUNCTION(BlueprintPure, Category = "FPG") int32 GetSessionCredit() const { return SessionCredit; }
	UFUNCTION(BlueprintPure, Category = "FPG") int32 GetKills() const { return Kills; }
	UFUNCTION(BlueprintPure, Category = "FPG") float GetDistanceMeters() const { return DistanceMeters; }

	void AddCredit(int32 Amount);
	bool TrySpendCredit(int32 Amount);

	void AddKill() { ++Kills; }
	void SetDistanceMeters(float InMeters) { DistanceMeters = InMeters; }

	void ResetRun();

private:
	UPROPERTY(Replicated) int32 SessionCredit = 0;
	UPROPERTY(Replicated) int32 Kills = 0;
	UPROPERTY(Replicated) float DistanceMeters = 0.f;
};
