// Alex Goldberg rights reserved

#include "AI/BTDecorator_AldegonCanSummon.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/Aldegon/GodsentAldegonComponent.h"
#include "GameFramework/Pawn.h"

UBTDecorator_AldegonCanSummon::UBTDecorator_AldegonCanSummon()
{
	NodeName = TEXT("Aldegon Can Summon");

	// It's a check made when the branch is picked, not something to watch
	bAllowAbortNone = true;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

FString UBTDecorator_AldegonCanSummon::GetStaticDescription() const
{
	FString Description = FString::Printf(TEXT("%s under cap"), *UEnum::GetDisplayValueAsText(SummonType).ToString());

	if (AbilityTag.IsValid())
		Description += FString::Printf(TEXT(", %s ready"), *AbilityTag.ToString());

	return Description;
}

bool UBTDecorator_AldegonCanSummon::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	const UGodsentAldegonComponent* Aldegon = Pawn ? Pawn->FindComponentByClass<UGodsentAldegonComponent>() : nullptr;

	if (!Aldegon)
		return false;

	return Aldegon->CanSummonNow(SummonType, AbilityTag);
}
