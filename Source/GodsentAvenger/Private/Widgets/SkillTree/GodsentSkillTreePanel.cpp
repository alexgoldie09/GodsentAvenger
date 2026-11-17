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

	if (USkillTreeComponent* SkillTree = GetSkillTree())
	{
		BoundSkillTree = SkillTree;
		SkillTree->OnSkillUnlocked.AddUniqueDynamic(this, &ThisClass::HandleSkillUnlocked);
		SkillTree->OnSkillPointsChanged.AddUniqueDynamic(this, &ThisClass::HandleSkillPointsChanged);
	}

	RefreshTree();
	RefreshDisplayWindow();
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
		UCanvasPanelSlot* ConnectorSlot = Connector ? Cast<UCanvasPanelSlot>(Connector->Slot) : nullptr;

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
	UGodsentSkillNodeWidget* Node = FindNode(InitialFocusSkill);

	if (!Node && Nodes.Num() > 0)
		Node = Nodes[0];

	if (Node)
		Node->FocusNode();
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

	// A line glows once the skill at its end is owned
	for (UGodsentSkillConnectorWidget* Connector : Connectors)
	{
		if (Connector)
			Connector->SetGlowing(SkillTree->IsSkillUnlocked(Connector->ToSkillTag));
	}

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
		FocusedNode = Node;
	else if (FocusedNode.Get() == Node)
		FocusedNode.Reset();

	// Moving between nodes loses and gains focus in the same frame, so the window never flickers
	RefreshDisplayWindow();
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
