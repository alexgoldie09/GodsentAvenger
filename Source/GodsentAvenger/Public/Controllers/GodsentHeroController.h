// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/PlayerController.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "GodsentHeroController.generated.h"

class UAbilitySystemComponent;
class USkillTreeComponent;
class UUserWidget;

/**
 * The player's controller. Team 0, so enemies (team 1) treat the hero as hostile.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentHeroController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AGodsentHeroController();

	//~ Begin IGenericTeamAgentInterface Interface.
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~ End IGenericTeamAgentInterface Interface.

	/// The current heir's skill points and unlocked skills (loaded from the lineage subsystem, written back on every change)
	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree")
	USkillTreeComponent* GetSkillTreeComponent() const { return SkillTreeComponent; }

	// ---- Rebirth (the game mode decides when; the controller shows the widgets) ----

	/// Called by the name widget's confirm button. Names the heir if the name passes the rules; on success closes the prompt
	/// and tells the game mode, which opens the rebirth skill tree (every heir after the founder) or starts play.
	/// Out Reason says what's wrong when it returns false.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Rebirth")
	bool SubmitHeirName(const FString& ProposedName, FText& OutReason);

	/// Called by the rebirth skill tree's Continue button: closes it and tells the game mode to start play
	UFUNCTION(BlueprintCallable, Category = "Godsent|Rebirth")
	void CloseRebirthSkillTree();

	/// Opens the name prompt (paused). False if no Heir Name Widget Class is set.
	bool ShowHeirNamePrompt();

	/// Opens the rebirth skill tree (paused). False if no Rebirth Skill Tree Widget Class is set.
	bool ShowRebirthSkillTree();

	/// Shows the lose screen (not paused, so the world carries on behind it)
	void ShowLoseScreen();

	/// Shows the win screen (not paused)
	void ShowWinScreen();

	/// Unpauses and gives input back to the game
	void ResumeGameplay();

	/// Stops (or restores) the heir's movement, camera and ability input, e.g. while waiting for the name prompt
	void SetHeroInputBlocked(bool bBlocked);

	/// Abilities the cheat manager's GodsentGrantAbility can grant (debug only; Shipping has no cheat manager)
	const TArray<FGodsentHeroAbilitySet>& GetDebugGrantableAbilities() const { return DebugGrantableAbilities; }

protected:
	/// Reports to the game mode, which decides whether to show the name prompt or start play
	virtual void BeginPlay() override;

	//~ Begin AController Interface.
	/// Loads the lineage's skills into the skill tree, re-applies them to the new heir, and listens for its death (reported to the game mode)
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	//~ End AController Interface.

	/// Name entry popup (text box + confirm button calling Submit Heir Name). Shown, with the game paused,
	/// at the start of a new game and after every death (after the game mode's Heir Name Prompt Delay).
	UPROPERTY(EditDefaultsOnly, Category = "Rebirth")
	TSubclassOf<UUserWidget> HeirNameWidgetClass;

	/// Skill tree shown (paused) after naming every heir after the founder, so the new point can be spent.
	/// Needs a Continue button that calls Close Rebirth Skill Tree.
	UPROPERTY(EditDefaultsOnly, Category = "Rebirth")
	TSubclassOf<UUserWidget> RebirthSkillTreeWidgetClass;

	/// Shown after the heir dies (after the game mode's Lose Screen Delay). Its Try Again button calls the lineage's Open Last Level.
	UPROPERTY(EditDefaultsOnly, Category = "Rebirth")
	TSubclassOf<UUserWidget> LoseScreenWidgetClass;

	/// Shown when the game mode's Handle Run Won is called (after its Win Screen Delay)
	UPROPERTY(EditDefaultsOnly, Category = "Rebirth")
	TSubclassOf<UUserWidget> WinScreenWidgetClass;

	/// Draw order of the rebirth, lose and win widgets (above the HUD overlay)
	UPROPERTY(EditDefaultsOnly, Category = "Rebirth")
	int32 RebirthWidgetZOrder = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SkillTree")
	TObjectPtr<USkillTreeComponent> SkillTreeComponent;

	/// Abilities the debug commands can grant. Leave Input Tag empty for passive/OnGiven ones (e.g. Light)
	UPROPERTY(EditDefaultsOnly, Category = "Debug", meta = (TitleProperty = "AbilityToGrant"))
	TArray<FGodsentHeroAbilitySet> DebugGrantableAbilities;

private:
	/// Adds a widget and gives it focus (no cursor); pauses the game clock too if bPause (not the pause menu)
	void ShowMenuWidget(UUserWidget* Widget, bool bPause);

	void RemoveWidget(TObjectPtr<UUserWidget>& Widget);

	void OnHeroDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
	void UnbindHeroDeath();

	FGenericTeamId HeroTeamId;

	UPROPERTY()
	TObjectPtr<UUserWidget> HeirNameWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> RebirthSkillTreeWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> EndScreenWidget;

	TWeakObjectPtr<UAbilitySystemComponent> BoundHeroASC;
	FDelegateHandle HeroDeadTagHandle;
};
