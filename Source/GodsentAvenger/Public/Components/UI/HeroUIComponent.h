// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/UI/PawnUIComponent.h"
#include "HeroUIComponent.generated.h"

class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAbilityCooldownBegin, FGameplayTag, AbilityInputTag, float, CooldownDuration, float, TimeRemaining);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityIconSlotUpdatedDelegate, FGameplayTag, AbilityInputTag, TSoftObjectPtr<UMaterialInterface>, SoftWeaponMaterial);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInfiniteStaminaGrantedDelegate);

/**
 * Hero-only UI events (stamina bar and ability cooldowns).
 */
UCLASS()
class GODSENTAVENGER_API UHeroUIComponent : public UPawnUIComponent
{
	GENERATED_BODY()

public:
	/// Fires with CurrentStamina / MaxStamina whenever either changes
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnPercentageChangedDelegate OnCurrentStaminaChanged;

	/// Fires with the new MaxStamina when it changes (the stamina bar width follows it)
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnMaxValueChangedDelegate OnMaxStaminaChanged;

	/// Immortal Vigor: stamina is now infinite, so the HUD can play the bar's send-off and hide it.
	/// Can fire more than once (on grant and on Refresh Status Bars), so the widget should only play it the first time.
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnInfiniteStaminaGrantedDelegate OnInfiniteStaminaGranted;
	
	/// Fires when an ability's icon slot is updated (e.g. Light Attack, Heal)
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "UI")
	FOnAbilityIconSlotUpdatedDelegate OnAbilityIconSlotUpdated;

	/// Broadcast by an ability when its cooldown starts (e.g. Heal)
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "UI")
	FOnAbilityCooldownBegin OnAbilityCooldownBegin;

	/// Health and stamina (values, max widths and Immortal Vigor)
	virtual void RefreshStatusBars() override;

	/// True when the hero has Immortal Vigor
	UFUNCTION(BlueprintPure, Category = "UI")
	bool HasInfiniteStamina() const;
};
