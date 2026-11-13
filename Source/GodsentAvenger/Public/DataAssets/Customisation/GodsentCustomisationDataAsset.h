// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GodsentCustomisationDataAsset.generated.h"

class UGodsentVoiceSetDataAsset;
class UTexture;

/// A pattern the player can pick for the secondary armour
USTRUCT(BlueprintType)
struct FGodsentPatternOption
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pattern")
	FText DisplayName;

	/// Goes into Pattern Texture Object and Pattern Texture Sample on the pattern material
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pattern")
	TSoftObjectPtr<UTexture> Texture;
};

/// A voice the player can pick
USTRUCT(BlueprintType)
struct FGodsentVoiceOption
{
	GENERATED_BODY()

	/// Shown in the customiser ("Voice A"...)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voice")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voice")
	TSoftObjectPtr<UGodsentVoiceSetDataAsset> VoiceSet;
};

/**
 * Everything the customiser can offer (DA_Customisation, set on BP_GodsentGameInstance).
 * The save only stores indexes into these lists. Textures and voices are soft, so they load only when used.
 */
UCLASS(BlueprintType)
class GODSENTAVENGER_API UGodsentCustomisationDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/// The catalogue set on the game instance (nullptr if none)
	UFUNCTION(BlueprintPure, Category = "Godsent|Customiser", meta = (WorldContext = "WorldContextObject"))
	static UGodsentCustomisationDataAsset* GetCustomisationCatalogue(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Godsent|Customiser")
	int32 GetPatternCount() const { return Patterns.Num(); }

	UFUNCTION(BlueprintPure, Category = "Godsent|Customiser")
	FText GetPatternName(int32 Index) const;

	/// Loads (if needed) and returns a pattern texture, e.g. for the customiser's preview image
	UFUNCTION(BlueprintCallable, Category = "Godsent|Customiser")
	UTexture* LoadPatternTexture(int32 Index) const;

	/// Which pattern uses this texture (0 if none)
	int32 FindPatternIndex(const UTexture* Texture) const;

	UFUNCTION(BlueprintPure, Category = "Godsent|Customiser")
	int32 GetVoiceCount() const { return Voices.Num(); }

	UFUNCTION(BlueprintPure, Category = "Godsent|Customiser")
	FText GetVoiceName(int32 Index) const;

	/// The voice set for an index (not loaded yet; the appearance component loads it)
	TSoftObjectPtr<UGodsentVoiceSetDataAsset> GetVoiceSet(int32 Index) const;

	/// Wraps an index into 0..Count-1, for the left/right selectors (Count 0 gives 0)
	UFUNCTION(BlueprintPure, Category = "Godsent|Customiser")
	static int32 WrapOptionIndex(int32 Index, int32 Count);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Customiser", meta = (TitleProperty = "DisplayName"))
	TArray<FGodsentPatternOption> Patterns;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Customiser", meta = (TitleProperty = "DisplayName"))
	TArray<FGodsentVoiceOption> Voices;

	/// The voice a brand-new heir starts with
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Customiser", meta = (ClampMin = "0"))
	int32 DefaultVoiceIndex = 0;
};
