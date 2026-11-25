// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GodsentCheatManager.generated.h"

class AGodsentHeroController;
class UGodsentAbilitySystemComponent;
class UGodsentAldegonComponent;
struct FGameplayTag;

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

	/// Gives the current heir their Saintly Revival back, e.g. "GodsentResetRevival"
	UFUNCTION(Exec)
	void GodsentResetRevival();

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

	// ---- Aldegon (Stage 17: test his body without his behaviour tree) ----

	/// Gets him up off the throne; he takes off once he's up
	UFUNCTION(Exec)
	void GodsentAldegonWake();

	/// Grounded → Airborne
	UFUNCTION(Exec)
	void GodsentAldegonTakeOff();

	/// Airborne: flies over the player
	UFUNCTION(Exec)
	void GodsentAldegonFlyToPlayer();

	/// Airborne: flies to a random reachable spot within Radius of the player, e.g. "GodsentAldegonFlyRandom 1500"
	UFUNCTION(Exec)
	void GodsentAldegonFlyRandom(float Radius = 1500.f);

	/// Airborne: lands on the navmesh right below him
	UFUNCTION(Exec)
	void GodsentAldegonLand();

	/// Airborne: lands Distance away from the player, on his side, e.g. "GodsentAldegonLandNearPlayer 600"
	UFUNCTION(Exec)
	void GodsentAldegonLandNearPlayer(float Distance = 600.f);

	/// Any awake state: flies back to the throne, sits down and heals
	UFUNCTION(Exec)
	void GodsentAldegonReturnHome();

	/// Launches him like a knockback would (airborne: he falls and lands, ending up Grounded)
	UFUNCTION(Exec)
	void GodsentAldegonKnockDown();

	/// 1 = face the player while flying, 0 = face where he's going
	UFUNCTION(Exec)
	void GodsentAldegonFacePlayer(bool bFacePlayer);

	/// Logs his state, whether he's flying and how high he is, plus the fight counters
	UFUNCTION(Exec)
	void GodsentAldegonState();

	// ---- Aldegon fight (Stage 18) ----

	/// Starts the fight as if you walked into the arena (targets you and wakes him)
	UFUNCTION(Exec)
	void GodsentAldegonStartFight();

	/// Ends the fight as if you walked out (he clears his summons and goes home)
	UFUNCTION(Exec)
	void GodsentAldegonEndFight();

	/// Takes this % of his max health off, to test health bands and frenzy, e.g. "GodsentAldegonHurt 15"
	UFUNCTION(Exec)
	void GodsentAldegonHurt(float Percent = 10.f);

	// ---- Aldegon summons (Stage 20) ----

	/// Casts Summon now if he can (any state; cap and cooldown still apply)
	UFUNCTION(Exec)
	void GodsentAldegonSummon();

	/// Casts Life Siphon now if he can (cap and cooldown still apply)
	UFUNCTION(Exec)
	void GodsentAldegonSiphon();

	/// Kills every add and totem he has out
	UFUNCTION(Exec)
	void GodsentAldegonKillSummons();

private:
	AGodsentHeroController* GetHeroController() const;
	UGodsentAbilitySystemComponent* GetHeroASC() const;

	/// The first Aldegon in the level (null, with a log, if there isn't one)
	UGodsentAldegonComponent* FindAldegon(const TCHAR* CommandName) const;

	/// Logs whether an Aldegon command was accepted (most only work in certain states)
	void LogAldegonCommand(const TCHAR* CommandName, bool bAccepted, const UGodsentAldegonComponent* Aldegon) const;

	/// Tries to activate one of his abilities by tag and logs the result
	void TryAldegonAbility(const TCHAR* CommandName, const FGameplayTag& AbilityTag) const;
};
