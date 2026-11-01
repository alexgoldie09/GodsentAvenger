// Alex Goldberg rights reserved

#include "AI/BTDecorator_CanSeeTarget.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Controllers/GodsentAIController.h"

UBTDecorator_CanSeeTarget::UBTDecorator_CanSeeTarget()
{
	NodeName = TEXT("Native Can See Target");

	// Sight changes aren't observed here, so this decorator only gates entry
	bAllowAbortNone = true;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
	FlowAbortMode = EBTFlowAbortMode::None;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
}

void UBTDecorator_CanSeeTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
		TargetActorKey.ResolveSelectedKey(*BBAsset);
}

FString UBTDecorator_CanSeeTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Can see %s"), *TargetActorKey.SelectedKeyName.ToString());
}

bool UBTDecorator_CanSeeTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AGodsentAIController* AIController = Cast<AGodsentAIController>(OwnerComp.GetAIOwner());
	const UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();

	if (!AIController || !BlackboardComponent)
		return false;

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetActorKey.SelectedKeyName));

	return AIController->IsActorCurrentlySeen(TargetActor);
}
