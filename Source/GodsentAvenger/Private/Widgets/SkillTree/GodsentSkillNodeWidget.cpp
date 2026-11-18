// Alex Goldberg rights reserved

#include "Widgets/SkillTree/GodsentSkillNodeWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SkillTree/SkillTreeComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SkillTree/GodsentSkillTreePanel.h"

void UGodsentSkillNodeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (NodeButton)
		NodeButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleNodeClicked);
}

/// In the designer: show the fallback icon so the layout reads properly
void UGodsentSkillNodeWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime() && FallbackIcon)
		ApplyIcon(FallbackIcon);
}

void UGodsentSkillNodeWidget::NativeDestruct()
{
	if (IconLoadHandle.IsValid())
		IconLoadHandle->CancelHandle();

	IconLoadHandle.Reset();
	Super::NativeDestruct();
}

void UGodsentSkillNodeWidget::SetOwningPanel(UGodsentSkillTreePanel* InPanel)
{
	OwningPanel = InPanel;
}

void UGodsentSkillNodeWidget::RefreshNode(const USkillTreeComponent* SkillTree)
{
	if (!SkillTree)
		return;

	// Icon: only looked up once
	if (!bIconRequested)
	{
		bIconRequested = true;

		FGodsentSkillNode SkillNode;

		if (SkillTree->GetSkillNode(SkillTag, SkillNode) && !SkillNode.Icon.IsNull())
			LoadIcon(SkillNode.Icon);
		else if (FallbackIcon)
			ApplyIcon(FallbackIcon);
	}

	const EGodsentSkillState NewState = SkillTree->GetSkillState(SkillTag);

	if (bHasState && NewState == CurrentState)
		return;

	CurrentState = NewState;
	bHasState = true;
	BP_OnSkillStateChanged(CurrentState);
}

void UGodsentSkillNodeWidget::FocusNode()
{
	if (!NodeButton)
		return;

	// User focus covers the gamepad as well as the keyboard
	if (APlayerController* PlayerController = GetOwningPlayer())
		NodeButton->SetUserFocus(PlayerController);
	else
		NodeButton->SetKeyboardFocus();
}

/// The node itself got focus (e.g. navigation picked it): hand it to the button, so Enter / Cross presses it
FReply UGodsentSkillNodeWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	if (NodeButton)
		return FReply::Handled().SetUserFocus(NodeButton->TakeWidget(), InFocusEvent.GetCause());

	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

UWidget* UGodsentSkillNodeWidget::GetFocusTarget() const
{
	return NodeButton;
}

void UGodsentSkillNodeWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);

	BP_OnNodeFocusChanged(true);

	UE_LOG(LogTemp, Verbose, TEXT("[SkillTree] %s focused"), *GetName());

	if (UGodsentSkillTreePanel* Panel = OwningPanel.Get())
		Panel->NotifyNodeFocusChanged(this, true);
}

void UGodsentSkillNodeWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);

	BP_OnNodeFocusChanged(false);

	if (UGodsentSkillTreePanel* Panel = OwningPanel.Get())
		Panel->NotifyNodeFocusChanged(this, false);
}

void UGodsentSkillNodeWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (bFocusOnHover)
		FocusNode();
}

void UGodsentSkillNodeWidget::HandleNodeClicked()
{
	UGodsentSkillTreePanel* Panel = OwningPanel.Get();
	BP_OnUnlockAttempted(Panel && Panel->TryUnlockNode(this));
}

void UGodsentSkillNodeWidget::LoadIcon(const TSoftObjectPtr<UTexture2D>& Icon)
{
	// Already in memory: no need to wait a frame
	if (UTexture2D* LoadedIcon = Icon.Get())
	{
		ApplyIcon(LoadedIcon);
		return;
	}

	TWeakObjectPtr<UGodsentSkillNodeWidget> WeakThis = this;
	const FSoftObjectPath IconPath = Icon.ToSoftObjectPath();

	IconLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(IconPath, [WeakThis, IconPath]()
	{
		if (UGodsentSkillNodeWidget* Node = WeakThis.Get())
			Node->ApplyIcon(Cast<UTexture2D>(IconPath.ResolveObject()));
	});
}

void UGodsentSkillNodeWidget::ApplyIcon(UTexture2D* Texture)
{
	// Keeps the image's own size from the designer
	if (IconImage && Texture)
		IconImage->SetBrushFromTexture(Texture, false);
}
