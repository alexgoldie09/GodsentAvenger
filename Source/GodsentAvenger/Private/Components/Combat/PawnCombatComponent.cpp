// Alex Goldberg rights reserved

#include "Components/Combat/PawnCombatComponent.h"
#include "Components/BoxComponent.h"
#include "Items/Weapons/GodsentWeaponBase.h"

/// Registers a spawned weapon under a tag, optionally as the equipped weapon
void UPawnCombatComponent::RegisterSpawnedWeapon(FGameplayTag InWeaponTagToRegister,
	AGodsentWeaponBase* InWeaponToRegister, bool bRegisterAsEquippedWeapon)
{
	checkf(!CharacterCarriedWeaponMap.Contains(InWeaponTagToRegister), TEXT("Weapon with tag %s is already registered in the character's carried weapon map!"), *InWeaponTagToRegister.ToString());
	check(InWeaponToRegister);

	CharacterCarriedWeaponMap.Emplace(InWeaponTagToRegister, InWeaponToRegister);

	InWeaponToRegister->OnWeaponHitTarget.BindUObject(this, &ThisClass::OnHitTargetActor);
	InWeaponToRegister->OnWeaponPulledFromTarget.BindUObject(this, &ThisClass::OnWeaponPulledFromTargetActor);

	if (bRegisterAsEquippedWeapon)
		CurrentEquippedWeaponTag = InWeaponTagToRegister;
}

/// Gets a carried weapon by its tag
AGodsentWeaponBase* UPawnCombatComponent::GetCharacterCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const
{
	if (const TObjectPtr<AGodsentWeaponBase>* FoundWeapon = CharacterCarriedWeaponMap.Find(InWeaponTagToGet))
		return *FoundWeapon;

	return nullptr;
}

/// Gets the equipped weapon, or nullptr
AGodsentWeaponBase* UPawnCombatComponent::GetCharacterCurrentEquippedWeapon() const
{
	if (!CurrentEquippedWeaponTag.IsValid())
		return nullptr;

	return GetCharacterCarriedWeaponByTag(CurrentEquippedWeaponTag);
}

/// Turns hit collision on or off for the equipped weapon or one of the body (hand) boxes
void UPawnCombatComponent::ToggleWeaponCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType)
{
	if (ToggleDamageType == EToggleDamageType::CurrentEquippedWeapon)
		ToggleCurrentEquippedWeaponCollision(bShouldEnable);
	else
		ToggleBodyCollisionBoxCollision(bShouldEnable, ToggleDamageType);
}

void UPawnCombatComponent::OnHitTargetActor(AActor* HitActor)
{
}

void UPawnCombatComponent::OnWeaponPulledFromTargetActor(AActor* InteractedActor)
{
}

void UPawnCombatComponent::ToggleCurrentEquippedWeaponCollision(bool bShouldEnable)
{
	AGodsentWeaponBase* CurrentEquippedWeapon = GetCharacterCurrentEquippedWeapon();

	if (!CurrentEquippedWeapon)
		return;

	if (bShouldEnable)
	{
		CurrentEquippedWeapon->GetWeaponCollisionBox()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		CurrentEquippedWeapon->GetWeaponCollisionBox()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OverlappedActors.Empty();
	}
}

void UPawnCombatComponent::ToggleBodyCollisionBoxCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType)
{
}
