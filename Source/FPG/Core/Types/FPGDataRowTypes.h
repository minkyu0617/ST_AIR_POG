// FPG — DataTable 행 구조체와 공용 열거형.
//
// docs/16 §16.12의 Core/Types/ — "공용 enum, 구조체 (의존 없음)".
// 열거형 정의는 docs/17 §17.2, 열 정의는 docs/17 §17.3~§17.7을 따릅니다.
//
// 🔴 열을 추가·변경할 때는 반드시 docs/17을 함께 고치십시오.
//    여기와 CSV 헤더와 문서가 어긋나면 값이 조용히 기본값으로 떨어지고,
//    "CSV를 고쳤는데 게임이 안 변한다"는 추적하기 힘든 상황이 됩니다.
//    검증 규칙 E4(Tools/validate_data.py)가 DT_Flight에 대해 이걸 막아줍니다.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FPGDataRowTypes.generated.h"

// ── 열거형 (docs/17 §17.2) ───────────────────────────────────
// 숫자가 아니라 **문자열로 저장**합니다. 숫자로 두면 나중에 순서가 바뀔 때
// 기존 데이터가 전부 어긋납니다.

UENUM(BlueprintType)
enum class EItemGrade : uint8
{
	Common,
	Rare,
	Epic,
	Legendary
};

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Gun,
	Attack,
	Defense,
	Utility,
	RaceOnly
};

UENUM(BlueprintType)
enum class EModeMask : uint8
{
	All,
	BattleOnly,
	SpeedOnly,
	SingleOnly
};

// ── 행 구조체 ────────────────────────────────────────────────
//
// CSV의 **첫 열은 행 이름(RowName)** 이 되며 UPROPERTY로 선언하지 않습니다.
// 즉 DT_Aircraft.csv의 `Id` 열은 아래 구조체에 나타나지 않고 FName 키가 됩니다.

/** DT_Aircraft.csv — docs/17 §17.3 */
USTRUCT(BlueprintType)
struct FFPGAircraftRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FName NameKey;
	UPROPERTY(EditAnywhere) FName DescKey;

	/** 🔴 다섯 값의 합은 반드시 100입니다. P2W 방지의 핵심 (P8, 검증 규칙 V2). */
	UPROPERTY(EditAnywhere) int32 Speed = 0;
	UPROPERTY(EditAnywhere) int32 Accel = 0;
	UPROPERTY(EditAnywhere) int32 Turn = 0;
	UPROPERTY(EditAnywhere) int32 Durability = 0;
	UPROPERTY(EditAnywhere) int32 BoostPower = 0;

	UPROPERTY(EditAnywhere) float BaseHP = 100.f;

	UPROPERTY(EditAnywhere) float MaxSpeed = 0.f;
	UPROPERTY(EditAnywhere) float MinSpeed = 0.f;
	UPROPERTY(EditAnywhere) float BoostSpeed = 0.f;

	UPROPERTY(EditAnywhere) float RollRate = 0.f;
	UPROPERTY(EditAnywhere) float PitchRate = 0.f;
	UPROPERTY(EditAnywhere) float YawRate = 0.f;

	UPROPERTY(EditAnywhere) float BoostDuration = 0.f;
	UPROPERTY(EditAnywhere) float BoostCooldown = 0.f;

	UPROPERTY(EditAnywhere) int32 ExtraItemSlots = 0;

	/** 에셋이 아직 없어 비어 있을 수 있습니다. 검증 V3·V4는 "비어 있지 않으면 존재해야 한다"로 구현하십시오. */
	UPROPERTY(EditAnywhere) FSoftClassPath AbilityClass;
	UPROPERTY(EditAnywhere) FSoftObjectPath MeshPath;
	UPROPERTY(EditAnywhere) FSoftObjectPath IconPath;

	/** 0 = 기본 제공. 곡선(DT_LevelCurve)에 없는 것이 정상입니다 (D-14). */
	UPROPERTY(EditAnywhere) int32 UnlockLevel = 0;

	UPROPERTY(EditAnywhere) int32 StorePrice_Same = 0;
	UPROPERTY(EditAnywhere) int32 StorePrice_Upgrade = 0;
};

/** DT_Item.csv — docs/17 §17.4 */
USTRUCT(BlueprintType)
struct FFPGItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) EItemCategory Category = EItemCategory::Attack;
	UPROPERTY(EditAnywhere) EItemGrade Grade = EItemGrade::Common;

	UPROPERTY(EditAnywhere) FSoftClassPath EffectClass;

	/** 🔴 음수 = 회복입니다 (docs/17 §17.4 규약). 회복 전용 열을 따로 두지 않습니다. */
	UPROPERTY(EditAnywhere) float Damage = 0.f;

	/** −1 = 무한. */
	UPROPERTY(EditAnywhere) int32 Ammo = 0;

	UPROPERTY(EditAnywhere) float Cooldown = 0.f;

	/** 0 = 히트스캔. */
	UPROPERTY(EditAnywhere) float Range = 0.f;
	UPROPERTY(EditAnywhere) float ProjectileSpeed = 0.f;

	UPROPERTY(EditAnywhere) bool RequiresLockOn = false;

	UPROPERTY(EditAnywhere) float Duration = 0.f;
	UPROPERTY(EditAnywhere) float Radius = 0.f;

	UPROPERTY(EditAnywhere) EModeMask ModeMask = EModeMask::All;

	UPROPERTY(EditAnywhere) int32 ShopPrice = 0;

	/** 상대에게 경고가 뜨는 선행 시간 (docs/06 밸런싱 원칙 1). */
	UPROPERTY(EditAnywhere) float WarnBeforeFire = 0.f;
};

/** DT_Module.csv — docs/17 §17.5 */
USTRUCT(BlueprintType)
struct FFPGModuleRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FName NameKey;
	UPROPERTY(EditAnywhere) int32 UnlockLevel = 0;
	UPROPERTY(EditAnywhere) int32 CoinPrice = 0;
	UPROPERTY(EditAnywhere) int32 InGamePrice = 0;

	/** "스탯:배율" 세미콜론 구분. 덧셈이 아니라 **곱셈**입니다 (docs/17 §17.5). */
	UPROPERTY(EditAnywhere) FString StatModifiers;
};

/**
 * `Id,Value` 두 열짜리 표의 공용 행.
 * DT_Flight · DT_Economy · DT_Progression · DT_DifficultyScaling이 이 형태입니다.
 */
USTRUCT(BlueprintType)
struct FFPGValueRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float Value = 0.f;
};
