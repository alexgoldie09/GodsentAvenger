// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "DataAsset_SkillTree.generated.h"

/**
 * Every skill in the tree (24 nodes: 4 trees x 2 categories x 3 tiers), looked up by skill tag.
 */
UCLASS()
class GODSENTAVENGER_API UDataAsset_SkillTree : public UDataAsset
{
	GENERATED_BODY()

public:
	/// The skill with this tag, or nullptr if it isn't in the tree
	const FGodsentSkillNode* FindSkillNode(const FGameplayTag& InSkillTag) const;

	/// All skills; order doesn't matter (prerequisites decide the tier order)
	UPROPERTY(EditDefaultsOnly, Category = "SkillTree", meta = (TitleProperty = "SkillTag"))
	TArray<FGodsentSkillNode> SkillNodes;
};
