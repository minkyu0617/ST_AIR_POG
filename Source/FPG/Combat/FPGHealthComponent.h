// FPG — 기체 HP와 손상 상태.
//
// docs/02 §2.5 HP/피격 모델 · docs/16 §16.3 상태의 소유 위치
//
// 🔴 P3 — HP는 **자원**이므로 절대 예측하지 않습니다.
//    서버만 깎고 클라이언트는 복제를 기다립니다. 예측하면 "맞췄는데 안 죽는"
//    류의 버그가 끝없이 나옵니다 (docs/16 §16.6).
//    M1은 싱글이지만 리슨 서버로 돌리므로(AR2) 지금부터 이 구조를 지킵니다.
//    그래야 M3에서 멀티가 "추가"가 되고 "재작성"이 되지 않습니다.
//
// 🔴 HP는 Pawn에 둡니다 (docs/16 §16.3).
//    킬 수·크레딧·순위는 PlayerState입니다 — 격추당해 Pawn이 파괴돼도 남아야
//    하기 때문입니다. HP는 반대로 기체와 함께 사라져야 맞습니다.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Flight/FlightTypes.h"
#include "FPGHealthComponent.generated.h"

/** docs/17 §17.2 */
UENUM(BlueprintType)
enum class EDamageState : uint8
{
	Healthy,
	Damaged,
	Critical,
	Dead
};

/**
 * 손상 상태별 효과 수치. DT_Health.csv에서 옵니다 (P5).
 * 기본값은 docs/02 §2.5의 표와 같으며 데이터가 없을 때의 안전망입니다.
 */
USTRUCT()
struct FFPGHealthTuning
{
	GENERATED_BODY()

	/** 이 비율(%) 미만이면 손상 / 심각. docs/02 §2.5의 60·30 구간. */
	UPROPERTY() float DamagedThresholdPct = 60.f;
	UPROPERTY() float CriticalThresholdPct = 30.f;

	UPROPERTY() float DamagedMaxSpeedMult = 0.90f;   // 최고 속도 −10%
	UPROPERTY() float DamagedTurnRateMult = 1.00f;
	UPROPERTY() float CriticalMaxSpeedMult = 0.75f;  // 최고 속도 −25%
	UPROPERTY() float CriticalTurnRateMult = 0.85f;  // 선회율 −15%

	/** docs/02 §2.4 정적 지형 충돌. */
	UPROPERTY() float TerrainCollisionDamage = 60.f;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FFPGOnDamaged, float /*Amount*/, AActor* /*Instigator*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FFPGOnDamageStateChanged, EDamageState /*NewState*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FFPGOnDeath, AActor* /*Killer*/);

UCLASS(ClassGroup = (FPG), meta = (BlueprintSpawnableComponent))
class FPG_API UFPGHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPGHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ── 서버 전용 변경 ───────────────────────────────────────
	//
	// 🔴 클라이언트에서 부르면 아무 일도 일어나지 않습니다(경고만 남습니다).
	//    이걸 예측 가능하게 만들지 마십시오 — P3 위반입니다.

	/** @return 실제로 깎인 양. 이미 죽었거나 권한이 없으면 0. */
	float ApplyDamage(float Amount, AActor* Instigator = nullptr);

	/** 음수는 무시합니다. 정비소 POI의 전량 회복은 `ResetToFull()`. */
	float Heal(float Amount);

	void ResetToFull();

	/** DT_Aircraft의 BaseHP를 반영합니다. 기체 교체 시에도 호출됩니다. */
	void InitializeFromData(float InMaxHP, const FFPGHealthTuning& InTuning);

	// ── 조회 ─────────────────────────────────────────────────
	UFUNCTION(BlueprintPure, Category = "FPG|Health")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category = "FPG|Health")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category = "FPG|Health")
	float GetHealthRatio() const { return MaxHP > 0.f ? CurrentHP / MaxHP : 0.f; }

	UFUNCTION(BlueprintPure, Category = "FPG|Health")
	EDamageState GetDamageState() const;

	UFUNCTION(BlueprintPure, Category = "FPG|Health")
	bool IsDead() const { return CurrentHP <= 0.f; }

	/** 현재 손상 상태에 해당하는 비행 성능 배율 (docs/02 §2.5). */
	FFPGFlightModifiers GetFlightModifiers() const;

	const FFPGHealthTuning& GetTuning() const { return Tuning; }

	// ── 알림 (docs/16 §16.9) ─────────────────────────────────
	// EventBus가 생기기 전까지는 델리게이트로 둡니다. EventBus는 "게임플레이 →
	// UI" 단방향 통지가 목적이고(AR3), 여기서 필요한 건 컴포넌트 간 직접
	// 연결이라 델리게이트가 맞습니다.
	FFPGOnDamaged OnDamaged;
	FFPGOnDamageStateChanged OnDamageStateChanged;
	FFPGOnDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_CurrentHP(float OldHP);

private:
	void BroadcastStateIfChanged();

	/** 🔴 서버 권위. 클라이언트는 복제를 받을 뿐입니다 (P3). */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP)
	float CurrentHP = 100.f;

	UPROPERTY(Replicated)
	float MaxHP = 100.f;

	UPROPERTY()
	FFPGHealthTuning Tuning;

	/** 상태 전이 알림을 중복 발송하지 않기 위한 캐시. */
	EDamageState LastBroadcastState = EDamageState::Healthy;
};
