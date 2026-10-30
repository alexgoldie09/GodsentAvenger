// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_OrientToTargetActor.generated.h"

/**
 * While its branch runs, keeps turning the pawn towards the target (e.g. during the pause between attacks).
 * Ported from the tutorial; only turns on yaw.
 */
UCLASS()
class GODSENTAVENGER_API UBTService_OrientToTargetActor : public UBTService
{
	GENERATED_BODY()

	UBTService_OrientToTargetActor();

	//~ Begin UBTNode Interface
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTNode Interface

	//~ Begin UBTService Interface
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	//~ End UBTService Interface

	/// The actor to face (normally TargetActor)
	UPROPERTY(EditAnywhere, Category = "Target")
	FBlackboardKeySelector InTargetActorKey;

	/// Turn speed (higher is snappier)
	UPROPERTY(EditAnywhere, Category = "Target", meta = (ClampMin = "0.0"))
	float RotationInterpSpeed;
};
