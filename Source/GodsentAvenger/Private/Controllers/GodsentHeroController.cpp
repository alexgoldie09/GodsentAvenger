// Alex Goldberg rights reserved

#include "Controllers/GodsentHeroController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"

AGodsentHeroController::AGodsentHeroController()
{
	HeroTeamId = FGenericTeamId(0);
}

FGenericTeamId AGodsentHeroController::GetGenericTeamId() const
{
	return HeroTeamId;
}

/// Console command for testing skill tiers until the skill tree exists
void AGodsentHeroController::GodsentSetAbilityLevel(FName AbilityTagName, int32 NewLevel)
{
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = Cast<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()));

	if (!AbilityTag.IsValid() || !ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("GodsentSetAbilityLevel: unknown tag %s or no ability system"), *AbilityTagName.ToString());
		return;
	}

	const bool bFound = ASC->SetAbilityLevelByTag(AbilityTag, NewLevel);
	UE_LOG(LogTemp, Warning, TEXT("GodsentSetAbilityLevel: %s -> level %d (%s)"), *AbilityTagName.ToString(), NewLevel, bFound ? TEXT("set") : TEXT("ability not granted"));
}

/// Grants an ability from DebugGrantableAbilities by its Asset Tag, or sets its level if it's already granted
void AGodsentHeroController::GodsentGrantAbility(FName AbilityTagName, int32 Level)
{
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = Cast<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()));

	if (!AbilityTag.IsValid() || !ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("GodsentGrantAbility: unknown tag %s or no ability system"), *AbilityTagName.ToString());
		return;
	}

	Level = FMath::Max(Level, 1);

	// Already granted: just change the tier
	if (ASC->GetAbilityLevelByTag(AbilityTag) > 0)
	{
		ASC->SetAbilityLevelByTag(AbilityTag, Level);
		UE_LOG(LogTemp, Warning, TEXT("GodsentGrantAbility: %s already granted, level set to %d"), *AbilityTagName.ToString(), Level);
		return;
	}

	const FGodsentHeroAbilitySet* Entry = DebugGrantableAbilities.FindByPredicate([&AbilityTag](const FGodsentHeroAbilitySet& AbilitySet)
	{
		return AbilitySet.AbilityToGrant && AbilitySet.AbilityToGrant->GetDefaultObject<UGameplayAbility>()->GetAssetTags().HasTagExact(AbilityTag);
	});

	if (!Entry)
	{
		UE_LOG(LogTemp, Warning, TEXT("GodsentGrantAbility: %s isn't in BP_HeroController's Debug Grantable Abilities"), *AbilityTagName.ToString());
		return;
	}

	if (Entry->InputTag.IsValid())
	{
		ASC->GrantHeroAbility(*Entry, Level); // Input-bound (Heal, Dodge, Block...)
	}
	else
	{
		// No input: passive / OnGiven abilities (Light activates itself as soon as it's given)
		FGameplayAbilitySpec AbilitySpec(Entry->AbilityToGrant, Level);
		AbilitySpec.SourceObject = GetPawn();
		ASC->GiveAbility(AbilitySpec);
	}

	UE_LOG(LogTemp, Warning, TEXT("GodsentGrantAbility: granted %s at level %d"), *AbilityTagName.ToString(), Level);
}

/// Removes every granted ability with this Asset Tag
void AGodsentHeroController::GodsentRemoveAbility(FName AbilityTagName)
{
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = Cast<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()));

	if (!AbilityTag.IsValid() || !ASC)
		return;

	TArray<FGameplayAbilitySpecHandle> HandlesToRemove;

	for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.Ability->GetAssetTags().HasTagExact(AbilityTag))
			HandlesToRemove.Add(AbilitySpec.Handle);
	}

	for (const FGameplayAbilitySpecHandle& Handle : HandlesToRemove)
		ASC->ClearAbility(Handle); // Active ones (Light) are ended first, so OnEndAbility still runs

	UE_LOG(LogTemp, Warning, TEXT("GodsentRemoveAbility: removed %d ability(s) with %s"), HandlesToRemove.Num(), *AbilityTagName.ToString());
}
