// FPG — 모든 모드의 기반 GameMode.
//
// docs/16 §16.4 — GameMode는 **서버에만 존재**합니다.
// 규칙·승패 판정·부활·스폰만 담당하고, 그 외의 것을 여기 넣지 마십시오.
// 클라이언트에 있어야 할 판단이 여기로 오면 M3에서 전부 옮겨야 합니다.
//
// 모드별 파생(SpeedRace·Battle·SingleEndurance)은 접두어 없이 역할명을 씁니다.
// 모듈명이 이미 FPG라 /Script/FPG.SpeedRaceGameMode로 구분됩니다. (docs/16 §16.1)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPGGameModeBase.generated.h"

UCLASS()
class FPG_API AFPGGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPGGameModeBase();
};
