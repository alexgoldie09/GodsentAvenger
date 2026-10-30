// Alex Goldberg rights reserved

#include "AI/BTService_OrientToTargetActor.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"

UBTService_OrientToTargetActor::UBTService_OrientToTargetActor()
{
	NodeName = TEXT("Native Orient Rotation To Target Actor");

	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	RotationInterpSpeed = 5.f;
	Interval = 0.f;
	RandomDeviation = 0.f;
	bNotifyTick = true;

	InTargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InTargetActorKey), AActor::StaticClass());
}

void UBTService_OrientToTargetActor::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
		InTargetActorKey.ResolveSelectedKey(*BBAsset);
}

FString UBTService_OrientToTargetActor::GetStaticDescription() const
{
	return FString::Printf(TEXT("Keeps turning towards %s\n%s"),
		*InTargetActorKey.SelectedKeyName.ToString(), *GetStaticServiceDescription());
}

void UBTService_OrientToTargetActor::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	const AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(InTargetActorKey.SelectedKeyName));
	APawn* OwningPawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;

	if (!OwningPawn || !TargetActor)
		return;

	// Yaw only, so the pawn never tilts towards a target that is higher or lower
	const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(OwningPawn->GetActorLocation(), TargetActor->GetActorLocation());
	const FRotator CurrentRot = OwningPawn->GetActorRotation();
	const FRotator TargetRot = FMath::RInterpTo(CurrentRot, FRotator(CurrentRot.Pitch, LookAtRot.Yaw, CurrentRot.Roll), DeltaSeconds, RotationInterpSpeed);

	OwningPawn->SetActorRotation(TargetRot);
}
