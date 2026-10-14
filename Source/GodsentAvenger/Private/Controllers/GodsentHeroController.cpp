// Alex Goldberg rights reserved

#include "Controllers/GodsentHeroController.h"

AGodsentHeroController::AGodsentHeroController()
{
	HeroTeamId = FGenericTeamId(0);
}

FGenericTeamId AGodsentHeroController::GetGenericTeamId() const
{
	return HeroTeamId;
}
