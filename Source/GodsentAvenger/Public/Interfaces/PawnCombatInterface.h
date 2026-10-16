// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PawnCombatInterface.generated.h"

class UPawnCombatComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPawnCombatInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Lets abilities and the function library find a pawn's combat component without knowing its class.
 */
class GODSENTAVENGER_API IPawnCombatInterface
{
	GENERATED_BODY()

public:
	/// The pawn's combat component
	virtual UPawnCombatComponent* GetPawnCombatComponent() const = 0;
};
