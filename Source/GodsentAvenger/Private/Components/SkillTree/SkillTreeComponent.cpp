// Alex Goldberg rights reserved

#include "Components/SkillTree/SkillTreeComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"
#include "DataAssets/SkillTree/DataAsset_SkillTree.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

USkillTreeComponent::USkillTreeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USkillTreeComponent::BeginPlay()
{
	Super::BeginPlay();

	SkillPoints = StartingSkillPoints;
	OnSkillPointsChanged.Broadcast(SkillPoints);
}

/// Adds (or removes) skill points
void USkillTreeComponent::AddSkillPoints(int32 Amount)
{
	SkillPoints = FMath::Max(SkillPoints + Amount, 0);
	OnSkillPointsChanged.Broadcast(SkillPoints);
}

bool USkillTreeComponent::IsSkillUnlocked(FGameplayTag SkillTag) const
{
	return UnlockedSkills.HasTagExact(SkillTag);
}

/// Unlocked / Available / CantAfford / Locked / Unavailable
EGodsentSkillState USkillTreeComponent::GetSkillState(FGameplayTag SkillTag) const
{
	const FGodsentSkillNode* SkillNode = SkillTreeData ? SkillTreeData->FindSkillNode(SkillTag) : nullptr;

	if (!SkillNode || !SkillNode->IsImplemented())
		return EGodsentSkillState::Unavailable;

	if (UnlockedSkills.HasTagExact(SkillTag))
		return EGodsentSkillState::Unlocked;

	if (!UnlockedSkills.HasAllExact(SkillNode->Prerequisites))
		return EGodsentSkillState::Locked;

	if (SkillPoints < SkillNode->Cost)
		return EGodsentSkillState::CantAfford;

	return EGodsentSkillState::Available;
}

/// The skill's data, or false if it isn't in the tree
bool USkillTreeComponent::GetSkillNode(FGameplayTag SkillTag, FGodsentSkillNode& OutSkillNode) const
{
	const FGodsentSkillNode* SkillNode = SkillTreeData ? SkillTreeData->FindSkillNode(SkillTag) : nullptr;

	if (!SkillNode)
		return false;

	OutSkillNode = *SkillNode;
	return true;
}

/// Buys the skill if it's Available
bool USkillTreeComponent::TryUnlockSkill(FGameplayTag SkillTag)
{
	if (GetSkillState(SkillTag) != EGodsentSkillState::Available)
		return false;

	const FGodsentSkillNode* SkillNode = SkillTreeData->FindSkillNode(SkillTag);
	check(SkillNode);

	SkillPoints -= SkillNode->Cost;
	OnSkillPointsChanged.Broadcast(SkillPoints);

	CompleteUnlock(*SkillNode);
	return true;
}

/// Debug: unlocks the skill and any missing prerequisites for free
bool USkillTreeComponent::ForceUnlockSkill(const FGameplayTag& SkillTag)
{
	const FGodsentSkillNode* SkillNode = SkillTreeData ? SkillTreeData->FindSkillNode(SkillTag) : nullptr;

	if (!SkillNode || !SkillNode->IsImplemented())
		return false;

	if (UnlockedSkills.HasTagExact(SkillTag))
		return true;

	// Mark it first so a prerequisite loop in the data can't recurse forever
	UnlockedSkills.AddTag(SkillTag);

	for (const FGameplayTag& PrerequisiteTag : SkillNode->Prerequisites)
		ForceUnlockSkill(PrerequisiteTag);

	UnlockedSkills.RemoveTag(SkillTag);
	CompleteUnlock(*SkillNode);
	return true;
}

/// Re-applies every unlocked skill to the possessed hero (a new heir starts with nothing granted)
void USkillTreeComponent::ApplyAllUnlockedSkills()
{
	// Any previous handles belonged to the previous heir's ability system
	AppliedPassiveEffects.Empty();

	if (!SkillTreeData)
		return;

	for (const FGodsentSkillNode& SkillNode : SkillTreeData->SkillNodes)
	{
		if (UnlockedSkills.HasTagExact(SkillNode.SkillTag))
			ApplySkillNode(SkillNode);
	}
}

/// Adds the skill to the unlocked set, applies it and broadcasts
void USkillTreeComponent::CompleteUnlock(const FGodsentSkillNode& SkillNode)
{
	UnlockedSkills.AddTag(SkillNode.SkillTag);
	ApplySkillNode(SkillNode);
	OnSkillUnlocked.Broadcast(SkillNode.SkillTag);
}

/// Grants or levels the ability and applies the passive effect of one skill
void USkillTreeComponent::ApplySkillNode(const FGodsentSkillNode& SkillNode)
{
	UGodsentAbilitySystemComponent* ASC = GetPawnAbilitySystemComponent();

	if (!ASC)
		return;

	if (SkillNode.Ability.AbilityToGrant)
	{
		if (FGameplayAbilitySpec* ExistingSpec = ASC->FindAbilitySpecFromClass(SkillNode.Ability.AbilityToGrant))
		{
			// Already owned (e.g. Leap after Hop, or the sword attack granted by the weapon): raise its tier, never lower it
			if (ExistingSpec->Level < SkillNode.AbilityLevel)
			{
				ExistingSpec->Level = SkillNode.AbilityLevel;
				ASC->MarkAbilitySpecDirty(*ExistingSpec);
			}
		}
		else if (SkillNode.Ability.InputTag.IsValid())
		{
			// Bound to its key; its HUD icon broadcasts itself on grant
			ASC->GrantHeroAbility(SkillNode.Ability, SkillNode.AbilityLevel);
		}
		else
		{
			// No key: event-triggered passives (Holy Retribution)
			FGameplayAbilitySpec NewSpec(SkillNode.Ability.AbilityToGrant, SkillNode.AbilityLevel);
			NewSpec.SourceObject = ASC->GetAvatarActor();
			ASC->GiveAbility(NewSpec);
		}
	}

	if (SkillNode.PassiveEffect && !AppliedPassiveEffects.Contains(SkillNode.SkillTag))
	{
		const UGameplayEffect* PassiveEffectCDO = SkillNode.PassiveEffect->GetDefaultObject<UGameplayEffect>();
		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(PassiveEffectCDO, SkillNode.PassiveEffectLevel, ASC->MakeEffectContext());
		AppliedPassiveEffects.Add(SkillNode.SkillTag, Handle);
	}
}

/// The possessed hero's ability system, or nullptr
UGodsentAbilitySystemComponent* USkillTreeComponent::GetPawnAbilitySystemComponent() const
{
	const AController* OwningController = GetOwner<AController>();
	APawn* Pawn = OwningController ? OwningController->GetPawn() : nullptr;

	return Pawn ? Cast<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn)) : nullptr;
}
