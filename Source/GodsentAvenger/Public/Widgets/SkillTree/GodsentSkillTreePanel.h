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
 *  - veins are placed by hand; only ones with Auto Place Between Nodes are stretched between their From and To
 *  - nodes and veins follow the hero controller's skill tree (state, glow, skill points); a vein's glow flows out
 *    when a skill is bought, and shows the current state straight away when the screen opens
 *  - the focused node fills the display window; it hides when focus leaves the tree
 *  - the right stick scrolls the description
 *
 * Anything on TreeCanvas uses canvas positions in Tree Design Size (the Size Box around the canvas must match).
 *
 * Focusing the panel itself passes focus on to the last node used (or Initial Focus Skill), so a screen only has to
 * focus the panel: set it as the screen's Desired Focus Widget, or Set User Focus on it.
 */
UCLASS(Abstract)
class GODSENTAVENGER_API UGodsentSkillTreePanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/// Focuses the last node used, or Initial Focus Skill, or the first visible node
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

	/// Read Only: the tree can be looked at but nothing can be bought (the pause menu)
	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree")
	bool IsReadOnly() const { return bReadOnly; }

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
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

	/// Size of the canvas the tree is laid out on (Joel's full-screen layout). Must match the Size Box around TreeCanvas.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
	FVector2D TreeDesignSize = FVector2D(1920.f, 1080.f);

	/// The node focused when the screen opens (empty = the first visible node found)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree", meta = (Categories = "Skill"))
	FGameplayTag InitialFocusSkill;

	/// Focus a node one frame after the panel opens (after the screen and controller have set their own focus).
	/// On for the rebirth tree. Off for the pause screen page, which is built before it's shown: call Focus Initial Node there.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
	bool bFocusOnOpen = false;

	/// Look but don't buy: nodes still focus and show their details, but pressing them does nothing.
	/// Tick it on the pause menu's tree; leave it off on the rebirth tree.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Tree")
	bool bReadOnly = false;

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

	/// The node to focus when the panel gets focus: the last one used, or Initial Focus Skill, or the first visible one
	UGodsentSkillNodeWidget* GetNodeToFocus() const;

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

	/// Kept after focus leaves the tree, so coming back lands where you were
	TWeakObjectPtr<UGodsentSkillNodeWidget> LastFocusedNode;

	/// Focus On Open waiting for the first tick
	bool bFocusPending = false;

	/// A node lost focus: hide the window next tick unless another node took it (moving between nodes)
	bool bHideCheckPending = false;

	/// Last right stick Y value (analog events only arrive when it changes)
	float RightStickY = 0.f;

	/// False until the first refresh after opening, so veins show their state without replaying old unlocks
	bool bInitialRefreshDone = false;
};
