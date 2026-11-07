// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RotateToFaceTarget.generated.h"

struct FRotateToFaceTargetTaskMemory
{
	TWeakObjectPtr<APawn> OwningPawn;

	/// Set when the key is an actor (followed while it moves)
	TWeakObjectPtr<AActor> TargetActor;

	/// Used when the key is a vector (a point to face) or a rotator (a direction to match)
	FVector TargetLocation = FVector::ZeroVector;
	float TargetYaw = 0.f;
	bool bHasFixedYaw = false;
	bool bHasTargetLocation = false;

	bool IsValid() const
	{
		return OwningPawn.IsValid() && (TargetActor.IsValid() || bHasTargetLocation || bHasFixedYaw);
	}

	void Reset()
	{
		OwningPawn.Reset();
		TargetActor.Reset();
		bHasFixedYaw = false;
		bHasTargetLocation = false;
	}
};

/**
 * Turns the pawn towards the target until it is within Angle Precision, then succeeds.
 * Ported from the tutorial; only turns on yaw, so the pawn never tilts when the target is above or below it.
 * The key can be an actor (faces it, following it as it moves), a vector (faces that point) or a rotator
 * (turns to that direction, e.g. HomeRotation so a guard faces the way it was placed).
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

	/// The yaw the pawn should end up at (false if there's nothing to face, e.g. standing on the target)
	static bool GetDesiredYaw(const APawn* QueryPawn, const FRotateToFaceTargetTaskMemory& Memory, float& OutYaw);

	/// True once the pawn faces the desired yaw within Angle Precision (ignores height)
	bool HasReachedAnglePrecision(const APawn* QueryPawn, const FRotateToFaceTargetTaskMemory& Memory) const;

	/// How close (in degrees) the pawn must face the target to finish
	UPROPERTY(EditAnywhere, Category = "Face Target", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float AnglePrecision;

	/// Turn speed (higher is snappier)
	UPROPERTY(EditAnywhere, Category = "Face Target", meta = (ClampMin = "0.0"))
	float RotationInterpSpeed;

	/// What to face: an actor (normally TargetActor), a vector (a location) or a rotator (a direction, e.g. HomeRotation)
	UPROPERTY(EditAnywhere, Category = "Face Target")
	FBlackboardKeySelector InTargetToFaceKey;
};
