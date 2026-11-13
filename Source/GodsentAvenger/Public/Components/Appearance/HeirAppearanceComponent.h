// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GodsentLineageSubsystem.h"
#include "HeirAppearanceComponent.generated.h"

class UAudioComponent;
class UGodsentVoiceSetDataAsset;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
class USoundBase;
struct FStreamableHandle;

/**
 * Puts an heir's look and voice on a knight mesh. The hero and the menu display (BP_MenuHeroDisplay) both use it,
 * so the customiser preview and the knight in game always match.
 *
 * Colours and the pattern go onto the mesh's three materials (by slot name). The voice set is loaded in the
 * background and plays lines by tag (Play Hero Voice notifies in montages, or Play Voice Line).
 */
UCLASS(ClassGroup = (Godsent), meta = (BlueprintSpawnableComponent))
class GODSENTAVENGER_API UHeirAppearanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeirAppearanceComponent();

	/// Puts this look on the mesh and loads its voice
	UFUNCTION(BlueprintCallable, Category = "Godsent|Appearance")
	void ApplyAppearance(const FGodsentHeirAppearance& Appearance);

	/// Puts the family's saved look on the mesh (the default look if the player hasn't customised yet)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Appearance")
	void ApplyLineageAppearance();

	/// The look on the mesh right now (the customiser starts from this when continuing)
	UFUNCTION(BlueprintPure, Category = "Godsent|Appearance")
	FGodsentHeirAppearance GetCurrentAppearance() const { return CurrentAppearance; }

	/// The materials' own look, read before anything was applied (the customiser starts from this on New Game)
	UFUNCTION(BlueprintPure, Category = "Godsent|Appearance")
	FGodsentHeirAppearance GetDefaultAppearance() const { return DefaultAppearance; }

	/// The loaded voice (nullptr while it's still loading)
	UFUNCTION(BlueprintPure, Category = "Godsent|Appearance")
	UGodsentVoiceSetDataAsset* GetVoiceSet() const;

	/// Plays this voice's line for a tag from the mesh. Nothing if the voice doesn't have that line yet.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Appearance", meta = (GameplayTagFilter = "Voice"))
	UAudioComponent* PlayVoiceLine(FGameplayTag VoiceLine);

	/// Plays any line from the current voice (customiser preview). Waits for the voice if it's still loading.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Appearance")
	void PlayRandomVoiceLine();

	/// Stops the line that's playing, if any
	UFUNCTION(BlueprintCallable, Category = "Godsent|Appearance")
	void StopVoiceLine();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/// On for the hero and the menu display: show the family's look as soon as it spawns
	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	bool bApplyLineageAppearanceOnBeginPlay = true;

	/// A new line cuts off the last one, so grunts never stack up
	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	bool bInterruptPreviousVoiceLine = true;

	/// Material slot names on the knight mesh
	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Material Slots")
	FName MainArmourSlotName = TEXT("Metal_Tex");

	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Material Slots")
	FName SecondaryArmourSlotName = TEXT("Painted_Tex");

	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Material Slots")
	FName ClothSlotName = TEXT("Cloth_Tex");

	/// Material parameter names
	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Material Parameters")
	FName MetalColourParameter = TEXT("Metal Colour");

	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Material Parameters")
	FName PatternColourParameter = TEXT("Pattern Colour");

	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Material Parameters")
	FName PatternTextureObjectParameter = TEXT("Pattern Texture Object");

	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Material Parameters")
	FName PatternTextureSampleParameter = TEXT("Pattern Texture Sample");

	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Material Parameters")
	FName ClothColourParameter = TEXT("Cloth Colour");

private:
	/// The character's mesh, or the first skeletal mesh on the actor
	USkeletalMeshComponent* GetTargetMesh() const;

	/// The dynamic material for a slot (made the first time it's needed)
	UMaterialInstanceDynamic* GetSlotMaterial(FName SlotName) const;

	/// The look the mesh's materials have right now
	FGodsentHeirAppearance ReadAppearanceFromMesh() const;

	void LoadVoice(int32 VoiceIndex);
	void OnVoiceLoaded();
	UAudioComponent* PlaySound(USoundBase* Sound);

	FGodsentHeirAppearance CurrentAppearance;
	FGodsentHeirAppearance DefaultAppearance;

	UPROPERTY()
	TObjectPtr<UGodsentVoiceSetDataAsset> VoiceSet;

	/// The voice being loaded, and the handle keeping it loaded
	FSoftObjectPath LoadingVoicePath;
	TSharedPtr<FStreamableHandle> VoiceLoadHandle;

	bool bPlayRandomLineWhenLoaded = false;

	TWeakObjectPtr<UAudioComponent> ActiveVoiceLine;
};
