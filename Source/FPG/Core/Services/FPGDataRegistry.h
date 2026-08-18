// FPG — 모든 DataTable 접근을 모으는 곳.
//
// docs/16 §16.10 — "게임플레이 코드는 UDataTable을 직접 들지 않습니다.
// 로딩 방식이 바뀌어도(예: 나중에 서버에서 내려받기) 여기만 고칩니다."
//
// 🔴 P5 — 이 클래스가 있어야 P5(밸런스 수치를 코드에 하드코딩하지 않는다)가
//    실제로 성립합니다. 이게 없으면 CSV는 죽은 데이터이고 값은 C++ 기본값으로
//    돕니다. 밸런싱은 출시 후 수백 번 반복되므로 매번 재컴파일하면 패치 주기가
//    주 단위가 되고 그동안 유저는 떠납니다.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/Types/FPGDataRowTypes.h"
#include "Flight/FlightTypes.h"
#include "FPGDataRegistry.generated.h"

class UDataTable;

UCLASS()
class FPG_API UFPGDataRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ── 조회 (docs/16 §16.10 시그니처) ───────────────────────
	const FFPGAircraftRow* FindAircraft(FName Id) const;
	const FFPGItemRow*     FindItem(FName Id) const;
	const FFPGModuleRow*   FindModule(FName Id) const;

	/** DT_Flight 등 `Id,Value` 표의 값. 없으면 Fallback을 돌려줍니다. */
	float GetFlightValue(FName Key, float Fallback) const;

	/**
	 * 시뮬레이션 파라미터를 만듭니다 — 이 클래스의 존재 이유.
	 *
	 * 기체별 값(DT_Aircraft)과 기체 공통 값(DT_Flight)을 합쳐
	 * `FFPGFlightParams` 하나로 만들어 `UFlightMovementComponent`에 넘깁니다.
	 *
	 * 값이 없으면 인자로 받은 구조체의 기존 값(= C++ 기본값)을 유지합니다.
	 * 즉 **CSV가 비어도 비행은 되지만, 그건 P5가 깨진 상태**입니다.
	 * 그래서 누락은 조용히 넘기지 않고 ValidateAll()이 잡습니다.
	 *
	 * @return 기체를 찾았으면 true. false면 Out은 손대지 않습니다.
	 */
	bool BuildFlightParams(FName AircraftId, FFPGFlightParams& Out) const;

	/**
	 * 참조 무결성 검사 — docs/17 §17.14의 규칙 중 **런타임에서 가능한 것**.
	 *
	 * 에셋·문자열 테이블이 필요한 V3·V4·V7은 여기서 하지 않습니다.
	 * CSV 단계 검사는 이미 `Tools/validate_data.py`가 CI에서 수행하며,
	 * 이 함수는 **실제로 로드된 결과**에 대한 2차 방어선입니다.
	 * (CSV는 멀쩡한데 열 이름 불일치로 값이 안 들어온 경우 등)
	 */
	bool ValidateAll(TArray<FString>& OutErrors) const;

	/**
	 * 임의 디렉터리에서 다시 읽습니다. 테스트·툴 전용.
	 * @return 치명적 오류 없이 읽었으면 true.
	 */
	bool LoadFromDirectory(const FString& Directory, TArray<FString>& OutProblems);

	/** 기본 경로 = `<Project>/Config/DataTables`. */
	static FString GetDefaultDataDirectory();

private:
	/** CSV 한 장을 읽어 전이(transient) DataTable로 만듭니다. */
	UDataTable* LoadTable(const FString& Directory, const FString& TableName,
	                      const UScriptStruct* RowStruct, TArray<FString>& OutProblems);

	UPROPERTY(Transient) TObjectPtr<UDataTable> AircraftTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> ItemTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> ModuleTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> FlightTable;
};
