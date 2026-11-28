// Alex Goldberg rights reserved

#include "Widgets/GodsentTutorialPopupWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Controllers/GodsentHeroController.h"

UGodsentTutorialPopupWidget::UGodsentTutorialPopupWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The popup takes focus when it opens, so nothing is highlighted until the D-pad picks an arrow
	SetIsFocusable(true);

	NextKeys = { EKeys::Gamepad_FaceButton_Bottom, EKeys::Enter };
	BackKeys = { EKeys::Gamepad_FaceButton_Right, EKeys::BackSpace };
	HighlightBackArrowKeys = { EKeys::Gamepad_DPad_Left, EKeys::Gamepad_LeftStick_Left, EKeys::Left };
	HighlightNextArrowKeys = { EKeys::Gamepad_DPad_Right, EKeys::Gamepad_LeftStick_Right, EKeys::Right };
}

void UGodsentTutorialPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bArrowWasFocused = false;
	SetPage(0);
}

int32 UGodsentTutorialPopupWidget::GetPageIndex() const
{
	return PageSwitcher ? PageSwitcher->GetActiveWidgetIndex() : 0;
}

int32 UGodsentTutorialPopupWidget::GetPageCount() const
{
	return PageSwitcher ? PageSwitcher->GetNumWidgets() : 0;
}

void UGodsentTutorialPopupWidget::NextPage()
{
	if (IsLastPage())
	{
		ClosePopup();
		return;
	}

	SetPage(GetPageIndex() + 1);
}

void UGodsentTutorialPopupWidget::PreviousPage()
{
	if (!IsFirstPage())
		SetPage(GetPageIndex() - 1);
}

void UGodsentTutorialPopupWidget::ClosePopup()
{
	if (AGodsentHeroController* HeroController = Cast<AGodsentHeroController>(GetOwningPlayer()))
		HeroController->CloseTutorialPopup();
	else
		RemoveFromParent();
}

void UGodsentTutorialPopupWidget::SetPage(int32 NewPageIndex)
{
	if (PageSwitcher)
		PageSwitcher->SetActiveWidgetIndex(FMath::Clamp(NewPageIndex, 0, FMath::Max(GetPageCount() - 1, 0)));

	BP_OnPageChanged(GetPageIndex(), IsFirstPage(), IsLastPage());
}

bool UGodsentTutorialPopupWidget::IsArrowFocused(const UWidget* Arrow) const
{
	return Arrow && (Arrow->HasAnyUserFocus() || Arrow->HasFocusedDescendants());
}

void UGodsentTutorialPopupWidget::FocusArrow(UWidget* Arrow)
{
	if (Arrow && Arrow->IsVisible())
		Arrow->SetUserFocus(GetOwningPlayer());
}

/// Nothing in the popup has focus (an arrow was hidden, or something stole it): take it back (runs while paused)
void UGodsentTutorialPopupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!HasAnyUserFocus() && !HasFocusedDescendants())
	{
		// The back arrow disappeared on page 1 while highlighted: move the highlight to the next arrow
		if (bArrowWasFocused && NextArrow && NextArrow->IsVisible())
			FocusArrow(NextArrow);
		else
			SetUserFocus(GetOwningPlayer());
	}

	bArrowWasFocused = IsArrowFocused(BackArrow) || IsArrowFocused(NextArrow);
}

/// Also gets the keys an arrow didn't use (they bubble up from it)
FReply UGodsentTutorialPopupWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const bool bIsNextKey = NextKeys.Contains(Key);
	const bool bIsBackKey = BackKeys.Contains(Key);
	const bool bIsHighlightBack = HighlightBackArrowKeys.Contains(Key);
	const bool bIsHighlightNext = HighlightNextArrowKeys.Contains(Key);

	if (!bIsNextKey && !bIsBackKey && !bIsHighlightBack && !bIsHighlightNext)
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);

	// Holding a button doesn't flick through every page
	if (InKeyEvent.IsRepeat())
		return FReply::Handled();

	if (bIsHighlightBack)
		FocusArrow(BackArrow);
	else if (bIsHighlightNext)
		FocusArrow(NextArrow);
	else if (bIsBackKey)
		PreviousPage();
	else if (IsArrowFocused(BackArrow))
		PreviousPage(); // X with the back arrow highlighted (if the arrow didn't handle the press itself)
	else
		NextPage();

	return FReply::Handled();
}
