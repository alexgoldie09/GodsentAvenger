// Alex Goldberg rights reserved

#include "Widgets/SkillTree/GodsentSkillDisplayWindow.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "InputAction.h"

namespace
{
	/// [Block] in a description becomes the Block button's glyph
	const TCHAR* const GlyphTokenOpen = TEXT("[");
	const TCHAR* const GlyphTokenClose = TEXT("]");

	/// Skill tags go Skill.Tree.Branch.Name
	const TCHAR* const SkillTagSeparator = TEXT(".");
	constexpr int32 SkillTagTreeIndex = 1;
	constexpr int32 SkillTagBranchIndex = 2;
}

void UGodsentSkillDisplayWindow::NativeConstruct()
{
	Super::NativeConstruct();

	// Nothing is focused yet: hidden straight away, no fade
	bShowingSkill = false;
	HideTimeRemaining = 0.f;
	FinishHiding();
}

void UGodsentSkillDisplayWindow::NativeDestruct()
{
	if (IconLoadHandle.IsValid())
		IconLoadHandle->CancelHandle();

	IconLoadHandle.Reset();
	Super::NativeDestruct();
}

void UGodsentSkillDisplayWindow::ShowSkill(const FGodsentSkillNode& SkillNode)
{
	if (TitleText)
		TitleText->SetText(SkillNode.DisplayName);

	if (CategoryText)
		CategoryText->SetText(GetCategoryLabel(SkillNode.SkillTag));

	if (DescriptionText)
		DescriptionText->SetText(FormatGlyphTokens(SkillNode.Description));

	if (DescriptionScroll)
		DescriptionScroll->ScrollToStart();

	if (!SkillNode.Icon.IsNull())
		LoadIcon(SkillNode.Icon);
	else
		ApplyIcon(FallbackIcon);

	const bool bWasShowing = bShowingSkill;
	bShowingSkill = true;
	HideTimeRemaining = 0.f; // A fade-out in progress is cancelled

	// Not hit-testable itself, but the scroll box inside still takes the mouse wheel
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!bWasShowing)
		BP_OnWindowShown();
}

void UGodsentSkillDisplayWindow::HideWindow()
{
	if (!bShowingSkill)
		return;

	bShowingSkill = false;
	BP_OnWindowHidden();

	// Stay drawn while the fade-out plays
	if (HideDelay > 0.f)
		HideTimeRemaining = HideDelay;
	else
		FinishHiding();
}

void UGodsentSkillDisplayWindow::FinishHiding()
{
	// Hidden, not Collapsed: the window keeps its space so nothing around it moves
	SetVisibility(ESlateVisibility::Hidden);
}

void UGodsentSkillDisplayWindow::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (HideTimeRemaining <= 0.f)
		return;

	HideTimeRemaining -= InDeltaTime;

	if (HideTimeRemaining <= 0.f && !bShowingSkill)
		FinishHiding();
}

void UGodsentSkillDisplayWindow::ScrollDescription(float Pixels)
{
	if (!DescriptionScroll || !bShowingSkill)
		return;

	const float NewOffset = FMath::Clamp(DescriptionScroll->GetScrollOffset() + Pixels, 0.f, DescriptionScroll->GetScrollOffsetOfEnd());
	DescriptionScroll->SetScrollOffset(NewOffset);
}

FText UGodsentSkillDisplayWindow::FormatGlyphTokens(const FText& InText) const
{
	const FString Source = InText.ToString();
	FString Result;
	Result.Reserve(Source.Len());

	int32 Index = 0;

	while (Index < Source.Len())
	{
		const int32 Open = Source.Find(GlyphTokenOpen, ESearchCase::CaseSensitive, ESearchDir::FromStart, Index);

		if (Open == INDEX_NONE)
		{
			Result += Source.Mid(Index);
			break;
		}

		const int32 Close = Source.Find(GlyphTokenClose, ESearchCase::CaseSensitive, ESearchDir::FromStart, Open + 1);

		if (Close == INDEX_NONE)
		{
			Result += Source.Mid(Index);
			break;
		}

		Result += Source.Mid(Index, Open - Index);

		const FString Token = Source.Mid(Open + 1, Close - Open - 1);
		const TObjectPtr<UInputAction>* Action = GlyphTokenActions.Find(FName(*Token));
		const FKey BoundKey = Action ? UGodsentFunctionLibrary::GetBoundKeyForInputAction(GetOwningPlayer(), *Action, bPreferGamepadGlyphs, FallbackMappingContext) : EKeys::Invalid;

		// The decorator table has a row per key, named after the key (e.g. Gamepad_FaceButton_Right)
		if (BoundKey.IsValid())
			Result += FString::Printf(TEXT("<img id=\"%s\"/>"), *BoundKey.GetFName().ToString());
		else
			Result += Token;

		Index = Close + 1;
	}

	return FText::FromString(Result);
}

FText UGodsentSkillDisplayWindow::GetCategoryLabel(const FGameplayTag& SkillTag) const
{
	TArray<FString> TagParts;
	SkillTag.ToString().ParseIntoArray(TagParts, SkillTagSeparator);

	// Not a Skill.Tree.Branch.Name tag: leave the line empty
	if (!TagParts.IsValidIndex(SkillTagBranchIndex))
		return FText::GetEmpty();

	// WeaponWielding → Weapon Wielding
	const FString Tree = FName::NameToDisplayString(TagParts[SkillTagTreeIndex], false);
	const FString Branch = FName::NameToDisplayString(TagParts[SkillTagBranchIndex], false);

	FFormatNamedArguments Args;
	Args.Add(TEXT("Tree"), FText::FromString(Tree));
	Args.Add(TEXT("Branch"), FText::FromString(Branch));

	return FText::Format(CategoryFormat, Args);
}

void UGodsentSkillDisplayWindow::LoadIcon(const TSoftObjectPtr<UTexture2D>& Icon)
{
	if (IconLoadHandle.IsValid())
		IconLoadHandle->CancelHandle();

	IconLoadHandle.Reset();

	// Already in memory: show it now so the window never shows the last skill's icon
	if (UTexture2D* LoadedIcon = Icon.Get())
	{
		ApplyIcon(LoadedIcon);
		return;
	}

	ApplyIcon(FallbackIcon);

	TWeakObjectPtr<UGodsentSkillDisplayWindow> WeakThis = this;
	const FSoftObjectPath IconPath = Icon.ToSoftObjectPath();

	IconLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(IconPath, [WeakThis, IconPath]()
	{
		if (UGodsentSkillDisplayWindow* Window = WeakThis.Get())
			Window->ApplyIcon(Cast<UTexture2D>(IconPath.ResolveObject()));
	});
}

void UGodsentSkillDisplayWindow::ApplyIcon(UTexture2D* Texture)
{
	if (!IconImage)
		return;

	// No icon at all: hide the image rather than show the last skill's
	IconImage->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

	if (Texture)
		IconImage->SetBrushFromTexture(Texture, false);
}
