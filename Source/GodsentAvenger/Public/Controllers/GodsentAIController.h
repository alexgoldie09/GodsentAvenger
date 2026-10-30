// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionTypes.h"
#include "GodsentAIController.generated.h"

class UAbilitySystemComponent;
class UAISenseConfig_Sight;
class UAIPerceptionComponent;
struct FGameplayEventData;

/**
 * Enemy controller: team 1, sight perception and detour crowd avoidance.
 *
 * Target handling (blackboard key TargetActor):
 * - set when a hostile pawn is first seen, or (Aggro On Damage) when one hurts this enemy;
 * - with Forget Lost Targets on, cleared once the target has been out of sight for Forget Target Delay,
 *   unless it is still within Close Range Memory Distance (so circling behind the enemy doesn't shake it off).
 *
 * Also remembers the pawn's home (where it was possessed) for patrolling, writes HomeLocation and PatrolRadius
 * into the blackboard when a tree starts, and stops the tree when the pawn dies.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGodsentAIController(const FObjectInitializer& ObjectInitializer);

	//~ Begin IGenericTeamAgentInterface Interface.
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~ End IGenericTeamAgentInterface Interface.

	//~ Begin AAIController Interface.
	/// Also seeds HomeLocation and PatrolRadius in the new blackboard
	virtual bool RunBehaviorTree(UBehaviorTree* BTAsset) override;
	//~ End AAIController Interface.

	/// Where the pawn was when possessed; patrol points are picked around it
	UFUNCTION(BlueprintPure, Category = "Godsent|AI")
	FVector GetHomeLocation() const { return HomeLocation; }

	bool HasHomeLocation() const { return bHasHomeLocation; }

	UFUNCTION(BlueprintPure, Category = "Godsent|AI")
	AActor* GetTargetActor() const;

	/// Sets the blackboard target (and stops any pending forget)
	UFUNCTION(BlueprintCallable, Category = "Godsent|AI")
	void SetTargetActor(AActor* NewTargetActor);

	/// Clears the blackboard target, sending the enemy back to patrolling
	UFUNCTION(BlueprintCallable, Category = "Godsent|AI")
	void ClearTargetActor();

protected:
	virtual void BeginPlay() override;

	//~ Begin AController Interface.
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	//~ End AController Interface.

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> EnemyPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAISenseConfig_Sight> AISenseConfig_Sight;

	/// Sets the target when a hostile pawn is first seen; starts the forget timer when the target leaves sight
	UFUNCTION()
	virtual void OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/// On: losing sight of the target (out of the sight cone, behind a wall, past Lose Sight Radius) eventually clears it.
	/// Off (tutorial behaviour, e.g. the dummy): once seen, the target is chased until it dies.
	UPROPERTY(EditDefaultsOnly, Category = "AI|Target")
	bool bForgetLostTargets = false;

	/// Seconds out of sight before the target is forgotten (brief breaks behind a pillar don't count)
	UPROPERTY(EditDefaultsOnly, Category = "AI|Target", meta = (EditCondition = "bForgetLostTargets", ClampMin = "0.0", Units = "s"))
	float ForgetTargetDelay = 3.f;

	/// A target this close is never forgotten, even out of sight (e.g. rolling behind the enemy mid-fight)
	UPROPERTY(EditDefaultsOnly, Category = "AI|Target", meta = (EditCondition = "bForgetLostTargets", ClampMin = "0.0", Units = "cm"))
	float CloseRangeMemoryDistance = 400.f;

	/// Being damaged by a hostile pawn makes it the target if there isn't one (e.g. hit from behind before being seen)
	UPROPERTY(EditDefaultsOnly, Category = "AI|Target")
	bool bAggroOnDamage = true;

	/// Stop the behaviour tree and any movement when the pawn gets Shared.Status.Dead
	UPROPERTY(EditDefaultsOnly, Category = "AI|Target")
	bool bStopLogicOnDeath = true;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
	FName TargetActorKeyName = FName("TargetActor");

	/// Vector key written when the tree starts (skipped if the blackboard doesn't have it)
	UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
	FName HomeLocationKeyName = FName("HomeLocation");

	/// Float key written when the tree starts, from the enemy's Patrol Radius (skipped if the blackboard doesn't have it)
	UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
	FName PatrolRadiusKeyName = FName("PatrolRadius");

private:
	UPROPERTY(EditDefaultsOnly, Category = "Detour Crowd Avoidance Config")
	bool bEnableDetourCrowdAvoidance = true;

	UPROPERTY(EditDefaultsOnly, Category = "Detour Crowd Avoidance Config", meta = (EditCondition = "bEnableDetourCrowdAvoidance", UIMin = "1", UIMax = "4"))
	int32 DetourCrowdAvoidanceQuality = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Detour Crowd Avoidance Config", meta = (EditCondition = "bEnableDetourCrowdAvoidance"))
	float CollisionQueryRange = 600.f;

	/// True if sight currently reports the actor
	bool IsActorCurrentlySeen(AActor* Actor) const;

	bool IsValidTargetCandidate(const AActor* Actor) const;

	void StartForgetTargetTimer();
	void StopForgetTargetTimer();
	void OnForgetTargetTimerExpired();

	void OnPawnDamageTaken(const FGameplayEventData* Payload);
	void OnPawnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);

	void BindPawnAbilitySystemEvents(APawn* InPawn);
	void UnbindPawnAbilitySystemEvents();

	FVector HomeLocation = FVector::ZeroVector;
	bool bHasHomeLocation = false;

	FTimerHandle ForgetTargetTimerHandle;

	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystemComponent;
	FDelegateHandle DamageTakenDelegateHandle;
	FDelegateHandle DeadTagDelegateHandle;
};
