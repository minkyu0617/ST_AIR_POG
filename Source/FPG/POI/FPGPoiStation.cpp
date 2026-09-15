#include "POI/FPGPoiStation.h"

#include "FPG.h"
#include "Combat/FPGHealthComponent.h"
#include "Combat/FPGWeaponSlotComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/FPGGameInstance.h"
#include "Core/Services/FPGDataRegistry.h"
#include "Core/Services/FPGEventBus.h"
#include "Flight/FPGAircraftPawn.h"
#include "Flight/FPGFlightMovementComponent.h"
#include "Modes/FPGPlayerState.h"
#include "UObject/ConstructorHelpers.h"

AFPGPoiStation::AFPGPoiStation()
{
	PrimaryActorTick.bCanEverTick = true;

	DockZone = CreateDefaultSubobject<USphereComponent>(TEXT("DockZone"));
	SetRootComponent(DockZone);
	// 600 m radius. Crossing it at the 20000 uu/s (200 m/s) dock speed limit takes ~6 s,
	// comfortably longer than the 3 s transaction, so a slow pass-through actually completes.
	DockZone->SetSphereRadius(60000.f);
	DockZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DockZone->SetCollisionObjectType(ECC_WorldDynamic);
	DockZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	DockZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DockZone->SetGenerateOverlapEvents(true);

	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetupAttachment(DockZone);
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RingMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (RingMesh.Succeeded())
	{
		Marker->SetStaticMesh(RingMesh.Object);
		Marker->SetRelativeScale3D(FVector(90.f, 90.f, 3.f));
	}
}

void AFPGPoiStation::BeginPlay()
{
	Super::BeginPlay();

	DockZone->OnComponentBeginOverlap.AddDynamic(this, &AFPGPoiStation::OnDockZoneBeginOverlap);
	DockZone->OnComponentEndOverlap.AddDynamic(this, &AFPGPoiStation::OnDockZoneEndOverlap);
}

float AFPGPoiStation::GetDockProgress() const
{
	if (!DockingAircraft)
	{
		return 0.f;
	}

	const UFPGGameInstance* GameInstance = GetGameInstance<UFPGGameInstance>();
	const UFPGDataRegistry* Registry = GameInstance ? GameInstance->GetDataRegistry() : nullptr;
	const FFPGPoiRow* Row = Registry ? Registry->FindPoi(PoiId) : nullptr;
	const float DockTime = Row ? Row->DockTimeSec : 3.f;

	return DockTime > KINDA_SMALL_NUMBER ? FMath::Clamp(DockElapsed / DockTime, 0.f, 1.f) : 0.f;
}

void AFPGPoiStation::OnDockZoneBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*,
	int32, bool, const FHitResult&)
{
	if (AFPGAircraftPawn* Aircraft = Cast<AFPGAircraftPawn>(OtherActor))
	{
		DockingAircraft = Aircraft;
		DockElapsed = 0.f;
	}
}

void AFPGPoiStation::OnDockZoneEndOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	if (DockingAircraft == OtherActor)
	{
		DockingAircraft = nullptr;
		DockElapsed = 0.f;
	}
}

void AFPGPoiStation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!DockingAircraft)
	{
		return;
	}

	const UFPGGameInstance* GameInstance = GetGameInstance<UFPGGameInstance>();
	UFPGDataRegistry* Registry = GameInstance ? GameInstance->GetDataRegistry() : nullptr;
	const FFPGPoiRow* Row = Registry ? Registry->FindPoi(PoiId) : nullptr;
	if (!Row)
	{
		return;
	}

	const UFPGFlightMovementComponent* Movement = DockingAircraft->GetFlightMovement();
	if (!Movement)
	{
		return;
	}

	// Too fast to dock. The player has to give up speed to use a shop - that is the whole cost.
	if (Movement->GetSpeed() > Row->DockSpeedLimit)
	{
		DockElapsed = 0.f;
		return;
	}

	DockElapsed += DeltaTime;

	if (UFPGEventBus* Bus = GameInstance ? GameInstance->GetEventBus() : nullptr)
	{
		Bus->OnPoiDockProgress.Broadcast(PoiId, GetDockProgress());
	}

	if (DockElapsed >= Row->DockTimeSec)
	{
		CompleteTransaction(DockingAircraft);
		DockElapsed = 0.f;
		DockingAircraft = nullptr;
	}
}

void AFPGPoiStation::CompleteTransaction(AFPGAircraftPawn* Aircraft)
{
	const UFPGGameInstance* GameInstance = GetGameInstance<UFPGGameInstance>();
	UFPGDataRegistry* Registry = GameInstance ? GameInstance->GetDataRegistry() : nullptr;
	const FFPGPoiRow* Row = Registry ? Registry->FindPoi(PoiId) : nullptr;
	UFPGEventBus* Bus = GameInstance ? GameInstance->GetEventBus() : nullptr;
	if (!Row || !Aircraft)
	{
		return;
	}

	AFPGPlayerState* PlayerState = Aircraft->GetPlayerState<AFPGPlayerState>();
	UFPGHealthComponent* Health = Aircraft->GetHealth();

	FString Summary;

	switch (Row->Type)
	{
	case EFPGPoiType::RepairBay:
	{
		// Repair cost escalates per visit, so a repair bay is a resource you can exhaust,
		// not a free reset (docs/04 §4.1).
		const int32 Cost = FMath::RoundToInt(Row->BaseRepairCost * FMath::Pow(Row->RepairCostMultiplier, VisitCount));

		if (!Health || Health->GetHealthPercent() >= 1.f)
		{
			Summary = TEXT("Repair bay: hull already intact");
			break;
		}
		if (PlayerState && !PlayerState->TrySpendCredit(Cost))
		{
			Summary = FString::Printf(TEXT("Repair bay: need %d credits"), Cost);
			break;
		}

		Health->FullRepair();
		++VisitCount;
		Summary = FString::Printf(TEXT("Repaired to full for %d credits"), Cost);
		break;
	}

	case EFPGPoiType::WeaponShop:
	{
		const int32 Cost = Row->AmmoRefillCost;
		if (PlayerState && !PlayerState->TrySpendCredit(Cost))
		{
			Summary = FString::Printf(TEXT("Weapon shop: need %d credits"), Cost);
			break;
		}
		if (UFPGWeaponSlotComponent* Weapons = Aircraft->GetWeapons())
		{
			Weapons->RefillAmmo();
		}
		Summary = FString::Printf(TEXT("Rearmed for %d credits"), Cost);
		break;
	}

	case EFPGPoiType::AircraftStore:
	default:
		// The aircraft store needs the shop UI; that lands after M1.
		Summary = TEXT("Aircraft store: not implemented in M1");
		break;
	}

	UE_LOG(LogFPG, Log, TEXT("POI %s: %s"), *PoiId.ToString(), *Summary);

	if (Bus)
	{
		Bus->OnPoiTransaction.Broadcast(PoiId, Summary);
		Bus->Toast(Summary);
	}
}
