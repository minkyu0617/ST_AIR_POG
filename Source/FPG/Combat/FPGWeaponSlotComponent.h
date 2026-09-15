// Slot 0 always holds the vulcan, so LMB always does something (docs/03 C-6).
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Types/FPGDataRows.h"
#include "FPGWeaponSlotComponent.generated.h"

class UFPGDataRegistry;

USTRUCT()
struct FPG_API FFPGWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY() FName ItemId;
	/** -1 = unlimited. */
	UPROPERTY() int32 Ammo = 0;
	UPROPERTY() float CooldownRemaining = 0.f;

	bool IsEmpty() const { return ItemId.IsNone(); }
};

UCLASS(ClassGroup = (FPG), meta = (BlueprintSpawnableComponent))
class FPG_API UFPGWeaponSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPGWeaponSlotComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void InitializeSlots(UFPGDataRegistry* InRegistry, int32 ExtraSlots);

	/** Q / mouse wheel. Skips empty slots but always finds slot 0. */
	void CycleSlot();
	void SelectSlot(int32 Index);

	/** LMB. Returns true when a shot was actually fired. */
	bool TryFire();

	UFUNCTION(BlueprintPure, Category = "FPG|Weapon") int32 GetSelectedIndex() const { return SelectedIndex; }
	UFUNCTION(BlueprintPure, Category = "FPG|Weapon") FName GetSelectedItemId() const;
	UFUNCTION(BlueprintPure, Category = "FPG|Weapon") float GetHeatPercent() const;
	UFUNCTION(BlueprintPure, Category = "FPG|Weapon") bool IsOverheated() const { return bOverheated; }
	int32 GetSlotAmmo(int32 Index) const;

	void RefillAmmo();

	/** Gun heat model (docs/06 §6.2): 2.5 s of fire, 3 s to cool. */
	UPROPERTY(EditDefaultsOnly, Category = "FPG|Weapon") float MaxHeat = 2.5f;
	UPROPERTY(EditDefaultsOnly, Category = "FPG|Weapon") float CoolRate = 0.833f;
	UPROPERTY(EditDefaultsOnly, Category = "FPG|Weapon") float GunRange = 90000.f;

private:
	UPROPERTY(Transient) TObjectPtr<UFPGDataRegistry> Registry;

	UPROPERTY() TArray<FFPGWeaponSlot> Slots;
	int32 SelectedIndex = 0;

	float Heat = 0.f;
	bool bOverheated = false;

	bool FireHitscan(const FFPGItemRow& Item);
};
