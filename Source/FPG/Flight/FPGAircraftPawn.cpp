#include "Flight/FPGAircraftPawn.h"

#include "FPG.h"
#include "Camera/CameraComponent.h"
#include "Combat/FPGHealthComponent.h"
#include "Combat/FPGWeaponSlotComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/FPGGameInstance.h"
#include "Core/Services/FPGDataRegistry.h"
#include "Core/Services/FPGEventBus.h"
#include "Flight/FPGFlightMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

AFPGAircraftPawn::AFPGAircraftPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	// The root is a bare collision sphere. It is what the movement sweep tests against,
	// and it stays axis-aligned with the actor so forward is always +X.
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->SetSphereRadius(400.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_Pawn);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);

	// Visual only. Programmer art: an engine cone pitched to point down +X.
	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(Collision);
	Hull->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh.Succeeded())
	{
		Hull->SetStaticMesh(ConeMesh.Object);
		Hull->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
		Hull->SetRelativeScale3D(FVector(0.4f, 0.4f, 1.2f));
	}

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Collision);
	SpringArm->TargetArmLength = 1400.f;
	SpringArm->SocketOffset = FVector(0.f, 0.f, 350.f);
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	// Camera follows only part of the roll; full roll makes the horizon unreadable and causes
	// motion sickness (docs/11 §11.4).
	SpringArm->bInheritRoll = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 8.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	FlightMovement = CreateDefaultSubobject<UFPGFlightMovementComponent>(TEXT("FlightMovement"));
	Health = CreateDefaultSubobject<UFPGHealthComponent>(TEXT("Health"));
	Weapons = CreateDefaultSubobject<UFPGWeaponSlotComponent>(TEXT("Weapons"));
}

void AFPGAircraftPawn::BeginPlay()
{
	Super::BeginPlay();

	if (FlightMovement)
	{
		FlightMovement->OnCrashed.AddUObject(this, &AFPGAircraftPawn::HandleCrashed);
	}
	if (Health)
	{
		Health->OnDied.AddUObject(this, &AFPGAircraftPawn::HandleDied);
	}
}

void AFPGAircraftPawn::ApplyAircraftData(FName InAircraftId)
{
	UFPGGameInstance* GameInstance = GetGameInstance<UFPGGameInstance>();
	UFPGDataRegistry* Registry = GameInstance ? GameInstance->GetDataRegistry() : nullptr;
	if (!Registry)
	{
		UE_LOG(LogFPG, Error, TEXT("ApplyAircraftData: no data registry."));
		return;
	}

	const FFPGAircraftRow* Row = Registry->FindAircraft(InAircraftId);
	if (!Row)
	{
		UE_LOG(LogFPG, Error, TEXT("ApplyAircraftData: unknown aircraft '%s'."), *InAircraftId.ToString());
		return;
	}

	AircraftId = InAircraftId;

	if (FlightMovement) { FlightMovement->InitializeFromRow(*Row); }
	if (Health)         { Health->InitializeHealth(Row->BaseHP); }
	if (Weapons)        { Weapons->InitializeSlots(Registry, Row->ExtraItemSlots); }

	if (FlightMovement)
	{
		// Start parked on the runway; the countdown releases the throttle.
		FlightMovement->ResetTo(GetActorTransform(), 0.f);
	}

	UE_LOG(LogFPG, Log, TEXT("Aircraft '%s' applied (HP %.0f, max %.0f uu/s)."),
		*InAircraftId.ToString(), Row->BaseHP, Row->MaxSpeed);
}

void AFPGAircraftPawn::ApplyMove(const FFPGMove& Move)
{
	if (!FlightMovement)
	{
		return;
	}

	FFPGMove Effective = Move;
	if (bInputLocked)
	{
		// Countdown: time still advances so boost recharges, but the craft ignores the stick.
		Effective.Buttons = 0;
	}

	const FVector Before = FlightMovement->GetState().Location;
	FlightMovement->ApplyMove(Effective);
	const FVector After = FlightMovement->GetState().Location;

	if (!bInputLocked)
	{
		DistanceTravelledCm += FVector::Dist(Before, After);
	}

	if (Move.Has(EFPGInputFlag::Fire) && !bInputLocked)
	{
		FireSelected();
	}
}

void AFPGAircraftPawn::FireSelected()
{
	if (Weapons && !bInputLocked && Health && !Health->IsDead())
	{
		Weapons->TryFire();
	}
}

void AFPGAircraftPawn::CycleItemSlot()
{
	if (Weapons && !bInputLocked)
	{
		Weapons->CycleSlot();
	}
}

void AFPGAircraftPawn::HandleCrashed(const FHitResult& Hit)
{
	if (!Health || Health->IsDead())
	{
		return;
	}

	UE_LOG(LogFPG, Log, TEXT("Crashed into %s."), *GetNameSafe(Hit.GetActor()));
	Health->ApplyDamage(CrashDamage, nullptr);
}

void AFPGAircraftPawn::HandleDied()
{
	if (FlightMovement)
	{
		FlightMovement->MarkDestroyed();
	}
	bInputLocked = true;

	if (UFPGGameInstance* GameInstance = GetGameInstance<UFPGGameInstance>())
	{
		if (UFPGEventBus* Bus = GameInstance->GetEventBus())
		{
			Bus->OnDeath.Broadcast(this);
		}
	}
}
