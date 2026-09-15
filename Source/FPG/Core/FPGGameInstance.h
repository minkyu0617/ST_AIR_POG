// Owns the services and survives level transitions (docs/16 §16.3).
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FPGGameInstance.generated.h"

class UFPGDataRegistry;
class UFPGEventBus;

UCLASS()
class FPG_API UFPGGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFPGDataRegistry* GetDataRegistry() const;
	UFPGEventBus* GetEventBus() const;
};
