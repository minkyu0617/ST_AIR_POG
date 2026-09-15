#include "Modes/FPGSingleEnduranceGameMode.h"

#include "FPG.h"
#include "Core/FPGGameInstance.h"
#include "Core/Services/FPGEventBus.h"
#include "Flight/FPGAircraftPawn.h"
#include "Modes/FPGGameState.h"
#include "Modes/FPGPlayerState.h"

AFPGSingleEnduranceGameMode::AFPGSingleEnduranceGameMode()
{
	StartingAircraftId = NAME_None; // falls back to the first row in DT_Aircraft
}

void AFPGSingleEnduranceGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const AFPGGameState* State = GetGameState<AFPGGameState>();
	if (!State || State->GetMatchPhase() != EFPGMatchPhase::InProgress)
	{
		return;
	}

	AFPGAircraftPawn* Aircraft = GetPlayerAircraft();
	if (!Aircraft)
	{
		return;
	}

	const float Meters = Aircraft->GetDistanceTravelledMeters();

	AFPGPlayerState* PlayerState = Aircraft->GetPlayerState<AFPGPlayerState>();
	if (!PlayerState)
	{
		return;
	}

	PlayerState->SetDistanceMeters(Meters);

	// Award credits once per whole kilometre so the player accumulates something to
	// spend at a POI. This is the loop that makes the detour decision meaningful.
	const float Km = Meters / 1000.f;
	while (Km >= LastAwardedKm + 1.f)
	{
		LastAwardedKm += 1.f;
		PlayerState->AddCredit(CreditPerKm);
	}
}

void AFPGSingleEnduranceGameMode::OnPlayerDied(AFPGAircraftPawn* Aircraft)
{
	SetPhase(EFPGMatchPhase::PostMatch);

	const float Meters = Aircraft ? Aircraft->GetDistanceTravelledMeters() : 0.f;
	BestDistanceMeters = FMath::Max(BestDistanceMeters, Meters);

	const FString Summary = FString::Printf(TEXT("Run over - %.0f m flown. Press R to restart."), Meters);
	UE_LOG(LogFPG, Log, TEXT("%s"), *Summary);

	if (const UFPGGameInstance* GameInstance = GetGameInstance<UFPGGameInstance>())
	{
		if (UFPGEventBus* Bus = GameInstance->GetEventBus())
		{
			Bus->Toast(Summary);
		}
	}
}
