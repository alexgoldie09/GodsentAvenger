// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BTService.h"
#include "BTService_AddTagWhileActive.generated.h"

struct FAddTagWhileActiveMemory
{
	bool bTagAdded = false;
};

/**
 * Gives the pawn a loose gameplay tag while its branch is running, and always removes it when the branch ends
 * (finished, failed or aborted). Pair it with a tag-gated Infinite effect for a temporary stat change,
 * e.g. Enemy.Status.Charging + GE_Zombie_ChargeSpeed = the zombie runs while it charges in to slam.
 */
UCLASS()
class GODSENTAVENGER_API UBTService_AddTagWhileActive : public UBTService
{
	GENERATED_BODY()

	UBTService_AddTagWhileActive();

	//~ Begin UBTNode Interface
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTNode Interface

	//~ Begin UBTAuxiliaryNode Interface
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	//~ End UBTAuxiliaryNode Interface

	/// Tag the pawn has while this branch runs
	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag TagToAdd;
};
