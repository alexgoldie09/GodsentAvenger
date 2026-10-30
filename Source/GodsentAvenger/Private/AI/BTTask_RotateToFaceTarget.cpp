// Alex Goldberg rights reserved

#include "AI/BTTask_RotateToFaceTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"

UBTTask_RotateToFaceTarget::UBTTask_RotateToFaceTarget()
{
	NodeName = TEXT("Native Rotate To Face Target");
	AnglePrecision = 10.f;
	RotationInterpSpeed = 5.f;

	INIT_TASK_NODE_NOTIFY_FLAGS();

	bNotifyTick = true;
	bNotifyTaskFinished = true;
	bCreateNodeInstance = false;

	InTargetToFaceKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InTargetToFaceKey), AActor::StaticClass());
}

void UBTTask_RotateToFaceTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
		InTargetToFaceKey.ResolveSelectedKey(*BBAsset);
}

uint16 UBTTask_RotateToFaceTarget::GetInstanceMemorySize() const
{
	return sizeof(FRotateToFaceTargetTaskMemory);
}

FString UBTTask_RotateToFaceTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Turns to face %s until within %s degrees"),
		*InTargetToFaceKey.SelectedKeyName.ToString(), *FString::SanitizeFloat(AnglePrecision));
}

EBTNodeResult::Type UBTTask_RotateToFaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(InTargetToFaceKey.SelectedKeyName));
	APawn* OwningPawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;

	FRotateToFaceTargetTaskMemory* Memory = CastInstanceNodeMemory<FRotateToFaceTargetTaskMemory>(NodeMemory);
	check(Memory);

	Memory->OwningPawn = OwningPawn;
	Memory->TargetActor = TargetActor;

	if (!Memory->IsValid())
		return EBTNodeResult::Failed;

	if (HasReachedAnglePrecision(OwningPawn, TargetActor))
	{
		Memory->Reset();
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_RotateToFaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FRotateToFaceTargetTaskMemory* Memory = CastInstanceNodeMemory<FRotateToFaceTargetTaskMemory>(NodeMemory);

	if (!Memory->IsValid())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	APawn* OwningPawn = Memory->OwningPawn.Get();
	const AActor* TargetActor = Memory->TargetActor.Get();

	if (HasReachedAnglePrecision(OwningPawn, TargetActor))
	{
		Memory->Reset();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Yaw only: the tutorial version also pitched the pawn when the target stood higher or lower
	const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(OwningPawn->GetActorLocation(), TargetActor->GetActorLocation());
	const FRotator CurrentRot = OwningPawn->GetActorRotation();
	const FRotator TargetRot = FMath::RInterpTo(CurrentRot, FRotator(CurrentRot.Pitch, LookAtRot.Yaw, CurrentRot.Roll), DeltaSeconds, RotationInterpSpeed);

	OwningPawn->SetActorRotation(TargetRot);
}

/// True once the pawn faces the target within Angle Precision (ignores height)
bool UBTTask_RotateToFaceTarget::HasReachedAnglePrecision(const APawn* QueryPawn, const AActor* TargetActor) const
{
	const FVector OwnerForward = QueryPawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector OwnerToTargetNorm = (TargetActor->GetActorLocation() - QueryPawn->GetActorLocation()).GetSafeNormal2D();

	// Standing right on top of the target: no meaningful direction, treat it as facing
	if (OwnerToTargetNorm.IsNearlyZero())
		return true;

	const float DotResult = FMath::Clamp(FVector::DotProduct(OwnerForward, OwnerToTargetNorm), -1.f, 1.f);
	const float AngleDiff = UKismetMathLibrary::DegAcos(DotResult);

	return AngleDiff <= AnglePrecision;
}
