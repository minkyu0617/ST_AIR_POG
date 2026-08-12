// FPG — 절차적 도심 배경.
//
// 목적: 하늘 위 트인 공간만 있으면 속도·이동을 체감하기 어렵습니다.
// 랜드마크가 일정 간격으로 스쳐 지나가야 "내가 얼마나 빠른가"가 느껴집니다.
//
// M1은 프로그래머 아트가 허용되는 단계라(docs/13) 엔진 기본 도형(큐브·원기둥)을
// 절차적으로 배치합니다. .uasset은 코드로 만들 수 없으므로 이 방식이 지금
// 유일하게 가능한 "도시"입니다. 실제 아트는 M2 이후 Content/로 대체됩니다.
//
// 충돌은 기본 꺼짐입니다 (bEnableCollision). 이 격자는 순수 배경용으로 배치돼
// docs/02 §2.4의 인지 거리 규칙을 지키지 않으므로, 충돌을 켜면 곳곳에서
// 불시에 부딪힙니다. docs/02 §2.4가 "마천루"를 정적 지형 해저드로 이미
// 명시해 두었으니, 실제 해저드로 쓰려면 간격·인지 거리를 그 규칙에 맞춰
// 다시 잡고 bEnableCollision을 켜십시오. 그 시점에 데미지 처리를
// HealthComponent와 연결합니다 (FPGAircraftPawn.cpp TODO).
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

	/**
	 * 이 액터 원점(=보통 PlayerStart를 놓는 자리) 주변에는 건물을 세우지 않는 반경(cm).
	 *
	 * 격자 Row 0이 원래 원점에 바로 붙어 있어, 생성기를 스폰 지점 근처에 두면
	 * 기체가 건물 속에서 스폰될 수 있습니다.
	 *
	 * 기본 30,000(300m)은 docs/02 §2.4의 장애물 인지 거리 규칙(순항 속도 기준
	 * 최소 1.2초 전 인지 가능 = 약 21,600cm)에 여유를 더한 값입니다. `bEnableCollision`을
	 * 켜서 실제 장애물 코스로 쓸 때는 이보다 더 늘려야 할 수 있습니다.
	 */
	UPROPERTY(EditAnywhere, Category = "FPG|City")
	float ClearRadius = 30000.f;

	/**
	 * 건물에 충돌을 줄지 여부. **기본 꺼짐 — 시각 전용 배경입니다.**
	 *
	 * 격자 간격(120m)·건물 폭(최대 42m)이 docs/02 §2.4의 "회피 가능함이 항상
	 * 먼저 보여야 한다" 규칙에 맞춰 배치된 것이 아닙니다. 순항 속도(180m/s)로
	 * 촘촘한 격자를 그대로 통과하려 하면 인지 거리를 못 채우는 배치가 곳곳에
	 * 생겨, 원래 목적("속도 체감용 배경")과 다르게 장애물 코스가 되어 버립니다.
	 *
	 * docs/02 §2.4의 "정적 지형(마천루)" 해저드를 실제로 구현할 때는 이걸 켜되,
	 * 간격·인지 거리 규칙에 맞춰 파라미터를 다시 잡아야 합니다.
	 */
	UPROPERTY(EditAnywhere, Category = "FPG|City")
	bool bEnableCollision = false;

	/** 각진 건물. */
	UPROPERTY(VisibleAnywhere, Category = "FPG|City")
	TObjectPtr<UInstancedStaticMeshComponent> Buildings;

	/** 원형 랜드마크 타워. */
	UPROPERTY(VisibleAnywhere, Category = "FPG|City")
	TObjectPtr<UInstancedStaticMeshComponent> Towers;

private:
	void GenerateCity();
};
