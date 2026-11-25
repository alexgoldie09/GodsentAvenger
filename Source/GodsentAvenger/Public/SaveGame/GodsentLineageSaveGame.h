// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GodsentLineageSubsystem.h"
#include "GodsentLineageSaveGame.generated.h"

/**
 * What goes on disk: the save format version, the family's progress, and the bosses ever beaten.
 * Add-on ownership and the quality profile are never saved: the platform decides both at every launch (DLC-01, ENH-01).
 * Graphics options are saved separately by Unreal itself (GameUserSettings.ini), not here.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentLineageSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/// Bump this when FGodsentLineageData changes shape, and upgrade older saves after loading.
	/// 2: heir appearance and the customiser added.
	/// 3: Saintly Revival used flag added (older saves load it as false, so nothing to upgrade).
	/// 4: Defeated Bosses added (older saves load it empty, so nothing to upgrade).
	static constexpr int32 CurrentSaveVersion = 4;

	/// Saves older than this have no heir appearance
	static constexpr int32 FirstVersionWithCustomiser = 2;

	/// The format version this save was written with
	UPROPERTY(SaveGame)
	int32 SaveVersion = 0;

	/// The family's progress
	UPROPERTY(SaveGame)
	FGodsentLineageData Lineage;

	/// Every boss ever beaten on this save (GameData.Boss tags). Kept outside the lineage so New Game doesn't clear it.
	UPROPERTY(SaveGame)
	FGameplayTagContainer DefeatedBosses;
};
