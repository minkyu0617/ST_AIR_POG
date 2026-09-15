#include "Modes/FPGPlayerState.h"

#include "FPG.h"
#include "Core/FPGGameInstance.h"
#include "Core/Services/FPGEventBus.h"
#include "Net/UnrealNetwork.h"

void AFPGPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPGPlayerState, SessionCredit);
	DOREPLIFETIME(AFPGPlayerState, Kills);
	DOREPLIFETIME(AFPGPlayerState, DistanceMeters);
}

void AFPGPlayerState::AddCredit(int32 Amount)
{
	if (Amount == 0)
	{
		return;
	}

	SessionCredit = FMath::Max(0, SessionCredit + Amount);

	if (const UFPGGameInstance* GameInstance = GetGameInstance<UFPGGameInstance>())
	{
		if (UFPGEventBus* Bus = GameInstance->GetEventBus())
		{
			Bus->OnCreditChanged.Broadcast(SessionCredit, Amount);
		}
	}
}

bool AFPGPlayerState::TrySpendCredit(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}
	if (SessionCredit < Amount)
	{
		return false;
	}

	AddCredit(-Amount);
	return true;
}

void AFPGPlayerState::ResetRun()
{
	SessionCredit = 0;
	Kills = 0;
	DistanceMeters = 0.f;
}
