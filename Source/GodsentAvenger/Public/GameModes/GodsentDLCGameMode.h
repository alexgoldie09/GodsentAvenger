// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameModes/GodsentBaseGameMode.h"
#include "GodsentDLCGameMode.generated.h"

/**
 * Game mode for the DLC (add-on) map. A side mode: the current heir plays with the family's skills, but there's no
 * name prompt, deaths don't move the family on, and the DLC map is never the main menu's Continue level.
 * Ownership of the add-on is re-checked by the base class whenever the map loads.
 *
 * Home for the DLC's own rules later (spawner waves and so on).
 */
UCLASS()
class GODSENTAVENGER_API AGodsentDLCGameMode : public AGodsentBaseGameMode
{
	GENERATED_BODY()

public:
	AGodsentDLCGameMode();
};
