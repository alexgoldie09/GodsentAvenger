// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GodsentSkillConnectorWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTexture2D;
class USkillTreeComponent;

/// One point along a vein: once Skill Tag is owned, the glow reaches Progress
USTRUCT(BlueprintType)
struct FGodsentVeinGlowStop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glow", meta = (Categories = "Skill"))
	FGameplayTag SkillTag;

	/// How far along the vein the glow reaches: 0 = outer tip, 1 = heart end (where the node sits on the vein)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Progress = 1.f;
};

/**
 * A vein on the skill tree (WBP_SkillConnector), placed by hand on the panel's canvas (position, size, rotation, flip).
 *
 * The glow flows in from the outer tip towards the heart, as far as the furthest owned Glow Stop.
 * Buying a skill makes it flow out over Glow Fill Time; opening the screen shows the current state straight away.
 *
 * Vein Texture is Joel's vein image; Flow Map is its matching _Flow image (black = outer tip, white = heart end).
 * Tick Reverse Flow if this vein is placed with its heart end pointing away from the heart.
 *
 * In the designer the plain Vein Texture is shown so it can be lined up by eye; in game it switches to Vein Material.
 * Auto Place Between Nodes is the old straight-line mode (stretched and rotated between From and To).
 */
UCLASS(Abstract)
class GODSENTAVENGER_API UGodsentSkillConnectorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/// Joel's vein image
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vein")
	TObjectPtr<UTexture2D> VeinTexture;

	/// The vein's _Flow image. Empty = the whole vein lights at once.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vein")
	TObjectPtr<UTexture2D> FlowMap;

	/// Flips which end the glow starts from (for a vein placed the other way round)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vein")
	bool bReverseFlow = false;

	/// The skills along this vein and how far each one lights it. Empty = a decoration that never glows.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vein", meta = (TitleProperty = "SkillTag"))
	TArray<FGodsentVeinGlowStop> GlowStops;

	/// Old straight-line mode: the panel stretches and rotates this between From and To
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auto Place")
	bool bAutoPlaceBetweenNodes = false;

	/// Start of the line. Empty = the panel's CentreAnchor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auto Place", meta = (Categories = "Skill", EditCondition = "bAutoPlaceBetweenNodes"))
	FGameplayTag FromSkillTag;

	/// End of the line. With no Glow Stops, the line lights fully once this is owned.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auto Place", meta = (Categories = "Skill", EditCondition = "bAutoPlaceBetweenNodes"))
	FGameplayTag ToSkillTag;

	/// Line thickness on the canvas in auto-place mode
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Auto Place", meta = (ClampMin = "1.0", EditCondition = "bAutoPlaceBetweenNodes"))
	float LineThickness = 24.f;

	/// Works out how far the glow should reach from what's owned. Animate = flow out to it rather than jump.
	void RefreshGlow(const USkillTreeComponent* SkillTree, bool bAnimate);

	/// Sets how far the glow reaches (0-1)
	UFUNCTION(BlueprintCallable, Category = "Godsent|SkillTree")
	void SetGlowProgress(float NewProgress, bool bAnimate);

	/// How far the glow reaches right now (mid-animation included)
	UFUNCTION(BlueprintPure, Category = "Godsent|SkillTree")
	float GetGlowProgress() const { return DisplayedProgress; }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/// The glow has a new target (Animated = it's flowing out to it, e.g. a skill was just bought)
	UFUNCTION(BlueprintImplementableEvent, Category = "Vein", meta = (DisplayName = "On Glow Target Changed"))
	void BP_OnGlowTargetChanged(float NewProgress, bool bAnimated);

	/// The vein image. Must be called LineImage.
	UPROPERTY(BlueprintReadOnly, Category = "Vein", meta = (BindWidget))
	TObjectPtr<UImage> LineImage;

	/// M_SkillVein. Each vein makes its own copy in game, with its own textures.
	UPROPERTY(EditDefaultsOnly, Category = "Vein|Material")
	TObjectPtr<UMaterialInterface> VeinMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Vein|Material")
	FName VeinTextureParameter = TEXT("VeinTexture");

	UPROPERTY(EditDefaultsOnly, Category = "Vein|Material")
	FName FlowMapParameter = TEXT("FlowMap");

	UPROPERTY(EditDefaultsOnly, Category = "Vein|Material")
	FName GlowProgressParameter = TEXT("GlowProgress");

	UPROPERTY(EditDefaultsOnly, Category = "Vein|Material")
	FName ReverseFlowParameter = TEXT("ReverseFlow");

	/// Seconds for the glow to flow the whole length of a vein
	UPROPERTY(EditDefaultsOnly, Category = "Vein|Glow", meta = (ClampMin = "0.0", Units = "s"))
	float GlowFillTime = 0.6f;

	/// Only used when there's no Vein Material: the image is tinted between these instead
	UPROPERTY(EditDefaultsOnly, Category = "Vein|Glow")
	FLinearColor UnlitColour = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Vein|Glow")
	FLinearColor LitColour = FLinearColor(1.f, 0.8f, 0.35f, 1.f);

private:
	/// Sends the displayed progress to the material (or the tint)
	void ApplyProgress();

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> VeinMaterialInstance;

	float DisplayedProgress = 0.f;
	float TargetProgress = 0.f;
	bool bHasTarget = false;
};
