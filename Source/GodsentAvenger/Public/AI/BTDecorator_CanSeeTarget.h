// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CanSeeTarget.generated.h"

/**
 * Passes only while the enemy's sight currently has the target (in its cone, in range, nothing in between).
 * Put it on ranged attacks so they aren't fired into walls. Checked when the branch is tried (no aborts).
 */
UCLASS()
class GODSENTAVENGER_API UBTDecorator_CanSeeTarget : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CanSeeTarget();

protected:
	//~ Begin UBTNode Interface
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTNode Interface

	//~ Begin UBTDecorator Interface
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	//~ End UBTDecorator Interface

	/// The target (normally TargetActor)
	UPROPERTY(EditAnywhere, Category = "Target")
	FBlackboardKeySelector TargetActorKey;
};
