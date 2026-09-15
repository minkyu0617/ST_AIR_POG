#include "Combat/FPGWeaponSlotComponent.h"

#include "FPG.h"
#include "Combat/FPGHealthComponent.h"
#include "Core/Services/FPGDataRegistry.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	const FName VulcanId(TEXT("ITEM_VULCAN"));
}

UFPGWeaponSlotComponent::UFPGWeaponSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFPGWeaponSlotComponent::InitializeSlots(UFPGDataRegistry* InRegistry, int32 ExtraSlots)
{
	Registry = InRegistry;

	Slots.Reset();
	// Slot 0 is the permanent gun. Slots 1..n are pickups; M1 leaves them empty.
	FFPGWeaponSlot Gun;
	Gun.ItemId = VulcanId;
	Gun.Ammo = -1;
	Slots.Add(Gun);

	const int32 ItemSlots = 2 + FMath::Max(0, ExtraSlots);
	for (int32 Index = 0; Index < ItemSlots; ++Index)
	{
		Slots.Add(FFPGWeaponSlot());
	}

	SelectedIndex = 0;
}

void UFPGWeaponSlotComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (FFPGWeaponSlot& Slot : Slots)
	{
		Slot.CooldownRemaining = FMath::Max(0.f, Slot.CooldownRemaining - DeltaTime);
	}

	Heat = FMath::Max(0.f, Heat - CoolRate * DeltaTime);
	if (bOverheated && Heat <= 0.f)
	{
		bOverheated = false;
	}
}

void UFPGWeaponSlotComponent::CycleSlot()
{
	if (Slots.Num() == 0)
	{
		return;
	}

	for (int32 Step = 1; Step <= Slots.Num(); ++Step)
	{
		const int32 Candidate = (SelectedIndex + Step) % Slots.Num();
		// Slot 0 is never empty, so this always terminates on something usable.
		if (!Slots[Candidate].IsEmpty())
		{
			SelectedIndex = Candidate;
			return;
		}
	}
}

void UFPGWeaponSlotComponent::SelectSlot(int32 Index)
{
	if (Slots.IsValidIndex(Index) && !Slots[Index].IsEmpty())
	{
		SelectedIndex = Index;
	}
}

FName UFPGWeaponSlotComponent::GetSelectedItemId() const
{
	return Slots.IsValidIndex(SelectedIndex) ? Slots[SelectedIndex].ItemId : NAME_None;
}

int32 UFPGWeaponSlotComponent::GetSlotAmmo(int32 Index) const
{
	return Slots.IsValidIndex(Index) ? Slots[Index].Ammo : 0;
}

float UFPGWeaponSlotComponent::GetHeatPercent() const
{
	return MaxHeat > KINDA_SMALL_NUMBER ? FMath::Clamp(Heat / MaxHeat, 0.f, 1.f) : 0.f;
}

void UFPGWeaponSlotComponent::RefillAmmo()
{
	for (FFPGWeaponSlot& Slot : Slots)
	{
		if (Slot.IsEmpty() || Slot.Ammo < 0 || !Registry)
		{
			continue;
		}
		if (const FFPGItemRow* Row = Registry->FindItem(Slot.ItemId))
		{
			Slot.Ammo = Row->Ammo;
		}
	}
	Heat = 0.f;
	bOverheated = false;
}

bool UFPGWeaponSlotComponent::TryFire()
{
	if (!Registry || !Slots.IsValidIndex(SelectedIndex))
	{
		return false;
	}

	FFPGWeaponSlot& Slot = Slots[SelectedIndex];
	if (Slot.IsEmpty() || Slot.CooldownRemaining > 0.f)
	{
		return false;
	}

	const FFPGItemRow* Row = Registry->FindItem(Slot.ItemId);
	if (!Row)
	{
		return false;
	}

	const bool bIsGun = (Row->Category == EFPGItemCategory::Gun);
	if (bIsGun && bOverheated)
	{
		return false;
	}

	if (Slot.Ammo == 0)
	{
		return false;
	}

	if (Slot.Ammo > 0)
	{
		--Slot.Ammo;
		if (Slot.Ammo == 0)
		{
			// Spent item slots fall back to the gun so the fire button never goes dead.
			Slot.ItemId = NAME_None;
			SelectedIndex = 0;
		}
	}

	Slot.CooldownRemaining = Row->Cooldown;

	if (bIsGun)
	{
		Heat += Row->Cooldown > 0.f ? Row->Cooldown : 0.083f;
		if (Heat >= MaxHeat)
		{
			Heat = MaxHeat;
			bOverheated = true;
		}
	}

	return FireHitscan(*Row);
}

bool UFPGWeaponSlotComponent::FireHitscan(const FFPGItemRow& Item)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return false;
	}

	const FVector Start = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 500.f;
	const float Range = Item.Range > 0.f ? Item.Range : GunRange;
	const FVector End = Start + Owner->GetActorForwardVector() * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FPGWeaponTrace), /*bTraceComplex=*/false, Owner);
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

	if (bHit && Hit.GetActor())
	{
		if (UFPGHealthComponent* Health = Hit.GetActor()->FindComponentByClass<UFPGHealthComponent>())
		{
			Health->ApplyDamage(Item.Damage, Owner);
		}
	}

	// M1 uses a debug line in place of VFX. Programmer art is allowed at this milestone.
	DrawDebugLine(World, Start, bHit ? Hit.ImpactPoint : End, bHit ? FColor::Red : FColor::Yellow,
		false, 0.08f, 0, 12.f);

	return true;
}
