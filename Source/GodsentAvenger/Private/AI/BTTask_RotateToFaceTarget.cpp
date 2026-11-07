// Alex Goldberg rights reserved

#include "AI/BTTask_RotateToFaceTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/Pawn.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Rotator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "AISystem.h"

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
	InTargetToFaceKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InTargetToFaceKey));
	InTargetToFaceKey.AddRotatorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InTargetToFaceKey));
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
	const UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	APawn* OwningPawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;

	FRotateToFaceTargetTaskMemory* Memory = CastInstanceNodeMemory<FRotateToFaceTargetTaskMemory>(NodeMemory);
	check(Memory);

	Memory->Reset();
	Memory->OwningPawn = OwningPawn;

	if (BlackboardComponent)
	{
		const TSubclassOf<UBlackboardKeyType> KeyType = InTargetToFaceKey.SelectedKeyType;

		if (KeyType == UBlackboardKeyType_Object::StaticClass())
		{
			Memory->TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(InTargetToFaceKey.SelectedKeyName));
		}
		else if (KeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			const FVector Location = BlackboardComponent->GetValueAsVector(InTargetToFaceKey.SelectedKeyName);
			Memory->bHasTargetLocation = FAISystem::IsValidLocation(Location);
			Memory->TargetLocation = Location;
		}
		else if (KeyType == UBlackboardKeyType_Rotator::StaticClass())
		{
			const FRotator Rotation = BlackboardComponent->GetValueAsRotator(InTargetToFaceKey.SelectedKeyName);
			Memory->bHasFixedYaw = FAISystem::IsValidRotation(Rotation);
			Memory->TargetYaw = Rotation.Yaw;
		}
	}

	if (!Memory->IsValid())
		return EBTNodeResult::Failed;

	if (HasReachedAnglePrecision(OwningPawn, *Memory))
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
	float DesiredYaw = 0.f;

	if (HasReachedAnglePrecision(OwningPawn, *Memory) || !GetDesiredYaw(OwningPawn, *Memory, DesiredYaw))
	{
		Memory->Reset();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Yaw only: the tutorial version also pitched the pawn when the target stood higher or lower
	const FRotator CurrentRot = OwningPawn->GetActorRotation();
	const FRotator TargetRot = FMath::RInterpTo(CurrentRot, FRotator(CurrentRot.Pitch, DesiredYaw, CurrentRot.Roll), DeltaSeconds, RotationInterpSpeed);

	OwningPawn->SetActorRotation(TargetRot);
}

/// The yaw the pawn should end up at (false if there's nothing to face, e.g. standing on the target)
bool UBTTask_RotateToFaceTarget::GetDesiredYaw(const APawn* QueryPawn, const FRotateToFaceTargetTaskMemory& Memory, float& OutYaw)
{
	if (Memory.bHasFixedYaw)
	{
		OutYaw = Memory.TargetYaw;
		return true;
	}

	const AActor* TargetActor = Memory.TargetActor.Get();
	const FVector TargetLocation = TargetActor ? TargetActor->GetActorLocation() : Memory.TargetLocation;
	const FVector ToTarget = (TargetLocation - QueryPawn->GetActorLocation()).GetSafeNormal2D();

	if (ToTarget.IsNearlyZero())
		return false;

	OutYaw = ToTarget.Rotation().Yaw;
	return true;
}

/// True once the pawn faces the desired yaw within Angle Precision (ignores height)
bool UBTTask_RotateToFaceTarget::HasReachedAnglePrecision(const APawn* QueryPawn, const FRotateToFaceTargetTaskMemory& Memory) const
{
	float DesiredYaw = 0.f;

	// Standing right on top of the target: no meaningful direction, treat it as facing
	if (!GetDesiredYaw(QueryPawn, Memory, DesiredYaw))
		return true;

	const float AngleDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(QueryPawn->GetActorRotation().Yaw, DesiredYaw));

	return AngleDiff <= AnglePrecision;
}
