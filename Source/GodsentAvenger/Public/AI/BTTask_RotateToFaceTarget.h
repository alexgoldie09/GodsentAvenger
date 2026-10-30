// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RotateToFaceTarget.generated.h"

struct FRotateToFaceTargetTaskMemory
{
	TWeakObjectPtr<APawn> OwningPawn;
	TWeakObjectPtr<AActor> TargetActor;

	bool IsValid() const
	{
		return OwningPawn.IsValid() && TargetActor.IsValid();
	}

	void Reset()
	{
		OwningPawn.Reset();
		TargetActor.Reset();
	}
};

/**
 * Turns the pawn towards the target until it is within Angle Precision, then succeeds.
 * Ported from the tutorial; only turns on yaw, so the pawn never tilts when the target is above or below it.
 */
UCLASS()
class GODSENTAVENGER_API UBTTask_RotateToFaceTarget : public UBTTaskNode
{
	GENERATED_BODY()

	UBTTask_RotateToFaceTarget();

	//~ Begin UBTTaskNode Interface
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	//~ End UBTTaskNode Interface

	/// True once the pawn faces the target within Angle Precision (ignores height)
	bool HasReachedAnglePrecision(const APawn* QueryPawn, const AActor* TargetActor) const;

	/// How close (in degrees) the pawn must face the target to finish
	UPROPERTY(EditAnywhere, Category = "Face Target", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float AnglePrecision;

	/// Turn speed (higher is snappier)
	UPROPERTY(EditAnywhere, Category = "Face Target", meta = (ClampMin = "0.0"))
	float RotationInterpSpeed;

	/// The actor to face (normally TargetActor)
	UPROPERTY(EditAnywhere, Category = "Face Target")
	FBlackboardKeySelector InTargetToFaceKey;
};
