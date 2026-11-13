// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_PlayHeroVoice.generated.h"

class UGodsentVoiceSetDataAsset;

/**
 * Plays a voice line by tag from the heir's chosen voice (use instead of Play Sound for voice lines in montages).
 * In the animation editor it plays the line from Preview Voice Set instead, so you can hear the timing.
 */
UCLASS(meta = (DisplayName = "Play Hero Voice"))
class GODSENTAVENGER_API UAnimNotify_PlayHeroVoice : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/// Which line to play (Voice.Attack1, Voice.Death...)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice", meta = (Categories = "Voice"))
	FGameplayTag VoiceLine;

#if WITH_EDITORONLY_DATA
	/// Animation editor only: the voice to hear while previewing. Not in the game (stripped when packaging).
	UPROPERTY(EditAnywhere, Category = "Voice")
	TSoftObjectPtr<UGodsentVoiceSetDataAsset> PreviewVoiceSet;
#endif
};
