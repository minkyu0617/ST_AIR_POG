// DataTable row structs. Column names must match the CSVs in Config/DataTables (docs/17).
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Core/Types/FPGTypes.h"
#include "FPGDataRows.generated.h"

/** DT_Aircraft. Stat sum (Speed+Accel+Turn+Durability+BoostPower) must be exactly 100. */
USTRUCT(BlueprintType)
struct FPG_API FFPGAircraftRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName NameKey;

	// Balance axes - sum must equal 100 (docs/07 §7.2). Enforced by ValidateAll().
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Speed = 20;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Accel = 20;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Turn = 20;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Durability = 20;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BoostPower = 20;

	// Derived flight values, in unreal units per second / degrees per second (docs/02 §2.2).
	// NOTE: docs/02 lists speeds as "u/s" but its own km/h note implies metres per second.
	// Resolved here as: doc value x 100 = uu/s (1 uu = 1 cm). 180 m/s ~= 648 km/h. See docs/14 D-14.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float BaseHP = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxSpeed = 26000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MinSpeed = 7000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoostSpeed = 38000.f;
	/** Throttle-up acceleration, uu/s^2. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AccelRate = 5500.f;
	/** Throttle-down deceleration, uu/s^2. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float DecelRate = 7500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float RollRate = 140.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float PitchRate = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float YawRate = 65.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoostDuration = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoostCooldown = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ExtraItemSlots = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString AbilityClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 UnlockLevel = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 StorePrice_Same = 150;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 StorePrice_Upgrade = 300;
};

/** DT_Item. Negative Damage means heal (docs/17 §17.4). */
USTRUCT(BlueprintType)
struct FPG_API FFPGItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) EFPGItemCategory Category = EFPGItemCategory::Attack;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EFPGItemGrade Grade = EFPGItemGrade::Common;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString EffectClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Damage = 0.f;
	/** -1 = unlimited. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Ammo = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Cooldown = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Range = 0.f;
	/** 0 = hitscan. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float ProjectileSpeed = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool RequiresLockOn = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Duration = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Radius = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EFPGModeMask ModeMask = EFPGModeMask::All;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ShopPrice = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float WarnBeforeFire = 0.f;
};

/** DT_Poi. */
USTRUCT(BlueprintType)
struct FPG_API FFPGPoiRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) EFPGPoiType Type = EFPGPoiType::RepairBay;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName NameKey;
	/** Max speed allowed to begin docking, in uu/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float DockSpeedLimit = 20000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float DockTimeSec = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BaseRepairCost = 60;
	/** Cost multiplier applied per additional visit in the same run. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float RepairCostMultiplier = 1.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BoostRefillCost = 30;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AmmoRefillCost = 25;
};
