// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnExtensionComponentBase.h"
#include "PawnUIComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPercentageChangedDelegate, float, NewPercent);

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
};
