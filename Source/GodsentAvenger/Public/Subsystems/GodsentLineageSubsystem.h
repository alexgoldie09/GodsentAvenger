// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentLineageSubsystem.generated.h"

class APlayerController;
class SWidget;
class UGodsentSavePopupWidget;
class UUserWidget;

/**
 * How the current heir looks and sounds. Plain data only: the pattern and voice are indexes into the
 * customisation catalogue (DA_Customisation), never asset references, so it can be saved as-is.
 */
USTRUCT(BlueprintType)
struct FGodsentHeirAppearance
{
	GENERATED_BODY()

	/// Main armour (Metal Colour on the metal material)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Appearance")
	FLinearColor MainArmourColour = FLinearColor::White;

	/// Secondary armour (Metal Colour on the pattern material)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Appearance")
	FLinearColor SecondaryArmourColour = FLinearColor::White;

	/// The pattern on the secondary armour (Pattern Colour on the pattern material)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Appearance")
	FLinearColor PatternColour = FLinearColor::White;

	/// Cloth (Cloth Colour on the cloth material)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Appearance")
	FLinearColor ClothColour = FLinearColor::White;

	/// Which pattern texture (index into the catalogue's Patterns)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Appearance")
	int32 PatternIndex = 0;

	/// Which voice (index into the catalogue's Voices)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Appearance")
	int32 VoiceIndex = 0;

	/// False until the player has customised. Until then the knight keeps the materials' own colours.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Appearance")
	bool bIsSet = false;
};

/**
 * The family's progress across every heir. Plain data only (numbers, strings, tags, colours, no object references),
 * so it's written to the save game as-is (see UGodsentLineageSaveGame).
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

	/// How the current heir looks and sounds (set in the customiser)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Lineage")
	FGodsentHeirAppearance HeirAppearance;

	/// True from a new game or an heir's death until the next heir has been through the customiser
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Lineage")
	bool bAwaitingCustomisation = true;

	/// True from an heir's death (or a new game) until the next heir is named
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Lineage")
	bool bAwaitingHeirName = true;

	/// The level the family was last in (GameData.Level.*), so Retry (and later Continue) returns there, DLC map included
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Lineage", meta = (Categories = "GameData.Level"))
	FGameplayTag LastLevelTag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLineageChangedDelegate, const FGodsentLineageData&, LineageData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLineageSaveFailedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSavePopupClosedDelegate);

/**
 * Owns the lineage data. A game instance subsystem lives for the whole game session, so it survives the level reload on Retry,
 * unlike the controller (and its skill tree component), which is destroyed and recreated with the level.
 *
 * Saving: the lineage is loaded from one save slot when the game starts, and every save point goes through CommitProgress()
 * (on death, on loading in, when a skill is bought, and on Start Game). The file is a checksum-wrapped USaveGame, so a damaged
 * file is always recognised instead of being half-read.
 *   - A save that fails shows the save popup (Retry / Continue without saving) and play carries on (SAV-02).
 *     Anything that would change level waits until the popup is closed, so the message is never lost in a level change.
 *   - A save that can't be read is reported on the main menu (Start a new family / Back) and never crashes (SAV-03).
 *     The damaged file is left alone until the player chooses to replace it.
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

	/// The one save slot (Saved/SaveGames/Lineage.sav). Start Game overwrites it.
	static const TCHAR* SaveSlotName;
	static constexpr int32 SaveUserIndex = 0;

	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	/// The subsystem from any world object (nullptr if there's no game instance yet)
	static UGodsentLineageSubsystem* Get(const UObject* WorldContextObject);

	/// A copy of the lineage data (Break it in Blueprint for the heir name, generation...)
	UFUNCTION(BlueprintPure, Category = "Godsent|Lineage")
	FGodsentLineageData GetLineageData() const { return LineageData; }

	/// The single save point: writes the lineage to the save slot. Called on death, on loading in, when a skill is bought,
	/// and on Start Game. If the write fails, the save popup appears (Retry / Continue without saving).
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage")
	void CommitProgress();

	/// Resets the lineage to a brand-new family and saves it straight away, so the old family can't come back on Continue
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage")
	void StartNewGame();

	/// Main menu's Start Game button: New Game (wipes the lineage), then loads the game map
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage", meta = (WorldContext = "WorldContextObject"))
	void StartNewGameAndOpenLevel(const UObject* WorldContextObject);

	/// Retry and Continue buttons: reloads the level the lineage was last in (the game map if none is recorded)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage", meta = (WorldContext = "WorldContextObject"))
	void OpenLastLevel(const UObject* WorldContextObject);

	/// Opens a level from the game instance's level sets, after the add-on ownership check (main menu's add-on button).
	/// False, with Out Reason, when the level needs an add-on the player doesn't own or isn't set up.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage", meta = (WorldContext = "WorldContextObject", GameplayTagFilter = "GameData.Level"))
	bool OpenGameLevel(const UObject* WorldContextObject, FGameplayTag LevelTag, FText& OutReason);

	/// Main Menu buttons (pause menu, lose screen, win screen): commits, then opens GameData.Level.MainMenuMap.
	/// The lineage is kept, so the main menu's Continue picks it back up.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Lineage", meta = (WorldContext = "WorldContextObject"))
	void OpenMainMenu(const UObject* WorldContextObject);

	/// True once a family has been founded (the founder named), this session or in the loaded save. Drives the main menu's Continue button.
	UFUNCTION(BlueprintPure, Category = "Godsent|Lineage")
	bool HasLineageToContinue() const { return !LineageData.HeirName.IsEmpty(); }

	/// The heir died: next generation, +1 skill point, the next heir needs a name, then commit
	void RecordHeirDeath();

	/// True when an heir has to be named before play (new game, or after a death)
	UFUNCTION(BlueprintPure, Category = "Godsent|Lineage")
	bool NeedsHeirName() const { return LineageData.bAwaitingHeirName; }

	// ---- Customiser ----

	/// True when the next heir still has to go through the customiser (new game, or after a death)
	UFUNCTION(BlueprintPure, Category = "Godsent|Customiser")
	bool NeedsCustomisation() const { return LineageData.bAwaitingCustomisation; }

	/// The saved look (check bIsSet: false means the player hasn't customised yet)
	UFUNCTION(BlueprintPure, Category = "Godsent|Customiser")
	FGodsentHeirAppearance GetHeirAppearance() const { return LineageData.HeirAppearance; }

	/// The customiser's Apply & Continue. Start New Game wipes the old family first (New Game), otherwise the current
	/// lineage carries on (Continue / Try Again). Saves the look, then opens the lineage's level.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Customiser", meta = (WorldContext = "WorldContextObject"))
	void ConfirmCustomisation(const UObject* WorldContextObject, const FGodsentHeirAppearance& Appearance, bool bStartNewGame);

	/// Try Again after a death: back to the main menu map, straight into the customiser for the next heir
	UFUNCTION(BlueprintCallable, Category = "Godsent|Customiser", meta = (WorldContext = "WorldContextObject"))
	void OpenMainMenuToCustomiser(const UObject* WorldContextObject);

	/// The main menu game mode asks this once: true if it should open on the customiser instead of the menu
	bool ConsumeCustomiserRequest();

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

	// ---- Saving ----

	/// What happened when the save was read at startup (No Save / Loaded / Damaged)
	UFUNCTION(BlueprintPure, Category = "Godsent|Save")
	EGodsentSaveLoadResult GetLoadResult() const { return LoadResult; }

	/// True while the save popup is on screen
	UFUNCTION(BlueprintPure, Category = "Godsent|Save")
	bool IsSavePopupShowing() const { return SavePopup != nullptr; }

	/// Fires every time a save fails (the popup is shown automatically; this is for anything extra, e.g. a HUD icon)
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Save")
	FOnLineageSaveFailedDelegate OnSaveFailed;

	/// Fires right after the save popup closes. Menus that were under it bind this and Set User Focus on their button,
	/// so focus is never lost (e.g. WBP_MainMenu after Back on the damaged-save popup).
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Save")
	FOnSavePopupClosedDelegate OnSavePopupClosed;

	/// The main menu game mode calls this once the menu is up: if the save couldn't be read, shows the load popup.
	/// Return Focus To: the menu that gets focus back when the popup closes.
	void ShowLoadProblemIfAny(APlayerController* PlayerController, UUserWidget* ReturnFocusTo = nullptr);

	/// Called by the save popup's buttons
	void HandleSavePopupChoice(UGodsentSavePopupWidget* Popup, bool bPrimary);

	/// Prints the lineage to the log without saving (GodsentPrintLineage)
	void LogLineage() const;

	/// Letters added, removed or swapped to turn A into B (Levenshtein distance, case-sensitive)
	static int32 GetNameEditDistance(const FString& A, const FString& B);

private:
	void BroadcastLineageChanged();

	/// Reads the save slot into LineageData and sets LoadResult
	void LoadFromSaveSlot();

	/// Writes LineageData to the save slot. False if it couldn't be written.
	bool WriteToSaveSlot() const;

	/// Makes loaded data safe to use (no negative points, valid skill tags only, name rules...)
	void SanitiseLoadedData();

	/// Runs Action now, or once the save popup closes if it's showing (used before changing level)
	void RunWhenSaveSettled(TFunction<void()> Action);

	void ShowSavePopup(APlayerController* PlayerController, EGodsentSavePopupKind Kind, bool bRepeated, UUserWidget* ReturnFocusTo = nullptr);
	void CloseSavePopup();
	void RunPendingActions();

	/// A level change destroys any widget still on screen: forget a popup from the old level
	void HandlePostLoadMap(UWorld* LoadedWorld);

	/// Opens the level set up for this tag in the game instance's level sets, after the add-on ownership check.
	/// False if there isn't one or it needs an add-on the player doesn't own.
	bool OpenLevelByTag(UWorld* World, const FGameplayTag& LevelTag, FText* OutReason = nullptr) const;

	FGodsentLineageData LineageData;

	/// Set by Open Main Menu To Customiser, read once by the main menu. Runtime only, never saved.
	bool bOpenCustomiserOnMenuLoad = false;

	EGodsentSaveLoadResult LoadResult = EGodsentSaveLoadResult::NoSave;

	UPROPERTY()
	TObjectPtr<UGodsentSavePopupWidget> SavePopup;

	/// What to put back when the popup closes: the focused menu widget (none = gameplay) and whether the game was paused
	TWeakPtr<SWidget> FocusBeforePopup;
	bool bWasPausedBeforePopup = false;

	/// Set when the caller knows which menu is underneath (more reliable than FocusBeforePopup on the frame a menu opens)
	TWeakObjectPtr<UUserWidget> ReturnFocusWidget;

	/// Level changes waiting for the popup to close
	TArray<TFunction<void()>> PendingAfterPopup;

	FDelegateHandle PostLoadMapHandle;
};