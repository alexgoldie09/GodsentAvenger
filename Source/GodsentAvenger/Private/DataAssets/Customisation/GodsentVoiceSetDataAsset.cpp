// Alex Goldberg rights reserved

#include "DataAssets/Customisation/GodsentVoiceSetDataAsset.h"
#include "Sound/SoundBase.h"

USoundBase* UGodsentVoiceSetDataAsset::GetLine(FGameplayTag VoiceLine) const
{
	const TObjectPtr<USoundBase>* Sound = Lines.Find(VoiceLine);
	return Sound ? Sound->Get() : nullptr;
}

USoundBase* UGodsentVoiceSetDataAsset::GetRandomLine() const
{
	TArray<USoundBase*> Sounds;

	for (const TPair<FGameplayTag, TObjectPtr<USoundBase>>& Line : Lines)
	{
		if (Line.Value)
			Sounds.Add(Line.Value);
	}

	return Sounds.Num() > 0 ? Sounds[FMath::RandRange(0, Sounds.Num() - 1)] : nullptr;
}
