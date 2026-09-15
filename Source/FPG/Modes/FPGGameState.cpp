#include "Modes/FPGGameState.h"

#include "Net/UnrealNetwork.h"

void AFPGGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPGGameState, MatchPhase);
	DOREPLIFETIME(AFPGGameState, CountdownRemaining);
}
