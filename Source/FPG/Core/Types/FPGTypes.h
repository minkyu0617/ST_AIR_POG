// Shared enums. Stored as strings in CSV so reordering never corrupts data (docs/17 §17.2).
#pragma once

#include "CoreMinimal.h"
#include "FPGTypes.generated.h"

UENUM(BlueprintType)
enum class EFPGItemGrade : uint8
{
	Common,
	Rare,
	Epic,
	Legendary
};

UENUM(BlueprintType)
enum class EFPGItemCategory : uint8
{
	Gun,
	Attack,
	Defense,
	Utility,
	RaceOnly
};

UENUM(BlueprintType)
enum class EFPGModeMask : uint8
{
	All,
	BattleOnly,
	SpeedOnly,
	SingleOnly
};

UENUM(BlueprintType)
enum class EFPGPoiType : uint8
{
	AircraftStore,
	WeaponShop,
	RepairBay
};

UENUM(BlueprintType)
enum class EFPGMatchPhase : uint8
{
	WaitingForPlayers,
	Boarding,
	Countdown,
	InProgress,
	SuddenDeath,
	PostMatch
};

/** Flight state machine (docs/16 §16.7). */
UENUM(BlueprintType)
enum class EFPGFlightState : uint8
{
	Normal,
	Vector,
	Stall,
	Destroyed
};

/** HP band -> status effects (docs/02 §2.5). */
UENUM(BlueprintType)
enum class EFPGDamageState : uint8
{
	Healthy,
	Damaged,
	Critical,
	Dead
};

/** Altitude layers (docs/05 §5.1). */
UENUM(BlueprintType)
enum class EFPGAltitudeLayer : uint8
{
	Canyon,
	CloudSea,
	Skyport,
	Stratos
};
