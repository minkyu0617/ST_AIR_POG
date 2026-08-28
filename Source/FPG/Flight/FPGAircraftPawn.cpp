#include "Flight/FPGAircraftPawn.h"

#include "Camera/CameraComponent.h"
#include "Combat/FPGHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/Services/FPGDataRegistry.h"
#include "Engine/GameInstance.h"
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
	Health = CreateDefaultSubobject<UFPGHealthComponent>(TEXT("Health"));

	// docs/16 AR2 — 싱글도 리슨 서버로 돌리므로 폰을 처음부터 복제 대상으로 둡니다.
	// 나중에 켜려 하면 "왜 클라이언트에 안 보이지"를 한참 헤매게 됩니다.
	bReplicates = true;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AFPGAircraftPawn::BeginPlay()
{
	Super::BeginPlay();

	// PlayerStart의 pitch/roll을 무시하고 항상 수평 비행 자세로 시작합니다.
	// 그대로 두면 스폰 지점의 회전을 물려받는데, PlayerStart가 아래를 보는
	// 각도로 놓여 있으면 카메라도 그대로 아래를 보게 됩니다 — "이동 중인지
	// 몰랐다"는 혼란의 원인. Yaw(진행 방향)만 스폰 값을 존중하고 Pitch/Roll은
	// 0으로 강제합니다.
	const FRotator SpawnRotator = GetActorRotation();
	const FRotator LevelRotator(0.f, SpawnRotator.Yaw, 0.f);
	SetActorRotation(LevelRotator);

	// DT_Aircraft·DT_Flight에서 읽어 주입합니다 (P5).
	// 실패해도 계속 진행합니다 — FFPGFlightParams의 C++ 기본값으로 날 수는 있어야
	// 데이터 문제로 개발이 멈추지 않습니다. 대신 레지스트리가 로그를 크게 남깁니다.
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const UFPGDataRegistry* Registry = GI->GetSubsystem<UFPGDataRegistry>())
		{
			FFPGFlightParams Params = Movement->GetParams();
			if (Registry->BuildFlightParams(AircraftId, Params))
			{
				Movement->SetParams(Params);
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("FPG: DT_Aircraft 에서 '%s' 를 찾지 못해 기본값으로 납니다 (P5 미적용 상태)"),
					*AircraftId.ToString());
			}

			// HP는 기체별(DT_Aircraft.BaseHP), 손상 효과는 공통(DT_Health)입니다.
			FFPGHealthTuning Tuning;
			Registry->BuildHealthTuning(Tuning);

			const FFPGAircraftRow* Row = Registry->FindAircraft(AircraftId);
			Health->InitializeFromData(Row ? Row->BaseHP : 0.f, Tuning);
		}
	}

	Health->OnDamageStateChanged.AddUObject(this, &AFPGAircraftPawn::HandleDamageStateChanged);
	Health->OnDeath.AddUObject(this, &AFPGAircraftPawn::HandleDeath);

	// ⚠️ SetParams() 다음에 호출해야 합니다. ResetToCruise()가 Params.CruiseSpeed를
	//    읽어 초기 스로틀을 역산하기 때문입니다.
	Movement->ResetToCruise(GetActorLocation(), LevelRotator.Quaternion());
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

		// docs/02 §2.4 정적 지형 충돌. 🔴 P3 — 데미지는 서버만 판정합니다.
		//
		// ⚠️ 매 프레임 계속 닿아 있으면 프레임마다 60씩 깎여 즉사합니다.
		//    무적 시간·넉백은 아직 없으므로, 충돌이 실제로 켜지는 시점
		//    (FPGCityBlockGenerator.bEnableCollision)에 반드시 함께 넣어야 합니다.
		//    지금은 도시 충돌이 기본 꺼짐이라 드러나지 않을 뿐입니다.
		if (HasAuthority() && Health && !Health->IsDead())
		{
			Health->ApplyDamage(Health->GetTuning().TerrainCollisionDamage, Hit.GetActor());
		}

		// 화면에 바로 표시합니다. "W를 눌러도 전진하지 않는다"류의 증상은
		// 원인이 충돌인지 입력인지 로그를 뒤지지 않고는 구분하기 어렵습니다.
		// 스폰 지점이 건물 속에 파묻히면 매 프레임 여기 걸리므로 즉시 드러납니다.
#if !UE_BUILD_SHIPPING
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				/*Key=*/ 7777, /*TimeToDisplay=*/ 0.f, FColor::Red,
				FString::Printf(TEXT("FPG: 충돌로 막힘 — %s"), *GetNameSafe(Hit.GetActor())));
		}
#endif

	}
}

void AFPGAircraftPawn::HandleDamageStateChanged(EDamageState NewState)
{
	// docs/02 §2.5 — 손상 60~31: 최고 속도 −10% / 심각 30~1: −25%, 선회율 −15%.
	//
	// Movement의 원본 Params는 건드리지 않고 배율만 갈아끼웁니다.
	// 정비소 POI에서 회복하면 배율이 1로 돌아가며 원래 성능이 정확히 복구됩니다.
	if (Movement && Health)
	{
		Movement->SetModifiers(Health->GetFlightModifiers());
	}
}

void AFPGAircraftPawn::HandleDeath(AActor* Killer)
{
	// 시뮬레이션을 멈춥니다. Step()이 Destroyed 상태에서 즉시 반환하므로
	// 기체는 마지막 위치에 정지합니다.
	if (Movement)
	{
		FFPGMoveState Dead = Movement->GetStateSnapshot();
		Dead.State = EFlightState::Destroyed;
		Movement->SetStateSnapshot(Dead);
	}

	// TODO(M1): docs/02 §2.5 — 스핀 → 폭발 → 파일럿 이젝션 연출.
	//           부활·기록 종료 판정은 GameMode의 몫입니다 (docs/16 §16.4).
	//           SingleEnduranceGameMode를 만들 때 OnDeath를 거기에 연결하십시오.
}
