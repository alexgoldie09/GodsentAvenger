// Alex Goldberg rights reserved


#include "GodsentGameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
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

	// Quality profile: the platform tells the game which model it's on; no flag means Base
	FString ProfileName;
	ActiveQualityProfile = EGodsentQualityProfile::Base;

	if (FParse::Value(FCommandLine::Get(), TEXT("trc_profile="), ProfileName))
	{
		if (ProfileName.Equals(TEXT("enhanced"), ESearchCase::IgnoreCase))
			ActiveQualityProfile = EGodsentQualityProfile::Enhanced;
		else if (!ProfileName.Equals(TEXT("base"), ESearchCase::IgnoreCase))
			UE_LOG(LogTemp, Warning, TEXT("[Profile] Unknown -trc_profile=%s, using Base"), *ProfileName);
	}

	UE_LOG(LogTemp, Display, TEXT("[Profile] -trc_profile: %s -> %s"),
		ProfileName.IsEmpty() ? TEXT("(none)") : *ProfileName,
		ActiveQualityProfile == EGodsentQualityProfile::Enhanced ? TEXT("Enhanced") : TEXT("Base"));
}

/// Applies the profile read in Init, now that the engine is fully up (scalability is ignored before this)
void UGodsentGameInstance::OnStart()
{
	Super::OnStart();

	// Applying in Play In Editor would change the editor's own scalability and frame rate
	if (!GIsEditor || !bSkipQualityProfileInPIE)
		ApplyQualityProfile(ActiveQualityProfile);
}

FGodsentQualityProfileSettings UGodsentGameInstance::GetQualityProfileSettings(EGodsentQualityProfile Profile) const
{
	return Profile == EGodsentQualityProfile::Enhanced ? EnhancedProfile : BaseProfile;
}

/// Get Game User Settings → Set Screen Resolution / Set Frame Rate Limit / Set Overall Scalability Level → Apply Settings
void UGodsentGameInstance::ApplyQualityProfile(EGodsentQualityProfile Profile)
{
	UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;

	if (!GameUserSettings)
		return;

	const FGodsentQualityProfileSettings Settings = GetQualityProfileSettings(Profile);

	GameUserSettings->SetScreenResolution(Settings.Resolution);
	GameUserSettings->SetFrameRateLimit(Settings.FrameRateLimit);
	GameUserSettings->SetOverallScalabilityLevel(Settings.OverallScalabilityLevel);
	GameUserSettings->ApplySettings(false);

	UE_LOG(LogTemp, Display, TEXT("[Profile] Applied %s: %dx%d, %.0f fps cap, scalability %d"),
		Profile == EGodsentQualityProfile::Enhanced ? TEXT("Enhanced") : TEXT("Base"),
		Settings.Resolution.X, Settings.Resolution.Y, Settings.FrameRateLimit, Settings.OverallScalabilityLevel);
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
