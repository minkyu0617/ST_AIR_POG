#include "Core/Services/FPGDataRegistry.h"

#include "Combat/FPGHealthComponent.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPGData, Log, All);

namespace
{
	/** DT_Flight의 키 이름. FFPGFlightParams의 필드와 1:1로 대응합니다. */
	namespace FlightKeys
	{
		static const FName CruiseSpeed(TEXT("CRUISE_SPEED"));
		static const FName AccelThrust(TEXT("ACCEL_THRUST"));
		static const FName DecelBrake(TEXT("DECEL_BRAKE"));
		static const FName ThrottleRate(TEXT("THROTTLE_RATE"));
		static const FName RollMaxAngle(TEXT("ROLL_MAX_ANGLE"));
		static const FName BankTurnAt30(TEXT("BANK_TURN_RATE_AT_30DEG"));
		static const FName BankTurnAt75(TEXT("BANK_TURN_RATE_AT_75DEG"));
		static const FName AttitudeInertia(TEXT("ATTITUDE_INERTIA_SEC"));
		static const FName SpeedLossRoll(TEXT("SPEED_LOSS_ROLL_PCT"));
		static const FName SpeedLossVector(TEXT("SPEED_LOSS_VECTOR_PCT"));
		static const FName SpeedLossMax(TEXT("SPEED_LOSS_MAX_PCT"));
		static const FName DiveAccelBonus(TEXT("DIVE_ACCEL_BONUS"));
		static const FName DiveAccelRefPitch(TEXT("DIVE_ACCEL_REF_PITCH"));
		static const FName StallSpeed(TEXT("STALL_SPEED"));
		static const FName StallEnterSec(TEXT("STALL_ENTER_SEC"));
		static const FName StallRecoverSpeed(TEXT("STALL_RECOVER_SPEED"));
		static const FName StallInputScale(TEXT("STALL_INPUT_SCALE"));
		static const FName StallNoseDownRate(TEXT("STALL_NOSE_DOWN_RATE"));
		static const FName AltitudeCeilingM(TEXT("ALTITUDE_CEILING_M"));
		static const FName AltitudeCeilingMult(TEXT("ALTITUDE_CEILING_POWER_MULT"));
	}

	/** DT_Health의 키. FFPGHealthTuning의 필드와 1:1 대응합니다. */
	namespace HealthKeys
	{
		static const FName DamagedThreshold(TEXT("DAMAGED_THRESHOLD_PCT"));
		static const FName CriticalThreshold(TEXT("CRITICAL_THRESHOLD_PCT"));
		static const FName DamagedMaxSpeed(TEXT("DAMAGED_MAX_SPEED_MULT"));
		static const FName DamagedTurnRate(TEXT("DAMAGED_TURN_RATE_MULT"));
		static const FName CriticalMaxSpeed(TEXT("CRITICAL_MAX_SPEED_MULT"));
		static const FName CriticalTurnRate(TEXT("CRITICAL_TURN_RATE_MULT"));
		static const FName TerrainCollisionDamage(TEXT("TERRAIN_COLLISION_DAMAGE"));
	}
}

FString UFPGDataRegistry::GetDefaultDataDirectory()
{
	return FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DataTables"));
}

void UFPGDataRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TArray<FString> Problems;
	LoadFromDirectory(GetDefaultDataDirectory(), Problems);

	for (const FString& Problem : Problems)
	{
		UE_LOG(LogFPGData, Warning, TEXT("%s"), *Problem);
	}

	TArray<FString> Errors;
	if (!ValidateAll(Errors))
	{
		// 크래시시키지 않습니다. 데이터가 없어도 C++ 기본값으로 날 수는 있어야
		// 개발이 멈추지 않습니다. 다만 P5가 깨진 상태이므로 크게 남깁니다.
		for (const FString& Error : Errors)
		{
			UE_LOG(LogFPGData, Error, TEXT("데이터 검증 실패: %s"), *Error);
		}
	}
}

UDataTable* UFPGDataRegistry::LoadTable(const FString& Directory, const FString& TableName,
                                        const UScriptStruct* RowStruct, TArray<FString>& OutProblems)
{
	const FString Path = FPaths::Combine(Directory, TableName + TEXT(".csv"));

	FString Csv;
	if (!FFileHelper::LoadFileToString(Csv, *Path))
	{
		OutProblems.Add(FString::Printf(TEXT("%s 를 읽을 수 없습니다 (%s)"), *TableName, *Path));
		return nullptr;
	}

	UDataTable* Table = NewObject<UDataTable>(this, UDataTable::StaticClass(), FName(*TableName), RF_Transient);
	Table->RowStruct = const_cast<UScriptStruct*>(RowStruct);

	// 엔진 파서를 씁니다. 따옴표·이스케이프 처리를 직접 짜면 반드시 어긋납니다.
	const TArray<FString> Errors = Table->CreateTableFromCSVString(Csv);
	for (const FString& Error : Errors)
	{
		OutProblems.Add(FString::Printf(TEXT("%s: %s"), *TableName, *Error));
	}

	return Table;
}

bool UFPGDataRegistry::LoadFromDirectory(const FString& Directory, TArray<FString>& OutProblems)
{
	const int32 ProblemsBefore = OutProblems.Num();

	AircraftTable = LoadTable(Directory, TEXT("DT_Aircraft"), FFPGAircraftRow::StaticStruct(), OutProblems);
	ItemTable     = LoadTable(Directory, TEXT("DT_Item"),     FFPGItemRow::StaticStruct(),     OutProblems);
	ModuleTable   = LoadTable(Directory, TEXT("DT_Module"),   FFPGModuleRow::StaticStruct(),   OutProblems);
	FlightTable   = LoadTable(Directory, TEXT("DT_Flight"),   FFPGValueRow::StaticStruct(),    OutProblems);
	HealthTable   = LoadTable(Directory, TEXT("DT_Health"),   FFPGValueRow::StaticStruct(),    OutProblems);

	// DT_Poi·DT_Map·DT_Mode·DT_Enemy·DT_LevelCurve·DT_ItemDropRate는 아직
	// 읽지 않습니다. 그 값을 쓰는 시스템(POI·모드 규칙·적기·진행도)이 M1
	// 범위 밖이라, 지금 읽어봐야 쓰는 곳이 없고 행 구조체만 늘어납니다.
	// 해당 시스템을 만들 때 여기에 한 줄씩 추가하십시오.

	return OutProblems.Num() == ProblemsBefore;
}

const FFPGAircraftRow* UFPGDataRegistry::FindAircraft(FName Id) const
{
	if (!AircraftTable) { return nullptr; }
	return AircraftTable->FindRow<FFPGAircraftRow>(Id, TEXT("FindAircraft"), /*bWarnIfRowMissing=*/false);
}

const FFPGItemRow* UFPGDataRegistry::FindItem(FName Id) const
{
	if (!ItemTable) { return nullptr; }
	return ItemTable->FindRow<FFPGItemRow>(Id, TEXT("FindItem"), false);
}

const FFPGModuleRow* UFPGDataRegistry::FindModule(FName Id) const
{
	if (!ModuleTable) { return nullptr; }
	return ModuleTable->FindRow<FFPGModuleRow>(Id, TEXT("FindModule"), false);
}

float UFPGDataRegistry::GetFlightValue(FName Key, float Fallback) const
{
	if (!FlightTable) { return Fallback; }
	if (const FFPGValueRow* Row = FlightTable->FindRow<FFPGValueRow>(Key, TEXT("GetFlightValue"), false))
	{
		return Row->Value;
	}
	return Fallback;
}

float UFPGDataRegistry::GetHealthValue(FName Key, float Fallback) const
{
	if (!HealthTable) { return Fallback; }
	if (const FFPGValueRow* Row = HealthTable->FindRow<FFPGValueRow>(Key, TEXT("GetHealthValue"), false))
	{
		return Row->Value;
	}
	return Fallback;
}

void UFPGDataRegistry::BuildHealthTuning(FFPGHealthTuning& Out) const
{
	Out.DamagedThresholdPct    = GetHealthValue(HealthKeys::DamagedThreshold,      Out.DamagedThresholdPct);
	Out.CriticalThresholdPct   = GetHealthValue(HealthKeys::CriticalThreshold,     Out.CriticalThresholdPct);
	Out.DamagedMaxSpeedMult    = GetHealthValue(HealthKeys::DamagedMaxSpeed,       Out.DamagedMaxSpeedMult);
	Out.DamagedTurnRateMult    = GetHealthValue(HealthKeys::DamagedTurnRate,       Out.DamagedTurnRateMult);
	Out.CriticalMaxSpeedMult   = GetHealthValue(HealthKeys::CriticalMaxSpeed,      Out.CriticalMaxSpeedMult);
	Out.CriticalTurnRateMult   = GetHealthValue(HealthKeys::CriticalTurnRate,      Out.CriticalTurnRateMult);
	Out.TerrainCollisionDamage = GetHealthValue(HealthKeys::TerrainCollisionDamage, Out.TerrainCollisionDamage);
}

bool UFPGDataRegistry::BuildFlightParams(FName AircraftId, FFPGFlightParams& Out) const
{
	const FFPGAircraftRow* Row = FindAircraft(AircraftId);
	if (!Row)
	{
		return false;
	}

	// ── 기체별 (DT_Aircraft) ─────────────────────────────────
	Out.MinSpeed      = Row->MinSpeed;
	Out.MaxSpeed      = Row->MaxSpeed;
	Out.BoostSpeed    = Row->BoostSpeed;
	Out.RollRate      = Row->RollRate;
	Out.PitchRate     = Row->PitchRate;
	Out.YawRate       = Row->YawRate;
	Out.BoostDuration = Row->BoostDuration;
	Out.BoostCooldown = Row->BoostCooldown;

	// ── 기체 공통 (DT_Flight) ────────────────────────────────
	// 값이 없으면 Out의 기존 값(C++ 기본값)을 유지합니다. 조용한 실패를 막는 건
	// ValidateAll()과 CI의 E4 규칙입니다.
	Out.CruiseSpeed               = GetFlightValue(FlightKeys::CruiseSpeed,         Out.CruiseSpeed);
	Out.AccelThrust               = GetFlightValue(FlightKeys::AccelThrust,         Out.AccelThrust);
	Out.DecelBrake                = GetFlightValue(FlightKeys::DecelBrake,          Out.DecelBrake);
	Out.ThrottleRate              = GetFlightValue(FlightKeys::ThrottleRate,        Out.ThrottleRate);
	Out.RollMaxAngle              = GetFlightValue(FlightKeys::RollMaxAngle,        Out.RollMaxAngle);
	Out.BankTurnRateAt30          = GetFlightValue(FlightKeys::BankTurnAt30,        Out.BankTurnRateAt30);
	Out.BankTurnRateAt75          = GetFlightValue(FlightKeys::BankTurnAt75,        Out.BankTurnRateAt75);
	Out.AttitudeInertiaSec        = GetFlightValue(FlightKeys::AttitudeInertia,     Out.AttitudeInertiaSec);
	Out.SpeedLossRollPct          = GetFlightValue(FlightKeys::SpeedLossRoll,       Out.SpeedLossRollPct);
	Out.SpeedLossVectorPct        = GetFlightValue(FlightKeys::SpeedLossVector,     Out.SpeedLossVectorPct);
	Out.SpeedLossMaxPct           = GetFlightValue(FlightKeys::SpeedLossMax,        Out.SpeedLossMaxPct);
	Out.DiveAccelBonus            = GetFlightValue(FlightKeys::DiveAccelBonus,      Out.DiveAccelBonus);
	Out.DiveAccelRefPitch         = GetFlightValue(FlightKeys::DiveAccelRefPitch,   Out.DiveAccelRefPitch);
	Out.StallSpeed                = GetFlightValue(FlightKeys::StallSpeed,          Out.StallSpeed);
	Out.StallEnterSec             = GetFlightValue(FlightKeys::StallEnterSec,       Out.StallEnterSec);
	Out.StallRecoverSpeed         = GetFlightValue(FlightKeys::StallRecoverSpeed,   Out.StallRecoverSpeed);
	Out.StallInputScale           = GetFlightValue(FlightKeys::StallInputScale,     Out.StallInputScale);
	Out.StallNoseDownRate         = GetFlightValue(FlightKeys::StallNoseDownRate,   Out.StallNoseDownRate);
	Out.AltitudeCeilingM          = GetFlightValue(FlightKeys::AltitudeCeilingM,    Out.AltitudeCeilingM);
	Out.AltitudeCeilingPowerMult  = GetFlightValue(FlightKeys::AltitudeCeilingMult, Out.AltitudeCeilingPowerMult);

	return true;
}

bool UFPGDataRegistry::ValidateAll(TArray<FString>& OutErrors) const
{
	const int32 ErrorsBefore = OutErrors.Num();

	if (!AircraftTable || AircraftTable->GetRowMap().Num() == 0)
	{
		OutErrors.Add(TEXT("DT_Aircraft 가 비어 있습니다"));
	}
	if (!FlightTable || FlightTable->GetRowMap().Num() == 0)
	{
		OutErrors.Add(TEXT("DT_Flight 가 비어 있습니다"));
	}

	// V2 — 기체 스탯 5개 합 = 100. P8(P2W 금지)의 기계적 방어선입니다.
	// CI의 파이썬 검증기와 중복이지만, 여기서 한 번 더 보는 이유는 **로드
	// 결과**를 검사하기 때문입니다. CSV는 멀쩡한데 열 이름이 어긋나 값이
	// 0으로 들어온 경우는 파이썬 쪽이 잡지 못합니다.
	if (AircraftTable)
	{
		for (const TPair<FName, uint8*>& Pair : AircraftTable->GetRowMap())
		{
			const FFPGAircraftRow* Row = reinterpret_cast<const FFPGAircraftRow*>(Pair.Value);
			if (!Row) { continue; }

			const int32 Sum = Row->Speed + Row->Accel + Row->Turn + Row->Durability + Row->BoostPower;
			if (Sum != 100)
			{
				OutErrors.Add(FString::Printf(
					TEXT("V2 위반 — %s 스탯 합이 %d 입니다 (기대 100). 열 이름 불일치일 수 있습니다"),
					*Pair.Key.ToString(), Sum));
			}

			// V9 — MinSpeed < MaxSpeed < BoostSpeed
			if (!(Row->MinSpeed < Row->MaxSpeed && Row->MaxSpeed < Row->BoostSpeed))
			{
				OutErrors.Add(FString::Printf(
					TEXT("V9 위반 — %s 속도 순서 (Min=%.0f Max=%.0f Boost=%.0f)"),
					*Pair.Key.ToString(), Row->MinSpeed, Row->MaxSpeed, Row->BoostSpeed));
			}
		}
	}

	// V10(대리) — Category=RaceOnly 와 ModeMask=SpeedOnly 의 정합성.
	if (ItemTable)
	{
		for (const TPair<FName, uint8*>& Pair : ItemTable->GetRowMap())
		{
			const FFPGItemRow* Row = reinterpret_cast<const FFPGItemRow*>(Pair.Value);
			if (!Row) { continue; }

			const bool bRaceOnly = (Row->Category == EItemCategory::RaceOnly);
			const bool bSpeedOnly = (Row->ModeMask == EModeMask::SpeedOnly);
			if (bRaceOnly != bSpeedOnly)
			{
				OutErrors.Add(FString::Printf(
					TEXT("V10 위반 — %s 의 Category 와 ModeMask 가 어긋납니다"), *Pair.Key.ToString()));
			}
		}
	}

	return OutErrors.Num() == ErrorsBefore;
}
