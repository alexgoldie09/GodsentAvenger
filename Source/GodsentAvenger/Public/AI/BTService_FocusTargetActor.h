// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_FocusTargetActor.generated.h"

/**
 * While its branch runs, the AI looks at the target with Gameplay focus priority, which beats the path-following
 * "look where you're going" focus. With the pawn's Use Controller Desired Rotation on (and Orient Rotation To Movement off)
 * the enemy keeps facing the player while it moves: strafing and backing off.
 * Clears the focus when the branch ends, so patrol goes back to facing the path.
 */
UCLASS()
class GODSENTAVENGER_API UBTService_FocusTargetActor : public UBTService
{
	GENERATED_BODY()

	UBTService_FocusTargetActor();

	//~ Begin UBTNode Interface
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTNode Interface

	//~ Begin UBTAuxiliaryNode Interface
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	//~ End UBTAuxiliaryNode Interface

	/// What to look at (normally TargetActor)
	UPROPERTY(EditAnywhere, Category = "Focus")
	FBlackboardKeySelector InTargetActorKey;

	void UpdateFocus(UBehaviorTreeComponent& OwnerComp) const;
};
