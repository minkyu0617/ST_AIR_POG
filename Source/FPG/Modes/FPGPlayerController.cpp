#include "Modes/FPGPlayerController.h"

#include "FPG.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Flight/FPGAircraftPawn.h"
#include "Flight/FPGFlightTypes.h"
#include "GameFramework/GameModeBase.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	UInputAction* MakeBoolAction(UObject* Outer, const TCHAR* Name)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = EInputActionValueType::Boolean;
		return Action;
	}
}

AFPGPlayerController::AFPGPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFPGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BuildInputActions();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}

	bShowMouseCursor = false;
}

void AFPGPlayerController::BuildInputActions()
{
	if (MappingContext)
	{
		return;
	}

	IA_Accelerate = MakeBoolAction(this, TEXT("IA_Accelerate"));
	IA_Decelerate = MakeBoolAction(this, TEXT("IA_Decelerate"));
	IA_RollLeft   = MakeBoolAction(this, TEXT("IA_RollLeft"));
	IA_RollRight  = MakeBoolAction(this, TEXT("IA_RollRight"));
	IA_VectorMod  = MakeBoolAction(this, TEXT("IA_VectorMod"));
	IA_Boost      = MakeBoolAction(this, TEXT("IA_Boost"));
	IA_Fire       = MakeBoolAction(this, TEXT("IA_Fire"));
	IA_CycleItem  = MakeBoolAction(this, TEXT("IA_CycleItem"));
	IA_Restart    = MakeBoolAction(this, TEXT("IA_Restart"));

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_FPGDefault"));

	// docs/03 confirmed layout.
	MappingContext->MapKey(IA_Accelerate, EKeys::W);
	MappingContext->MapKey(IA_Decelerate, EKeys::S);
	MappingContext->MapKey(IA_RollLeft,   EKeys::A);
	MappingContext->MapKey(IA_RollRight,  EKeys::D);
	MappingContext->MapKey(IA_VectorMod,  EKeys::SpaceBar);
	MappingContext->MapKey(IA_Boost,      EKeys::LeftControl);
	MappingContext->MapKey(IA_Fire,       EKeys::LeftMouseButton);

	// D-02: item cycling is Q, not Alt. Alt+Tab would minimise the game mid-fight.
	// Alt stays available as a secondary binding for anyone who wants the original layout.
	MappingContext->MapKey(IA_CycleItem, EKeys::Q);
	MappingContext->MapKey(IA_CycleItem, EKeys::LeftAlt);
	MappingContext->MapKey(IA_CycleItem, EKeys::MouseScrollDown);

	// M1 testing convenience, not a shipping binding.
	MappingContext->MapKey(IA_Restart, EKeys::R);

	// Gamepad (docs/03 §3.4) - Steam Deck needs this working from the start.
	MappingContext->MapKey(IA_Accelerate, EKeys::Gamepad_RightTrigger);
	MappingContext->MapKey(IA_Decelerate, EKeys::Gamepad_LeftTrigger);
	MappingContext->MapKey(IA_RollLeft,   EKeys::Gamepad_DPad_Left);
	MappingContext->MapKey(IA_RollRight,  EKeys::Gamepad_DPad_Right);
	MappingContext->MapKey(IA_VectorMod,  EKeys::Gamepad_LeftShoulder);
	MappingContext->MapKey(IA_Fire,       EKeys::Gamepad_RightShoulder);
	MappingContext->MapKey(IA_Boost,      EKeys::Gamepad_FaceButton_Bottom);
	MappingContext->MapKey(IA_CycleItem,  EKeys::Gamepad_FaceButton_Top);
}

void AFPGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	BuildInputActions();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Input)
	{
		UE_LOG(LogFPG, Error,
			TEXT("EnhancedInputComponent missing. Check DefaultInputComponentClass in Config/DefaultEngine.ini."));
		return;
	}

	// Payload binding keeps this to two handlers instead of eighteen.
	auto BindHeld = [this, Input](UInputAction* Action, EFPGInputFlag::Type Flag)
	{
		const uint8 Bits = static_cast<uint8>(Flag);
		Input->BindAction(Action, ETriggerEvent::Triggered, this, &AFPGPlayerController::SetFlag, Bits);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AFPGPlayerController::ClearFlag, Bits);
		Input->BindAction(Action, ETriggerEvent::Canceled, this, &AFPGPlayerController::ClearFlag, Bits);
	};

	BindHeld(IA_Accelerate, EFPGInputFlag::Accelerate);
	BindHeld(IA_Decelerate, EFPGInputFlag::Decelerate);
	BindHeld(IA_RollLeft,   EFPGInputFlag::RollLeft);
	BindHeld(IA_RollRight,  EFPGInputFlag::RollRight);
	BindHeld(IA_VectorMod,  EFPGInputFlag::VectorMod);
	BindHeld(IA_Boost,      EFPGInputFlag::Boost);
	BindHeld(IA_Fire,       EFPGInputFlag::Fire);

	Input->BindAction(IA_CycleItem, ETriggerEvent::Started, this, &AFPGPlayerController::OnCycleItem);
	Input->BindAction(IA_Restart, ETriggerEvent::Started, this, &AFPGPlayerController::OnRestart);
}

void AFPGPlayerController::SetFlag(uint8 Flag)
{
	InputFlags |= Flag;
}

void AFPGPlayerController::ClearFlag(uint8 Flag)
{
	InputFlags &= static_cast<uint8>(~Flag);
}

void AFPGPlayerController::OnCycleItem()
{
	if (AFPGAircraftPawn* Aircraft = GetAircraft())
	{
		Aircraft->CycleItemSlot();
	}
}

void AFPGPlayerController::OnRestart()
{
	UE_LOG(LogFPG, Log, TEXT("Restart requested."));
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
}

AFPGAircraftPawn* AFPGPlayerController::GetAircraft() const
{
	return Cast<AFPGAircraftPawn>(GetPawn());
}

void AFPGPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ElapsedTime += DeltaTime;

	if (AFPGAircraftPawn* Aircraft = GetAircraft())
	{
		FFPGMove Move;
		Move.ClientTimestamp = ElapsedTime;
		Move.DeltaTime = DeltaTime;
		Move.Buttons = InputFlags;

		Aircraft->ApplyMove(Move);
	}
}
