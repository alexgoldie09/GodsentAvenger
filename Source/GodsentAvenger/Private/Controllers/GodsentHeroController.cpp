// Alex Goldberg rights reserved

#include "Controllers/GodsentHeroController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
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
