// Gameplay publishes, presentation subscribes (docs/16 §16.9).
// Strictly one-directional: gameplay -> UI / FX. Never used for gameplay-to-gameplay calls.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FPGEventBus.generated.h"

class AActor;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FFPGOnDamageTaken, AActor* /*Victim*/, float /*Amount*/, float /*NewHP*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FFPGOnDeath, AActor* /*Victim*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FFPGOnCreditChanged, int32 /*NewTotal*/, int32 /*Delta*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FFPGOnPoiDockProgress, FName /*PoiId*/, float /*Progress01*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FFPGOnPoiTransaction, FName /*PoiId*/, const FString& /*Summary*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FFPGOnToast, const FString& /*Message*/);

UCLASS()
class FPG_API UFPGEventBus : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	FFPGOnDamageTaken OnDamageTaken;
	FFPGOnDeath OnDeath;
	FFPGOnCreditChanged OnCreditChanged;
	FFPGOnPoiDockProgress OnPoiDockProgress;
	FFPGOnPoiTransaction OnPoiTransaction;
	FFPGOnToast OnToast;

	/** Convenience wrapper so callers do not have to null-check the delegate. */
	void Toast(const FString& Message);
};
