// M1 stand-in for the real HUD. Canvas only - no UMG assets, so everything stays in text
// and the vertical slice is testable without any editor authoring (docs/09 §9.4).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FPGDebugHUD.generated.h"

class AFPGAircraftPawn;

UCLASS()
class FPG_API AFPGDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	AFPGDebugHUD();

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

private:
	void DrawFlightPanel(AFPGAircraftPawn* Aircraft);
	void DrawCountdown();
	void DrawRadar(AFPGAircraftPawn* Aircraft);
	void DrawToast();
	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Colour);

	void HandleToast(const FString& Message);

	FString ToastMessage;
	float ToastRemaining = 0.f;
};
