// Alex Goldberg rights reserved

#pragma once

UENUM()
enum class EGodsentConfirmType : uint8
{
	Yes,
	No
};

UENUM()
enum class EGodsentValidType : uint8
{
	Valid,
	Invalid
};

UENUM()
enum class EGodsentSuccessType : uint8
{
	Successful,
	Failed
};

UENUM()
enum class EGodsentCountDownActionInput : uint8
{
	Start,
	Cancel
};

UENUM()
enum class EGodsentCountDownActionOutput : uint8
{
	Updated,
	Completed,
	Cancelled
};

/// Outcome of a hit landing on a (possibly) blocking defender
enum class EGodsentBlockResult : uint8
{
	None, // Not blocked: full damage
	Blocked, // Partial block: damage still applies, reduced by the block's DefensePower bonus
	FullBlocked, // Full block (Increased Defence skill): no damage
	Parried	// Hit landed inside the parry window: no damage, attacker staggers
};

UENUM(BlueprintType)
enum class EGodsentInputMode : uint8
{
	GameOnly,
	UIOnly
};

/// Where a skill stands for the current heir (drives the skill tree buttons)
UENUM(BlueprintType)
enum class EGodsentSkillState : uint8
{
	Unlocked,		// Already owned
	Available,		// Prerequisites owned and enough skill points: can be bought
	CantAfford,		// Prerequisites owned but not enough skill points
	Locked,			// A prerequisite (the previous tier) isn't owned yet
	Unavailable		// Not in the skill tree data, or not built yet (grants nothing)
};
