// Alex Goldberg rights reserved

#include "Widgets/SkillTree/GodsentSkillConnectorWidget.h"
#include "Components/Image.h"
#include "Components/SkillTree/SkillTreeComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/// Glow progress at either end of a vein
	constexpr float VeinGlowNone = 0.f;
	constexpr float VeinGlowFull = 1.f;

	/// Reverse Flow as a material scalar
	constexpr float FlowForward = 0.f;
	constexpr float FlowReversed = 1.f;
}

/// Designer and game: show the plain vein so it can be lined up by eye
void UGodsentSkillConnectorWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Keeps the size set on the canvas (the texture just fills it)
	if (LineImage && VeinTexture)
		LineImage->SetBrushFromTexture(VeinTexture, false);
}

/// Game only: swap to this vein's own copy of the glow material
void UGodsentSkillConnectorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LineImage && VeinMaterial && !IsDesignTime())
	{
		VeinMaterialInstance = UMaterialInstanceDynamic::Create(VeinMaterial, this);

		if (VeinTexture)
			VeinMaterialInstance->SetTextureParameterValue(VeinTextureParameter, VeinTexture);

		// No flow map: the material's default (black) lights the whole vein at once
		if (FlowMap)
			VeinMaterialInstance->SetTextureParameterValue(FlowMapParameter, FlowMap);

		VeinMaterialInstance->SetScalarParameterValue(ReverseFlowParameter, bReverseFlow ? FlowReversed : FlowForward);

		LineImage->SetBrushFromMaterial(VeinMaterialInstance);
	}

	ApplyProgress();
}

void UGodsentSkillConnectorWidget::RefreshGlow(const USkillTreeComponent* SkillTree, bool bAnimate)
{
	if (!SkillTree)
		return;

	float NewTarget = VeinGlowNone;

	// The furthest owned stop wins
	for (const FGodsentVeinGlowStop& Stop : GlowStops)
	{
		if (SkillTree->IsSkillUnlocked(Stop.SkillTag))
			NewTarget = FMath::Max(NewTarget, Stop.Progress);
	}

	// Old straight lines: fully lit once their To skill is owned
	if (GlowStops.IsEmpty() && ToSkillTag.IsValid() && SkillTree->IsSkillUnlocked(ToSkillTag))
		NewTarget = VeinGlowFull;

	SetGlowProgress(NewTarget, bAnimate);
}

void UGodsentSkillConnectorWidget::SetGlowProgress(float NewProgress, bool bAnimate)
{
	NewProgress = FMath::Clamp(NewProgress, VeinGlowNone, VeinGlowFull);

	if (bHasTarget && FMath::IsNearlyEqual(NewProgress, TargetProgress))
		return;

	TargetProgress = NewProgress;
	bHasTarget = true;

	// Only flowing outwards is animated; going back (a new family) just jumps
	const bool bFlow = bAnimate && GlowFillTime > 0.f && TargetProgress > DisplayedProgress;

	if (!bFlow)
	{
		DisplayedProgress = TargetProgress;
		ApplyProgress();
	}

	BP_OnGlowTargetChanged(TargetProgress, bFlow);
}

void UGodsentSkillConnectorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (DisplayedProgress == TargetProgress)
		return;

	if (GlowFillTime <= 0.f)
	{
		DisplayedProgress = TargetProgress;
		ApplyProgress();
		return;
	}

	DisplayedProgress = FMath::FInterpConstantTo(DisplayedProgress, TargetProgress, InDeltaTime, (VeinGlowFull - VeinGlowNone) / GlowFillTime);
	ApplyProgress();
}

void UGodsentSkillConnectorWidget::ApplyProgress()
{
	if (!LineImage || IsDesignTime())
		return;

	if (VeinMaterialInstance)
		VeinMaterialInstance->SetScalarParameterValue(GlowProgressParameter, DisplayedProgress);
	else
		LineImage->SetColorAndOpacity(FMath::Lerp(UnlitColour, LitColour, DisplayedProgress));
}
