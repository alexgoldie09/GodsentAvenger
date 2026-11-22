// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_AldegonStateCheck.generated.h"

class UBlackboardComponent;
class UGodsentAldegonComponent;

/**
 * Aldegon's switching rules, all in one place (put on BT_Aldegon's root). Each tick it writes to the blackboard:
 *
 * - Aldegon State: Dormant, Waking, Grounded, Airborne... (the tree's branches gate on this)
 * - Should Ground (airborne): the player is within Ground Trigger distance (after Min Airborne Time),
 *   or enough health bands were crossed while airborne
 * - Should Take Off (grounded): a health band was crossed, or Max Consecutive Claws reached,
 *   or all his adds and totems are dead, the player is beyond Resummon Distance and a summon is ready
 * - In Claw Range, Is Frenzied, living adds and totems, claws in a row (for decorators and debugging)
 *
 * Every number comes from the Godsent Aldegon component (CT_AldegonStats or its settings), none from here.
 */
UCLASS(meta = (DisplayName = "Aldegon State Check"))
class GODSENTAVENGER_API UBTService_AldegonStateCheck : public UBTService
{
	GENERATED_BODY()

	UBTService_AldegonStateCheck();

	//~ Begin UBTNode Interface
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTNode Interface

	//~ Begin UBTService Interface
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	//~ End UBTService Interface

	/// The player (normally TargetActor)
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector InTargetActorKey;

	/// Enum key (EGodsentAldegonState), normally AldegonState
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector OutAldegonStateKey;

	/// Bool, normally ShouldGround
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector OutShouldGroundKey;

	/// Bool, normally ShouldTakeOff
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector OutShouldTakeOffKey;

	/// Bool, normally InClawRange. None to skip.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector OutInClawRangeKey;

	/// Bool, normally IsFrenzied. None to skip.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector OutIsFrenziedKey;

	/// Bool, normally IsKnockedDown. None to skip.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector OutIsKnockedDownKey;

	/// Int, normally LivingAdds. None to skip.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector OutLivingAddsKey;

	/// Int, normally LivingTotems. None to skip.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector OutLivingTotemsKey;

	/// Int, normally ConsecutiveClaws. None to skip.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector OutConsecutiveClawsKey;

private:
	static void SetBoolIfSet(UBlackboardComponent& Blackboard, const FBlackboardKeySelector& Key, bool bValue);
	static void SetIntIfSet(UBlackboardComponent& Blackboard, const FBlackboardKeySelector& Key, int32 Value);
};
