// Alex Goldberg rights reserved


#include "GodsentGameInstance.h"
#include "Engine/World.h"

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
