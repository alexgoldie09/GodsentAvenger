// Alex Goldberg rights reserved

#include "AnimNotifies/AnimNotify_PlayHeroVoice.h"
#include "Components/Appearance/HeirAppearanceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DataAssets/Customisation/GodsentVoiceSetDataAsset.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

FString UAnimNotify_PlayHeroVoice::GetNotifyName_Implementation() const
{
	return VoiceLine.IsValid() ? FString::Printf(TEXT("Voice: %s"), *VoiceLine.ToString().RightChop(FString(TEXT("Voice.")).Len())) : TEXT("Voice: (none)");
}

void UAnimNotify_PlayHeroVoice::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
		return;

	const AActor* Owner = MeshComp->GetOwner();

	if (UHeirAppearanceComponent* Appearance = Owner ? Owner->FindComponentByClass<UHeirAppearanceComponent>() : nullptr)
	{
		Appearance->PlayVoiceLine(VoiceLine);
		return;
	}

#if WITH_EDITOR
	// Animation editor preview: no hero, so use the preview voice
	const UWorld* World = MeshComp->GetWorld();

	if (World && World->WorldType == EWorldType::EditorPreview)
	{
		if (const UGodsentVoiceSetDataAsset* VoiceSet = PreviewVoiceSet.LoadSynchronous())
		{
			if (USoundBase* Sound = VoiceSet->GetLine(VoiceLine))
				UGameplayStatics::PlaySound2D(MeshComp, Sound);
		}
	}
#endif
}
