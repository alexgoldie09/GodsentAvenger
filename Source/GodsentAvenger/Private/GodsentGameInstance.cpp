// Alex Goldberg rights reserved


#include "GodsentGameInstance.h"

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
