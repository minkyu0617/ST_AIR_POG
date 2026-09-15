#include "Combat/FPGHealthComponent.h"

#include "FPG.h"

UFPGHealthComponent::UFPGHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFPGHealthComponent::InitializeHealth(float InMaxHP)
{
	MaxHP = FMath::Max(1.f, InMaxHP);
	HP = MaxHP;
	DamageState = EFPGDamageState::Healthy;
}

void UFPGHealthComponent::ApplyDamage(float Amount, AActor* /*Instigator*/)
{
	if (DamageState == EFPGDamageState::Dead || FMath::IsNearlyZero(Amount))
	{
		return;
	}

	const float Previous = HP;
	HP = FMath::Clamp(HP - Amount, 0.f, MaxHP);

	const float Delta = HP - Previous;
	if (!FMath::IsNearlyZero(Delta))
	{
		OnHealthChanged.Broadcast(HP, Delta);
	}

	RefreshDamageState();
}

void UFPGHealthComponent::FullRepair()
{
	if (DamageState == EFPGDamageState::Dead)
	{
		return;
	}

	const float Previous = HP;
	HP = MaxHP;
	OnHealthChanged.Broadcast(HP, HP - Previous);
	RefreshDamageState();
}

float UFPGHealthComponent::GetHealthPercent() const
{
	return MaxHP > KINDA_SMALL_NUMBER ? FMath::Clamp(HP / MaxHP, 0.f, 1.f) : 0.f;
}

float UFPGHealthComponent::GetSpeedPenaltyScale() const
{
	switch (DamageState)
	{
	case EFPGDamageState::Damaged:  return 0.90f;
	case EFPGDamageState::Critical: return 0.75f;
	case EFPGDamageState::Dead:     return 0.f;
	default:                        return 1.f;
	}
}

void UFPGHealthComponent::RefreshDamageState()
{
	const EFPGDamageState Previous = DamageState;
	const float Percent = GetHealthPercent();

	if (HP <= 0.f)          { DamageState = EFPGDamageState::Dead; }
	else if (Percent <= 0.30f) { DamageState = EFPGDamageState::Critical; }
	else if (Percent <= 0.60f) { DamageState = EFPGDamageState::Damaged; }
	else                       { DamageState = EFPGDamageState::Healthy; }

	if (Previous != EFPGDamageState::Dead && DamageState == EFPGDamageState::Dead)
	{
		OnDied.Broadcast();
	}
}
