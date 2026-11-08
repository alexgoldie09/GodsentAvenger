// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentSavePopupWidget.generated.h"

/**
 * Parent class for WBP_SavePopup: the save error popup (UI-07, SAV-02, SAV-03).
 * The lineage subsystem creates, shows and closes it; the Blueprint only lays it out:
 *   - On Popup Shown: set the title, message and two button labels, then Set User Focus on the primary button.
 *   - Primary button clicked -> Choose Primary. Secondary button clicked -> Choose Secondary.
 * Circle (Gamepad Face Button Right) and Escape choose the secondary option, handled here.
 * If anything takes focus away while it's up (e.g. a text box clearing focus after its commit), the popup takes it back
 * on the next frame by firing On Popup Shown again, so focus is never lost (UI-03).
 * The wording is in Class Defaults (Messages) so it can be edited without touching C++.
 */
UCLASS(Abstract)
class GODSENTAVENGER_API UGodsentSavePopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/// Fills the popup for this message and fires On Popup Shown. bRepeated: a Retry has just failed again.
	void SetupPopup(EGodsentSavePopupKind InKind, bool bRepeated);

	/// Primary button: Retry (save failed) or Start a new family (load failed)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Save Popup")
	void ChoosePrimary();

	/// Secondary button (and Circle / Escape): Continue without saving (save failed) or Back (load failed)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Save Popup")
	void ChooseSecondary();

	UFUNCTION(BlueprintPure, Category = "Godsent|Save Popup")
	EGodsentSavePopupKind GetPopupKind() const { return PopupKind; }

protected:
	/// Set the texts and focus the primary button here
	UFUNCTION(BlueprintImplementableEvent, Category = "Godsent|Save Popup", meta = (DisplayName = "On Popup Shown"))
	void BP_OnPopupShown(EGodsentSavePopupKind Kind, const FText& Title, const FText& Message, const FText& PrimaryLabel, const FText& SecondaryLabel);

	/// Circle and Escape go back (the secondary option), so the popup follows the same rule as every other menu (UI-04)
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/// Takes focus back if something else grabbed it (runs while paused: widgets keep ticking)
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ---- Wording (plain language, no codes or file names: UI-07) ----

	UPROPERTY(EditDefaultsOnly, Category = "Messages")
	FText SaveFailedTitle = NSLOCTEXT("GodsentSave", "SaveFailedTitle", "Progress not saved");

	UPROPERTY(EditDefaultsOnly, Category = "Messages", meta = (MultiLine = true))
	FText SaveFailedMessage = NSLOCTEXT("GodsentSave", "SaveFailedMessage",
		"Your family's progress couldn't be saved. Your last save is still safe.\nTry again, or keep playing and the game will save again at the next opportunity.");

	UPROPERTY(EditDefaultsOnly, Category = "Messages", meta = (MultiLine = true))
	FText SaveFailedAgainMessage = NSLOCTEXT("GodsentSave", "SaveFailedAgainMessage",
		"Still couldn't save. Check that your storage has free space, then try again, or keep playing without saving for now.");

	UPROPERTY(EditDefaultsOnly, Category = "Messages")
	FText RetryLabel = NSLOCTEXT("GodsentSave", "Retry", "Retry");

	UPROPERTY(EditDefaultsOnly, Category = "Messages")
	FText ContinueWithoutSavingLabel = NSLOCTEXT("GodsentSave", "ContinueWithoutSaving", "Continue");

	UPROPERTY(EditDefaultsOnly, Category = "Messages")
	FText LoadFailedTitle = NSLOCTEXT("GodsentSave", "LoadFailedTitle", "Saved family can't be loaded");

	UPROPERTY(EditDefaultsOnly, Category = "Messages", meta = (MultiLine = true))
	FText LoadFailedMessage = NSLOCTEXT("GodsentSave", "LoadFailedMessage",
		"Your saved progress is damaged and can't be loaded.\nStart a new family to replace it, or go back to the menu.");

	UPROPERTY(EditDefaultsOnly, Category = "Messages")
	FText StartNewFamilyLabel = NSLOCTEXT("GodsentSave", "StartNewFamily", "Start Again");

	UPROPERTY(EditDefaultsOnly, Category = "Messages")
	FText BackLabel = NSLOCTEXT("GodsentSave", "Back", "Back");

private:
	void Choose(bool bPrimary);

	EGodsentSavePopupKind PopupKind = EGodsentSavePopupKind::SaveFailed;
	bool bShowingRepeated = false;
};