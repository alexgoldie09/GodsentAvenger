// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "BTTask_AldegonFlight.generated.h"

class UGodsentAldegonComponent;

/**
 * Tells Aldegon to Take Off, Fly To a spot or Land At a spot, and waits until he's done.
 *
 * Take Off and Land At change his state, so the branch they're in normally aborts straight away.
 * That's fine: aborting doesn't stop him, he carries on going up or down, and the tree picks
 * the matching branch once he gets there. Fly To keeps him Airborne, so it really waits; aborting it stops him where he is.
 */
UCLASS(meta = (DisplayName = "Aldegon Flight"))
class GODSENTAVENGER_API UBTTask_AldegonFlight : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AldegonFlight();

	//~ Begin UBTTaskNode Interface
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTTaskNode Interface

protected:
	UPROPERTY(EditAnywhere, Category = "Flight")
	EGodsentAldegonFlightOrder Order = EGodsentAldegonFlightOrder::LandAt;

	/// Where to fly or land (a vector, e.g. GroundingLocation, or an actor). Not used for Take Off.
	UPROPERTY(EditAnywhere, Category = "Flight", meta = (EditCondition = "Order != EGodsentAldegonFlightOrder::TakeOff"))
	FBlackboardKeySelector InDestinationKey;

	/// Turn to face this actor while flying (normally TargetActor). None = face where he's going.
	UPROPERTY(EditAnywhere, Category = "Flight")
	FBlackboardKeySelector InFaceWhileFlyingKey;

	/// Logs how each order ended (and why it failed or was cut off) to the Output Log
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bLogResult = true;

private:
	bool GetDestination(const UBehaviorTreeComponent& OwnerComp, FVector& OutDestination) const;

	/// "[Aldegon BT] Fly To FlyLocation: <what happened>"
	void LogResult(const FString& What) const;

	/// Finishes with a log line saying why
	void FinishWithLog(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result, const FString& Why);

	TWeakObjectPtr<UGodsentAldegonComponent> CachedAldegon;
	bool bSetFacingTarget = false;
};