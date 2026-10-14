// Alex Goldberg rights reserved

#include "Components/UI/EnemyUIComponent.h"
#include "Widgets/GodsentWidgetBase.h"

/// Registers a widget drawn for this enemy
void UEnemyUIComponent::RegisterEnemyDrawnWidget(UGodsentWidgetBase* InWidgetToRegister)
{
	EnemyDrawnWidgets.Add(InWidgetToRegister);
}

/// Removes every registered widget from the screen
void UEnemyUIComponent::RemoveEnemyDrawnWidgetsIfAny()
{
	for (UGodsentWidgetBase* Widget : EnemyDrawnWidgets)
	{
		if (Widget)
			Widget->RemoveFromParent();
	}

	EnemyDrawnWidgets.Empty();
}
