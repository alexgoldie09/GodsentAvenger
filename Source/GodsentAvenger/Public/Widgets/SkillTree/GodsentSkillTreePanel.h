// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GodsentSkillTreePanel.generated.h"

class UCanvasPanel;
class UTextBlock;
class UWidget;
class USkillTreeComponent;
class UGodsentSkillConnectorWidget;
class UGodsentSkillDisplayWindow;
class UGodsentSkillNodeWidget;

/**
 * The whole skill tree (WBP_SkillTreePanel), used by both the safe zone skill tree and the rebirth skill tree.
 *
 * Every WBP_SkillNode and WBP_SkillConnector placed directly on TreeCanvas is found automatically:
 *  - lines are placed between their From and To nodes (CentreAnchor when From is empty), in the designer too
 *  - nodes and lines follow the hero controller's skill tree (state, glow, skill points)
 *  - the focused node fills the display window; it hides when focus leaves the tree
 *  - the right stick scrolls the description
 *
 * Anything on TreeCanvas uses canvas positions in Tree Design Size (the Size Box around the canvas must match).
 */
UCLASS(Abstract)
class GODSENTAVENGER_API UGodsentSkillTreePanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/// Focuses Initial Focus Skill (or the first node). Call it when the screen opens.
	UFUNCTION(BlueprintCallable, Category = "Godsent|SkillTree")
	void FocusInitialNode();

	/// Re-reads every node's state, the line glows and the skill points
	UFUNCTION(BlueprintCallable, Category = "Godsent|SkillTree")
	void RefreshTree();

	/// Places every line between its two nodes (runs on construct; call again if nodes move at runtime)
	UFUNCTION(BlueprintCallable, Category = "Godsent|SkillTree")
	void LayoutConnectors();

	/// The node that has focus, or nullptr
	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree")
	UGodsentSkillNodeWidget* GetFocusedNode() const { return FocusedNode.Get(); }

	/// Called by nodes as they gain or lose focus
	void NotifyNodeFocusChanged(UGodsentSkillNodeWidget* Node, bool bFocused);

	/// Called by a node when it's pressed. True if the skill was bought.
	bool TryUnlockNode(UGodsentSkillNodeWidget* Node);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;

	/// Holds every node, line, label and the centre anchor. Must be called TreeCanvas.
	UPROPERTY(BlueprintReadOnly, Category = "Skill Tree", meta = (BindWidget))
	TObjectPtr<UCanvasPanel> TreeCanvas;

	/// Where the first line of each branch starts (the middle of the cross). Optional; must be called CentreAnchor.
	UPROPERTY(BlueprintReadOnly, Category = "Skill Tree", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CentreAnchor;

	/// Must be called DisplayWindow
	UPROPERTY(BlueprintReadOnly, Category = "Skill Tree", meta = (BindWidgetOptional))
	TObjectPtr<UGodsentSkillDisplayWindow> DisplayWindow;

	/// Skill point counter. Optional; must be called PointsText.
	UPROPERTY(BlueprintReadOnly, Category = "Skill Tree", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PointsText;

	/// {Points} is replaced with the current skill points
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
	FText PointsFormat = NSLOCTEXT("SkillTree", "PointsFormat", "Skill Points: {Points}");

	/// Size of the canvas the tree is laid out on. Must match the Size Box around TreeCanvas.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
	FVector2D TreeDesignSize = FVector2D(1000.f, 1000.f);

	/// The node focused when the screen opens (empty = the first node found)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree", meta = (Categories = "Skill"))
	FGameplayTag InitialFocusSkill;

	/// Lines draw under everything else on the canvas
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
	int32 ConnectorZOrder = -1;

	/// Right stick: pixels per second at full tilt
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree|Description Scroll", meta = (ClampMin = "0.0"))
	float DescriptionScrollSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree|Description Scroll", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StickDeadZone = 0.2f;

private:
	/// Finds every node and line placed directly on TreeCanvas
	void GatherTreeWidgets();

	/// Centre of a widget on TreeCanvas, in canvas space. False if it isn't directly on the canvas.
	bool GetCanvasCentre(const UWidget* Widget, FVector2D& OutCentre) const;

	/// The node with this skill tag, or nullptr
	UGodsentSkillNodeWidget* FindNode(const FGameplayTag& SkillTag) const;

	/// Shows the focused node in the display window, or hides it
	void RefreshDisplayWindow();

	UFUNCTION()
	void HandleSkillUnlocked(FGameplayTag SkillTag);

	UFUNCTION()
	void HandleSkillPointsChanged(int32 NewSkillPoints);

	/// The hero controller's skill tree, or nullptr (e.g. in the designer)
	USkillTreeComponent* GetSkillTree() const;

	UPROPERTY()
	TArray<TObjectPtr<UGodsentSkillNodeWidget>> Nodes;

	UPROPERTY()
	TArray<TObjectPtr<UGodsentSkillConnectorWidget>> Connectors;

	TWeakObjectPtr<USkillTreeComponent> BoundSkillTree;
	TWeakObjectPtr<UGodsentSkillNodeWidget> FocusedNode;

	/// Last right stick Y value (analog events only arrive when it changes)
	float RightStickY = 0.f;
};
