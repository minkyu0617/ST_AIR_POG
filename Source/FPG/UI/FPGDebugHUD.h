// FPG — 개발용 계측 오버레이.
//
// ⚠️ **이것은 docs/09 §9.4의 실제 HUD가 아닙니다.**
//    실제 HUD는 UMG 위젯(.uasset)이 필요해 M2 이후에 만듭니다.
//    이 클래스는 손맛 튜닝 중 수치를 눈으로 확인하기 위한 개발 전용 오버레이이며,
//    Shipping 빌드에서는 아무것도 그리지 않습니다.
//
// 왜 필요한가:
//   속도·고도·스로틀이 안 보이면 손맛 피드백이 "빠른 것 같다" 수준에 머뭅니다.
//   M1의 통과 기준이 손맛이므로(P1), 튜닝할 값을 눈으로 볼 수 있어야 합니다.
//   특히 D-16(단위 100배 오류) 같은 문제는 숫자를 봤다면 즉시 드러났을 것입니다.
//
// docs/09 §9.4의 설계 원칙은 지킵니다:
//   "화면 중앙 60%는 항상 비워 둡니다. 비행 게임에서 HUD가 시야를 가리면
//    장애물 회피가 불가능해지고, 이는 곧 '조작이 구리다'는 리뷰로 돌아옵니다."
//   → 좌하단·우하단 모서리에만 그립니다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FPGDebugHUD.generated.h"

class UFlightMovementComponent;

UCLASS()
class FPG_API AFPGDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	AFPGDebugHUD();

	virtual void DrawHUD() override;
	virtual void PostInitializeComponents() override;

	/** 콘솔 명령 `FPG.ToggleDebugHUD` 로도 끌 수 있게 합니다. */
	UFUNCTION(Exec)
	void FPGToggleDebugHUD();

protected:
	/** 스크린샷·영상 촬영 시 끌 수 있도록. */
	UPROPERTY(EditAnywhere, Category = "FPG|Debug")
	bool bVisible = true;

private:
	UFlightMovementComponent* FindFlightMovement() const;

	void DrawFlightPanel(const UFlightMovementComponent& Movement, float PanelX, float PanelY);
	void DrawBar(float X, float Y, float Width, float Ratio, const FLinearColor& Color);

	/** 화면 크기에 비례해 글자 크기를 맞춥니다. 4K에서 개미만 해지지 않도록. */
	float GetUIScale() const;
};
