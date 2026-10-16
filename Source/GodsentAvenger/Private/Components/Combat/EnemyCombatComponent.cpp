// Alex Goldberg rights reserved

#include "Components/Combat/EnemyCombatComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
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
	
#if !UE_BUILD_SHIPPING
	const FString AttackerName = GetNameSafe(GetOwningPawn());
	const FString TargetName = GetNameSafe(HitActor);
	Debug::Print(FString::Printf(TEXT("[EnemyHit] %s -> %s"), *AttackerName, *TargetName), FColor::Orange);
#endif

	OverlappedActors.AddUnique(HitActor);

	// Invincible targets (e.g. mid-dodge with Graced Evasion) ignore the hit entirely
	if (UGodsentFunctionLibrary::NativeDoesActorHaveTag(HitActor, GodsentGameplayTags::Shared_Status_Invincible))
		return;

	const bool bIsMyAttackUnblockable = UGodsentFunctionLibrary::NativeDoesActorHaveTag(GetOwningPawn(), GodsentGameplayTags::Enemy_Status_Unblockable);

	const EGodsentBlockResult BlockResult = UGodsentFunctionLibrary::NativeResolveBlock(GetOwningPawn(), HitActor, bIsMyAttackUnblockable, BlockFacingThreshold);

#if !UE_BUILD_SHIPPING
	const TCHAR* BlockResultText = TEXT("None");
	switch (BlockResult)
	{
	case EGodsentBlockResult::Blocked:     BlockResultText = TEXT("Blocked"); break;
	case EGodsentBlockResult::FullBlocked: BlockResultText = TEXT("FullBlocked"); break;
	case EGodsentBlockResult::Parried:     BlockResultText = TEXT("Parried"); break;
	default:                               BlockResultText = TEXT("None"); break;
	}

	Debug::Print(
		FString::Printf(
			TEXT("[EnemyHit] %s -> %s | Result=%s | Unblockable=%s"),
			*AttackerName,
			*TargetName,
			BlockResultText,
			bIsMyAttackUnblockable ? TEXT("true") : TEXT("false")),
		FColor::Cyan);
#endif
	
	// Full block and parry stop the damage; a partial block still lets it through (reduced by the block's defence bonus)
	if (UGodsentFunctionLibrary::NativeHandleBlockResult(GetOwningPawn(), HitActor, BlockResult))
		return;
	
	FGameplayEventData EventData;
	EventData.Instigator = GetOwningPawn();
	EventData.Target = HitActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetOwningPawn(),
		GodsentGameplayTags::Shared_Event_MeleeHit,
		EventData);
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
