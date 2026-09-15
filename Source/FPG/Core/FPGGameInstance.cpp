#include "Core/FPGGameInstance.h"

#include "FPG.h"
#include "Core/Services/FPGDataRegistry.h"
#include "Core/Services/FPGEventBus.h"

void UFPGGameInstance::Init()
{
	Super::Init();

	// Touch the subsystems so DataRegistry validation runs at startup rather than on first use.
	GetSubsystem<UFPGDataRegistry>();
	GetSubsystem<UFPGEventBus>();

	UE_LOG(LogFPG, Log, TEXT("FPG game instance initialised."));
}

UFPGDataRegistry* UFPGGameInstance::GetDataRegistry() const
{
	return const_cast<UFPGGameInstance*>(this)->GetSubsystem<UFPGDataRegistry>();
}

UFPGEventBus* UFPGGameInstance::GetEventBus() const
{
	return const_cast<UFPGGameInstance*>(this)->GetSubsystem<UFPGEventBus>();
}
