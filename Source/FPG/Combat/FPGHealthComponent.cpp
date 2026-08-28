#include "Combat/FPGHealthComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPGHealth, Log, All);

UFPGHealthComponent::UFPGHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;   // 이벤트 기반. 매 프레임 볼 것이 없습니다 (docs/16 §16.13)
	SetIsReplicatedByDefault(true);
}

void UFPGHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// HP는 전원에게 복제합니다. 적 기체의 HP 바를 그리려면 필요하고,
	// docs/16 §16.6의 복제 표에서도 AircraftPawn의 HP는 "전원" 대상입니다.
	DOREPLIFETIME(UFPGHealthComponent, CurrentHP);
	DOREPLIFETIME(UFPGHealthComponent, MaxHP);
}

void UFPGHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	LastBroadcastState = GetDamageState();
}

void UFPGHealthComponent::InitializeFromData(float InMaxHP, const FFPGHealthTuning& InTuning)
{
	Tuning = InTuning;

	if (InMaxHP > 0.f)
	{
		MaxHP = InMaxHP;
	}
	CurrentHP = MaxHP;
	LastBroadcastState = EDamageState::Healthy;
}

EDamageState UFPGHealthComponent::GetDamageState() const
{
	if (CurrentHP <= 0.f)
	{
		return EDamageState::Dead;
	}

	const float Pct = GetHealthRatio() * 100.f;
	if (Pct < Tuning.CriticalThresholdPct) { return EDamageState::Critical; }
	if (Pct < Tuning.DamagedThresholdPct)  { return EDamageState::Damaged; }
	return EDamageState::Healthy;
}

FFPGFlightModifiers UFPGHealthComponent::GetFlightModifiers() const
{
	FFPGFlightModifiers Out;

	switch (GetDamageState())
	{
	case EDamageState::Damaged:
		Out.MaxSpeedMult = Tuning.DamagedMaxSpeedMult;
		Out.TurnRateMult = Tuning.DamagedTurnRateMult;
		break;

	case EDamageState::Critical:
		Out.MaxSpeedMult = Tuning.CriticalMaxSpeedMult;
		Out.TurnRateMult = Tuning.CriticalTurnRateMult;
		break;

	case EDamageState::Dead:
		// 격추 연출은 이동을 멈추는 것이 아니라 EFlightState::Destroyed 가
		// 담당합니다(스핀 → 폭발 → 이젝션, docs/02 §2.5). 여기서 성능을
		// 0으로 만들면 그 연출이 부자연스러워집니다.
		break;

	default:
		break;
	}

	return Out;
}

float UFPGHealthComponent::ApplyDamage(float Amount, AActor* Instigator)
{
	// 🔴 P3 — 서버만 자원을 바꿉니다. 클라이언트 호출은 무시합니다.
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		UE_LOG(LogFPGHealth, Warning,
			TEXT("ApplyDamage 가 권한 없이 호출됐습니다. HP는 서버만 바꿉니다 (P3)."));
		return 0.f;
	}

	if (Amount <= 0.f || IsDead())
	{
		return 0.f;
	}

	const float Applied = FMath::Min(Amount, CurrentHP);
	CurrentHP -= Applied;

	OnDamaged.Broadcast(Applied, Instigator);

	// 서버에서는 OnRep이 불리지 않으므로 직접 알립니다.
	// 이걸 빠뜨리면 리슨 서버(호스트) 화면에서만 상태 변화가 안 보이는,
	// 원인 찾기 까다로운 버그가 됩니다.
	BroadcastStateIfChanged();

	if (IsDead())
	{
		OnDeath.Broadcast(Instigator);
	}

	return Applied;
}

float UFPGHealthComponent::Heal(float Amount)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || Amount <= 0.f)
	{
		return 0.f;
	}

	// 격추된 기체는 회복으로 되살리지 않습니다. 부활은 GameMode의 몫입니다
	// (docs/16 §16.4 — 부활 규칙은 서버의 GameMode가 판단).
	if (IsDead())
	{
		return 0.f;
	}

	const float Applied = FMath::Min(Amount, MaxHP - CurrentHP);
	CurrentHP += Applied;
	BroadcastStateIfChanged();
	return Applied;
}

void UFPGHealthComponent::ResetToFull()
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	CurrentHP = MaxHP;
	BroadcastStateIfChanged();
}

void UFPGHealthComponent::OnRep_CurrentHP(float OldHP)
{
	// 클라이언트 쪽 알림. 서버는 ApplyDamage/Heal 에서 직접 알립니다.
	if (OldHP > CurrentHP)
	{
		OnDamaged.Broadcast(OldHP - CurrentHP, nullptr);
	}

	BroadcastStateIfChanged();

	if (IsDead() && OldHP > 0.f)
	{
		OnDeath.Broadcast(nullptr);
	}
}

void UFPGHealthComponent::BroadcastStateIfChanged()
{
	const EDamageState NewState = GetDamageState();
	if (NewState != LastBroadcastState)
	{
		LastBroadcastState = NewState;
		OnDamageStateChanged.Broadcast(NewState);
	}
}
