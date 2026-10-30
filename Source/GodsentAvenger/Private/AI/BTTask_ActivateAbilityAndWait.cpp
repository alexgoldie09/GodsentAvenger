// Alex Goldberg rights reserved

#include "AI/BTTask_ActivateAbilityAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"

UBTTask_ActivateAbilityAndWait::UBTTask_ActivateAbilityAndWait()
{
	NodeName = TEXT("Native Activate Ability And Wait");

	INIT_TASK_NODE_NOTIFY_FLAGS();

	// One instance per AI: it holds a delegate binding and the running ability's handle
	bCreateNodeInstance = true;
	bNotifyTick = false;
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTTask_ActivateAbilityAndWait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	bIsAborting = false;
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();

	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);

	if (!ASC || !AbilityTagToActivate.IsValid())
		return EBTNodeResult::Failed;

	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTagToActivate.GetSingleTagContainer(), MatchingSpecs);

	// Random order, then try each until one activates (a picked one may be on cooldown)
	TArray<FGameplayAbilitySpecHandle> CandidateHandles;

	for (const FGameplayAbilitySpec* Spec : MatchingSpecs)
	{
		if (Spec && !Spec->IsActive())
			CandidateHandles.Add(Spec->Handle);
	}

	for (int32 Index = CandidateHandles.Num() - 1; Index > 0; --Index)
		CandidateHandles.Swap(Index, FMath::RandRange(0, Index));

	for (const FGameplayAbilitySpecHandle& Handle : CandidateHandles)
	{
		if (!ASC->TryActivateAbility(Handle))
			continue;

		const FGameplayAbilitySpec* ActivatedSpec = ASC->FindAbilitySpecFromHandle(Handle);

		// Ended during activation (e.g. failed to commit), or the tree doesn't need to wait
		if (!bWaitForAbilityToEnd || !ActivatedSpec || !ActivatedSpec->IsActive())
			return EBTNodeResult::Succeeded;

		CachedOwnerComp = &OwnerComp;
		CachedASC = ASC;
		ActiveAbilityHandle = Handle;
		AbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnAbilityEnded);

		return EBTNodeResult::InProgress;
	}

	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_ActivateAbilityAndWait::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(ActiveAbilityHandle) : nullptr;

	if (!Spec || !Spec->IsActive())
		return EBTNodeResult::Aborted;

	if (bCancelAbilityOnAbort)
	{
		// Unbind first so the cancel's end callback doesn't try to finish the task as well
		UnbindAbilityEnded();
		ASC->CancelAbilityHandle(ActiveAbilityHandle);
		return EBTNodeResult::Aborted;
	}

	// Let the attack play out; OnAbilityEnded finishes the abort
	bIsAborting = true;
	return EBTNodeResult::InProgress;
}

void UBTTask_ActivateAbilityAndWait::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	UnbindAbilityEnded();
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
	CachedOwnerComp.Reset();
	bIsAborting = false;

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

FString UBTTask_ActivateAbilityAndWait::GetStaticDescription() const
{
	return FString::Printf(TEXT("Activates %s%s"),
		*AbilityTagToActivate.ToString(),
		bWaitForAbilityToEnd ? TEXT(" and waits for it to end") : TEXT(""));
}

/// Finishes the task (or the pending abort) when the attack ends, whether it completed or was cancelled
void UBTTask_ActivateAbilityAndWait::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (AbilityEndedData.AbilitySpecHandle != ActiveAbilityHandle)
		return;

	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	const bool bWasAborting = bIsAborting;

	UnbindAbilityEnded();

	if (!OwnerComp)
		return;

	if (bWasAborting)
		FinishLatentAbort(*OwnerComp);
	else
		// Succeeded even when cancelled (e.g. staggered), so the tree moves on to the pause instead of trying another attack
		FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
}

void UBTTask_ActivateAbilityAndWait::UnbindAbilityEnded()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
		ASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);

	AbilityEndedDelegateHandle.Reset();
	CachedASC.Reset();
}
