// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_TrackTargetActor.generated.h"

/**
 * Put on the combat branch. Each tick:
 * - writes the distance to the target (replaces the Blueprint BTService_DistToTarget if you like),
 * - drops the target when it dies or is destroyed, or when the pawn has been dragged too far from home (leash).
 * Dropping the target makes a "TargetActor Is Set" decorator abort the combat branch, so the enemy goes back to patrolling.
 */
UCLASS()
class GODSENTAVENGER_API UBTService_TrackTargetActor : public UBTService
{
	GENERATED_BODY()

	UBTService_TrackTargetActor();

	//~ Begin UBTNode Interface
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTNode Interface

	//~ Begin UBTService Interface
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	//~ End UBTService Interface

	/// The target (normally TargetActor)
	UPROPERTY(EditAnywhere, Category = "Target")
	FBlackboardKeySelector InTargetActorKey;

	/// Float key that receives the distance to the target (normally DistToTarget). Leave as None to skip.
	UPROPERTY(EditAnywhere, Category = "Target")
	FBlackboardKeySelector OutDistToTargetKey;

	/// Measure the distance on the ground only (ignores height differences such as stairs)
	UPROPERTY(EditAnywhere, Category = "Target")
	bool bUse2DDistance = true;

	/// Give up the chase once the pawn is this far from its home location. 0 = never (only losing sight drops the target).
	UPROPERTY(EditAnywhere, Category = "Target", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxChaseDistanceFromHome = 0.f;
};
