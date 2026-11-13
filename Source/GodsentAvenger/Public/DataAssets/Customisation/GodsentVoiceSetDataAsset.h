// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GodsentVoiceSetDataAsset.generated.h"

class USoundBase;

/**
 * One hero voice (DA_Voice_A, DA_Voice_B...): which sound plays for each voice line tag.
 * Montages and cues only ask for a tag (Voice.Attack1, Voice.Death...), so swapping the voice never touches them.
 */
UCLASS(BlueprintType)
class GODSENTAVENGER_API UGodsentVoiceSetDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/// The sound for a voice line, or nullptr if this voice doesn't have it yet
	UFUNCTION(BlueprintPure, Category = "Godsent|Voice")
	USoundBase* GetLine(FGameplayTag VoiceLine) const;

	/// Any line this voice has (the customiser's preview)
	UFUNCTION(BlueprintPure, Category = "Godsent|Voice")
	USoundBase* GetRandomLine() const;

	/// Voice line tag -> sound cue. Leave a tag out if this voice doesn't have that line.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voice", meta = (Categories = "Voice", ForceInlineRow))
	TMap<FGameplayTag, TObjectPtr<USoundBase>> Lines;
};
