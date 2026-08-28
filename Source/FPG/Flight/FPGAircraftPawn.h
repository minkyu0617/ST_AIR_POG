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
class UFPGHealthComponent;
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
enum class EDamageState : uint8;

UCLASS()
class FPG_API AFPGAircraftPawn : public APawn
{
	GENERATED_BODY()

public:
	AFPGAircraftPawn();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "FPG|Flight")
	UFlightMovementComponent* GetFlightMovement() const { return Movement; }

	UFUNCTION(BlueprintPure, Category = "FPG|Health")
	UFPGHealthComponent* GetHealth() const { return Health; }

protected:
	virtual void BeginPlay() override;

	/** 손상 상태가 바뀌면 비행 성능 배율을 다시 계산합니다 (docs/02 §2.5). */
	UFUNCTION()
	void HandleDamageStateChanged(EDamageState NewState);

	UFUNCTION()
	void HandleDeath(AActor* Killer);

	/** 루트. 충돌은 여기서 봅니다. */
	UPROPERTY(VisibleAnywhere, Category = "FPG")
	TObjectPtr<UStaticMeshComponent> Hull;

	UPROPERTY(VisibleAnywhere, Category = "FPG")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "FPG")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "FPG")
	TObjectPtr<UFlightMovementComponent> Movement;

	UPROPERTY(VisibleAnywhere, Category = "FPG")
	TObjectPtr<UFPGHealthComponent> Health;

	/** docs/03 §3.5 — 리바인딩 가능해야 하므로 데이터로 둡니다. */
	UPROPERTY(EditAnywhere, Config, Category = "FPG|Input")
	FFPGKeyBindings KeyBindings;

	/**
	 * 이 폰이 사용할 기체의 데이터 ID (DT_Aircraft.csv의 행 이름).
	 *
	 * M1은 기본기 Falcon 하나만 씁니다. 기체 선택·교체(POI 상점)가 생기면
	 * GameMode나 PlayerState가 이 값을 정해 주게 됩니다 — 킬 수·크레딧처럼
	 * Pawn이 파괴돼도 남아야 하는 값은 PlayerState에 둡니다 (docs/16 §16.3).
	 */
	UPROPERTY(EditAnywhere, Category = "FPG|Flight")
	FName AircraftId = TEXT("AIRCRAFT_FALCON");

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
