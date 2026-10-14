// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/GodsentGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "GameFramework/Pawn.h"

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

		const FActiveGameplayEffectHandle ActiveGameplayEffectHandle = NativeApplyEffectSpecHandleToTarget(HitPawn, InSpecHandle);

		if (ActiveGameplayEffectHandle.WasSuccessfullyApplied())
		{
			FGameplayEventData EventData;
			EventData.Instigator = OwningPawn;
			EventData.Target = HitPawn;

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				HitPawn,
				GodsentGameplayTags::Shared_Event_HitReact,
				EventData);
		}
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
