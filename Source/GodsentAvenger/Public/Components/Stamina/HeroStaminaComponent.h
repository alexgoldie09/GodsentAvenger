// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/PawnExtensionComponentBase.h"
#include "HeroStaminaComponent.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/**
 * The hero's stamina rules that aren't plain attribute maths.
 *
 * Regen delay: spending stamina, or finishing an ability that blocks regen (attack, dodge, block...), adds
 * Player.Status.StaminaRegenDelayed for StaminaRegenDelay seconds (curve table: Hero.StaminaRegenDelay).
 * GE_Hero_StaminaRegen ignores that tag, so regen only starts once the hero has stopped spending for a moment.
 *
 * Immortal Vigor: when Player.Passive.ImmortalVigor arrives, stamina fills up and the HUD is told
 * (the attribute set keeps it full from then on).
 */
UCLASS()
class GODSENTAVENGER_API UHeroStaminaComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()

public:
	/// True while regen is waiting out the delay
	UFUNCTION(BlueprintPure, Category = "Godsent|Stamina")
	bool IsRegenDelayed() const { return bRegenDelayed; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnCurrentStaminaChanged(const FOnAttributeChangeData& Data);
	void OnRegenBlockedTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnImmortalVigorTagChanged(const FGameplayTag Tag, int32 NewCount);

	/// Adds the delayed tag (if it isn't on) and restarts the countdown
	void StartRegenDelay();

	/// Removes the delayed tag so regen can run
	void EndRegenDelay();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	FDelegateHandle StaminaChangedHandle;
	FDelegateHandle RegenBlockedTagHandle;
	FDelegateHandle ImmortalVigorTagHandle;

	FTimerHandle RegenDelayTimerHandle;

	/// We add the delayed tag once and take it off once, so other counts on the ASC are never touched
	bool bRegenDelayed = false;
};
