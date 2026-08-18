#include "Modes/FPGGameModeBase.h"
#include "Flight/FPGAircraftPawn.h"
#include "UI/FPGDebugHUD.h"

AFPGGameModeBase::AFPGGameModeBase()
{
	DefaultPawnClass = AFPGAircraftPawn::StaticClass();

	// ⚠️ 실제 HUD(docs/09 §9.4)가 아니라 개발용 계측 오버레이입니다.
	//    UMG 위젯이 생기는 M2에 교체하십시오. Shipping 에서는 그리지 않습니다.
	HUDClass = AFPGDebugHUD::StaticClass();

	// docs/16 AR2 — 싱글도 리슨 서버로 실행합니다.
	// 싱글을 네트워크와 무관하게 짜면 M3에서 재작성이 됩니다.
	bUseSeamlessTravel = true;
}
