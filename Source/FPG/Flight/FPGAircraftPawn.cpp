#include "Flight/FPGAircraftPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Flight/FlightMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

AFPGAircraftPawn::AFPGAircraftPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	SetRootComponent(Hull);
	Hull->SetCollisionProfileName(TEXT("Pawn"));

	// 프로그래머 아트. M1은 손맛만 봅니다. (docs/13 M1 "프로그래머 아트 허용")
	// 원뿔을 눕혀 기수 방향(+X)을 알아볼 수 있게 합니다.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh.Succeeded())
	{
		Hull->SetStaticMesh(ConeMesh.Object);
		Hull->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
		Hull->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.2f));
	}

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Hull);
	SpringArm->TargetArmLength = 900.f;
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 250.f));
	SpringArm->bDoCollisionTest = false;   // 지형에 카메라가 끼면 고속 비행에서 시야가 튑니다

	// 카메라 지연이 곧 속도감입니다. 없으면 기체가 화면에 못박힌 듯 보입니다.
	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraLagSpeed = 8.f;
	SpringArm->CameraRotationLagSpeed = 6.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->FieldOfView = 95.f;   // docs/03 §3.5의 FOV 70~110 범위 안

	Movement = CreateDefaultSubobject<UFlightMovementComponent>(TEXT("FlightMovement"));

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AFPGAircraftPawn::BeginPlay()
{
	Super::BeginPlay();

	// TODO(M1): DT_Aircraft·DT_Flight에서 읽어 SetParams()로 주입해야 합니다. (P5)
	//           FPGDataRegistry가 생기기 전까지는 FFPGFlightParams의 기본값으로 납니다.
	Movement->ResetToCruise(GetActorLocation(), GetActorQuat());
}

void AFPGAircraftPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DeltaSeconds <= 0.f)
	{
		return;
	}

	SimTime += DeltaSeconds;

	// ── 1. 입력 수집 ─────────────────────────────────────────
	FFPGMove Move;
	Move.ClientTimestamp = SimTime;
	Move.DeltaTime = DeltaSeconds;

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		Move.InputFlags = UFPGInputHandler::BuildInputFlags(*PC, KeyBindings);
	}

	// ── 2. 시뮬레이션 ────────────────────────────────────────
	// M3에서는 이 사이에 "PendingMoves에 보관 + 서버로 전송"이 들어갑니다.
	// SimulateMove() 자체는 그대로 둡니다. (docs/16 §16.5)
	Movement->SimulateMove(Move, DeltaSeconds);

	// ── 3. 결과를 액터에 반영 ────────────────────────────────
	// 충돌은 순수 함수 밖, 즉 여기서 처리합니다.
	const FFPGMoveState Simulated = Movement->GetStateSnapshot();

	FHitResult Hit;
	SetActorLocationAndRotation(Simulated.Location, Simulated.Rotation, /*bSweep=*/true, &Hit);

	if (Hit.bBlockingHit)
	{
		// 스윕이 막히면 액터는 충돌 지점에 멈추는데 시뮬레이션은 그걸 모릅니다.
		// 그대로 두면 둘이 계속 벌어져 다음 프레임부터 지형을 뚫고 갑니다.
		// 실제 위치를 시뮬레이션에 되먹여 둘을 다시 붙입니다.
		FFPGMoveState Corrected = Simulated;
		Corrected.Location = GetActorLocation();
		Movement->SetStateSnapshot(Corrected);

		// TODO(M1): docs/02 §2.4 — 지형 충돌은 즉사 또는 HP -60.
		//           HealthComponent가 생기면 여기서 데미지를 넘깁니다.
	}
}
