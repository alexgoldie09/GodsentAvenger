// Alex Goldberg rights reserved

#include "Widgets/GodsentWidgetBase.h"
#include "Components/UI/EnemyUIComponent.h"
#include "Components/UI/HeroUIComponent.h"
#include "Interfaces/PawnUIInterface.h"

/// Finds the owning player's Hero UI Component and hands it to Blueprint
void UGodsentWidgetBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IPawnUIInterface* PawnUIInterface = Cast<IPawnUIInterface>(GetOwningPlayerPawn()))
	{
		if (UHeroUIComponent* HeroUIComponent = PawnUIInterface->GetHeroUIComponent())
			BP_OnOwningHeroUIComponentInitialized(HeroUIComponent);
	}
}

/// Hooks an enemy widget up to its enemy
void UGodsentWidgetBase::InitEnemyCreatedWidget(AActor* OwningEnemyActor)
{
	if (IPawnUIInterface* PawnUIInterface = Cast<IPawnUIInterface>(OwningEnemyActor))
	{
		UEnemyUIComponent* EnemyUIComponent = PawnUIInterface->GetEnemyUIComponent();
		checkf(EnemyUIComponent, TEXT("Enemy UI Component is null for Owning Enemy Actor: %s"), *OwningEnemyActor->GetActorNameOrLabel());
		BP_OnOwningEnemyUIComponentInitialized(EnemyUIComponent);
	}
}
