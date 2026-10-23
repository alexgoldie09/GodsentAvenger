// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/GodsentGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"

/// Activates OnGiven abilities as soon as they're granted
void UGodsentGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (AbilityActivationPolicy == EGodsentAbilityActivationPolicy::OnGiven)
	{
		if (ActorInfo && !Spec.IsActive())
			ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

/// Removes OnGiven abilities once they end
void UGodsentGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (AbilityActivationPolicy == EGodsentAbilityActivationPolicy::OnGiven)
	{
		if (ActorInfo)
			ActorInfo->AbilitySystemComponent->ClearAbility(Handle);
	}
}

/// Gets the owner's combat component
UPawnCombatComponent* UGodsentGameplayAbility::GetPawnCombatComponentFromActorInfo() const
{
	return GetAvatarActorFromActorInfo()->FindComponentByClass<UPawnCombatComponent>();
}

/// Gets the owner's Godsent Ability System Component
UGodsentAbilitySystemComponent* UGodsentGameplayAbility::GetGodsentAbilitySystemComponentFromActorInfo() const
{
	return Cast<UGodsentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

/// Applies an effect spec to a target, with Successful and Failed exec pins
FActiveGameplayEffectHandle UGodsentGameplayAbility::BP_ApplyEffectSpecHandleToTargetActor(AActor* TargetActor,
	const FGameplayEffectSpecHandle& InSpecHandle, EGodsentSuccessType& OutSuccessType)
{
	const FActiveGameplayEffectHandle ActiveEffectHandle = NativeApplyEffectSpecHandleToTarget(TargetActor, InSpecHandle);
	OutSuccessType = ActiveEffectHandle.WasSuccessfullyApplied() ? EGodsentSuccessType::Successful : EGodsentSuccessType::Failed;
	return ActiveEffectHandle;
}

/// Applies an effect spec to every hostile pawn in a set of hit results and sends each one a hit react event
void UGodsentGameplayAbility::ApplyGameplayEffectSpecHandleToHitResults(const FGameplayEffectSpecHandle& InSpecHandle,
	const TArray<FHitResult>& HitResults)
{
	if (HitResults.IsEmpty())
		return;

	APawn* OwningPawn = CastChecked<APawn>(GetAvatarActorFromActorInfo());

	for (const FHitResult& HitResult : HitResults)
	{
		APawn* HitPawn = Cast<APawn>(HitResult.GetActor());

		if (!HitPawn || !UGodsentFunctionLibrary::IsTargetPawnHostile(OwningPawn, HitPawn))
			continue;

		// Stagger is handled by the attribute set when the spec carries Shared.Effect.HitStun
		NativeApplyEffectSpecHandleToTarget(HitPawn, InSpecHandle);
	}
}

/// Applies an effect spec to a target and returns the active effect handle
FActiveGameplayEffectHandle UGodsentGameplayAbility::NativeApplyEffectSpecHandleToTarget(AActor* TargetActor,
	const FGameplayEffectSpecHandle& InSpecHandle)
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	check(TargetASC && InSpecHandle.IsValid());

	return GetGodsentAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
		*InSpecHandle.Data,
		TargetASC);
}

/// Applies an effect spec to every living hostile pawn within Radius of Origin
int32 UGodsentGameplayAbility::ApplyEffectSpecHandleToHostilesInRadius(const FGameplayEffectSpecHandle& InSpecHandle,
	FVector Origin, float Radius, bool bCausesHitStun, FGameplayTag RequiredTargetTag, bool bDrawDebug)
{
	APawn* OwningPawn = Cast<APawn>(GetAvatarActorFromActorInfo());

	if (!OwningPawn || !InSpecHandle.IsValid() || Radius <= 0.f)
		return 0;

	if (bCausesHitStun)
		InSpecHandle.Data->AddDynamicAssetTag(GodsentGameplayTags::Shared_Effect_HitStun);

	TArray<APawn*> HostilePawns;
	UGodsentFunctionLibrary::GetHostilePawnsInRadius(OwningPawn, Origin, Radius, HostilePawns);

	int32 NumTargetsHit = 0;

	for (APawn* HostilePawn : HostilePawns)
	{
		if (RequiredTargetTag.IsValid() && !UGodsentFunctionLibrary::NativeDoesActorHaveTag(HostilePawn, RequiredTargetTag))
			continue;

		if (NativeApplyEffectSpecHandleToTarget(HostilePawn, InSpecHandle).WasSuccessfullyApplied())
			++NumTargetsHit;
	}

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
		DrawDebugSphere(OwningPawn->GetWorld(), Origin, Radius, 16, NumTargetsHit > 0 ? FColor::Green : FColor::Red, false, 1.f);
#endif

	return NumTargetsHit;
}
