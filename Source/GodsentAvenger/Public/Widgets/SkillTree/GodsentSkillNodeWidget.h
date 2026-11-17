// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentSkillNodeWidget.generated.h"

class UButton;
class UImage;
class UTexture2D;
class USkillTreeComponent;
class UGodsentSkillTreePanel;
struct FStreamableHandle;

/**
 * One skill on the tree (WBP_SkillNode). Icon only: the name and description show in the display window.
 *
 * Needs a Button called NodeButton (GodsentButton is fine) and can have an Image called IconImage.
 * Set Skill Tag on each placed node. The tree panel finds every node on its canvas, keeps their state up to date,
 * and shows the focused node in the display window.
 *
 * Mouse hover focuses the node too, so mouse and gamepad always agree on which node is shown.
 */
UCLASS(Abstract)
class GODSENTAVENGER_API UGodsentSkillNodeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/// The skill this node buys (same tag as in DA_SkillTree)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Node", meta = (Categories = "Skill"))
	FGameplayTag SkillTag;

	/// Shown when the skill has no icon in DA_SkillTree yet (e.g. one per branch)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Node")
	TObjectPtr<UTexture2D> FallbackIcon;

	/// Called by the tree panel when it finds this node
	void SetOwningPanel(UGodsentSkillTreePanel* InPanel);

	/// Reads the node's state (and icon, the first time) from the skill tree
	void RefreshNode(const USkillTreeComponent* SkillTree);

	/// Gives this node keyboard/gamepad focus
	UFUNCTION(BlueprintCallable, Category = "Godsent|SkillTree")
	void FocusNode();

	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree")
	EGodsentSkillState GetSkillState() const { return CurrentState; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/// Unlocked / Available / Can't Afford / Locked / Unavailable changed: swap the background, border or colours here
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Node", meta = (DisplayName = "On Skill State Changed"))
	void BP_OnSkillStateChanged(EGodsentSkillState NewState);

	/// Gained or lost focus (the focus outline)
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Node", meta = (DisplayName = "On Node Focus Changed"))
	void BP_OnNodeFocusChanged(bool bFocused);

	/// The node was pressed: true if the skill was bought (play the unlock sound/flash), false if it couldn't be
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Node", meta = (DisplayName = "On Unlock Attempted"))
	void BP_OnUnlockAttempted(bool bUnlocked);

	/// Mouse hover focuses the node (so the display window follows the mouse too)
	UPROPERTY(EditAnywhere, Category = "Skill Node")
	bool bFocusOnHover = true;

	/// The button the player presses. Must be called NodeButton.
	UPROPERTY(BlueprintReadOnly, Category = "Skill Node", meta = (BindWidget))
	TObjectPtr<UButton> NodeButton;

	/// The skill's icon. Optional; must be called IconImage.
	UPROPERTY(BlueprintReadOnly, Category = "Skill Node", meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

private:
	UFUNCTION()
	void HandleNodeClicked();

	/// Loads the icon in the background, then puts it on IconImage
	void LoadIcon(const TSoftObjectPtr<UTexture2D>& Icon);
	void ApplyIcon(UTexture2D* Texture);

	TWeakObjectPtr<UGodsentSkillTreePanel> OwningPanel;

	EGodsentSkillState CurrentState = EGodsentSkillState::Unavailable;
	bool bHasState = false;
	bool bIconRequested = false;

	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
