// FPG — 키 입력을 FFPGMove의 비트 플래그로 변환합니다.
//
// docs/03 §3.1(확정 키 매핑) · §3.3(입력 상태 머신) · §3.5(리바인딩 필수)
//
// 왜 별도 파일인가:
//   이동 시뮬레이션은 "어떤 키를 눌렀는가"를 알면 안 됩니다. InputFlags만 압니다.
//   그래야 게임패드·리플레이·봇 AI가 같은 SimulateMove()를 그대로 쓸 수 있습니다.
//   이 파일이 그 경계입니다.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "FPGInputHandler.generated.h"

class APlayerController;

/**
 * 키 바인딩 표.
 *
 * docs/03 §3.5 — "전체 키 리바인딩 필수 (Steam 리뷰 감점 요인 1순위)".
 * 그래서 키를 코드 곳곳에 흩지 않고 여기 한 곳에 모읍니다.
 * `Config`라 `DefaultInput.ini`나 사용자 설정으로 덮어쓸 수 있습니다.
 */
USTRUCT(BlueprintType)
struct FFPGKeyBindings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey Thrust    = EKeys::W;
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey Brake     = EKeys::S;
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey RollLeft  = EKeys::A;
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey RollRight = EKeys::D;

	/** 벡터 기동 모디파이어. 홀드 방식입니다. (docs/03 C-2 · D-04) */
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey Vector = EKeys::SpaceBar;

	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey Boost = EKeys::LeftControl;

	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey Fire = EKeys::LeftMouseButton;

	/**
	 * 아이템 변경. D-02로 Alt에서 Q로 옮겼습니다.
	 * Alt는 Tab(지도)과 만나면 게임 창을 최소화시키므로 기본 매핑에서 제외합니다.
	 * 원안 사용자를 위해 대안 바인딩으로 지정하는 것은 가능합니다.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey CycleItem = EKeys::Q;

	/** 게임패드 — Steam Deck 대응 필수. (docs/03 §3.4) */
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey PadThrust = EKeys::Gamepad_RightTrigger;
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey PadBrake  = EKeys::Gamepad_LeftTrigger;
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey PadVector = EKeys::Gamepad_LeftShoulder;
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey PadBoost  = EKeys::Gamepad_FaceButton_Bottom;
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input") FKey PadFire   = EKeys::Gamepad_RightShoulder;
};

/**
 * 키 상태 → InputFlags.
 *
 * 매 프레임 "지금 눌려 있는가"를 읽습니다. 이벤트가 아니라 상태를 보는 이유는
 * SimulateMove()가 프레임 단위 스냅샷을 받기 때문입니다. 눌림/뗌 이벤트로 만들면
 * 프레임을 건너뛸 때 입력이 유실됩니다.
 */
UCLASS()
class FPG_API UFPGInputHandler : public UObject
{
	GENERATED_BODY()

public:
	/** 왼쪽 스틱 등 아날로그 축은 이 임계값을 넘으면 눌린 것으로 봅니다. */
	static constexpr float AnalogThreshold = 0.35f;

	static uint8 BuildInputFlags(const APlayerController& PC, const FFPGKeyBindings& Keys);
};
