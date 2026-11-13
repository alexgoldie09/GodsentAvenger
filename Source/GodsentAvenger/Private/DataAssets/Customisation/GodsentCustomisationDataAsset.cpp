// Alex Goldberg rights reserved

#include "DataAssets/Customisation/GodsentCustomisationDataAsset.h"
#include "DataAssets/Customisation/GodsentVoiceSetDataAsset.h"
#include "Engine/Texture.h"
#include "GodsentGameInstance.h"
#include "Kismet/GameplayStatics.h"

UGodsentCustomisationDataAsset* UGodsentCustomisationDataAsset::GetCustomisationCatalogue(const UObject* WorldContextObject)
{
	const UGodsentGameInstance* GameInstance = Cast<UGodsentGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
	return GameInstance ? GameInstance->GetCustomisationCatalogue() : nullptr;
}

FText UGodsentCustomisationDataAsset::GetPatternName(int32 Index) const
{
	return Patterns.IsValidIndex(Index) ? Patterns[Index].DisplayName : FText::GetEmpty();
}

UTexture* UGodsentCustomisationDataAsset::LoadPatternTexture(int32 Index) const
{
	// Small textures: a direct load is fine here (customiser, or once when the hero spawns)
	return Patterns.IsValidIndex(Index) ? Patterns[Index].Texture.LoadSynchronous() : nullptr;
}

int32 UGodsentCustomisationDataAsset::FindPatternIndex(const UTexture* Texture) const
{
	if (!Texture)
		return 0;

	const FSoftObjectPath TexturePath(Texture);

	for (int32 Index = 0; Index < Patterns.Num(); ++Index)
	{
		if (Patterns[Index].Texture.ToSoftObjectPath() == TexturePath)
			return Index;
	}

	return 0;
}

FText UGodsentCustomisationDataAsset::GetVoiceName(int32 Index) const
{
	return Voices.IsValidIndex(Index) ? Voices[Index].DisplayName : FText::GetEmpty();
}

TSoftObjectPtr<UGodsentVoiceSetDataAsset> UGodsentCustomisationDataAsset::GetVoiceSet(int32 Index) const
{
	return Voices.IsValidIndex(Index) ? Voices[Index].VoiceSet : TSoftObjectPtr<UGodsentVoiceSetDataAsset>();
}

int32 UGodsentCustomisationDataAsset::WrapOptionIndex(int32 Index, int32 Count)
{
	if (Count <= 0)
		return 0;

	return ((Index % Count) + Count) % Count;
}
