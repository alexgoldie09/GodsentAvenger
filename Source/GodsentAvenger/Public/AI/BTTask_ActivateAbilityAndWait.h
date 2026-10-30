// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ActivateAbilityAndWait.generated.h"

class UAbilitySystemComponent;
struct FAbilityEndedData;

/**
 * Activates a random granted ability matching the tag and stays running until that ability ends.
 * Unlike BTTask_ActivateAbilityByTag (Blueprint), the tree can't move or turn the pawn mid-attack,
 * and a Wait placed after it counts from the end of the swing, not the start.
 *
 * Fails if no matching ability could activate (none granted, on cooldown, blocked by a tag such as HitReact or Dead),
 * so a Selector can fall through to another attack.
 */
UCLASS()
class GODSENTAVENGER_API UBTTask_ActivateAbilityAndWait : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ActivateAbilityAndWait();

	//~ Begin UBTTaskNode Interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTTaskNode Interface

protected:
	/// Ability tag to activate. A parent tag matches every child (Enemy.Ability.Melee picks Light or Heavy at random).
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Enemy.Ability"))
	FGameplayTag AbilityTagToActivate;

	/// Off: succeed as soon as the ability starts (same as the Blueprint task)
	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bWaitForAbilityToEnd = true;

	/// When the branch is aborted (e.g. the target was lost): on = cancel the attack straight away, off = let it finish first
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (EditCondition = "bWaitForAbilityToEnd"))
	bool bCancelAbilityOnAbort = false;

private:
	void OnAbilityEnded(const FAbilityEndedData& AbilityEndedData);

	void UnbindAbilityEnded();

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FGameplayAbilitySpecHandle ActiveAbilityHandle;
	FDelegateHandle AbilityEndedDelegateHandle;
	bool bIsAborting = false;
};
