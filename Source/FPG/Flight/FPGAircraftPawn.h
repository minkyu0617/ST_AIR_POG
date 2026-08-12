// FPG — 플레이어가 조종하는 기체.
//
// docs/16 §16.4 클래스 구조.
//
// 역할 경계:
//   Pawn      = 입력 수집, 시뮬레이션 호출, 결과를 액터에 반영, 충돌 처리
//   Movement  = 순수 시뮬레이션 (SimulateMove) — 월드를 모릅니다
//   GameMode  = 규칙·승패·부활 (서버 전용)
// 이 분리를 M1부터 지켜야 M3에서 멀티가 "추가"가 됩니다. (docs/16 AR2)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Flight/FPGInputHandler.h"
#include "FPGAircraftPawn.generated.h"

class UFlightMovementComponent;
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;

UCLASS()
class FPG_API AFPGAircraftPawn : public APawn
{
	GENERATED_BODY()

public:
	AFPGAircraftPawn();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "FPG|Flight")
	UFlightMovementComponent* GetFlightMovement() const { return Movement; }

protected:
	virtual void BeginPlay() override;

	/** 루트. 충돌은 여기서 봅니다. */
	UPROPERTY(VisibleAnywhere, Category = "FPG")
	TObjectPtr<UStaticMeshComponent> Hull;

	UPROPERTY(VisibleAnywhere, Category = "FPG")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "FPG")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "FPG")
	TObjectPtr<UFlightMovementComponent> Movement;

	/** docs/03 §3.5 — 리바인딩 가능해야 하므로 데이터로 둡니다. */
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input")
	FFPGKeyBindings KeyBindings;

private:
	/**
	 * 시뮬레이션 시각.
	 *
	 * `GetWorld()->GetTimeSeconds()`를 쓰지 않고 직접 누적합니다.
	 * 월드 시각은 일시정지·슬로모션·레벨 전환에 영향을 받아
	 * Move의 타임스탬프가 단조롭지 않게 될 수 있습니다.
	 */
	float SimTime = 0.f;
};
