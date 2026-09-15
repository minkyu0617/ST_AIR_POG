#include "Core/Services/FPGDataRegistry.h"

#include "FPG.h"
#include "Engine/DataTable.h"

namespace
{
	// M1 loads tables from /Game/Data. They are imported from Config/DataTables/*.csv.
	const TCHAR* AircraftTablePath = TEXT("/Game/Data/DT_Aircraft.DT_Aircraft");
	const TCHAR* ItemTablePath     = TEXT("/Game/Data/DT_Item.DT_Item");
	const TCHAR* PoiTablePath      = TEXT("/Game/Data/DT_Poi.DT_Poi");

	UDataTable* LoadTable(const TCHAR* Path)
	{
		UDataTable* Table = LoadObject<UDataTable>(nullptr, Path);
		if (!Table)
		{
			UE_LOG(LogFPG, Warning, TEXT("DataRegistry: table not found at %s - import the CSV first (docs/18)."), Path);
		}
		return Table;
	}
}

void UFPGDataRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadTables();

	TArray<FString> Errors;
	TArray<FString> Warnings;
	const bool bOk = ValidateAll(Errors, Warnings);

	for (const FString& Warning : Warnings)
	{
		UE_LOG(LogFPG, Warning, TEXT("DataTable warning: %s"), *Warning);
	}
	for (const FString& Error : Errors)
	{
		UE_LOG(LogFPG, Error, TEXT("DataTable error: %s"), *Error);
	}

	UE_LOG(LogFPG, Log, TEXT("DataRegistry initialised. Valid=%s Errors=%d Warnings=%d"),
		bOk ? TEXT("true") : TEXT("false"), Errors.Num(), Warnings.Num());
}

void UFPGDataRegistry::LoadTables()
{
	AircraftTable = LoadTable(AircraftTablePath);
	ItemTable = LoadTable(ItemTablePath);
	PoiTable = LoadTable(PoiTablePath);
}

const FFPGAircraftRow* UFPGDataRegistry::FindAircraft(FName Id) const
{
	return AircraftTable ? AircraftTable->FindRow<FFPGAircraftRow>(Id, TEXT("FindAircraft"), false) : nullptr;
}

const FFPGItemRow* UFPGDataRegistry::FindItem(FName Id) const
{
	return ItemTable ? ItemTable->FindRow<FFPGItemRow>(Id, TEXT("FindItem"), false) : nullptr;
}

const FFPGPoiRow* UFPGDataRegistry::FindPoi(FName Id) const
{
	return PoiTable ? PoiTable->FindRow<FFPGPoiRow>(Id, TEXT("FindPoi"), false) : nullptr;
}

FName UFPGDataRegistry::GetDefaultAircraftId() const
{
	if (!AircraftTable)
	{
		return NAME_None;
	}

	const TArray<FName> RowNames = AircraftTable->GetRowNames();
	return RowNames.Num() > 0 ? RowNames[0] : NAME_None;
}

bool UFPGDataRegistry::ValidateAll(TArray<FString>& OutErrors, TArray<FString>& OutWarnings) const
{
	if (!AircraftTable)
	{
		OutErrors.Add(TEXT("DT_Aircraft is missing."));
	}
	else
	{
		TArray<FFPGAircraftRow*> Rows;
		AircraftTable->GetAllRows<FFPGAircraftRow>(TEXT("ValidateAll"), Rows);
		const TArray<FName> Names = AircraftTable->GetRowNames();

		for (int32 Index = 0; Index < Rows.Num(); ++Index)
		{
			const FFPGAircraftRow* Row = Rows[Index];
			const FString RowName = Names.IsValidIndex(Index) ? Names[Index].ToString() : FString::FromInt(Index);

			// V2: stat sum must be exactly 100 so no aircraft is a strict upgrade (docs/07 §7.2).
			const int32 StatSum = Row->Speed + Row->Accel + Row->Turn + Row->Durability + Row->BoostPower;
			if (StatSum != 100)
			{
				OutErrors.Add(FString::Printf(TEXT("DT_Aircraft[%s]: stat sum is %d, must be 100."), *RowName, StatSum));
			}

			// V9: speed ordering must hold or the flight simulation clamps incoherently.
			if (!(Row->MinSpeed < Row->MaxSpeed && Row->MaxSpeed < Row->BoostSpeed))
			{
				OutErrors.Add(FString::Printf(
					TEXT("DT_Aircraft[%s]: requires MinSpeed < MaxSpeed < BoostSpeed (got %.0f / %.0f / %.0f)."),
					*RowName, Row->MinSpeed, Row->MaxSpeed, Row->BoostSpeed));
			}

			if (Row->BaseHP <= 0.f)
			{
				OutErrors.Add(FString::Printf(TEXT("DT_Aircraft[%s]: BaseHP must be positive."), *RowName));
			}

			if (Row->NameKey.IsNone())
			{
				OutWarnings.Add(FString::Printf(TEXT("DT_Aircraft[%s]: NameKey is empty."), *RowName));
			}

			// Asset paths are warnings in M1 - the meshes and ability blueprints do not exist yet.
			if (!Row->AbilityClass.IsEmpty())
			{
				OutWarnings.Add(FString::Printf(TEXT("DT_Aircraft[%s]: AbilityClass '%s' is not resolved in M1."),
					*RowName, *Row->AbilityClass));
			}
		}
	}

	if (!ItemTable)
	{
		OutErrors.Add(TEXT("DT_Item is missing."));
	}
	else
	{
		TArray<FFPGItemRow*> Rows;
		ItemTable->GetAllRows<FFPGItemRow>(TEXT("ValidateAll"), Rows);
		const TArray<FName> Names = ItemTable->GetRowNames();

		for (int32 Index = 0; Index < Rows.Num(); ++Index)
		{
			const FFPGItemRow* Row = Rows[Index];
			const FString RowName = Names.IsValidIndex(Index) ? Names[Index].ToString() : FString::FromInt(Index);

			if (Row->Ammo == 0)
			{
				OutErrors.Add(FString::Printf(TEXT("DT_Item[%s]: Ammo 0 is never usable (use -1 for unlimited)."), *RowName));
			}
			if (Row->Cooldown < 0.f)
			{
				OutErrors.Add(FString::Printf(TEXT("DT_Item[%s]: Cooldown must not be negative."), *RowName));
			}
		}
	}

	if (!PoiTable)
	{
		OutErrors.Add(TEXT("DT_Poi is missing."));
	}

	return OutErrors.Num() == 0;
}
