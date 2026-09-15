// HP and the damage-state bands from docs/02 §2.5.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Types/FPGTypes.h"
#include "FPGHealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FFPGOnHealthChanged, float /*NewHP*/, float /*Delta*/);
DECLARE_MULTICAST_DELEGATE(FFPGOnDied);

UCLASS(ClassGroup = (FPG), meta = (BlueprintSpawnableComponent))
class FPG_API UFPGHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPGHealthComponent();

	void InitializeHealth(float InMaxHP);

	/** Positive amount damages, negative heals. Server-authoritative in M3. */
	void ApplyDamage(float Amount, AActor* Instigator = nullptr);

	void FullRepair();

	UFUNCTION(BlueprintPure, Category = "FPG|Health") float GetHP() const { return HP; }
	UFUNCTION(BlueprintPure, Category = "FPG|Health") float GetMaxHP() const { return MaxHP; }
	UFUNCTION(BlueprintPure, Category = "FPG|Health") float GetHealthPercent() const;
	UFUNCTION(BlueprintPure, Category = "FPG|Health") EFPGDamageState GetDamageState() const { return DamageState; }
	UFUNCTION(BlueprintPure, Category = "FPG|Health") bool IsDead() const { return DamageState == EFPGDamageState::Dead; }

	/** Top speed multiplier imposed by the current damage band (docs/02 §2.5). */
	float GetSpeedPenaltyScale() const;

	FFPGOnHealthChanged OnHealthChanged;
	FFPGOnDied OnDied;

private:
	float HP = 100.f;
	float MaxHP = 100.f;
	EFPGDamageState DamageState = EFPGDamageState::Healthy;

	void RefreshDamageState();
};
