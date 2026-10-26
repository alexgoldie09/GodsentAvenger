// Alex Goldberg rights reserved

#include "DataAssets/SkillTree/DataAsset_SkillTree.h"

/// The skill with this tag, or nullptr if it isn't in the tree
const FGodsentSkillNode* UDataAsset_SkillTree::FindSkillNode(const FGameplayTag& InSkillTag) const
{
	if (!InSkillTag.IsValid())
		return nullptr;

	return SkillNodes.FindByPredicate([&InSkillTag](const FGodsentSkillNode& SkillNode)
	{
		return SkillNode.SkillTag.MatchesTagExact(InSkillTag);
	});
}
