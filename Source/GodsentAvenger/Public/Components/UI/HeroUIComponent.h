// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/UI/PawnUIComponent.h"
#include "HeroUIComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAbilityCooldownBegin, FGameplayTag, AbilityInputTag, float, CooldownDuration, float, TimeRemaining);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityIconSlotUpdatedDelegate, FGameplayTag, AbilityInputTag, TSoftObjectPtr<UMaterialInterface>, SoftWeaponMaterial);
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
	
	/// Fires when an ability's icon slot is updated (e.g. Light Attack, Heal)
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "UI")
	FOnAbilityIconSlotUpdatedDelegate OnAbilityIconSlotUpdated;

	/// Broadcast by an ability when its cooldown starts (e.g. Heal)
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "UI")
	FOnAbilityCooldownBegin OnAbilityCooldownBegin;
};
