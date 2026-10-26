// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "SkillTreeComponent.generated.h"

class UDataAsset_SkillTree;
class UGodsentAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillPointsChangedDelegate, int32, NewSkillPoints);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillUnlockedDelegate, FGameplayTag, SkillTag);

/**
 * Skill points and unlocked skills for the current lineage.
 * Lives on the hero controller (not the hero) so a new heir can be possessed and simply have everything re-applied.
 */
UCLASS(ClassGroup = (Godsent), meta = (BlueprintSpawnableComponent))
class GODSENTAVENGER_API USkillTreeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USkillTreeComponent();

	/// Fires whenever the skill point total changes (the HUD / skill tree counter)
	UPROPERTY(BlueprintAssignable, Category = "SkillTree")
	FOnSkillPointsChangedDelegate OnSkillPointsChanged;

	/// Fires after a skill is unlocked and applied (every skill button refreshes its state)
	UPROPERTY(BlueprintAssignable, Category = "SkillTree")
	FOnSkillUnlockedDelegate OnSkillUnlocked;

	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree")
	int32 GetSkillPoints() const { return SkillPoints; }

	/// Adds (or with a negative amount removes) skill points. Rebirth will call this with 1.
	UFUNCTION(BlueprintCallable, Category = "Godsent|SkillTree")
	void AddSkillPoints(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree", meta = (GameplayTagFilter = "Skill"))
	bool IsSkillUnlocked(FGameplayTag SkillTag) const;

	/// Unlocked / Available / CantAfford / Locked / Unavailable, for the skill buttons
	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree", meta = (GameplayTagFilter = "Skill"))
	EGodsentSkillState GetSkillState(FGameplayTag SkillTag) const;

	/// The skill's data (name, description, icon, cost). Returns false if it isn't in the tree.
	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree", meta = (GameplayTagFilter = "Skill"))
	bool GetSkillNode(FGameplayTag SkillTag, FGodsentSkillNode& OutSkillNode) const;

	/// Buys the skill if it's Available: spends the points, applies it to the hero and broadcasts. Returns true on success.
	UFUNCTION(BlueprintCallable, Category = "Godsent|SkillTree", meta = (GameplayTagFilter = "Skill"))
	bool TryUnlockSkill(FGameplayTag SkillTag);

	/// Debug: unlocks the skill and any missing prerequisites for free
	bool ForceUnlockSkill(const FGameplayTag& SkillTag);

	/// Re-applies every unlocked skill to the possessed hero. The controller calls this after possessing a new heir.
	void ApplyAllUnlockedSkills();

	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree")
	FGameplayTagContainer GetUnlockedSkills() const { return UnlockedSkills; }

protected:
	virtual void BeginPlay() override;

	/// The skill tree data asset (DA_SkillTree)
	UPROPERTY(EditDefaultsOnly, Category = "SkillTree")
	TObjectPtr<UDataAsset_SkillTree> SkillTreeData;

	/// Points the first heir starts with (0 in the real game; the cheat command adds more)
	UPROPERTY(EditDefaultsOnly, Category = "SkillTree", meta = (ClampMin = "0"))
	int32 StartingSkillPoints = 0;

private:
	/// Grants or levels the ability and applies the passive effect of one skill
	void ApplySkillNode(const FGodsentSkillNode& SkillNode);

	/// Adds the skill to the unlocked set, applies it and broadcasts
	void CompleteUnlock(const FGodsentSkillNode& SkillNode);

	/// The possessed hero's ability system, or nullptr
	UGodsentAbilitySystemComponent* GetPawnAbilitySystemComponent() const;

	int32 SkillPoints = 0;

	FGameplayTagContainer UnlockedSkills;

	/// Passive effects already applied to the current hero, so a passive is never stacked twice
	TMap<FGameplayTag, FActiveGameplayEffectHandle> AppliedPassiveEffects;
};
