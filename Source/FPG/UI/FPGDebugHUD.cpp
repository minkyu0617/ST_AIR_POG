#include "UI/FPGDebugHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Flight/FPGAircraftPawn.h"
#include "Flight/FlightMovementComponent.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** 1 UU = 1cm 이므로 u/s → km/h 는 ×0.036 입니다. (D-16) */
	constexpr float UUPerSecToKmh = 0.036f;

	FLinearColor StateColor(EFlightState State)
	{
		switch (State)
		{
		case EFlightState::Stall:     return FLinearColor(1.f, 0.35f, 0.f);
		case EFlightState::Vector:    return FLinearColor(0.4f, 0.8f, 1.f);
		case EFlightState::Destroyed: return FLinearColor::Red;
		default:                      return FLinearColor::White;
		}
	}

	const TCHAR* StateText(EFlightState State)
	{
		switch (State)
		{
		case EFlightState::Stall:     return TEXT("STALL");
		case EFlightState::Vector:    return TEXT("VECTOR");
		case EFlightState::Destroyed: return TEXT("DOWN");
		default:                      return TEXT("NORMAL");
		}
	}
}

AFPGDebugHUD::AFPGDebugHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFPGDebugHUD::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Shipping 에서는 아예 켜지지 않게 합니다. 개발 계측이 출시 빌드에
	// 남는 것은 docs/10 §10.7 릴리스 체크리스트의 "치트 콘솔 비활성"과 같은 종류의 사고입니다.
#if UE_BUILD_SHIPPING
	bVisible = false;
#endif
}

void AFPGDebugHUD::FPGToggleDebugHUD()
{
	bVisible = !bVisible;
}

UFlightMovementComponent* AFPGDebugHUD::FindFlightMovement() const
{
	const APlayerController* PC = GetOwningPlayerController();
	if (!PC) { return nullptr; }

	const AFPGAircraftPawn* Aircraft = Cast<AFPGAircraftPawn>(PC->GetPawn());
	return Aircraft ? Aircraft->GetFlightMovement() : nullptr;
}

float AFPGDebugHUD::GetUIScale() const
{
	if (!Canvas) { return 1.f; }
	// 1080p 를 기준으로 비례. 너무 작아지거나 커지지 않게 묶습니다.
	return FMath::Clamp(Canvas->SizeY / 1080.f, 0.8f, 2.5f);
}

void AFPGDebugHUD::DrawBar(float X, float Y, float Width, float Ratio, const FLinearColor& Color)
{
	const float Scale = GetUIScale();
	const float Height = 8.f * Scale;

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), X, Y, Width, Height);
	DrawRect(Color, X, Y, Width * FMath::Clamp(Ratio, 0.f, 1.f), Height);
}

void AFPGDebugHUD::DrawFlightPanel(const UFlightMovementComponent& Movement, float PanelX, float PanelY)
{
	const float Scale = GetUIScale();
	const float LineHeight = 20.f * Scale;
	const float BarWidth = 180.f * Scale;
	float Y = PanelY;

	const FFPGFlightParams& Params = Movement.GetParams();
	const float Speed = Movement.GetSpeed();

	// ── 속도 ─────────────────────────────────────────────────
	// km/h 를 앞에 둡니다. docs/02 가 목표 체감을 km/h 로 적어 두었고
	// (순항 648km/h), 사람이 판단하기도 이쪽이 쉽습니다.
	DrawText(
		FString::Printf(TEXT("SPD  %.0f km/h   (%.0f u/s)"), Speed * UUPerSecToKmh, Speed),
		FLinearColor::White, PanelX, Y, nullptr, Scale);
	Y += LineHeight;

	// 최대 속도 대비 비율. 부스트 중에는 1을 넘을 수 있어 BoostSpeed 기준으로 잡습니다.
	DrawBar(PanelX, Y, BarWidth, Speed / FMath::Max(Params.BoostSpeed, 1.f), FLinearColor(0.3f, 0.9f, 0.4f));
	Y += LineHeight;

	// ── 고도 ─────────────────────────────────────────────────
	const float AltitudeM = Movement.GetAltitudeMeters();
	const bool bOverCeiling = AltitudeM > Params.AltitudeCeilingM;
	DrawText(
		FString::Printf(TEXT("ALT  %.0f m%s"), AltitudeM,
			bOverCeiling ? TEXT("   [고도 상한 초과 — 출력 감소]") : TEXT("")),
		bOverCeiling ? FLinearColor(1.f, 0.4f, 0.f) : FLinearColor::White,
		PanelX, Y, nullptr, Scale);
	Y += LineHeight;

	// ── 스로틀 ───────────────────────────────────────────────
	// D-04(Space 홀드 중 자동 스로틀 유지)가 실제로 동작하는지 눈으로 확인하는 지점입니다.
	DrawText(FString::Printf(TEXT("THR  %.0f%%"), Movement.GetThrottle() * 100.f),
		FLinearColor::White, PanelX, Y, nullptr, Scale);
	Y += LineHeight;
	DrawBar(PanelX, Y, BarWidth, Movement.GetThrottle(), FLinearColor(0.9f, 0.8f, 0.2f));
	Y += LineHeight;

	// ── 부스트 ───────────────────────────────────────────────
	const float BoostRemaining = Movement.GetBoostRemaining();
	const float BoostCooldown = Movement.GetBoostCooldownRemaining();

	if (BoostRemaining > 0.f)
	{
		DrawText(FString::Printf(TEXT("BOOST  %.1fs"), BoostRemaining),
			FLinearColor(0.4f, 0.8f, 1.f), PanelX, Y, nullptr, Scale);
		Y += LineHeight;
		DrawBar(PanelX, Y, BarWidth, BoostRemaining / FMath::Max(Params.BoostDuration, 0.01f),
			FLinearColor(0.4f, 0.8f, 1.f));
	}
	else if (BoostCooldown > 0.f)
	{
		DrawText(FString::Printf(TEXT("BOOST  쿨다운 %.1fs"), BoostCooldown),
			FLinearColor(0.6f, 0.6f, 0.6f), PanelX, Y, nullptr, Scale);
		Y += LineHeight;
		DrawBar(PanelX, Y, BarWidth, 1.f - (BoostCooldown / FMath::Max(Params.BoostCooldown, 0.01f)),
			FLinearColor(0.45f, 0.45f, 0.45f));
	}
	else
	{
		DrawText(TEXT("BOOST  준비됨"), FLinearColor(0.4f, 0.8f, 1.f), PanelX, Y, nullptr, Scale);
		Y += LineHeight;
		DrawBar(PanelX, Y, BarWidth, 1.f, FLinearColor(0.4f, 0.8f, 1.f));
	}
	Y += LineHeight * 1.5f;

	// ── 자세 · 상태 ──────────────────────────────────────────
	const FRotator Attitude = Movement.GetStateSnapshot().Rotation.Rotator();
	DrawText(FString::Printf(TEXT("PITCH %+.0f°   ROLL %+.0f°"), Attitude.Pitch, Attitude.Roll),
		FLinearColor(0.8f, 0.8f, 0.8f), PanelX, Y, nullptr, Scale);
	Y += LineHeight;

	const EFlightState State = Movement.GetFlightState();
	DrawText(FString::Printf(TEXT("STATE %s"), StateText(State)),
		StateColor(State), PanelX, Y, nullptr, Scale);
}

void AFPGDebugHUD::DrawHUD()
{
	Super::DrawHUD();

#if !UE_BUILD_SHIPPING
	if (!bVisible || !Canvas) { return; }

	const UFlightMovementComponent* Movement = FindFlightMovement();
	if (!Movement) { return; }

	const float Scale = GetUIScale();

	// docs/09 §9.4 — 화면 중앙 60%는 비웁니다. 좌하단에만 그립니다.
	// 패널 높이를 대략 잡아 하단에서 띄웁니다.
	const float PanelX = 24.f * Scale;
	const float PanelHeight = 220.f * Scale;
	const float PanelY = Canvas->SizeY - PanelHeight - (24.f * Scale);

	DrawFlightPanel(*Movement, PanelX, PanelY);

	// 이게 실제 HUD가 아님을 화면에서도 분명히 합니다.
	DrawText(TEXT("[개발용 오버레이 — FPG.ToggleDebugHUD 로 토글]"),
		FLinearColor(0.5f, 0.5f, 0.5f),
		PanelX, Canvas->SizeY - (16.f * Scale), nullptr, Scale * 0.85f);
#endif
}
