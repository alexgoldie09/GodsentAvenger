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

		// Slate: the save popup puts focus back on the menu button that had it
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
