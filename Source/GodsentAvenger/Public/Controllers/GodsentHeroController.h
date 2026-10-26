// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/PlayerController.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "GodsentHeroController.generated.h"

class USkillTreeComponent;

/**
 * The player's controller. Team 0, so enemies (team 1) treat the hero as hostile.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentHeroController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AGodsentHeroController();

	//~ Begin IGenericTeamAgentInterface Interface.
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~ End IGenericTeamAgentInterface Interface.

	/// Skill points and unlocked skills (lives here so it outlasts each heir)
	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree")
	USkillTreeComponent* GetSkillTreeComponent() const { return SkillTreeComponent; }

	/// Debug: adds skill points, e.g. "GodsentGrantSkillPoints 24"
	UFUNCTION(Exec)
	void GodsentGrantSkillPoints(int32 Amount);

	/// Debug: unlocks a skill and its earlier tiers for free, e.g. "GodsentUnlockSkill Skill.Dexterity.Avoidance.GracedEvasion"
	UFUNCTION(Exec)
	void GodsentUnlockSkill(FName SkillTagName);

	/// Console command for testing skill tiers until the skill tree exists, e.g.
	/// "GodsentSetAbilityLevel Player.Ability.Dodge 3" (Graced Evasion) or "GodsentSetAbilityLevel Player.Ability.Attack.Light 2" (Combo)
	UFUNCTION(Exec)
	void GodsentSetAbilityLevel(FName AbilityTagName, int32 NewLevel);
	
	/// Grants an ability from DebugGrantableAbilities by its Asset Tag, or sets its level if it's already granted, e.g.
	/// "GodsentGrantAbility Player.Ability.Light 2"
	UFUNCTION(Exec)
	void GodsentGrantAbility(FName AbilityTagName, int32 Level);

	/// Removes every granted ability with this Asset Tag, e.g. "GodsentRemoveAbility Player.Ability.Light"
	UFUNCTION(Exec)
	void GodsentRemoveAbility(FName AbilityTagName);

protected:
	//~ Begin AController Interface.
	/// Re-applies the lineage's unlocked skills to every newly possessed heir
	virtual void OnPossess(APawn* InPawn) override;
	//~ End AController Interface.

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SkillTree")
	TObjectPtr<USkillTreeComponent> SkillTreeComponent;

	/// Abilities the debug commands can grant. Leave Input Tag empty for passive/OnGiven ones (e.g. Light)
	UPROPERTY(EditDefaultsOnly, Category = "Debug", meta = (TitleProperty = "AbilityToGrant"))
	TArray<FGodsentHeroAbilitySet> DebugGrantableAbilities;

private:
	FGenericTeamId HeroTeamId;
};
