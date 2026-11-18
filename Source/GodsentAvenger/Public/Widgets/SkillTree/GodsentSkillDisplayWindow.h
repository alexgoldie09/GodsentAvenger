// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GodsentSkillDisplayWindow.generated.h"

class UImage;
class UInputAction;
class UInputMappingContext;
class URichTextBlock;
class UScrollBox;
class UTextBlock;
class UTexture2D;
struct FGodsentSkillNode;
struct FStreamableHandle;

/**
 * The panel to the right of the tree (WBP_SkillDisplayWindow): divider, icon, title and a scrolling description.
 * Shown for the focused node, hidden (space kept, so the tree never moves) when nothing on the tree is focused.
 *
 * Button glyphs: write [Token] in a description, e.g. "Press [Block] while being hit to parry".
 * Glyph Token Actions maps each token to an Input Action. The key bound to that action right now is looked up and
 * shown as <img id="KeyName"/>, e.g. <img id="Gamepad_LeftShoulder"/>. Rebinding later just changes the key it finds.
 * The rich text's image decorator table has one row per key, named after the key.
 */
UCLASS(Abstract)
class GODSENTAVENGER_API UGodsentSkillDisplayWindow : public UUserWidget
{
	GENERATED_BODY()

public:
	/// Fills the window from a skill and shows it (the scroll goes back to the top)
	void ShowSkill(const FGodsentSkillNode& SkillNode);

	/// Hides the window but keeps its space, so the tree doesn't shift (after Hide Delay, so a fade-out can play)
	UFUNCTION(BlueprintCallable, Category = "Godsent|SkillTree")
	void HideWindow();

	/// Scrolls the description by this many pixels (positive = down)
	UFUNCTION(BlueprintCallable, Category = "Godsent|SkillTree")
	void ScrollDescription(float Pixels);

	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree")
	bool IsShowingSkill() const { return bShowingSkill; }

	/// Swaps every [Token] for its button glyph. Unknown tokens just lose their brackets.
	UFUNCTION(BlueprintPure, Category = "Godsent|Glyphs")
	FText FormatGlyphTokens(const FText& InText) const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/// The window just opened (fade in here if you like)
	UFUNCTION(BlueprintImplementableEvent, Category = "Display Window", meta = (DisplayName = "On Window Shown"))
	void BP_OnWindowShown();

	/// The window just closed
	UFUNCTION(BlueprintImplementableEvent, Category = "Display Window", meta = (DisplayName = "On Window Hidden"))
	void BP_OnWindowHidden();

	UPROPERTY(BlueprintReadOnly, Category = "Display Window", meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Display Window", meta = (BindWidget))
	TObjectPtr<URichTextBlock> DescriptionText;

	UPROPERTY(BlueprintReadOnly, Category = "Display Window", meta = (BindWidget))
	TObjectPtr<UScrollBox> DescriptionScroll;

	UPROPERTY(BlueprintReadOnly, Category = "Display Window", meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	/// Shown when the skill has no icon yet
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display Window")
	TObjectPtr<UTexture2D> FallbackIcon;

	/// Seconds the window stays drawn after On Window Hidden, so a fade-out animation can finish (match its length)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display Window", meta = (ClampMin = "0.0", Units = "s"))
	float HideDelay = 0.f;

	/// Token (without brackets) → the action whose button it shows, e.g. Block → IA_Block
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glyphs")
	TMap<FName, TObjectPtr<UInputAction>> GlyphTokenActions;

	/// Read if the player has no mapping context applied right now (e.g. between heirs)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glyphs")
	TObjectPtr<UInputMappingContext> FallbackMappingContext;

	/// Show the gamepad button when an action has both a key and a button
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glyphs")
	bool bPreferGamepadGlyphs = true;

private:
	void LoadIcon(const TSoftObjectPtr<UTexture2D>& Icon);
	void ApplyIcon(UTexture2D* Texture);

	bool bShowingSkill = false;

	/// Counts down Hide Delay before the window is actually hidden
	float HideTimeRemaining = 0.f;

	void FinishHiding();

	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
