// Alex Goldberg rights reserved

#include "Controllers/GodsentCheatManager.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "Components/SkillTree/SkillTreeComponent.h"
#include "Controllers/GodsentHeroController.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GodsentLineageSubsystem.h"

AGodsentHeroController* UGodsentCheatManager::GetHeroController() const
{
	return Cast<AGodsentHeroController>(GetOuterAPlayerController());
}

UGodsentAbilitySystemComponent* UGodsentCheatManager::GetHeroASC() const
{
	const APlayerController* PlayerController = GetOuterAPlayerController();
	return PlayerController ? Cast<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerController->GetPawn())) : nullptr;
}

void UGodsentCheatManager::GodsentNewGame()
{
	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(GetOuterAPlayerController()))
		Lineage->StartNewGameAndOpenLevel(GetOuterAPlayerController());
}

void UGodsentCheatManager::GodsentSetGeneration(int32 NewGeneration)
{
	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(GetOuterAPlayerController()))
	{
		Lineage->DebugSetGeneration(NewGeneration);
		UE_LOG(LogTemp, Warning, TEXT("GodsentSetGeneration: generation %d, enemy tier %d (applies to enemies spawned from now on)"),
			Lineage->GetLineageData().Generation, Lineage->GetEnemyTier());
	}
}

void UGodsentCheatManager::GodsentPrintLineage()
{
	if (const UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(GetOuterAPlayerController()))
		Lineage->LogLineage();
}

void UGodsentCheatManager::GodsentGrantSkillPoints(int32 Amount)
{
	const AGodsentHeroController* HeroController = GetHeroController();
	USkillTreeComponent* SkillTree = HeroController ? HeroController->GetSkillTreeComponent() : nullptr;

	if (SkillTree)
		SkillTree->AddSkillPoints(Amount);

	UE_LOG(LogTemp, Warning, TEXT("GodsentGrantSkillPoints: now %d"), SkillTree ? SkillTree->GetSkillPoints() : 0);
}

void UGodsentCheatManager::GodsentUnlockSkill(FName SkillTagName)
{
	const AGodsentHeroController* HeroController = GetHeroController();
	USkillTreeComponent* SkillTree = HeroController ? HeroController->GetSkillTreeComponent() : nullptr;

	const FGameplayTag SkillTag = FGameplayTag::RequestGameplayTag(SkillTagName, false);
	const bool bUnlocked = SkillTag.IsValid() && SkillTree && SkillTree->ForceUnlockSkill(SkillTag);

	UE_LOG(LogTemp, Warning, TEXT("GodsentUnlockSkill: %s %s"), *SkillTagName.ToString(),
		bUnlocked ? TEXT("unlocked") : TEXT("not found, not built yet, or no skill tree data"));
}

void UGodsentCheatManager::GodsentSetAbilityLevel(FName AbilityTagName, int32 NewLevel)
{
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = GetHeroASC();

	if (!AbilityTag.IsValid() || !ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("GodsentSetAbilityLevel: unknown tag %s or no ability system"), *AbilityTagName.ToString());
		return;
	}

	const bool bFound = ASC->SetAbilityLevelByTag(AbilityTag, NewLevel);
	UE_LOG(LogTemp, Warning, TEXT("GodsentSetAbilityLevel: %s -> level %d (%s)"), *AbilityTagName.ToString(), NewLevel, bFound ? TEXT("set") : TEXT("ability not granted"));
}

void UGodsentCheatManager::GodsentGrantAbility(FName AbilityTagName, int32 Level)
{
	const AGodsentHeroController* HeroController = GetHeroController();
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = GetHeroASC();

	if (!HeroController || !AbilityTag.IsValid() || !ASC)
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

	const FGodsentHeroAbilitySet* Entry = HeroController->GetDebugGrantableAbilities().FindByPredicate([&AbilityTag](const FGodsentHeroAbilitySet& AbilitySet)
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
		AbilitySpec.SourceObject = HeroController->GetPawn();
		ASC->GiveAbility(AbilitySpec);
	}

	UE_LOG(LogTemp, Warning, TEXT("GodsentGrantAbility: granted %s at level %d"), *AbilityTagName.ToString(), Level);
}

void UGodsentCheatManager::GodsentRemoveAbility(FName AbilityTagName)
{
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = GetHeroASC();

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