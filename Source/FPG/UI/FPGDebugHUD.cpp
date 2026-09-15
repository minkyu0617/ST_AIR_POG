#include "UI/FPGDebugHUD.h"

#include "FPG.h"
#include "Combat/FPGHealthComponent.h"
#include "Combat/FPGWeaponSlotComponent.h"
#include "Core/FPGGameInstance.h"
#include "Core/Services/FPGEventBus.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Flight/FPGAircraftPawn.h"
#include "Flight/FPGFlightMovementComponent.h"
#include "Modes/FPGGameState.h"
#include "Modes/FPGPlayerState.h"
#include "POI/FPGPoiStation.h"
#include "EngineUtils.h"

namespace
{
	constexpr float PanelX = 40.f;
	constexpr float LineHeight = 22.f;
	constexpr float RadarRadius = 110.f;
	/** Radar shows 2 km around the craft (docs/05 §5.3). */
	constexpr float RadarRangeCm = 200000.f;
}

AFPGDebugHUD::AFPGDebugHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFPGDebugHUD::BeginPlay()
{
	Super::BeginPlay();

	if (const UFPGGameInstance* GameInstance = GetGameInstance<UFPGGameInstance>())
	{
		if (UFPGEventBus* Bus = GameInstance->GetEventBus())
		{
			// UI subscribes; gameplay never calls the HUD directly (docs/16 A4).
			Bus->OnToast.AddUObject(this, &AFPGDebugHUD::HandleToast);
		}
	}
}

void AFPGDebugHUD::HandleToast(const FString& Message)
{
	ToastMessage = Message;
	ToastRemaining = 3.5f;
}

void AFPGDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	AFPGAircraftPawn* Aircraft = Cast<AFPGAircraftPawn>(GetOwningPawn());

	DrawFlightPanel(Aircraft);
	DrawRadar(Aircraft);
	DrawCountdown();
	DrawToast();

	// Centre of the screen stays clear - HUD must never block obstacle reading (FPG.md P6).
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;
	DrawLine(CX - 14.f, CY, CX - 5.f, CY, FLinearColor::White, 1.5f);
	DrawLine(CX + 5.f, CY, CX + 14.f, CY, FLinearColor::White, 1.5f);
	DrawLine(CX, CY - 14.f, CX, CY - 5.f, FLinearColor::White, 1.5f);
	DrawLine(CX, CY + 5.f, CX, CY + 14.f, FLinearColor::White, 1.5f);
}

void AFPGDebugHUD::DrawFlightPanel(AFPGAircraftPawn* Aircraft)
{
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	float Y = 40.f;

	if (!Aircraft)
	{
		DrawText(TEXT("No aircraft possessed."), FLinearColor::Red, PanelX, Y, Font);
		return;
	}

	const UFPGFlightMovementComponent* Movement = Aircraft->GetFlightMovement();
	const UFPGHealthComponent* Health = Aircraft->GetHealth();
	const UFPGWeaponSlotComponent* Weapons = Aircraft->GetWeapons();
	const AFPGPlayerState* PlayerState = Aircraft->GetPlayerState<AFPGPlayerState>();

	if (Movement)
	{
		const TCHAR* StateText = TEXT("NORMAL");
		FLinearColor StateColour = FLinearColor::White;

		switch (Movement->GetFlightState())
		{
		case EFPGFlightState::Vector:    StateText = TEXT("VECTOR");    StateColour = FLinearColor(0.4f, 0.8f, 1.f); break;
		case EFPGFlightState::Stall:     StateText = TEXT("STALL");     StateColour = FLinearColor(1.f, 0.4f, 0.1f); break;
		case EFPGFlightState::Destroyed: StateText = TEXT("DESTROYED"); StateColour = FLinearColor::Red; break;
		default: break;
		}

		DrawText(FString::Printf(TEXT("SPEED   %6.0f uu/s   (%.0f km/h)"),
			Movement->GetSpeed(), Movement->GetSpeedKmh()), FLinearColor::White, PanelX, Y, Font);
		Y += LineHeight;

		DrawText(FString::Printf(TEXT("ALT     %6.0f m"), Movement->GetAltitudeMeters()),
			FLinearColor::White, PanelX, Y, Font);
		Y += LineHeight;

		DrawText(FString::Printf(TEXT("STATE   %s"), StateText), StateColour, PanelX, Y, Font);
		Y += LineHeight + 4.f;

		DrawText(TEXT("BOOST"), FLinearColor::White, PanelX, Y, Font);
		DrawBar(PanelX + 90.f, Y + 5.f, 180.f, 12.f, Movement->GetBoostPercent(),
			FLinearColor(0.2f, 0.7f, 1.f));
		Y += LineHeight;
	}

	if (Health)
	{
		const float Percent = Health->GetHealthPercent();
		const FLinearColor Colour = Percent > 0.6f ? FLinearColor::Green
			: (Percent > 0.3f ? FLinearColor::Yellow : FLinearColor::Red);

		DrawText(FString::Printf(TEXT("HP  %3.0f"), Health->GetHP()), Colour, PanelX, Y, Font);
		DrawBar(PanelX + 90.f, Y + 5.f, 180.f, 12.f, Percent, Colour);
		Y += LineHeight + 4.f;
	}

	if (Weapons)
	{
		const FString Slot = Weapons->GetSelectedItemId().IsNone()
			? TEXT("(empty)")
			: Weapons->GetSelectedItemId().ToString();

		DrawText(FString::Printf(TEXT("SLOT %d  %s%s"),
			Weapons->GetSelectedIndex(), *Slot,
			Weapons->IsOverheated() ? TEXT("  [OVERHEAT]") : TEXT("")),
			Weapons->IsOverheated() ? FLinearColor::Red : FLinearColor::White, PanelX, Y, Font);
		Y += LineHeight;

		DrawText(TEXT("HEAT"), FLinearColor::White, PanelX, Y, Font);
		DrawBar(PanelX + 90.f, Y + 5.f, 180.f, 12.f, Weapons->GetHeatPercent(),
			FLinearColor(1.f, 0.5f, 0.1f));
		Y += LineHeight + 8.f;
	}

	DrawText(FString::Printf(TEXT("DISTANCE  %.0f m"), Aircraft->GetDistanceTravelledMeters()),
		FLinearColor(0.7f, 1.f, 0.7f), PanelX, Y, Font);
	Y += LineHeight;

	if (PlayerState)
	{
		DrawText(FString::Printf(TEXT("CREDITS   %d"), PlayerState->GetSessionCredit()),
			FLinearColor(1.f, 0.9f, 0.4f), PanelX, Y, Font);
		Y += LineHeight;
	}

	Y += 8.f;
	DrawText(TEXT("W/S throttle  A/D roll  Space+WASD vector  Ctrl boost"),
		FLinearColor(0.6f, 0.6f, 0.6f), PanelX, Y, Font);
	Y += LineHeight;
	DrawText(TEXT("LMB fire  Q cycle slot  R restart"),
		FLinearColor(0.6f, 0.6f, 0.6f), PanelX, Y, Font);
}

void AFPGDebugHUD::DrawCountdown()
{
	const AFPGGameState* State = GetWorld() ? GetWorld()->GetGameState<AFPGGameState>() : nullptr;
	if (!State || !Canvas)
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.35f;

	switch (State->GetMatchPhase())
	{
	case EFPGMatchPhase::Boarding:
		DrawText(TEXT("BOARDING"), FLinearColor::White, CX - 50.f, CY, Font, 1.4f);
		break;

	case EFPGMatchPhase::Countdown:
	{
		const int32 Count = FMath::CeilToInt(State->GetCountdownRemaining());
		DrawText(Count > 0 ? FString::FromInt(Count) : TEXT("GO!"),
			FLinearColor::Yellow, CX - 12.f, CY, Font, 2.2f);
		break;
	}

	case EFPGMatchPhase::PostMatch:
		DrawText(TEXT("RUN OVER  -  press R"), FLinearColor::Red, CX - 110.f, CY, Font, 1.4f);
		break;

	default:
		break;
	}
}

void AFPGDebugHUD::DrawRadar(AFPGAircraftPawn* Aircraft)
{
	if (!Canvas || !Aircraft)
	{
		return;
	}

	// Bottom-left, mirroring the shipping minimap position (docs/05 §5.3).
	const float CX = 40.f + RadarRadius;
	const float CY = Canvas->ClipY - 40.f - RadarRadius;

	// Ring.
	const int32 Segments = 32;
	for (int32 Index = 0; Index < Segments; ++Index)
	{
		const float A0 = (2.f * PI * Index) / Segments;
		const float A1 = (2.f * PI * (Index + 1)) / Segments;
		DrawLine(CX + FMath::Cos(A0) * RadarRadius, CY + FMath::Sin(A0) * RadarRadius,
			CX + FMath::Cos(A1) * RadarRadius, CY + FMath::Sin(A1) * RadarRadius,
			FLinearColor(0.3f, 0.5f, 0.3f), 1.f);
	}

	// Own craft, always nose-up at the centre.
	DrawLine(CX, CY + 6.f, CX, CY - 8.f, FLinearColor::White, 2.f);

	const FVector Origin = Aircraft->GetActorLocation();
	const float Yaw = Aircraft->GetActorRotation().Yaw;

	for (TActorIterator<AFPGPoiStation> It(GetWorld()); It; ++It)
	{
		const AFPGPoiStation* Poi = *It;
		const FVector Delta = Poi->GetActorLocation() - Origin;
		const float PlanarDistance = FVector2D(Delta.X, Delta.Y).Size();

		// Rotate into craft-relative space so the radar is nose-up.
		const float Rad = FMath::DegreesToRadians(-Yaw);
		const float RX = Delta.X * FMath::Cos(Rad) - Delta.Y * FMath::Sin(Rad);
		const float RY = Delta.X * FMath::Sin(Rad) + Delta.Y * FMath::Cos(Rad);

		// Screen Y grows downward and forward should read as up.
		FVector2D Screen(RY / RadarRangeCm * RadarRadius, -RX / RadarRangeCm * RadarRadius);

		// Clamp out-of-range contacts to the rim so direction is still readable.
		if (Screen.Size() > RadarRadius)
		{
			Screen = Screen.GetSafeNormal() * RadarRadius;
		}

		const float RelativeAltitude = Delta.Z * 0.01f;
		const FLinearColor Colour = RelativeAltitude > 300.f ? FLinearColor(0.4f, 0.7f, 1.f)
			: (RelativeAltitude < -300.f ? FLinearColor(1.f, 0.85f, 0.3f) : FLinearColor::White);

		DrawRect(Colour, CX + Screen.X - 3.f, CY + Screen.Y - 3.f, 6.f, 6.f);

		// Absolute + relative altitude, because "6,200 m" alone does not say up or down (docs/05 §5.2).
		UFont* Font = GEngine ? GEngine->GetTinyFont() : nullptr;
		DrawText(FString::Printf(TEXT("%s %.0fm %+.0f"),
			*Poi->GetPoiId().ToString(),
			Poi->GetActorLocation().Z * 0.01f,
			RelativeAltitude),
			Colour, CX + Screen.X + 6.f, CY + Screen.Y - 6.f, Font);

		if (Poi->GetDockProgress() > 0.f)
		{
			DrawText(FString::Printf(TEXT("DOCKING %.0f%%"), Poi->GetDockProgress() * 100.f),
				FLinearColor::Green, CX - 40.f, CY - RadarRadius - 24.f, Font);
		}

		// Distance readout under the ring for the nearest station.
		if (PlanarDistance < RadarRangeCm)
		{
			DrawText(FString::Printf(TEXT("%.0f m"), PlanarDistance * 0.01f),
				Colour, CX + Screen.X + 6.f, CY + Screen.Y + 4.f, Font);
		}
	}
}

void AFPGDebugHUD::DrawToast()
{
	if (ToastRemaining <= 0.f || !Canvas)
	{
		return;
	}

	ToastRemaining -= GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;

	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	const float X = Canvas->ClipX * 0.5f - 210.f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 10.f, 24.f, 440.f, 30.f);
	DrawText(ToastMessage, FLinearColor::White, X, 32.f, Font);
}

void AFPGDebugHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Colour)
{
	DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 0.8f), X, Y, Width, Height);
	DrawRect(Colour, X, Y, Width * FMath::Clamp(Fraction, 0.f, 1.f), Height);
}
