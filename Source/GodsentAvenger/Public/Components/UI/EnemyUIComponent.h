// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/UI/PawnUIComponent.h"
#include "EnemyUIComponent.generated.h"

class UGodsentWidgetBase;

/**
 * Enemy UI: tracks widgets drawn for an enemy (e.g. the boss health bar) so they can be removed on death.
 */
UCLASS()
class GODSENTAVENGER_API UEnemyUIComponent : public UPawnUIComponent
{
	GENERATED_BODY()

public:
	/// Registers a widget drawn for this enemy
	UFUNCTION(BlueprintCallable)
	void RegisterEnemyDrawnWidget(UGodsentWidgetBase* InWidgetToRegister);

	/// Removes every registered widget from the screen
	UFUNCTION(BlueprintCallable)
	void RemoveEnemyDrawnWidgetsIfAny();

private:
	/// Widgets drawn for this enemy
	UPROPERTY()
	TArray<TObjectPtr<UGodsentWidgetBase>> EnemyDrawnWidgets;
};
