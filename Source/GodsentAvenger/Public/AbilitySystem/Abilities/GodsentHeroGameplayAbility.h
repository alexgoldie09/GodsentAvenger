// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GodsentGameplayAbility.h"
#include "GodsentHeroGameplayAbility.generated.h"

class AGodsentHeroCharacter;
class AGodsentHeroController;
class UHeroCombatComponent;
class UMaterialInterface;

/**
 * Base class for the hero's abilities.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentHeroGameplayAbility : public UGodsentGameplayAbility
{
	GENERATED_BODY()

public:
	/// Gets (and caches) the hero character that owns this ability
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	AGodsentHeroCharacter* GetHeroCharacterFromActorInfo();

	/// Gets (and caches) the hero's player controller
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	AGodsentHeroController* GetHeroControllerFromActorInfo();

	/// Gets the hero's combat component
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	UHeroCombatComponent* GetHeroCombatComponentFromActorInfo();

	/// Builds the damage spec for a hero attack: weapon base damage plus the combo count for the attack type.
	/// The damage calculation scales it by AttackPower / DefensePower.
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	FGameplayEffectSpecHandle CreateHeroDamageEffectSpecHandle(TSubclassOf<UGameplayEffect> EffectClass, float InWeaponBaseDamage, FGameplayTag InCurrentAttackTypeTag, int32 InUsedComboCount);

	/// Gets the total and remaining time of the cooldown effect carrying InCooldownTag.
	/// Returns true while the cooldown is still running.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability")
	bool GetAbilityRemainingCooldownByTag(FGameplayTag InCooldownTag, float& TotalCooldownTime, float& RemainingCooldownTime);

	/// The hero's AttackSpeed attribute, for the Rate pin of Play Montage And Wait (1 = normal speed)
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	float GetAttackSpeedMontageRate() const;
	
	/// Sends this ability's icon, and its cooldown if one is running, to the HUD slot for its input tag.
	/// Called automatically on grant and on cooldown; the ASC's Broadcast Hero Ability UI calls it for widgets made later.
	void BroadcastAbilityUI(const FGameplayAbilitySpec& Spec, const FGameplayAbilityActorInfo* ActorInfo) const;

protected:
	//~ Begin UGameplayAbility Interface.
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	//~ End UGameplayAbility Interface.

	/// HUD icon for this ability's slot. Leave empty for abilities without a slot (attack, dodge, block...)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSoftObjectPtr<UMaterialInterface> SoftAbilityIconMaterial;

	/// The tag this ability's cooldown GE grants (e.g. Player.Cooldown.Heal), so the HUD can show the countdown
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI", meta = (Categories = "Player.Cooldown"))
	FGameplayTag AbilityCooldownTag;
	
	/// When above 0, the cooldown GE is applied at this level instead of the ability level
	/// (e.g. the heal sets it to ChargeTier so bigger charges cool down longer). Reset it each activation.
	UPROPERTY(BlueprintReadWrite, Category = "Cooldown")
	int32 CooldownLevelOverride = 0;
	
	/// When above 0, the cost GE is checked and applied at this level instead of the ability level.
	/// The activation check uses it too, so set the starting value in Class Defaults and reset it when the ability ends.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cost")
	int32 CostLevelOverride = 0;

private:
	/// Cached hero character, to avoid casting on every call
	TWeakObjectPtr<AGodsentHeroCharacter> CachedHeroCharacter;

	/// Cached hero controller, to avoid casting on every call
	TWeakObjectPtr<AGodsentHeroController> CachedHeroController;
	
	/// Sends the cooldown to the HUD if AbilityCooldownTag's effect is running
	void BroadcastCooldownIfRunning(const FGameplayTag& InputTag, const FGameplayAbilityActorInfo* ActorInfo) const;
};
