// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PawnUIInterface.generated.h"

class UPawnUIComponent;
class UHeroUIComponent;
class UEnemyUIComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPawnUIInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Lets the attribute set and widgets find a pawn's UI component without knowing its class.
 */
class GODSENTAVENGER_API IPawnUIInterface
{
	GENERATED_BODY()

public:
	/// The pawn's UI component (health bar events)
	virtual UPawnUIComponent* GetPawnUIComponent() const = 0;

	/// The hero's UI component, or nullptr on enemies
	virtual UHeroUIComponent* GetHeroUIComponent() const;

	/// The enemy's UI component, or nullptr on the hero
	virtual UEnemyUIComponent* GetEnemyUIComponent() const;
};
