// Alex Goldberg rights reserved

#include "AI/BTService_FocusTargetActor.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

UBTService_FocusTargetActor::UBTService_FocusTargetActor()
{
	NodeName = TEXT("Native Focus Target Actor");

	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	bNotifyTick = true;

	// Only re-checks the key (in case the target changes); the controller does the turning every frame
	Interval = 0.25f;
	RandomDeviation = 0.f;

	InTargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InTargetActorKey), AActor::StaticClass());
}

void UBTService_FocusTargetActor::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
		InTargetActorKey.ResolveSelectedKey(*BBAsset);
}

FString UBTService_FocusTargetActor::GetStaticDescription() const
{
	return FString::Printf(TEXT("Looks at %s while active"), *InTargetActorKey.SelectedKeyName.ToString());
}

void UBTService_FocusTargetActor::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
	UpdateFocus(OwnerComp);
}

void UBTService_FocusTargetActor::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UpdateFocus(OwnerComp);
}

void UBTService_FocusTargetActor::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
		AIController->ClearFocus(EAIFocusPriority::Gameplay);

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UBTService_FocusTargetActor::UpdateFocus(UBehaviorTreeComponent& OwnerComp) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	const UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();

	if (!AIController || !BlackboardComponent)
		return;

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(InTargetActorKey.SelectedKeyName));

	if (!TargetActor)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	if (AIController->GetFocusActorForPriority(EAIFocusPriority::Gameplay) != TargetActor)
		AIController->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
}
