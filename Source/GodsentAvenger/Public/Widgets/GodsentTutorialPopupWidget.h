// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "GodsentTutorialPopupWidget.generated.h"

class UWidget;
class UWidgetSwitcher;

/**
 * Parent class for WBP_TutorialPopup: a few pages of "how to play" in a Widget Switcher.
 * - It opens with nothing highlighted: Next Keys (X / Enter) go forward a page (on the last page they close it),
 *   Back Keys (Circle / Backspace) go back (nothing on the first page).
 * - The D-pad (or arrow keys / left stick) highlights the Back or Next arrow, and X then presses the highlighted one,
 *   for players who don't know Circle goes back. Wire each arrow's click to Previous Page / Next Page.
 * - On Page Changed fires on open and after every page turn, to hide the back arrow on page 1 or relabel the
 *   next arrow "Begin" on the last page.
 * Shown by a Godsent Tutorial Trigger through the hero controller: the game is paused while it's up, there's no cursor,
 * and it takes focus back if anything steals it.
 */
UCLASS(Abstract)
class GODSENTAVENGER_API UGodsentTutorialPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGodsentTutorialPopupWidget(const FObjectInitializer& ObjectInitializer);

	/// Forward a page, or close on the last page
	UFUNCTION(BlueprintCallable, Category = "Godsent|Tutorial")
	void NextPage();

	/// Back a page (nothing on the first page)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Tutorial")
	void PreviousPage();

	/// Closes the popup and unpauses
	UFUNCTION(BlueprintCallable, Category = "Godsent|Tutorial")
	void ClosePopup();

	UFUNCTION(BlueprintPure, Category = "Godsent|Tutorial")
	int32 GetPageIndex() const;

	UFUNCTION(BlueprintPure, Category = "Godsent|Tutorial")
	int32 GetPageCount() const;

	UFUNCTION(BlueprintPure, Category = "Godsent|Tutorial")
	bool IsFirstPage() const { return GetPageIndex() <= 0; }

	UFUNCTION(BlueprintPure, Category = "Godsent|Tutorial")
	bool IsLastPage() const { return GetPageIndex() >= GetPageCount() - 1; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/// Opened, or the page changed: hide/show the arrows, relabel "Next" to "Begin" on the last page, etc.
	UFUNCTION(BlueprintImplementableEvent, Category = "Godsent|Tutorial", meta = (DisplayName = "On Page Changed"))
	void BP_OnPageChanged(int32 PageIndex, bool bIsFirstPage, bool bIsLastPage);

	/// One child per page, in order
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial", meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> PageSwitcher;

	/// The back arrow (any focusable widget, e.g. TPWBP_PauseScreenButton). Optional.
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BackArrow;

	/// The next arrow. Optional.
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> NextArrow;

	/// Forward a page (close on the last). With an arrow highlighted, presses that arrow instead.
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial|Keys")
	TArray<FKey> NextKeys;

	/// Back a page, whatever is highlighted
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial|Keys")
	TArray<FKey> BackKeys;

	/// Highlights the back arrow
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial|Keys")
	TArray<FKey> HighlightBackArrowKeys;

	/// Highlights the next arrow
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial|Keys")
	TArray<FKey> HighlightNextArrowKeys;

private:
	/// Shows a page and tells the Blueprint
	void SetPage(int32 NewPageIndex);

	/// The arrow (or something inside it) has focus
	bool IsArrowFocused(const UWidget* Arrow) const;

	/// Highlights an arrow, if it's showing
	void FocusArrow(UWidget* Arrow);

	/// An arrow had focus last frame (so if it vanishes, the other arrow takes over rather than nothing)
	bool bArrowWasFocused = false;
};
