// Alex Goldberg rights reserved

#include "AI/BTTask_AldegonFlight.h"
#include "AIController.h"
#include "AISystem.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Components/Aldegon/GodsentAldegonComponent.h"
#include "Components/Flight/GodsentFlightComponent.h"
#include "GameFramework/Pawn.h"

UBTTask_AldegonFlight::UBTTask_AldegonFlight()
{
	NodeName = TEXT("Aldegon Flight");

	INIT_TASK_NODE_NOTIFY_FLAGS();

	// One per AI: it remembers whether it set his facing target
	bCreateNodeInstance = true;
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	InDestinationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InDestinationKey));
	InDestinationKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InDestinationKey), AActor::StaticClass());
	InDestinationKey.AllowNoneAsValue(true);

	InFaceWhileFlyingKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InFaceWhileFlyingKey), AActor::StaticClass());
	InFaceWhileFlyingKey.AllowNoneAsValue(true);
}

void UBTTask_AldegonFlight::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		InDestinationKey.ResolveSelectedKey(*BBAsset);
		InFaceWhileFlyingKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTTask_AldegonFlight::GetStaticDescription() const
{
	FString Description = UEnum::GetDisplayValueAsText(Order).ToString();

	if (Order != EGodsentAldegonFlightOrder::TakeOff)
		Description += FString::Printf(TEXT(" %s"), *InDestinationKey.SelectedKeyName.ToString());

	if (InFaceWhileFlyingKey.SelectedKeyName != NAME_None)
		Description += FString::Printf(TEXT("\nFacing %s"), *InFaceWhileFlyingKey.SelectedKeyName.ToString());

	return Description;
}

void UBTTask_AldegonFlight::LogResult(const FString& What) const
{
	if (!bLogResult)
		return;

	FString OrderText = UEnum::GetDisplayValueAsText(Order).ToString();

	if (Order != EGodsentAldegonFlightOrder::TakeOff)
		OrderText += FString::Printf(TEXT(" %s"), *InDestinationKey.SelectedKeyName.ToString());

	UE_LOG(LogTemp, Log, TEXT("[Aldegon BT] %s: %s"), *OrderText, *What);
}

void UBTTask_AldegonFlight::FinishWithLog(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result, const FString& Why)
{
	LogResult(FString::Printf(TEXT("%s (%s)"), Result == EBTNodeResult::Succeeded ? TEXT("succeeded") : TEXT("FAILED"), *Why));
	FinishLatentTask(OwnerComp, Result);
}

bool UBTTask_AldegonFlight::GetDestination(const UBehaviorTreeComponent& OwnerComp, FVector& OutDestination) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!Blackboard || InDestinationKey.SelectedKeyName == NAME_None)
		return false;

	// Read the key's type from the blackboard itself (this node is copied per AI, and the copy doesn't keep the resolved type)
	const FBlackboard::FKey KeyID = Blackboard->GetKeyID(InDestinationKey.SelectedKeyName);

	if (Blackboard->GetKeyType(KeyID) == UBlackboardKeyType_Object::StaticClass())
	{
		const AActor* DestinationActor = Cast<AActor>(Blackboard->GetValueAsObject(InDestinationKey.SelectedKeyName));

		if (!DestinationActor)
			return false;

		OutDestination = DestinationActor->GetActorLocation();
		return true;
	}

	OutDestination = Blackboard->GetValueAsVector(InDestinationKey.SelectedKeyName);
	return FAISystem::IsValidLocation(OutDestination);
}

EBTNodeResult::Type UBTTask_AldegonFlight::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	bSetFacingTarget = false;

	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UGodsentAldegonComponent* Aldegon = Pawn ? Pawn->FindComponentByClass<UGodsentAldegonComponent>() : nullptr;

	CachedAldegon = Aldegon;

	if (!Aldegon)
	{
		LogResult(TEXT("FAILED (no Godsent Aldegon component on the pawn)"));
		return EBTNodeResult::Failed;
	}

	FVector Destination = FVector::ZeroVector;

	if (Order != EGodsentAldegonFlightOrder::TakeOff && !GetDestination(OwnerComp, Destination))
	{
		LogResult(TEXT("FAILED (destination key is empty or not a vector/actor)"));
		return EBTNodeResult::Failed;
	}

	bool bStarted = false;

	switch (Order)
	{
	case EGodsentAldegonFlightOrder::TakeOff:
		bStarted = Aldegon->TakeOff();
		break;

	case EGodsentAldegonFlightOrder::FlyTo:
		bStarted = Aldegon->FlyTo(Destination);
		break;

	case EGodsentAldegonFlightOrder::LandAt:
		bStarted = Aldegon->LandAt(Destination);
		break;
	}

	if (!bStarted)
	{
		LogResult(FString::Printf(TEXT("FAILED (refused while %s)"), *UEnum::GetDisplayValueAsText(Aldegon->GetAldegonState()).ToString()));
		return EBTNodeResult::Failed;
	}

	LogResult(TEXT("started"));

	// Facing after the order: reaching Airborne or Grounded clears it again for you
	if (const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		if (AActor* FaceActor = Cast<AActor>(Blackboard->GetValueAsObject(InFaceWhileFlyingKey.SelectedKeyName)))
		{
			Aldegon->SetFacingTarget(FaceActor);
			bSetFacingTarget = true;
		}
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_AldegonFlight::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	const UGodsentAldegonComponent* Aldegon = CachedAldegon.Get();

	if (!Aldegon)
	{
		FinishWithLog(OwnerComp, EBTNodeResult::Failed, TEXT("Aldegon is gone"));
		return;
	}

	const EGodsentAldegonState State = Aldegon->GetAldegonState();
	const FString StateText = UEnum::GetDisplayValueAsText(State).ToString();

	switch (Order)
	{
	case EGodsentAldegonFlightOrder::TakeOff:
		if (State == EGodsentAldegonState::Airborne)
			FinishWithLog(OwnerComp, EBTNodeResult::Succeeded, TEXT("reached Airborne"));
		else if (State != EGodsentAldegonState::TakingOff)
			FinishWithLog(OwnerComp, EBTNodeResult::Failed, FString::Printf(TEXT("state changed to %s"), *StateText));
		break;

	case EGodsentAldegonFlightOrder::LandAt:
		if (State == EGodsentAldegonState::Grounded)
			FinishWithLog(OwnerComp, EBTNodeResult::Succeeded, TEXT("landed"));
		else if (State != EGodsentAldegonState::Landing)
			FinishWithLog(OwnerComp, EBTNodeResult::Failed, FString::Printf(TEXT("state changed to %s"), *StateText));
		break;

	case EGodsentAldegonFlightOrder::FlyTo:
	{
		const UGodsentFlightComponent* Flight = Aldegon->GetOwner()->FindComponentByClass<UGodsentFlightComponent>();

		// Knocked down or landed some other way
		if (!Flight)
			FinishWithLog(OwnerComp, EBTNodeResult::Failed, TEXT("no Godsent Flight component"));
		else if (State != EGodsentAldegonState::Airborne)
			FinishWithLog(OwnerComp, EBTNodeResult::Failed, FString::Printf(TEXT("state changed to %s"), *StateText));
		else if (Flight->GetCurrentAction() != EGodsentFlightAction::FlyTo)
			FinishWithLog(OwnerComp, EBTNodeResult::Succeeded, TEXT("arrived"));
		break;
	}
	}
}

EBTNodeResult::Type UBTTask_AldegonFlight::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// Normal for Take Off / Land At (his state changed, so his branch stopped). For Fly To it means something cut in.
	LogResult(TEXT("aborted (the branch was interrupted, e.g. Come Down or a state change)"));

	// Take Off and Land At carry on (that's usually why we were aborted). Fly To stops where he is,
	// but only if it's still our flight: Return Home (player left the arena) gives him a new one we mustn't cancel.
	if (Order == EGodsentAldegonFlightOrder::FlyTo)
	{
		if (UGodsentAldegonComponent* Aldegon = CachedAldegon.Get())
		{
			UGodsentFlightComponent* Flight = Aldegon->GetOwner()->FindComponentByClass<UGodsentFlightComponent>();
			const bool bStillOurFlight = Flight && Aldegon->GetAldegonState() == EGodsentAldegonState::Airborne
				&& Flight->GetCurrentAction() == EGodsentFlightAction::FlyTo;

			if (bStillOurFlight)
				Flight->StopFlightAction();
		}
	}

	return EBTNodeResult::Aborted;
}

void UBTTask_AldegonFlight::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	// Fly To stays Airborne, so nothing else would clear the facing it set
	if (bSetFacingTarget && Order == EGodsentAldegonFlightOrder::FlyTo)
	{
		if (UGodsentAldegonComponent* Aldegon = CachedAldegon.Get())
			Aldegon->SetFacingTarget(nullptr);
	}

	bSetFacingTarget = false;
	CachedAldegon.Reset();

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
