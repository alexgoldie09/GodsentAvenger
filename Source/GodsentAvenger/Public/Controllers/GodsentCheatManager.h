// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GodsentCheatManager.generated.h"

class AGodsentHeroController;
class UGodsentAbilitySystemComponent;

/**
 * Every Godsent debug console command lives here, not on the hero controller.
 * Unreal never creates a cheat manager in Shipping builds (UE_WITH_CHEAT_MANAGER is 0) and Shipping has no console,
 * so none of these exist in the submitted build (BLD-04). They work as before in the editor and Development builds.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/// Wipes the lineage (and its save) and reloads the map, e.g. "GodsentNewGame"
	UFUNCTION(Exec)
	void GodsentNewGame();

	/// Sets the generation (enemy tier = 1 + (generation - 1) / 5), e.g. "GodsentSetGeneration 6"
	UFUNCTION(Exec)
	void GodsentSetGeneration(int32 NewGeneration);

	/// Prints the lineage data to the log without saving, e.g. "GodsentPrintLineage"
	UFUNCTION(Exec)
	void GodsentPrintLineage();

	/// Adds skill points, e.g. "GodsentGrantSkillPoints 24"
	UFUNCTION(Exec)
	void GodsentGrantSkillPoints(int32 Amount);

	/// Unlocks a skill and its earlier tiers for free, e.g. "GodsentUnlockSkill Skill.Dexterity.Avoidance.GracedEvasion"
	UFUNCTION(Exec)
	void GodsentUnlockSkill(FName SkillTagName);

	/// Sets a granted ability's tier, e.g. "GodsentSetAbilityLevel Player.Ability.Dodge 3"
	UFUNCTION(Exec)
	void GodsentSetAbilityLevel(FName AbilityTagName, int32 NewLevel);

	/// Grants an ability from BP_HeroController's Debug Grantable Abilities by its Asset Tag, or sets its level if already granted,
	/// e.g. "GodsentGrantAbility Player.Ability.Light 2"
	UFUNCTION(Exec)
	void GodsentGrantAbility(FName AbilityTagName, int32 Level);

	/// Removes every granted ability with this Asset Tag, e.g. "GodsentRemoveAbility Player.Ability.Light"
	UFUNCTION(Exec)
	void GodsentRemoveAbility(FName AbilityTagName);

private:
	AGodsentHeroController* GetHeroController() const;
	UGodsentAbilitySystemComponent* GetHeroASC() const;
};
