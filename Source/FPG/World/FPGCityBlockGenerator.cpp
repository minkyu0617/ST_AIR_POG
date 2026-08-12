#include "World/FPGCityBlockGenerator.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AFPGCityBlockGenerator::AFPGCityBlockGenerator()
{
	PrimaryActorTick.bCanEverTick = false;   // 정적 배치. 매 프레임 갱신할 이유가 없습니다 (docs/16 §16.13)

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	Buildings = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Buildings"));
	Buildings->SetupAttachment(SceneRoot);
	Buildings->SetMobility(EComponentMobility::Static);
	Buildings->SetCollisionProfileName(TEXT("BlockAll"));   // docs/02 §2.4 정적 지형 해저드

	Towers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Towers"));
	Towers->SetupAttachment(SceneRoot);
	Towers->SetMobility(EComponentMobility::Static);
	Towers->SetCollisionProfileName(TEXT("BlockAll"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Buildings->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Towers->SetStaticMesh(CylinderMesh.Object);
	}
}

void AFPGCityBlockGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GenerateCity();
}

void AFPGCityBlockGenerator::GenerateCity()
{
	Buildings->ClearInstances();
	Towers->ClearInstances();

	UStaticMesh* CubeAsset = Buildings->GetStaticMesh();
	UStaticMesh* CylinderAsset = Towers->GetStaticMesh();
	if (!CubeAsset || !CylinderAsset)
	{
		return;   // 쿠킹 전 에디터 상태 등, 에셋 참조가 아직 없을 수 있습니다.
	}

	// BasicShapes 원본 크기를 기준으로 목표 치수까지의 배율을 역산합니다.
	// 엔진 기본 도형의 실제 크기를 가정하지 않고 항상 애셋에서 직접 읽습니다.
	const FVector CubeUnitSize = CubeAsset->GetBounds().BoxExtent * 2.f;
	const FVector CylinderUnitSize = CylinderAsset->GetBounds().BoxExtent * 2.f;

	const int32 Rows = FMath::Max(1, RowCount);
	const int32 Cols = FMath::Max(1, ColumnCount);

	FRandomStream Stream(Seed);

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Col = 0; Col < Cols; ++Col)
		{
			const float BaseX = Row * CellSpacing;
			const float BaseY = (Col - (Cols - 1) * 0.5f) * CellSpacing;

			const float JitterX = Stream.FRandRange(-PositionJitter, PositionJitter);
			const float JitterY = Stream.FRandRange(-PositionJitter, PositionJitter);

			const float Height = Stream.FRandRange(MinHeight, MaxHeight);
			const float Footprint = Stream.FRandRange(MinFootprint, MaxFootprint);

			// 건물 원점이 바닥 중심이 되도록, 절반 높이만큼 위로 올립니다.
			const FVector Location(BaseX + JitterX, BaseY + JitterY, Height * 0.5f);

			if (Stream.FRand() < TowerChance)
			{
				const float Radius = Footprint * 0.5f;
				const FVector Scale(
					Radius * 2.f / CylinderUnitSize.X,
					Radius * 2.f / CylinderUnitSize.Y,
					Height / CylinderUnitSize.Z);
				Towers->AddInstance(FTransform(FRotator::ZeroRotator, Location, Scale));
			}
			else
			{
				// 완전히 격자에 딱 맞으면 인공적으로 보입니다. 약간의 요잉만 흔듭니다.
				const FRotator Rotation(0.f, Stream.FRandRange(-15.f, 15.f), 0.f);
				const FVector Scale(
					Footprint / CubeUnitSize.X,
					Footprint / CubeUnitSize.Y,
					Height / CubeUnitSize.Z);
				Buildings->AddInstance(FTransform(Rotation, Location, Scale));
			}
		}
	}
}
