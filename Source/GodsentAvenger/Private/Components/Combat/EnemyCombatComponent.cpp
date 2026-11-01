// Alex Goldberg rights reserved

#include "Components/Combat/EnemyCombatComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GodsentDebugHelper.h"
#include "Components/BoxComponent.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "Characters/GodsentEnemyCharacter.h"

/// Resolves block and parry; sends MeleeHit unless the hit was fully blocked or parried
void UEnemyCombatComponent::OnHitTargetActor(AActor* HitActor)
{
	if (OverlappedActors.Contains(HitActor))
		return;

	OverlappedActors.AddUnique(HitActor);

	// Invincible targets (e.g. mid-dodge with Graced Evasion) ignore the hit entirely
	if (UGodsentFunctionLibrary::NativeDoesActorHaveTag(HitActor, GodsentGameplayTags::Shared_Status_Invincible))
		return;

	const bool bIsMyAttackUnblockable = UGodsentFunctionLibrary::NativeDoesActorHaveTag(GetOwningPawn(), GodsentGameplayTags::Enemy_Status_Unblockable);

	const EGodsentBlockResult BlockResult = UGodsentFunctionLibrary::NativeResolveBlock(GetOwningPawn(), HitActor, bIsMyAttackUnblockable, BlockFacingThreshold);
	
	// Full block and parry stop the damage; a partial block still lets it through (reduced by the block's defence bonus)
	if (UGodsentFunctionLibrary::NativeHandleBlockResult(GetOwningPawn(), HitActor, BlockResult))
		return;

	// Hit landed outside the guard (from behind, side-on or unblockable) while blocking: break the guard
	// so the block defence bonus drops and the hit react is free to play
	const bool bGuardBroken = UGodsentFunctionLibrary::NativeBreakGuardIfBlocking(HitActor, BlockResult);

	FGameplayEventData EventData;
	EventData.Instigator = GetOwningPawn();
	EventData.Target = HitActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetOwningPawn(),
		GodsentGameplayTags::Shared_Event_MeleeHit,
		EventData);

	// A broken guard always staggers, even when this attack doesn't carry hit stun
	// (sent after MeleeHit so the damage lands first, and skipped if that damage killed the target)
	if (bGuardBroken)
		UGodsentFunctionLibrary::NativeSendGuardBreakStagger(GetOwningPawn(), HitActor);
}

/// Turns the left or right hand collision box on or off
void UEnemyCombatComponent::ToggleBodyCollisionBoxCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType)
{
	AGodsentEnemyCharacter* OwningEnemyCharacter = GetOwningPawn<AGodsentEnemyCharacter>();
	check(OwningEnemyCharacter);

	UBoxComponent* LeftHandCollisionBox = OwningEnemyCharacter->GetLeftHandCollisionBox();
	UBoxComponent* RightHandCollisionBox = OwningEnemyCharacter->GetRightHandCollisionBox();
	check(LeftHandCollisionBox && RightHandCollisionBox);

	switch (ToggleDamageType)
	{
	case EToggleDamageType::LeftHand:
		LeftHandCollisionBox->SetCollisionEnabled(bShouldEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		break;

	case EToggleDamageType::RightHand:
		RightHandCollisionBox->SetCollisionEnabled(bShouldEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		break;

	default:
		break;
	}

	if (!bShouldEnable)
		OverlappedActors.Empty();
}
