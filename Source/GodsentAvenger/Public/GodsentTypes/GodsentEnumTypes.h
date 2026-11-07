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

/// What a projectile hit when it ended (for its impact effects)
UENUM(BlueprintType)
enum class EGodsentProjectileImpact : uint8
{
	World,		// Hit a wall or the floor
	Pawn,		// Hit and damaged a hostile pawn (including through a partial block)
	Blocked,	// Stopped by a full block or a parry
	Fizzled		// Ran out of time, or its caster died
};

/// Where the current heir's run stands (set by the game mode)
UENUM(BlueprintType)
enum class EGodsentRunState : uint8
{
	Starting,		// Level loaded, waiting out the start delay before the name prompt
	NamingHeir,		// Name prompt open (paused)
	ChoosingSkills,	// Rebirth skill tree open (paused)
	Playing,		// The heir is in control
	Lost,			// The heir died: lose screen up (or about to be)
	Won				// The level was won: win screen up (or about to be)
};

/// Console model the game was launched for (-trc_profile=base / enhanced). Base when no flag is given.
UENUM(BlueprintType)
enum class EGodsentQualityProfile : uint8
{
	Base,		// Standard model: 1080p, 60 fps cap, Medium scalability (defaults)
	Enhanced	// Upgraded model: 4K, 30 fps cap, High scalability (defaults)
};
