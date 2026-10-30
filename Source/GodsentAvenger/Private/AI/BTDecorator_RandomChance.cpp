// Alex Goldberg rights reserved

#include "AI/BTDecorator_RandomChance.h"

UBTDecorator_RandomChance::UBTDecorator_RandomChance()
{
	NodeName = TEXT("Native Random Chance");

	// A dice roll can't be "observed" changing, so aborting makes no sense here
	bAllowAbortNone = true;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
	FlowAbortMode = EBTFlowAbortMode::None;
}

bool UBTDecorator_RandomChance::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	return FMath::FRand() < Chance;
}

FString UBTDecorator_RandomChance::GetStaticDescription() const
{
	return FString::Printf(TEXT("%d%% chance"), FMath::RoundToInt(Chance * 100.f));
}
