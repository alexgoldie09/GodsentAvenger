// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/GameInstance.h"
#include "GodsentGameInstance.generated.h"

USTRUCT(BlueprintType)
struct FGodsentGameLevelSet
{
	GENERATED_BODY()
	
	/// The gameplay tag that represents this level
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "GameData.Level"))
	FGameplayTag LevelTag;
	
	/// The level to load for this tag
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> Level;

	/// Add-on (DLC) that must be owned to open this level, i.e. listed in -trc_owned. None = base game, always available.
	UPROPERTY(EditDefaultsOnly)
	FName RequiredContentId;
	
	/// Checks whether this level set is valid (has a valid tag and a non-null level)
	bool IsValid() const
	{
		return LevelTag.IsValid() && !Level.IsNull();
	}
};

/**
 * Also owns add-on (DLC) ownership. The store decides what a player owns, so ownership is read from the -trc_owned
 * command line flag in Init (e.g. -trc_owned=GodsentDLC01, comma-separate several IDs), kept in memory only
 * (never written to save data), and checked every time add-on content is opened.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/// Reads -trc_owned from the command line
	virtual void Init() override;

	/// True if this add-on ID was listed in -trc_owned (None counts as owned: base-game content)
	UFUNCTION(BlueprintPure, Category = "Godsent|Add-ons")
	bool IsContentOwned(FName ContentId) const;

	/// Ownership check for opening a level: true for base-game levels and for add-on levels the player owns.
	/// Out Reason explains a refusal (shown on the main menu).
	UFUNCTION(BlueprintPure, Category = "Godsent|Add-ons", meta = (GameplayTagFilter = "GameData.Level"))
	bool CanOpenGameLevel(FGameplayTag LevelTag, FText& OutReason) const;

	/// The add-on a level needs (None for base-game levels)
	UFUNCTION(BlueprintPure, Category = "Godsent|Add-ons", meta = (GameplayTagFilter = "GameData.Level"))
	FName GetRequiredContentIdForLevel(FGameplayTag LevelTag) const;

private:
	/// Add-on IDs from -trc_owned. Runtime only: never saved.
	TSet<FName> OwnedContentIds;

	/// Editor convenience: in Play In Editor every add-on counts as owned, so the DLC map can be worked on directly.
	/// Has no effect in Standalone or packaged builds, where only -trc_owned counts (test the requirement there).
	UPROPERTY(EditDefaultsOnly, Category = "Add-ons")
	bool bTreatAddOnsAsOwnedInPIE = true;

protected:
	/// The set of levels available in the game, each associated with a gameplay tag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGodsentGameLevelSet> GodsentGameLevelSets;
	
public:
	/// Gets the level associated with a gameplay tag, or nullptr if not found
	UFUNCTION(BlueprintPure, meta = (GameplayTagFilter = "GameData.Level"))
	TSoftObjectPtr<UWorld> GetGameLevelByTag(FGameplayTag InTag) const;

	/// The tag of the level set whose map is this world (works in PIE too). Empty if the map isn't in the level sets.
	UFUNCTION(BlueprintPure)
	FGameplayTag FindLevelTagForWorld(const UWorld* World) const;
};
