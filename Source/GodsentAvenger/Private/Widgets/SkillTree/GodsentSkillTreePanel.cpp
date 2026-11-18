// Alex Goldberg rights reserved

#include "Widgets/SkillTree/GodsentSkillTreePanel.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SkillTree/SkillTreeComponent.h"
#include "Components/TextBlock.h"
#include "Controllers/GodsentHeroController.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "Widgets/SkillTree/GodsentSkillConnectorWidget.h"
#include "Widgets/SkillTree/GodsentSkillDisplayWindow.h"
#include "Widgets/SkillTree/GodsentSkillNodeWidget.h"

namespace
{
	/// Lines are centred on the midpoint between their two nodes
	const FVector2D ConnectorAlignment(0.5f, 0.5f);
	const FVector2D ConnectorPivot(0.5f, 0.5f);

	/// Every canvas child is measured from the canvas's top left
	const FAnchors TopLeftAnchor(0.f, 0.f);
}

void UGodsentSkillTreePanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// The panel takes focus only to hand it to a node
	SetIsFocusable(true);
}

/// A screen focused the panel: pass it straight on to a node
FReply UGodsentSkillTreePanel::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	const UGodsentSkillNodeWidget* Node = GetNodeToFocus();
	UWidget* FocusTarget = Node ? Node->GetFocusTarget() : nullptr;

	if (FocusTarget)
	{
		UE_LOG(LogTemp, Display, TEXT("[SkillTree] Panel got focus, passing it to %s"), *Node->GetName());
		return FReply::Handled().SetUserFocus(FocusTarget->TakeWidget(), InFocusEvent.GetCause());
	}

	UE_LOG(LogTemp, Warning, TEXT("[SkillTree] Panel got focus but has no visible node (%d found)"), Nodes.Num());
	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

void UGodsentSkillTreePanel::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Runs in the designer too, so lines follow nodes as you move them (after a compile)
	GatherTreeWidgets();
	LayoutConnectors();
}

void UGodsentSkillTreePanel::NativeConstruct()
{
	Super::NativeConstruct();

	GatherTreeWidgets();
	LayoutConnectors();
	bInitialRefreshDone = false;

	if (USkillTreeComponent* SkillTree = GetSkillTree())
	{
		BoundSkillTree = SkillTree;
		SkillTree->OnSkillUnlocked.AddUniqueDynamic(this, &ThisClass::HandleSkillUnlocked);
		SkillTree->OnSkillPointsChanged.AddUniqueDynamic(this, &ThisClass::HandleSkillPointsChanged);
	}

	RefreshTree();
	RefreshDisplayWindow();

	// Widgets tick while paused, so this still runs behind the rebirth pause
	bFocusPending = bFocusOnOpen;
}

void UGodsentSkillTreePanel::NativeDestruct()
{
	if (USkillTreeComponent* SkillTree = BoundSkillTree.Get())
	{
		SkillTree->OnSkillUnlocked.RemoveDynamic(this, &ThisClass::HandleSkillUnlocked);
		SkillTree->OnSkillPointsChanged.RemoveDynamic(this, &ThisClass::HandleSkillPointsChanged);
	}

	BoundSkillTree.Reset();
	FocusedNode.Reset();
	RightStickY = 0.f;

	Super::NativeDestruct();
}

void UGodsentSkillTreePanel::GatherTreeWidgets()
{
	Nodes.Reset();
	Connectors.Reset();

	if (!TreeCanvas)
		return;

	for (UWidget* Child : TreeCanvas->GetAllChildren())
	{
		if (UGodsentSkillNodeWidget* Node = Cast<UGodsentSkillNodeWidget>(Child))
		{
			Node->SetOwningPanel(this);
			Nodes.Add(Node);
		}
		else if (UGodsentSkillConnectorWidget* Connector = Cast<UGodsentSkillConnectorWidget>(Child))
		{
			Connectors.Add(Connector);
		}
	}
}

void UGodsentSkillTreePanel::LayoutConnectors()
{
	for (UGodsentSkillConnectorWidget* Connector : Connectors)
	{
		// Veins are placed by hand; only straight lines are positioned here
		if (!Connector || !Connector->bAutoPlaceBetweenNodes)
			continue;

		UCanvasPanelSlot* ConnectorSlot = Cast<UCanvasPanelSlot>(Connector->Slot);

		if (!ConnectorSlot)
			continue;

		// Empty From = the centre of the cross
		const UWidget* FromWidget = Connector->FromSkillTag.IsValid() ? FindNode(Connector->FromSkillTag) : CentreAnchor.Get();
		const UWidget* ToWidget = FindNode(Connector->ToSkillTag);

		FVector2D From;
		FVector2D To;

		if (!GetCanvasCentre(FromWidget, From) || !GetCanvasCentre(ToWidget, To))
		{
			UE_LOG(LogTemp, Warning, TEXT("[SkillTree] %s can't find its nodes (From %s, To %s)"),
				*Connector->GetName(), *Connector->FromSkillTag.ToString(), *Connector->ToSkillTag.ToString());
			continue;
		}

		const FVector2D Line = To - From;

		// Centred between the two nodes, as long as the gap, turned to point from one to the other
		ConnectorSlot->SetAnchors(TopLeftAnchor);
		ConnectorSlot->SetAlignment(ConnectorAlignment);
		ConnectorSlot->SetAutoSize(false);
		ConnectorSlot->SetPosition(From + Line * 0.5f);
		ConnectorSlot->SetSize(FVector2D(Line.Size(), Connector->LineThickness));
		ConnectorSlot->SetZOrder(ConnectorZOrder);

		Connector->SetRenderTransformPivot(ConnectorPivot);
		Connector->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Line.Y, Line.X)));
	}
}

bool UGodsentSkillTreePanel::GetCanvasCentre(const UWidget* Widget, FVector2D& OutCentre) const
{
	const UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;

	if (!CanvasSlot)
		return false;

	// Works for any point anchor; nodes are expected to have a fixed size (Size To Content off)
	const FVector2D AnchorPoint = CanvasSlot->GetAnchors().Minimum * TreeDesignSize;
	const FVector2D Size = CanvasSlot->GetSize();
	const FVector2D TopLeft = AnchorPoint + CanvasSlot->GetPosition() - CanvasSlot->GetAlignment() * Size;

	OutCentre = TopLeft + Size * 0.5f;
	return true;
}

UGodsentSkillNodeWidget* UGodsentSkillTreePanel::FindNode(const FGameplayTag& SkillTag) const
{
	if (!SkillTag.IsValid())
		return nullptr;

	for (UGodsentSkillNodeWidget* Node : Nodes)
	{
		if (Node && Node->SkillTag == SkillTag)
			return Node;
	}

	return nullptr;
}

void UGodsentSkillTreePanel::FocusInitialNode()
{
	UGodsentSkillNodeWidget* Node = GetNodeToFocus();

	if (!Node)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillTree] %s has no visible node to focus (%d nodes found on TreeCanvas)"), *GetName(), Nodes.Num());
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[SkillTree] Focusing %s (%s)"), *Node->GetName(), *Node->SkillTag.ToString());
	Node->FocusNode();
}

UGodsentSkillNodeWidget* UGodsentSkillTreePanel::GetNodeToFocus() const
{
	// Back where you left off
	if (UGodsentSkillNodeWidget* LastNode = LastFocusedNode.Get())
		if (LastNode->IsVisible())
			return LastNode;

	UGodsentSkillNodeWidget* Node = FindNode(InitialFocusSkill);

	if (Node && Node->IsVisible())
		return Node;

	// Otherwise the first node that's showing (the hidden heart node is collapsed)
	for (UGodsentSkillNodeWidget* Candidate : Nodes)
	{
		if (Candidate && Candidate->IsVisible())
			return Candidate;
	}

	return nullptr;
}

void UGodsentSkillTreePanel::RefreshTree()
{
	const USkillTreeComponent* SkillTree = GetSkillTree();

	if (!SkillTree)
		return;

	for (UGodsentSkillNodeWidget* Node : Nodes)
	{
		if (Node)
			Node->RefreshNode(SkillTree);
	}

	// The first refresh shows what's already owned; after that, new unlocks flow out
	for (UGodsentSkillConnectorWidget* Connector : Connectors)
	{
		if (Connector)
			Connector->RefreshGlow(SkillTree, bInitialRefreshDone);
	}

	bInitialRefreshDone = true;

	if (PointsText)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Points"), SkillTree->GetSkillPoints());
		PointsText->SetText(FText::Format(PointsFormat, Args));
	}
}

void UGodsentSkillTreePanel::NotifyNodeFocusChanged(UGodsentSkillNodeWidget* Node, bool bFocused)
{
	if (bFocused)
	{
		FocusedNode = Node;
		LastFocusedNode = Node;
		bHideCheckPending = false;
		RefreshDisplayWindow();
		return;
	}

	// Moving between nodes drops focus a moment before the next node gets it,
	// so wait a tick before deciding the window should go
	if (FocusedNode.Get() == Node)
	{
		FocusedNode.Reset();
		bHideCheckPending = true;
	}
}

bool UGodsentSkillTreePanel::TryUnlockNode(UGodsentSkillNodeWidget* Node)
{
	USkillTreeComponent* SkillTree = GetSkillTree();

	// The skill tree broadcasts on success, which refreshes the tree
	return Node && SkillTree && SkillTree->TryUnlockSkill(Node->SkillTag);
}

void UGodsentSkillTreePanel::RefreshDisplayWindow()
{
	if (!DisplayWindow)
		return;

	const UGodsentSkillNodeWidget* Node = FocusedNode.Get();
	const USkillTreeComponent* SkillTree = GetSkillTree();
	FGodsentSkillNode SkillNode;

	if (Node && SkillTree && SkillTree->GetSkillNode(Node->SkillTag, SkillNode))
		DisplayWindow->ShowSkill(SkillNode);
	else
		DisplayWindow->HideWindow();
}

void UGodsentSkillTreePanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bFocusPending)
	{
		bFocusPending = false;
		FocusInitialNode();
	}

	// Focus really left the tree (no node picked it up)
	if (bHideCheckPending)
	{
		bHideCheckPending = false;

		if (!FocusedNode.IsValid())
			RefreshDisplayWindow();
	}

	// Pushing up scrolls up
	if (DisplayWindow && FMath::Abs(RightStickY) > StickDeadZone)
		DisplayWindow->ScrollDescription(-RightStickY * DescriptionScrollSpeed * InDeltaTime);
}

FReply UGodsentSkillTreePanel::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	if (InAnalogEvent.GetKey() == EKeys::Gamepad_RightY)
	{
		RightStickY = InAnalogEvent.GetAnalogValue();
		return FReply::Handled();
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

void UGodsentSkillTreePanel::HandleSkillUnlocked(FGameplayTag SkillTag)
{
	RefreshTree();
}

void UGodsentSkillTreePanel::HandleSkillPointsChanged(int32 NewSkillPoints)
{
	RefreshTree();
}

USkillTreeComponent* UGodsentSkillTreePanel::GetSkillTree() const
{
	const AGodsentHeroController* HeroController = Cast<AGodsentHeroController>(GetOwningPlayer());
	return HeroController ? HeroController->GetSkillTreeComponent() : nullptr;
}
