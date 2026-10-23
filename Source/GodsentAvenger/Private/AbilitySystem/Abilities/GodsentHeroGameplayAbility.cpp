// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/GodsentAttributeSet.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "Characters/GodsentHeroCharacter.h"
#include "Controllers/GodsentHeroController.h"
#include "Components/Combat/HeroCombatComponent.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "GameplayEffect.h"
#include "Components/UI/HeroUIComponent.h"
#include "Interfaces/PawnUIInterface.h"
#include "Materials/MaterialInterface.h"

/// Gets (and caches) the hero character that owns this ability
AGodsentHeroCharacter* UGodsentHeroGameplayAbility::GetHeroCharacterFromActorInfo()
{
	if (!CachedHeroCharacter.IsValid())
		CachedHeroCharacter = Cast<AGodsentHeroCharacter>(GetAvatarActorFromActorInfo());

	return CachedHeroCharacter.IsValid() ? CachedHeroCharacter.Get() : nullptr;
}

/// Gets (and caches) the hero's player controller
AGodsentHeroController* UGodsentHeroGameplayAbility::GetHeroControllerFromActorInfo()
{
	if (!CachedHeroController.IsValid() && CurrentActorInfo)
		CachedHeroController = Cast<AGodsentHeroController>(CurrentActorInfo->PlayerController.Get());

	return CachedHeroController.IsValid() ? CachedHeroController.Get() : nullptr;
}

/// Gets the hero's combat component
UHeroCombatComponent* UGodsentHeroGameplayAbility::GetHeroCombatComponentFromActorInfo()
{
	AGodsentHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();
	return HeroCharacter ? HeroCharacter->GetHeroCombatComponent() : nullptr;
}

/// Builds the damage spec for a hero attack
FGameplayEffectSpecHandle UGodsentHeroGameplayAbility::CreateHeroDamageEffectSpecHandle(
	TSubclassOf<UGameplayEffect> EffectClass, float InWeaponBaseDamage, FGameplayTag InCurrentAttackTypeTag,
	int32 InUsedComboCount)
{
	check(EffectClass);

	FGameplayEffectContextHandle ContextHandle = GetGodsentAbilitySystemComponentFromActorInfo()->MakeEffectContext();
	ContextHandle.SetAbility(this);
	ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());
	ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

	FGameplayEffectSpecHandle EffectSpecHandle = GetGodsentAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
		EffectClass,
		GetAbilityLevel(),
		ContextHandle);

	EffectSpecHandle.Data->SetSetByCallerMagnitude(GodsentGameplayTags::Shared_SetByCaller_BaseDamage, InWeaponBaseDamage);

	if (InCurrentAttackTypeTag.IsValid())
		EffectSpecHandle.Data->SetSetByCallerMagnitude(InCurrentAttackTypeTag, InUsedComboCount);

	return EffectSpecHandle;
}

/// Gets the total and remaining time of the cooldown effect carrying InCooldownTag
bool UGodsentHeroGameplayAbility::GetAbilityRemainingCooldownByTag(FGameplayTag InCooldownTag, float& TotalCooldownTime,
	float& RemainingCooldownTime)
{
	check(InCooldownTag.IsValid());

	// Start from zero so "no cooldown running" returns false instead of reading uninitialised values
	TotalCooldownTime = 0.f;
	RemainingCooldownTime = 0.f;

	const FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(InCooldownTag.GetSingleTagContainer());
	const TArray<TPair<float, float>> TimeRemainingAndDuration = GetAbilitySystemComponentFromActorInfo()->GetActiveEffectsTimeRemainingAndDuration(CooldownQuery);

	if (!TimeRemainingAndDuration.IsEmpty())
	{
		RemainingCooldownTime = TimeRemainingAndDuration[0].Key;
		TotalCooldownTime = TimeRemainingAndDuration[0].Value;
	}

	return RemainingCooldownTime > 0.f;
}

/// The hero's AttackSpeed attribute, for the Rate pin of Play Montage And Wait
float UGodsentHeroGameplayAbility::GetAttackSpeedMontageRate() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!ASC)
		return 1.f;

	bool bFound = false;
	const float AttackSpeed = ASC->GetGameplayAttributeValue(UGodsentAttributeSet::GetAttackSpeedAttribute(), bFound);

	return bFound ? FMath::Max(AttackSpeed, 0.1f) : 1.f;
}

/// Snaps the hero to face the target-locked enemy
bool UGodsentHeroGameplayAbility::FaceLockedOnTarget()
{
	AGodsentHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();
	const UHeroCombatComponent* HeroCombatComponent = GetHeroCombatComponentFromActorInfo();
	AActor* LockedOnTarget = HeroCombatComponent ? HeroCombatComponent->GetLockedOnTarget() : nullptr;

	if (!HeroCharacter || !LockedOnTarget)
		return false;

	UGodsentFunctionLibrary::RotateActorToFaceTarget(HeroCharacter, LockedOnTarget);
	return true;
}

namespace 
{
	/// The hero's UI component from an ability's actor info, or nullptr
	UHeroUIComponent* GetHeroUIComponentFromActorInfo(const FGameplayAbilityActorInfo* ActorInfo)
	{
		if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
			return nullptr;

		const IPawnUIInterface* PawnUIInterface = Cast<IPawnUIInterface>(ActorInfo->AvatarActor.Get());
		return PawnUIInterface ? PawnUIInterface->GetHeroUIComponent() : nullptr;
	}
}

/// Puts the icon in the HUD as soon as the ability is granted
void UGodsentHeroGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	BroadcastAbilityUI(Spec, ActorInfo);
}

/// Starts the HUD countdown whenever the cooldown GE is applied (Commit Ability or Commit Ability Cooldown)
void UGodsentHeroGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();

	if (CooldownGE && CooldownLevelOverride > 0)
		ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CooldownGE, CooldownLevelOverride);
	else
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
		return;

	if (const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
		BroadcastCooldownIfRunning(Spec->GetDynamicSpecSourceTags().First(), ActorInfo);
}

/// Sends this ability's icon, and its cooldown if one is running, to the HUD slot for its input tag
void UGodsentHeroGameplayAbility::BroadcastAbilityUI(const FGameplayAbilitySpec& Spec, const FGameplayAbilityActorInfo* ActorInfo) const
{
	// Hero abilities are granted with exactly one dynamic tag: the input tag that the HUD slot listens for
	const FGameplayTag InputTag = Spec.GetDynamicSpecSourceTags().First();
	UHeroUIComponent* HeroUIComponent = GetHeroUIComponentFromActorInfo(ActorInfo);

	if (!InputTag.IsValid() || !HeroUIComponent)
		return;

	if (!SoftAbilityIconMaterial.IsNull())
		HeroUIComponent->OnAbilityIconSlotUpdated.Broadcast(InputTag, SoftAbilityIconMaterial);

	BroadcastCooldownIfRunning(InputTag, ActorInfo);
}

/// Sends the cooldown to the HUD if AbilityCooldownTag's effect is running
void UGodsentHeroGameplayAbility::BroadcastCooldownIfRunning(const FGameplayTag& InputTag, const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!InputTag.IsValid() || !AbilityCooldownTag.IsValid() || !ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
		return;

	UHeroUIComponent* HeroUIComponent = GetHeroUIComponentFromActorInfo(ActorInfo);

	if (!HeroUIComponent)
		return;

	const FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(AbilityCooldownTag.GetSingleTagContainer());
	const TArray<TPair<float, float>> TimeRemainingAndDuration = ActorInfo->AbilitySystemComponent->GetActiveEffectsTimeRemainingAndDuration(CooldownQuery);

	if (TimeRemainingAndDuration.IsEmpty() || TimeRemainingAndDuration[0].Key <= 0.f)
		return;

	HeroUIComponent->OnAbilityCooldownBegin.Broadcast(InputTag, TimeRemainingAndDuration[0].Value, TimeRemainingAndDuration[0].Key);
}

/// Checks the cost at CostLevelOverride when it's set, otherwise at the ability level
bool UGodsentHeroGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UGameplayEffect* CostGE = GetCostGameplayEffect();

	if (!CostGE || CostLevelOverride <= 0)
		return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	if (!ASC)
		return false;

	if (ASC->CanApplyAttributeModifiers(CostGE, CostLevelOverride, MakeEffectContext(Handle, ActorInfo)))
		return true;

	const FGameplayTag& CostFailTag = UAbilitySystemGlobals::Get().ActivateFailCostTag;

	if (OptionalRelevantTags && CostFailTag.IsValid())
		OptionalRelevantTags->AddTag(CostFailTag);

	return false;
}

/// Applies the cost at CostLevelOverride when it's set, otherwise at the ability level
void UGodsentHeroGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGameplayEffect* CostGE = GetCostGameplayEffect();

	if (CostGE && CostLevelOverride > 0)
		ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CostGE, CostLevelOverride);
	else
		Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}
