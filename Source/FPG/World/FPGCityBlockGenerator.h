// FPG — 절차적 도심 배경.
//
// 목적: 하늘 위 트인 공간만 있으면 속도·이동을 체감하기 어렵습니다.
// 랜드마크가 일정 간격으로 스쳐 지나가야 "내가 얼마나 빠른가"가 느껴집니다.
//
// M1은 프로그래머 아트가 허용되는 단계라(docs/13) 엔진 기본 도형(큐브·원기둥)을
// 절차적으로 배치합니다. .uasset은 코드로 만들 수 없으므로 이 방식이 지금
// 유일하게 가능한 "도시"입니다. 실제 아트는 M2 이후 Content/로 대체됩니다.
//
// 겸용: docs/02 §2.4가 "마천루"를 정적 지형 해저드로 이미 명시하고 있어,
// 이 건물에 충돌을 켜서 속도 체감용 배경이자 협곡 레이어 해저드를 겸합니다.
// 데미지 처리는 HealthComponent가 생긴 뒤 연결합니다 (FPGAircraftPawn.cpp TODO).
//
// CSV로 빼지 않은 이유: 이건 밸런스 수치(P5 대상)가 아니라 레벨 배치 파라미터입니다.
// 세션 간 공유될 필요가 없고, 레벨마다 다른 배치를 UE 관행대로 디테일 패널에서
// 직접 조정하는 편이 맞습니다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPGCityBlockGenerator.generated.h"

class UInstancedStaticMeshComponent;

UCLASS(meta = (DisplayName = "FPG City Block Generator"))
class FPG_API AFPGCityBlockGenerator : public AActor
{
	GENERATED_BODY()

public:
	AFPGCityBlockGenerator();

	/**
	 * 에디터에서 배치·이동·속성 변경 시마다 자동 호출됩니다.
	 * 같은 Seed → 항상 같은 배치(재현 가능한 레이아웃, 팀원과 스크린샷 공유에 유리).
	 */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	/** 이 값을 바꾸지 않는 한 배치는 항상 동일합니다. */
	UPROPERTY(EditAnywhere, Category = "FPG|City")
	int32 Seed = 20260812;

	/** 진행 방향(액터의 +X)으로 몇 블록을 세울지. */
	UPROPERTY(EditAnywhere, Category = "FPG|City", meta = (ClampMin = "1"))
	int32 RowCount = 40;

	/** 좌우 폭(액터의 ±Y)으로 몇 블록을 세울지. */
	UPROPERTY(EditAnywhere, Category = "FPG|City", meta = (ClampMin = "1"))
	int32 ColumnCount = 14;

	/** 블록 간격(cm). 기본 12,000 = 120m. */
	UPROPERTY(EditAnywhere, Category = "FPG|City")
	float CellSpacing = 12000.f;

	/** 격자가 너무 딱딱해 보이지 않도록 칸 안에서 흔드는 범위(cm). */
	UPROPERTY(EditAnywhere, Category = "FPG|City")
	float PositionJitter = 3000.f;

	UPROPERTY(EditAnywhere, Category = "FPG|City")
	float MinFootprint = 1800.f;   // 18m

	UPROPERTY(EditAnywhere, Category = "FPG|City")
	float MaxFootprint = 4200.f;   // 42m

	/** 건물 높이 범위(cm). 기본 30m~400m — docs/02 §2.4 협곡 레이어(0~1500m) 안에 들어옵니다. */
	UPROPERTY(EditAnywhere, Category = "FPG|City")
	float MinHeight = 3000.f;

	UPROPERTY(EditAnywhere, Category = "FPG|City")
	float MaxHeight = 40000.f;

	/** 원기둥 랜드마크 타워로 대체될 확률. 실루엣에 변화를 줘 랜드마크로 기억되게 합니다. */
	UPROPERTY(EditAnywhere, Category = "FPG|City", meta = (ClampMin = "0", ClampMax = "1"))
	float TowerChance = 0.12f;

	/** 각진 건물. */
	UPROPERTY(VisibleAnywhere, Category = "FPG|City")
	TObjectPtr<UInstancedStaticMeshComponent> Buildings;

	/** 원형 랜드마크 타워. */
	UPROPERTY(VisibleAnywhere, Category = "FPG|City")
	TObjectPtr<UInstancedStaticMeshComponent> Towers;

private:
	void GenerateCity();
};
