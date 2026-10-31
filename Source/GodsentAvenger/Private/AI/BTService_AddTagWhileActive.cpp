// Alex Goldberg rights reserved

#include "AI/BTService_AddTagWhileActive.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	UAbilitySystemComponent* GetOwnerPawnASC(const UBehaviorTreeComponent& OwnerComp)
	{
		const AAIController* AIController = OwnerComp.GetAIOwner();
		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AIController ? AIController->GetPawn() : nullptr);
	}
}

UBTService_AddTagWhileActive::UBTService_AddTagWhileActive()
{
	NodeName = TEXT("Native Add Tag While Active");

	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	// Nothing to do per tick; only entering and leaving the branch matter
	bNotifyTick = false;
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
}

uint16 UBTService_AddTagWhileActive::GetInstanceMemorySize() const
{
	return sizeof(FAddTagWhileActiveMemory);
}

void UBTService_AddTagWhileActive::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	if (InitType == EBTMemoryInit::Initialize)
		CastInstanceNodeMemory<FAddTagWhileActiveMemory>(NodeMemory)->bTagAdded = false;
}

FString UBTService_AddTagWhileActive::GetStaticDescription() const
{
	return FString::Printf(TEXT("Pawn has %s while this branch runs"), *TagToAdd.ToString());
}

void UBTService_AddTagWhileActive::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	FAddTagWhileActiveMemory* Memory = CastInstanceNodeMemory<FAddTagWhileActiveMemory>(NodeMemory);
	UAbilitySystemComponent* ASC = GetOwnerPawnASC(OwnerComp);

	if (!ASC || !TagToAdd.IsValid() || Memory->bTagAdded)
		return;

	ASC->AddLooseGameplayTag(TagToAdd);
	Memory->bTagAdded = true;
}

void UBTService_AddTagWhileActive::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FAddTagWhileActiveMemory* Memory = CastInstanceNodeMemory<FAddTagWhileActiveMemory>(NodeMemory);

	// Only remove what this node added, so another source of the same tag isn't stripped
	if (Memory->bTagAdded)
	{
		if (UAbilitySystemComponent* ASC = GetOwnerPawnASC(OwnerComp))
			ASC->RemoveLooseGameplayTag(TagToAdd);

		Memory->bTagAdded = false;
	}

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}
