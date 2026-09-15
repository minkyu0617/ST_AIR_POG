#include "Modes/FPGGameModeBase.h"

#include "FPG.h"
#include "Combat/FPGHealthComponent.h"
#include "Core/FPGGameInstance.h"
#include "Core/Services/FPGDataRegistry.h"
#include "Core/Services/FPGEventBus.h"
#include "Flight/FPGAircraftPawn.h"
#include "Flight/FPGFlightMovementComponent.h"
#include "Modes/FPGGameState.h"
#include "Modes/FPGPlayerController.h"
#include "Modes/FPGPlayerState.h"
#include "UI/FPGDebugHUD.h"

AFPGGameModeBase::AFPGGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = AFPGAircraftPawn::StaticClass();
	PlayerControllerClass = AFPGPlayerController::StaticClass();
	PlayerStateClass = AFPGPlayerState::StaticClass();
	GameStateClass = AFPGGameState::StaticClass();
	HUDClass = AFPGDebugHUD::StaticClass();
}

void AFPGGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (UFPGGameInstance* GameInstance = GetGameInstance<UFPGGameInstance>())
	{
		if (UFPGEventBus* Bus = GameInstance->GetEventBus())
		{
			Bus->OnDeath.AddUObject(this, &AFPGGameModeBase::HandleDeathEvent);
		}
	}

	SetPhase(EFPGMatchPhase::Boarding);
}

void AFPGGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AFPGAircraftPawn* Aircraft = Cast<AFPGAircraftPawn>(NewPlayer ? NewPlayer->GetPawn() : nullptr);
	if (!Aircraft)
	{
		return;
	}

	FName AircraftId = StartingAircraftId;
	if (AircraftId.IsNone())
	{
		const UFPGGameInstance* GameInstance = GetGameInstance<UFPGGameInstance>();
		const UFPGDataRegistry* Registry = GameInstance ? GameInstance->GetDataRegistry() : nullptr;
		AircraftId = Registry ? Registry->GetDefaultAircraftId() : NAME_None;
	}

	if (!AircraftId.IsNone())
	{
		Aircraft->ApplyAircraftData(AircraftId);
	}
	else
	{
		UE_LOG(LogFPG, Error, TEXT("No aircraft row available - import Config/DataTables/DT_Aircraft.csv (docs/18)."));
	}

	Aircraft->SetInputLocked(true);
}

void AFPGGameModeBase::SetPhase(EFPGMatchPhase NewPhase)
{
	PhaseElapsed = 0.f;

	if (AFPGGameState* State = GetGameState<AFPGGameState>())
	{
		State->SetMatchPhase(NewPhase);
	}

	UE_LOG(LogFPG, Log, TEXT("Match phase -> %d"), static_cast<int32>(NewPhase));
}

AFPGAircraftPawn* AFPGGameModeBase::GetPlayerAircraft() const
{
	const UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	return PC ? Cast<AFPGAircraftPawn>(PC->GetPawn()) : nullptr;
}

void AFPGGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AFPGGameState* State = GetGameState<AFPGGameState>();
	if (!State)
	{
		return;
	}

	PhaseElapsed += DeltaSeconds;

	switch (State->GetMatchPhase())
	{
	case EFPGMatchPhase::Boarding:
		if (PhaseElapsed >= BoardingSeconds)
		{
			SetPhase(EFPGMatchPhase::Countdown);
			State->SetCountdownRemaining(CountdownSeconds);
		}
		break;

	case EFPGMatchPhase::Countdown:
	{
		const float Remaining = FMath::Max(0.f, CountdownSeconds - PhaseElapsed);
		State->SetCountdownRemaining(Remaining);

		if (Remaining <= 0.f)
		{
			SetPhase(EFPGMatchPhase::InProgress);

			if (AFPGAircraftPawn* Aircraft = GetPlayerAircraft())
			{
				if (UFPGFlightMovementComponent* Movement = Aircraft->GetFlightMovement())
				{
					// GO: hand the craft a flying speed rather than dropping it from rest.
					Movement->ResetTo(Aircraft->GetActorTransform(), LaunchSpeed);
				}
				Aircraft->SetInputLocked(false);
			}

			OnMatchStarted();
		}
		break;
	}

	case EFPGMatchPhase::InProgress:
	case EFPGMatchPhase::SuddenDeath:
	case EFPGMatchPhase::PostMatch:
	case EFPGMatchPhase::WaitingForPlayers:
	default:
		break;
	}
}

void AFPGGameModeBase::HandleDeathEvent(AActor* Victim)
{
	if (bDeathHandled)
	{
		return;
	}

	AFPGAircraftPawn* Aircraft = Cast<AFPGAircraftPawn>(Victim);
	if (!Aircraft)
	{
		return;
	}

	bDeathHandled = true;
	OnPlayerDied(Aircraft);
}
