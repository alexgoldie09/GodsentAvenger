// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GodsentLineageSubsystem.generated.h"

/**
 * The family's progress across every heir. Plain data only (numbers, strings, tags, colours, no object references),
 * so it can be written to a save game as-is later.
 */
USTRUCT(BlueprintType)
struct FGodsentLineageData
{
	GENERATED_BODY()

	/// Which heir this is: 1 = the founder. Moves on by one each time an heir dies.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Lineage")
	int32 Generation = 1;

	/// Unspent skill points
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Lineage")
	int32 SkillPoints = 0;

	/// Every skill the family has unlocked (Skill.* tags). Each new heir gets them all re-applied.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Lineage")
	FGameplayTagContainer UnlockedSkills;

	/// The current heir's name. Empty until the founder is named.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Lineage")
	FString HeirName;

	/// Appearance placeholder (not used yet)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Lineage")
	FLinearColor HeirAppearanceColour = FLinearColor::White;

	/// True from an heir's death (or a new game) until the next heir is named
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Lineage")
	bool bAwaitingHeirName = true;

	/// The level the family was last in (GameData.Level.*), so Retry (and later Continue) returns there, DLC map included
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Lineage", meta = (Categories = "GameData.Level"))
	FGameplayTag LastLevelTag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLineageChangedDelegate, const FGodsentLineageData&, LineageData);

/**
 * Owns the lineage data. A game instance subsystem lives for the whole game session, so it survives the level reload on Retry,
 * unlike the controller (and its skill tree component), which is destroyed and recreated with the level.
 *
 * Every save point goes through CommitProgress(): on death, on loading in, and when a skill is bought.
 * It only logs for now; the real save goes inside it later without any calling code changing.
 */
UCLASS(BlueprintType)
class GODSENTAVENGER_API UGodsentLineageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/// Longest heir name allowed (about a full first and last name)
	static constexpr int32 MaxHeirNameLength = 70;

	/// Enemy tier goes up by one every this many generations (deaths)
	static constexpr int32 GenerationsPerEnemyTier = 5;

	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//~ End USubsystem Interface

	/// The subsystem from any world object (nullptr if there's no game instance yet)
	static UGodsentLineageSubsystem* Get(const UObject* WorldContextObject);

	/// A copy of the lineage data (Break it in Blueprint for the heir name, generation...)
	UFUNCTION(BlueprintPure, Category = "Godsent|Lineage")
	FGodsentLineageData GetLineageData() const { return LineageData; }

	/// The single save point. Called on death, on loading in, and when a skill is bought. Logs only, for now.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage")
	void CommitProgress();

	/// Resets the lineage to a brand-new family (what loading with no save file will do later)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage")
	void StartNewGame();

	/// Main menu's Start Game button: New Game (wipes the lineage), then loads the game map
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage", meta = (WorldContext = "WorldContextObject"))
	void StartNewGameAndOpenLevel(const UObject* WorldContextObject);

	/// Retry and Continue buttons: reloads the level the lineage was last in (the game map if none is recorded)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage", meta = (WorldContext = "WorldContextObject"))
	void OpenLastLevel(const UObject* WorldContextObject);

	/// Main Menu buttons (pause menu, lose screen, win screen): commits, then opens GameData.Level.MainMenuMap.
	/// The lineage is kept, so the main menu's Continue picks it back up.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage", meta = (WorldContext = "WorldContextObject"))
	void OpenMainMenu(const UObject* WorldContextObject);

	/// True once a family has been founded (the founder named). Drives the main menu's Continue button.
	/// No save files yet, so this only covers the current session; later it will also check for a save.
	UFUNCTION(BlueprintPure, Category = "Godsent|Lineage")
	bool HasLineageToContinue() const { return !LineageData.HeirName.IsEmpty(); }

	/// The heir died: next generation, +1 skill point, the next heir needs a name, then commit
	void RecordHeirDeath();

	/// True when an heir has to be named before play (new game, or after a death)
	UFUNCTION(BlueprintPure, Category = "Godsent|Lineage")
	bool NeedsHeirName() const { return LineageData.bAwaitingHeirName; }

	/// True for the founder (the first name is free; every later heir changes exactly one letter)
	UFUNCTION(BlueprintPure, Category = "Godsent|Lineage")
	bool IsNamingFounder() const { return LineageData.HeirName.IsEmpty(); }

	/// Checks a proposed name against the rules without applying it. Out Reason says what's wrong (empty when valid).
	/// Rules: 1-70 characters (letters, spaces, hyphens, apostrophes); after the founder, exactly one letter
	/// swapped, added or removed compared with the previous heir's name.
	UFUNCTION(BlueprintPure, Category = "Godsent|Lineage")
	bool ValidateHeirName(const FString& ProposedName, FText& OutReason) const;

	/// Names the new heir if the name passes Validate Heir Name. Doesn't commit (the game mode commits once the heir takes control).
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage")
	bool SetHeirName(const FString& ProposedName, FText& OutReason);

	/// Records which level the lineage is in
	void SetLastLevelTag(const FGameplayTag& LevelTag);

	/// Written by the skill tree component whenever its points or unlocked skills change
	void SetSkillProgress(int32 InSkillPoints, const FGameplayTagContainer& InUnlockedSkills);

	/// Debug: jumps the lineage to a generation (to test enemy tiers)
	void DebugSetGeneration(int32 NewGeneration);

	/// Level offset for enemies: 1 for generations 1-5, 2 for 6-10, and so on
	UFUNCTION(BlueprintPure, Category = "Godsent|Lineage")
	int32 GetEnemyTier() const;

	/// Fires whenever the lineage data changes (e.g. the overlay's heir name text)
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Lineage")
	FOnLineageChangedDelegate OnLineageChanged;

	/// Letters added, removed or swapped to turn A into B (Levenshtein distance, case-sensitive)
	static int32 GetNameEditDistance(const FString& A, const FString& B);

private:
	void BroadcastLineageChanged();

	/// Opens the level set up for this tag in the game instance's level sets. False if there isn't one.
	bool OpenLevelByTag(UWorld* World, const FGameplayTag& LevelTag) const;

	FGodsentLineageData LineageData;
};
