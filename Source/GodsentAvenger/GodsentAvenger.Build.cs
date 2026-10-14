// Alex Goldberg rights reserved

using UnrealBuildTool;

public class GodsentAvenger : ModuleRules
{
	public GodsentAvenger(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"GameplayAbilities",
			"GameplayTasks",
			"AIModule",
			"AnimGraphRuntime",
			"MotionWarping",
			"Niagara",
			"NavigationSystem",
			"UMG"
		});
	}
}
