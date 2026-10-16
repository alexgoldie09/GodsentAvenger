// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/PlayerController.h"
#include "GodsentHeroController.generated.h"

/**
 * The player's controller. Team 0, so enemies (team 1) treat the hero as hostile.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentHeroController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AGodsentHeroController();

	//~ Begin IGenericTeamAgentInterface Interface.
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~ End IGenericTeamAgentInterface Interface.

	/// Console command for testing skill tiers until the skill tree exists, e.g.
	/// "GodsentSetAbilityLevel Player.Ability.Dodge 3" (Graced Evasion) or "GodsentSetAbilityLevel Player.Ability.Attack.Light 2" (Combo)
	UFUNCTION(Exec)
	void GodsentSetAbilityLevel(FName AbilityTagName, int32 NewLevel);

private:
	FGenericTeamId HeroTeamId;
};
