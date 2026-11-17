// Alex Goldberg rights reserved

#include "Widgets/SkillTree/GodsentSkillConnectorWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/// Glow parameter values
	constexpr float LineGlowOff = 0.f;
	constexpr float LineGlowOn = 1.f;
}

void UGodsentSkillConnectorWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Start unlit (the panel lights it once it knows what's unlocked). Does nothing in the designer.
	ApplyGlow();
}

void UGodsentSkillConnectorWidget::SetGlowing(bool bInGlowing)
{
	if (bHasGlowState && bGlowing == bInGlowing)
		return;

	bGlowing = bInGlowing;
	bHasGlowState = true;

	ApplyGlow();
	BP_OnGlowChanged(bGlowing);
}

void UGodsentSkillConnectorWidget::ApplyGlow()
{
	// Never in the designer: a material made there gets saved into the widget and shared by every line
	if (!LineImage || IsDesignTime())
		return;

	// A material brush gets the Glow scalar; a plain texture gets a colour swap
	if (UMaterialInstanceDynamic* LineMaterial = LineImage->GetDynamicMaterial())
		LineMaterial->SetScalarParameterValue(GlowParameterName, bGlowing ? LineGlowOn : LineGlowOff);
	else
		LineImage->SetColorAndOpacity(bGlowing ? LitColour : UnlitColour);
}