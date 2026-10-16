// Alex Goldberg rights reserved

#include "Components/Combat/HeroCombatComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "Items/Weapons/GodsentHeroWeapon.h"

/// Gets a carried hero weapon by tag
AGodsentHeroWeapon* UHeroCombatComponent::GetHeroCarriedWeaponByTag(FGameplayTag InWeaponTag) const
{
	return Cast<AGodsentHeroWeapon>(GetCharacterCarriedWeaponByTag(InWeaponTag));
}

/// Gets the equipped hero weapon, or nullptr
AGodsentHeroWeapon* UHeroCombatComponent::GetHeroCurrentEquippedWeapon() const
{
	return Cast<AGodsentHeroWeapon>(GetCharacterCurrentEquippedWeapon());
}

/// Gets the equipped weapon's base damage at a level (0 with no weapon equipped)
float UHeroCombatComponent::GetHeroCurrentEquippedWeaponDamageAtLevel(float InLevel) const
{
	const AGodsentHeroWeapon* EquippedWeapon = GetHeroCurrentEquippedWeapon();
	return EquippedWeapon ? EquippedWeapon->HeroWeaponData.WeaponBaseDamage.GetValueAtLevel(InLevel) : 0.f;
}

/// Sends MeleeHit (for the attack ability to apply damage) and HitPause
void UHeroCombatComponent::OnHitTargetActor(AActor* HitActor)
{
	if (OverlappedActors.Contains(HitActor))
		return;

	OverlappedActors.AddUnique(HitActor);

	// Invincible targets (e.g. mid-dodge) ignore the hit entirely
	if (UGodsentFunctionLibrary::NativeDoesActorHaveTag(HitActor, GodsentGameplayTags::Shared_Status_Invincible))
		return;

	FGameplayEventData EventData;
	EventData.Instigator = GetOwningPawn();
	EventData.Target = HitActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetOwningPawn(),
		GodsentGameplayTags::Shared_Event_MeleeHit,
		EventData);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetOwningPawn(),
		GodsentGameplayTags::Player_Event_HitPause,
		FGameplayEventData());
}

/// Sends HitPause as the blade leaves the target
void UHeroCombatComponent::OnWeaponPulledFromTargetActor(AActor* InteractedActor)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetOwningPawn(),
		GodsentGameplayTags::Player_Event_HitPause,
		FGameplayEventData());
}
