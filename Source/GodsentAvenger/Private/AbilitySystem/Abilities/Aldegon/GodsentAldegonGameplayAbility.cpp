// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/Aldegon/GodsentAldegonGameplayAbility.h"
#include "Components/Aldegon/GodsentAldegonComponent.h"
#include "Controllers/GodsentAIController.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GodsentGameplayTags.h"

namespace
{
	/// Montage rate when Attack Speed isn't used
	constexpr float AldegonNormalMontageRate = 1.f;
}

UGodsentAldegonGameplayAbility::UGodsentAldegonGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

float UGodsentAldegonGameplayAbility::GetMoveMontageRate() const
{
	return bUseAttackSpeed ? GetAttackSpeedMontageRate() : AldegonNormalMontageRate;
}

UGodsentAldegonComponent* UGodsentAldegonGameplayAbility::GetAldegonComponent() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	return Avatar ? Avatar->FindComponentByClass<UGodsentAldegonComponent>() : nullptr;
}

AActor* UGodsentAldegonGameplayAbility::GetCombatTarget() const
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());

	if (const AGodsentAIController* AIController = Pawn ? Cast<AGodsentAIController>(Pawn->GetController()) : nullptr)
	{
		if (AActor* Target = AIController->GetTargetActor())
			return Target;
	}

	const UGodsentAldegonComponent* Aldegon = GetAldegonComponent();
	return Aldegon ? Aldegon->GetChallenger() : nullptr;
}

FGameplayEffectSpecHandle UGodsentAldegonGameplayAbility::MakeMoveDamageSpec(const FScalableFloat& Damage, bool bMagic, bool bCausesHitStun)
{
	if (!DamageEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no Damage Effect set (use GE_Shared_DealDamage)"), *GetNameSafe(this));
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = CreateEnemyDamageEffectSpecHandle(DamageEffect, Damage);

	if (!SpecHandle.IsValid())
		return SpecHandle;

	// Magic: straight through defence (like the ghost's Mindfire)
	if (bMagic)
		SpecHandle.Data->AddDynamicAssetTag(GodsentGameplayTags::Shared_Effect_IgnoreDefense);

	if (bCausesHitStun)
		SpecHandle.Data->AddDynamicAssetTag(GodsentGameplayTags::Shared_Effect_HitStun);

	return SpecHandle;
}

void UGodsentAldegonGameplayAbility::FinishMove(bool bStartCooldown)
{
	if (!IsActive())
		return;

	// The cooldown counts from the end of the move, not the start
	if (bStartCooldown)
		CommitAbilityCooldown(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false);

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, !bStartCooldown);
}
