#include "Modes/FPGGameModeBase.h"
#include "Flight/FPGAircraftPawn.h"

AFPGGameModeBase::AFPGGameModeBase()
{
	DefaultPawnClass = AFPGAircraftPawn::StaticClass();

	// docs/16 AR2 — 싱글도 리슨 서버로 실행합니다.
	// 싱글을 네트워크와 무관하게 짜면 M3에서 재작성이 됩니다.
	bUseSeamlessTravel = true;
}
