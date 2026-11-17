// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GodsentSkillConnectorWidget.generated.h"

class UImage;

/**
 * A line between two skill nodes (WBP_SkillConnector).
 *
 * Only From and To are set by hand. The tree panel places, stretches and rotates it between those two nodes
 * (in the designer too, after a compile), so moving a node moves its lines.
 *
 * Glows once the To skill is unlocked: sets the Glow scalar on LineImage's material (0/1), or swaps
 * between Unlit and Lit Colour if LineImage just uses a texture.
 */
UCLASS(Abstract)
class GODSENTAVENGER_API UGodsentSkillConnectorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/// Start of the line. Empty = the centre of the cross (the panel's CentreAnchor).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Connector", meta = (Categories = "Skill"))
	FGameplayTag FromSkillTag;

	/// End of the line. It glows once this skill is unlocked.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Connector", meta = (Categories = "Skill"))
	FGameplayTag ToSkillTag;

	/// How thick the line is on the canvas (include room for the glow)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Connector", meta = (ClampMin = "1.0"))
	float LineThickness = 24.f;

	/// Turns the glow on or off
	UFUNCTION(BlueprintCallable, Category = "Godsent|SkillTree")
	void SetGlowing(bool bInGlowing);

	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree")
	bool IsGlowing() const { return bGlowing; }

protected:
	virtual void NativePreConstruct() override;

	/// Glow changed (e.g. play a light-up animation when a skill is bought)
	UFUNCTION(BlueprintImplementableEvent, Category = "Connector", meta = (DisplayName = "On Glow Changed"))
	void BP_OnGlowChanged(bool bNowGlowing);

	/// The line itself. Must be called LineImage.
	UPROPERTY(BlueprintReadOnly, Category = "Connector", meta = (BindWidget))
	TObjectPtr<UImage> LineImage;

	/// Scalar on the line material: 0 = plain, 1 = glowing
	UPROPERTY(EditDefaultsOnly, Category = "Connector|Glow")
	FName GlowParameterName = TEXT("Glow");

	/// Only used when LineImage has a texture rather than a material
	UPROPERTY(EditDefaultsOnly, Category = "Connector|Glow")
	FLinearColor UnlitColour = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Connector|Glow")
	FLinearColor LitColour = FLinearColor(1.f, 0.8f, 0.35f, 1.f);

private:
	void ApplyGlow();

	bool bGlowing = false;
	bool bHasGlowState = false;
};
