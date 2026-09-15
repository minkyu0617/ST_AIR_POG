// Single access point for every DataTable (docs/16 §16.10).
// Gameplay code never holds a UDataTable directly, so the loading strategy can change in one place.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/Types/FPGDataRows.h"
#include "FPGDataRegistry.generated.h"

class UDataTable;

UCLASS()
class FPG_API UFPGDataRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const FFPGAircraftRow* FindAircraft(FName Id) const;
	const FFPGItemRow* FindItem(FName Id) const;
	const FFPGPoiRow* FindPoi(FName Id) const;

	/** Row name of the first aircraft, used as the M1 default. NAME_None when the table is empty. */
	FName GetDefaultAircraftId() const;

	/**
	 * Structural validation (docs/17 §17.14). Returns false when any hard rule fails.
	 * Runs on Initialize and from the automation test, so bad data fails early instead of at runtime.
	 */
	bool ValidateAll(TArray<FString>& OutErrors, TArray<FString>& OutWarnings) const;

private:
	UPROPERTY(Transient) TObjectPtr<UDataTable> AircraftTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> ItemTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> PoiTable;

	void LoadTables();
};
