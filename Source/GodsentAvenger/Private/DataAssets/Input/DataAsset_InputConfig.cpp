// Alex Goldberg rights reserved

#include "DataAssets/Input/DataAsset_InputConfig.h"
#include "InputAction.h"

/// Finds the native Input Action bound to a tag
UInputAction* UDataAsset_InputConfig::FindNativeInputActionByTag(const FGameplayTag& InInputTag) const
{
	for (const FGodsentInputActionConfig& Config : NativeInputActions)
	{
		if (Config.InputTag == InInputTag && Config.InputAction)
			return Config.InputAction;
	}

	return nullptr;
}
