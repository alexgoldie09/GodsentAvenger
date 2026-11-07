// Alex Goldberg rights reserved


#include "GodsentGameInstance.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

/// Reads -trc_owned=ID[,ID...] from the command line. Ownership lives only here, in memory, for this launch.
void UGodsentGameInstance::Init()
{
	Super::Init();

	OwnedContentIds.Reset();

	FString OwnedList;

	// bShouldStopOnSeparator = false so a comma-separated list is read whole
	if (FParse::Value(FCommandLine::Get(), TEXT("trc_owned="), OwnedList, false))
	{
		TArray<FString> Ids;
		OwnedList.ParseIntoArray(Ids, TEXT(","), true);

		for (FString& Id : Ids)
		{
			Id.TrimStartAndEndInline();

			if (!Id.IsEmpty())
				OwnedContentIds.Add(FName(*Id));
		}
	}

	UE_LOG(LogTemp, Display, TEXT("[Add-ons] -trc_owned: %s"), OwnedList.IsEmpty() ? TEXT("(none)") : *OwnedList);
}

bool UGodsentGameInstance::IsContentOwned(FName ContentId) const
{
	if (ContentId.IsNone())
		return true;

#if WITH_EDITOR
	// Play In Editor only; Standalone and packaged builds always go by -trc_owned
	const UWorld* World = GetWorld();

	if (bTreatAddOnsAsOwnedInPIE && World && World->WorldType == EWorldType::PIE)
		return true;
#endif

	return OwnedContentIds.Contains(ContentId);
}

FName UGodsentGameInstance::GetRequiredContentIdForLevel(FGameplayTag LevelTag) const
{
	for (const FGodsentGameLevelSet& LevelSet : GodsentGameLevelSets)
	{
		if (LevelSet.IsValid() && LevelSet.LevelTag == LevelTag)
			return LevelSet.RequiredContentId;
	}

	return NAME_None;
}

/// Checked at every access point: the main menu button, Continue / Try Again, and the add-on level itself on load
bool UGodsentGameInstance::CanOpenGameLevel(FGameplayTag LevelTag, FText& OutReason) const
{
	OutReason = FText::GetEmpty();

	const FName RequiredContentId = GetRequiredContentIdForLevel(LevelTag);
	const bool bOwned = IsContentOwned(RequiredContentId);

	if (!RequiredContentId.IsNone())
	{
		UE_LOG(LogTemp, Display, TEXT("[Add-ons] Ownership check for %s (%s): %s"),
			*LevelTag.ToString(), *RequiredContentId.ToString(), bOwned ? TEXT("owned") : TEXT("NOT owned"));
	}

	if (!bOwned)
		OutReason = FText::FromString(TEXT("You don't own this add-on."));

	return bOwned;
}

/// Gets the level associated with a gameplay tag, or nullptr if not found
TSoftObjectPtr<UWorld> UGodsentGameInstance::GetGameLevelByTag(FGameplayTag InTag) const
{
	for (const FGodsentGameLevelSet& LevelSet : GodsentGameLevelSets)
	{
		if (!LevelSet.IsValid())
			continue;
		
		if (LevelSet.LevelTag == InTag)
			return LevelSet.Level;
	}
	
	return TSoftObjectPtr<UWorld>();
}

/// The tag of the level set whose map is this world (works in PIE too)
FGameplayTag UGodsentGameInstance::FindLevelTagForWorld(const UWorld* World) const
{
	if (!World)
		return FGameplayTag();

	// In PIE the package is "/Game/Maps/UEDPIE_0_FeatureDevMap"; strip that so it matches the asset path
	const FString WorldPackageName = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());

	for (const FGodsentGameLevelSet& LevelSet : GodsentGameLevelSets)
	{
		if (LevelSet.IsValid() && LevelSet.Level.ToSoftObjectPath().GetLongPackageName() == WorldPackageName)
			return LevelSet.LevelTag;
	}

	return FGameplayTag();
}
