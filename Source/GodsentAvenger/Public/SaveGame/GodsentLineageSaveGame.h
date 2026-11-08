// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GodsentLineageSubsystem.h"
#include "GodsentLineageSaveGame.generated.h"

/**
 * What goes on disk: the save format version plus the family's progress, nothing else.
 * Add-on ownership and the quality profile are never saved: the platform decides both at every launch (DLC-01, ENH-01).
 * Graphics options are saved separately by Unreal itself (GameUserSettings.ini), not here.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentLineageSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/// Bump this when FGodsentLineageData changes shape, and upgrade older saves after loading (needed for the A3 port)
	static constexpr int32 CurrentSaveVersion = 1;

	/// The format version this save was written with
	UPROPERTY(SaveGame)
	int32 SaveVersion = 0;

	/// The family's progress
	UPROPERTY(SaveGame)
	FGodsentLineageData Lineage;
};
