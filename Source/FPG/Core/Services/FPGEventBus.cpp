#include "Core/Services/FPGEventBus.h"

#include "FPG.h"

void UFPGEventBus::Toast(const FString& Message)
{
	UE_LOG(LogFPG, Log, TEXT("[toast] %s"), *Message);
	OnToast.Broadcast(Message);
}
