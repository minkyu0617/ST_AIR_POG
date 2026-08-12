// FPG — 비행 이동 컴포넌트.
//
// docs/16 §16.5 "이 프로젝트 최대의 기술 리스크"
//
// 🔴 P4 — SimulateMove()는 M1부터 순수 함수로 분리합니다.
//    안 지키면 M3에서 이동 코드를 통째로 재작성합니다.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlightTypes.h"
#include "FlightMovementComponent.generated.h"

UCLASS(ClassGroup = (FPG), meta = (BlueprintSpawnableComponent))
class FPG_API UFlightMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlightMovementComponent();

	/**
	 * 🔴 **순수 함수.** 이 프로젝트에서 가장 중요한 계약입니다.
	 *
	 * (이전 상태 + 파라미터 + 입력 + 시간) 하나만으로 다음 상태가 결정됩니다.
	 * 클라이언트와 서버가 **같은 이 함수**를 돌려 같은 결과를 얻는 것이
	 * 예측·화해(docs/16 §16.5)의 전제입니다.
	 *
	 * 지켜야 할 것:
	 *   - `static` 입니다. 멤버에 접근할 수 없으므로 숨은 상태가 생길 수 없습니다.
	 *     **이 static을 떼지 마십시오.** 컴파일러가 강제해 주는 유일한 안전장치입니다.
	 *   - 난수, GetWorld(), 시각 조회, 프레임 카운터를 쓰지 마십시오.
	 *   - 액터·컴포넌트·월드를 건드리지 마십시오. 충돌 처리는 호출자의 몫입니다.
	 *
	 * 깨지면 화면이 끊임없이 튀고 원인 추적에 며칠이 걸립니다. (docs/16 §16.14)
	 */
	static FFPGMoveState Step(
		const FFPGMoveState& In,
		const FFPGFlightParams& Params,
		const FFPGMove& Move,
		float DeltaTime);

	// ── C++ 전용 ─────────────────────────────────────────────
	//
	// 🔴 시뮬레이션 상태(FFPGMove·FFPGMoveState)는 **Blueprint에 노출하지 않습니다.**
	//    BP에서 상태를 건드릴 수 있게 되는 순간 P4의 결정론이 깨지고,
	//    그 버그는 "가끔 화면이 튄다"로만 나타나 추적이 극히 어렵습니다.
	//    Blueprint의 역할은 UI·연출·튜닝입니다. (docs/10 §10.2)
	//    HUD가 필요한 값은 아래 스칼라 접근자로 따로 열어 두었습니다.

	/** 위 순수 함수를 현재 상태에 적용합니다. docs/16의 시그니처. */
	void SimulateMove(const FFPGMove& Move, float DeltaTime);

	FFPGMoveState GetStateSnapshot() const { return State; }

	/** 화해(M3)와 테스트에서 상태를 되돌릴 때 씁니다. */
	void SetStateSnapshot(const FFPGMoveState& NewState) { State = NewState; }

	/** DT_Aircraft·DT_Flight에서 읽어온 값을 주입합니다. (P5) */
	void SetParams(const FFPGFlightParams& NewParams) { Params = NewParams; }

	const FFPGFlightParams& GetParams() const { return Params; }

	/** 순항 속도에 해당하는 스로틀로 초기화합니다. */
	void ResetToCruise(const FVector& StartLocation, const FQuat& StartRotation);

	// ── HUD·연출용 읽기 전용 접근자 ──────────────────────────
	// 값을 복사해 돌려줄 뿐이라 시뮬레이션에 영향을 줄 수 없습니다.

	UFUNCTION(BlueprintPure, Category = "FPG|Flight")
	float GetSpeed() const { return State.Velocity.Size(); }

	UFUNCTION(BlueprintPure, Category = "FPG|Flight")
	float GetThrottle() const { return State.Throttle; }

	UFUNCTION(BlueprintPure, Category = "FPG|Flight")
	float GetBoostRemaining() const { return State.BoostRemaining; }

	UFUNCTION(BlueprintPure, Category = "FPG|Flight")
	float GetBoostCooldownRemaining() const { return State.BoostCooldownRemaining; }

	UFUNCTION(BlueprintPure, Category = "FPG|Flight")
	EFlightState GetFlightState() const { return State.State; }

	/** 고도(m). 1 UU = 1 cm. */
	UFUNCTION(BlueprintPure, Category = "FPG|Flight")
	float GetAltitudeMeters() const { return State.Location.Z / 100.f; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	FFPGMoveState State;

	UPROPERTY(EditDefaultsOnly, Category = "FPG|Flight")
	FFPGFlightParams Params;
};
