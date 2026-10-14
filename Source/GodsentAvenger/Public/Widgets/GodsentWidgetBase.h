// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GodsentWidgetBase.generated.h"

class UHeroUIComponent;
class UEnemyUIComponent;

/**
 * Parent class for HUD and enemy widgets. Hands the owner's UI component to Blueprint so it can bind to its events.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	/// Called on HUD widgets once the hero's UI component is found
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Owning Hero UI Component Initialized"))
	void BP_OnOwningHeroUIComponentInitialized(UHeroUIComponent* OwningHeroUIComponent);

	/// Called on enemy widgets once the enemy's UI component is found
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Owning Enemy UI Component Initialized"))
	void BP_OnOwningEnemyUIComponentInitialized(UEnemyUIComponent* OwningEnemyUIComponent);

public:
	/// Hooks an enemy widget (e.g. a health bar widget component) up to its enemy
	UFUNCTION(BlueprintCallable)
	void InitEnemyCreatedWidget(AActor* OwningEnemyActor);
};
