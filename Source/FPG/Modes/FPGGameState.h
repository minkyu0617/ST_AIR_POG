// Replicated match-wide state (docs/16 §16.3).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/Types/FPGTypes.h"
#include "FPGGameState.generated.h"

UCLASS()
class FPG_API AFPGGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "FPG") EFPGMatchPhase GetMatchPhase() const { return MatchPhase; }
	UFUNCTION(BlueprintPure, Category = "FPG") float GetCountdownRemaining() const { return CountdownRemaining; }

	void SetMatchPhase(EFPGMatchPhase InPhase) { MatchPhase = InPhase; }
	void SetCountdownRemaining(float InSeconds) { CountdownRemaining = InSeconds; }

private:
	UPROPERTY(Replicated) EFPGMatchPhase MatchPhase = EFPGMatchPhase::WaitingForPlayers;
	UPROPERTY(Replicated) float CountdownRemaining = 0.f;
};
