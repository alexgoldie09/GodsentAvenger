// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnExtensionComponentBase.h"
#include "PawnUIComponent.generated.h"

class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPercentageChangedDelegate, float, NewPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxValueChangedDelegate, float, NewMaxValue);

/**
 * UI events shared by the hero and enemies.
 */
UCLASS()
class GODSENTAVENGER_API UPawnUIComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()

public:
	/// Fires with CurrentHealth / MaxHealth whenever either changes
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnPercentageChangedDelegate OnCurrentHealthChanged;

	/// Fires with the new MaxHealth when it changes (the hero's health bar width follows it)
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnMaxValueChangedDelegate OnMaxHealthChanged;

	/// Sends every event above once with the current values. Call it after binding,
	/// since the starting values were set before the widget existed.
	UFUNCTION(BlueprintCallable, Category = "UI")
	virtual void RefreshStatusBars();

protected:
	/// The owner's ability system component, or nullptr
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
};
