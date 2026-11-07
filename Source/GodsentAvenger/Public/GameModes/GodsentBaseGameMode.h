// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentBaseGameMode.generated.h"

class AGodsentHeroController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunStateChangedDelegate, EGodsentRunState, NewState);

/**
 * The rules of the current run: when the heir is named, when play starts, and when the run is lost (or won).
 *
 * The game mode is rebuilt with every level load, so it holds no progress of its own. The family's progress lives in
 * UGodsentLineageSubsystem; this class decides *when* things happen and asks the hero controller to show the widgets:
 *
 *   Level loads → (Heir Name Prompt Delay) → name prompt → rebirth skill tree (every heir after the founder) → play
 *   Heir dies   → death recorded on the lineage → (Lose Screen Delay) → lose screen
 *
 * Side modes (the DLC) untick Tracks Lineage: the current heir plays with the family's skills, but there's no name prompt,
 * deaths don't move the family on, and the level is never remembered for Continue.
 * Levels that need an add-on check ownership again when they load, and send the player to the main menu if it isn't owned.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentBaseGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/// The game mode for this world (nullptr if the level uses a different one)
	static AGodsentBaseGameMode* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Godsent|Run")
	EGodsentRunState GetRunState() const { return RunState; }

	/// Fires whenever the run state changes (e.g. to hide the HUD while the lose screen is up)
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Run")
	FOnRunStateChangedDelegate OnRunStateChanged;

	// ---- Reported by the hero controller ----

	/// The controller has started in the level: name the heir (after the start delay) or start play straight away
	void HandleHeroControllerReady(AGodsentHeroController* HeroController);

	/// The name prompt was confirmed with a valid name (already stored on the lineage)
	void HandleHeirNamed(AGodsentHeroController* HeroController);

	/// The rebirth skill tree's Continue button was pressed
	void HandleRebirthSkillsChosen(AGodsentHeroController* HeroController);

	/// The heir gained the Dead tag
	void HandleHeroDefeated(AGodsentHeroController* HeroController);

	/// The level was won (e.g. call it from Aldegon's death). Shows the win screen after Win Screen Delay.
	/// Doesn't change the lineage; what winning means for the family is still to be decided.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Run")
	void HandleRunWon();

	/// Lose screen's Try Again: the main game reopens the lineage's last level (the next heir's name prompt);
	/// a side mode (Tracks Lineage off) reopens its own level, re-checking add-on ownership
	UFUNCTION(BlueprintCallable, Category = "Godsent|Run")
	void RestartRun();

	/// False for side modes such as the DLC
	UFUNCTION(BlueprintPure, Category = "Godsent|Run")
	bool TracksLineage() const { return bTracksLineage; }

protected:
	/// On: the main game (name prompt, deaths move the family on, Continue returns here).
	/// Off: a side mode such as the DLC (no name prompt, deaths aren't recorded, never the Continue level).
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	bool bTracksLineage = true;

	/// Seconds after the level loads before the name prompt appears (the heir can't move meanwhile). 0 = straight away.
	UPROPERTY(EditDefaultsOnly, Category = "Run", meta = (ClampMin = "0.0", Units = "s"))
	float HeirNamePromptDelay = 1.5f;

	/// Seconds after the heir dies before the lose screen appears (lets the death animation and dissolve play)
	UPROPERTY(EditDefaultsOnly, Category = "Run", meta = (ClampMin = "0.0", Units = "s"))
	float LoseScreenDelay = 3.f;

	/// Seconds after Handle Run Won before the win screen appears
	UPROPERTY(EditDefaultsOnly, Category = "Run", meta = (ClampMin = "0.0", Units = "s"))
	float WinScreenDelay = 3.f;

private:
	/// Add-on levels: false (and back to the main menu) if this level needs an add-on the player doesn't own
	bool CheckLevelOwnership();

	void ShowHeirNamePrompt(AGodsentHeroController* HeroController);

	/// The heir takes control: unpause, give input back, record the level and commit ("loading in")
	void StartHeirPlay(AGodsentHeroController* HeroController);

	void SetRunState(EGodsentRunState NewState);

	/// Runs Action on the controller after Delay seconds (straight away for 0), unless the controller is gone by then
	void RunAfterDelay(AGodsentHeroController* HeroController, float Delay, TFunction<void(AGodsentHeroController*)> Action);

	EGodsentRunState RunState = EGodsentRunState::Starting;

	FTimerHandle RunTimerHandle;
};
