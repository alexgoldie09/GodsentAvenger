// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/GameInstance.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentGameInstance.generated.h"

/// What a quality profile applies at startup
USTRUCT(BlueprintType)
struct FGodsentQualityProfileSettings
{
	GENERATED_BODY()

	FGodsentQualityProfileSettings() {}

	FGodsentQualityProfileSettings(const FIntPoint& InResolution, float InFrameRateLimit, int32 InOverallScalabilityLevel)
		: Resolution(InResolution), FrameRateLimit(InFrameRateLimit), OverallScalabilityLevel(InOverallScalabilityLevel)
	{
	}

	/// Screen resolution (only visible in Windowed or exclusive Fullscreen; Windowed Fullscreen uses the desktop's)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quality Profile")
	FIntPoint Resolution = FIntPoint(1920, 1080);

	/// Frame rate cap in frames per second (0 = uncapped)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quality Profile", meta = (ClampMin = "0.0", Units = "Hz"))
	float FrameRateLimit = 60.f;

	/// Overall scalability: 0 Low, 1 Medium, 2 High, 3 Epic, 4 Cinematic
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quality Profile", meta = (ClampMin = "0", ClampMax = "4"))
	int32 OverallScalabilityLevel = 1;
};

USTRUCT(BlueprintType)
struct FGodsentGameLevelSet
{
	GENERATED_BODY()
	
	/// The gameplay tag that represents this level
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "GameData.Level"))
	FGameplayTag LevelTag;
	
	/// The level to load for this tag
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> Level;

	/// Add-on (DLC) that must be owned to open this level, i.e. listed in -trc_owned. None = base game, always available.
	UPROPERTY(EditDefaultsOnly)
	FName RequiredContentId;
	
	/// Checks whether this level set is valid (has a valid tag and a non-null level)
	bool IsValid() const
	{
		return LevelTag.IsValid() && !Level.IsNull();
	}
};

/**
 * Also owns add-on (DLC) ownership. The store decides what a player owns, so ownership is read from the -trc_owned
 * command line flag in Init (e.g. -trc_owned=GodsentDLC01, comma-separate several IDs), kept in memory only
 * (never written to save data), and checked every time add-on content is opened.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/// Reads -trc_owned (add-on ownership) and -trc_profile (quality profile, applied straight away) from the command line
	virtual void Init() override;
	
	/// Applies the launch quality profile. Done here rather than in Init because Unreal only applies
    	/// scalability once the engine has finished starting up.
    	virtual void OnStart() override;

	// ---- Quality profiles (ENH-01) ----

	/// The profile this launch uses: -trc_profile=enhanced gives Enhanced, -trc_profile=base or no flag gives Base
	UFUNCTION(BlueprintPure, Category = "Godsent|Quality Profile")
	EGodsentQualityProfile GetActiveQualityProfile() const { return ActiveQualityProfile; }

	/// The settings a profile applies
	UFUNCTION(BlueprintPure, Category = "Godsent|Quality Profile")
	FGodsentQualityProfileSettings GetQualityProfileSettings(EGodsentQualityProfile Profile) const;

	/// Applies a profile's resolution, frame rate cap and scalability through the game user settings, and saves them.
	/// Called at startup with the launch profile; an options menu "Reset to profile" button can call it too.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Quality Profile")
	void ApplyQualityProfile(EGodsentQualityProfile Profile);

	/// True if this add-on ID was listed in -trc_owned (None counts as owned: base-game content)
	UFUNCTION(BlueprintPure, Category = "Godsent|Add-ons")
	bool IsContentOwned(FName ContentId) const;

	/// Ownership check for opening a level: true for base-game levels and for add-on levels the player owns.
	/// Out Reason explains a refusal (shown on the main menu).
	UFUNCTION(BlueprintPure, Category = "Godsent|Add-ons", meta = (GameplayTagFilter = "GameData.Level"))
	bool CanOpenGameLevel(FGameplayTag LevelTag, FText& OutReason) const;

	/// The add-on a level needs (None for base-game levels)
	UFUNCTION(BlueprintPure, Category = "Godsent|Add-ons", meta = (GameplayTagFilter = "GameData.Level"))
	FName GetRequiredContentIdForLevel(FGameplayTag LevelTag) const;

private:
	/// Add-on IDs from -trc_owned. Runtime only: never saved.
	TSet<FName> OwnedContentIds;

	/// Editor convenience: in Play In Editor every add-on counts as owned, so the DLC map can be worked on directly.
	/// Has no effect in Standalone or packaged builds, where only -trc_owned counts (test the requirement there).
	UPROPERTY(EditDefaultsOnly, Category = "Add-ons")
	bool bTreatAddOnsAsOwnedInPIE = true;

	/// Base console model: applied with -trc_profile=base, or when there's no flag
	UPROPERTY(EditDefaultsOnly, Category = "Quality Profiles")
	FGodsentQualityProfileSettings BaseProfile = FGodsentQualityProfileSettings(FIntPoint(1920, 1080), 60.f, 1);

	/// Enhanced console model: applied with -trc_profile=enhanced
	UPROPERTY(EditDefaultsOnly, Category = "Quality Profiles")
	FGodsentQualityProfileSettings EnhancedProfile = FGodsentQualityProfileSettings(FIntPoint(3840, 2160), 30.f, 2);

	/// Editor convenience: Play In Editor never applies a profile (it would change the editor's own scalability).
	/// Standalone and packaged builds always apply one.
	UPROPERTY(EditDefaultsOnly, Category = "Quality Profiles")
	bool bSkipQualityProfileInPIE = true;

	EGodsentQualityProfile ActiveQualityProfile = EGodsentQualityProfile::Base;

protected:
	/// The set of levels available in the game, each associated with a gameplay tag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGodsentGameLevelSet> GodsentGameLevelSets;
	
public:
	/// Gets the level associated with a gameplay tag, or nullptr if not found
	UFUNCTION(BlueprintPure, meta = (GameplayTagFilter = "GameData.Level"))
	TSoftObjectPtr<UWorld> GetGameLevelByTag(FGameplayTag InTag) const;

	/// The tag of the level set whose map is this world (works in PIE too). Empty if the map isn't in the level sets.
	UFUNCTION(BlueprintPure)
	FGameplayTag FindLevelTagForWorld(const UWorld* World) const;
};
