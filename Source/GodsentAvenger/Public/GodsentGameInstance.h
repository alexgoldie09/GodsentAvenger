// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/GameInstance.h"
#include "GodsentGameInstance.generated.h"

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
	
	/// Checks whether this level set is valid (has a valid tag and a non-null level)
	bool IsValid() const
	{
		return LevelTag.IsValid() && !Level.IsNull();
	}
};

UCLASS()
class GODSENTAVENGER_API UGodsentGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
protected:
	/// The set of levels available in the game, each associated with a gameplay tag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGodsentGameLevelSet> GodsentGameLevelSets;
	
public:
	/// Gets the level associated with a gameplay tag, or nullptr if not found
	UFUNCTION(BlueprintPure, meta = (GameplayTagFilter = "GameData.Level"))
	TSoftObjectPtr<UWorld> GetGameLevelByTag(FGameplayTag InTag) const;
};
