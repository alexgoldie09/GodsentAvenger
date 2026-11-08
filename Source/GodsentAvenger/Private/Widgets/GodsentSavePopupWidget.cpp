// Alex Goldberg rights reserved

#include "Widgets/GodsentSavePopupWidget.h"
#include "InputCoreTypes.h"
#include "Subsystems/GodsentLineageSubsystem.h"

void UGodsentSavePopupWidget::SetupPopup(EGodsentSavePopupKind InKind, bool bRepeated)
{
	PopupKind = InKind;
	bShowingRepeated = bRepeated;

	switch (PopupKind)
	{
	case EGodsentSavePopupKind::SaveFailed:
		BP_OnPopupShown(PopupKind, SaveFailedTitle, bRepeated ? SaveFailedAgainMessage : SaveFailedMessage, RetryLabel, ContinueWithoutSavingLabel);
		break;

	case EGodsentSavePopupKind::LoadFailed:
		BP_OnPopupShown(PopupKind, LoadFailedTitle, LoadFailedMessage, StartNewFamilyLabel, BackLabel);
		break;
	}
}

void UGodsentSavePopupWidget::ChoosePrimary()
{
	Choose(true);
}

void UGodsentSavePopupWidget::ChooseSecondary()
{
	Choose(false);
}

void UGodsentSavePopupWidget::Choose(bool bPrimary)
{
	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this))
		Lineage->HandleSavePopupChoice(this, bPrimary);
}

/// Nothing in the popup has focus (another widget cleared or stole it): give it back to the primary button
void UGodsentSavePopupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!HasAnyUserFocus() && !HasFocusedDescendants())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Save] Popup lost focus: taking it back"));
		SetupPopup(PopupKind, bShowingRepeated);
	}
}

/// Circle / Escape: the secondary option (Continue without saving, or Back)
FReply UGodsentSavePopupWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Escape)
	{
		ChooseSecondary();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}