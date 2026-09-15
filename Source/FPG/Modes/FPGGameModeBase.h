// Shared match flow. Mode-specific rules (win condition, respawn) live in subclasses,
// so adding a mode never touches flight or combat code (docs/16 A1).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/Types/FPGTypes.h"
#include "FPGGameModeBase.generated.h"

class AFPGAircraftPawn;

UCLASS()
class FPG_API AFPGGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPGGameModeBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	/** Boarding animation stand-in. Also absorbs load-time variance between clients. */
	UPROPERTY(EditDefaultsOnly, Category = "FPG|Match") float BoardingSeconds = 1.5f;
	/** The kart-racer style 3-2-1 (docs/02 §2.6). */
	UPROPERTY(EditDefaultsOnly, Category = "FPG|Match") float CountdownSeconds = 3.f;
	/** Speed granted at GO so the craft is already flying, not falling. */
	UPROPERTY(EditDefaultsOnly, Category = "FPG|Match") float LaunchSpeed = 18000.f;

	/** Row name in DT_Aircraft. Empty falls back to the first row. */
	UPROPERTY(EditDefaultsOnly, Category = "FPG|Match") FName StartingAircraftId;

	virtual void OnMatchStarted() {}
	virtual void OnPlayerDied(AFPGAircraftPawn* Aircraft) {}

	void SetPhase(EFPGMatchPhase NewPhase);
	AFPGAircraftPawn* GetPlayerAircraft() const;

private:
	float PhaseElapsed = 0.f;
	bool bDeathHandled = false;

	void HandleDeathEvent(AActor* Victim);
};
