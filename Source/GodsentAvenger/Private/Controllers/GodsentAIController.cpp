// Alex Goldberg rights reserved

#include "Controllers/GodsentAIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BrainComponent.h"
#include "TimerManager.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GodsentEnemyCharacter.h"
#include "GameFramework/Pawn.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Sight.h"

AGodsentAIController::AGodsentAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>("PathFollowingComponent"))
{
	// Defaults suit the dummy (sees everything). Per enemy, edit the Blueprint controller's
	// Enemy Perception Component -> Senses Config -> Sight (radius, lose radius, vision angle).
	AISenseConfig_Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("EnemySenseConfig_Sight"));
	AISenseConfig_Sight->DetectionByAffiliation.bDetectEnemies = true;
	AISenseConfig_Sight->DetectionByAffiliation.bDetectFriendlies = false;
	AISenseConfig_Sight->DetectionByAffiliation.bDetectNeutrals = false;
	AISenseConfig_Sight->SightRadius = 5000.f;
	AISenseConfig_Sight->LoseSightRadius = 5500.f; // Was 0 in the tutorial; must be at least Sight Radius once targets can be forgotten
	AISenseConfig_Sight->PeripheralVisionAngleDegrees = 360.f;

	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("EnemyPerceptionComponent"));
	EnemyPerceptionComponent->ConfigureSense(*AISenseConfig_Sight);
	EnemyPerceptionComponent->SetDominantSense(UAISenseConfig_Sight::StaticClass());
	EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ThisClass::OnEnemyPerceptionUpdated);

	SetGenericTeamId(FGenericTeamId(1));
}

/// Hostile to any pawn whose controller is on a lower team (the hero is team 0)
ETeamAttitude::Type AGodsentAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	// Perception can report non-pawn actors (e.g. a summoned totem), which have no controller
	const APawn* PawnToCheck = Cast<const APawn>(&Other);

	if (!PawnToCheck)
		return ETeamAttitude::Neutral;

	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(PawnToCheck->GetController());

	if (OtherTeamAgent && OtherTeamAgent->GetGenericTeamId() < GetGenericTeamId())
		return ETeamAttitude::Hostile;

	return ETeamAttitude::Friendly;
}

/// Starts the tree, then seeds HomeLocation and PatrolRadius (the Blueprint calls this from On Possess)
bool AGodsentAIController::RunBehaviorTree(UBehaviorTree* BTAsset)
{
	const bool bStarted = Super::RunBehaviorTree(BTAsset);

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		// Setting a key the blackboard doesn't have does nothing, so the dummy's blackboard is unaffected
		if (bHasHomeLocation)
			BlackboardComponent->SetValueAsVector(HomeLocationKeyName, HomeLocation);

		if (const AGodsentEnemyCharacter* EnemyCharacter = Cast<AGodsentEnemyCharacter>(GetPawn()))
			BlackboardComponent->SetValueAsFloat(PatrolRadiusKeyName, EnemyCharacter->GetPatrolRadius());
	}

	return bStarted;
}

AActor* AGodsentAIController::GetTargetActor() const
{
	const UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	return BlackboardComponent ? Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetActorKeyName)) : nullptr;
}

void AGodsentAIController::SetTargetActor(AActor* NewTargetActor)
{
	StopForgetTargetTimer();

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
		BlackboardComponent->SetValueAsObject(TargetActorKeyName, NewTargetActor);
}

void AGodsentAIController::ClearTargetActor()
{
	StopForgetTargetTimer();
	ClearFocus(EAIFocusPriority::Gameplay);

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
		BlackboardComponent->ClearValue(TargetActorKeyName);
}

void AGodsentAIController::BeginPlay()
{
	Super::BeginPlay();

	if (UCrowdFollowingComponent* CrowdFollowingComponent = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		CrowdFollowingComponent->SetCrowdSimulationState(bEnableDetourCrowdAvoidance ? ECrowdSimulationState::Enabled : ECrowdSimulationState::Disabled);

		switch (DetourCrowdAvoidanceQuality)
		{
		case 1: CrowdFollowingComponent->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Low); break;
		case 2: CrowdFollowingComponent->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Medium); break;
		case 3: CrowdFollowingComponent->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good); break;
		case 4: CrowdFollowingComponent->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High); break;
		default: break;
		}

		CrowdFollowingComponent->SetAvoidanceGroup(1);
		CrowdFollowingComponent->SetGroupsToAvoid(1);
		CrowdFollowingComponent->SetCrowdCollisionQueryRange(CollisionQueryRange);
	}
}

void AGodsentAIController::OnPossess(APawn* InPawn)
{
	// Home is the possess location; the Blueprint's On Possess (which runs the tree) fires after this returns
	if (InPawn)
	{
		HomeLocation = InPawn->GetActorLocation();
		bHasHomeLocation = true;
	}

	Super::OnPossess(InPawn);

	BindPawnAbilitySystemEvents(InPawn);
}

void AGodsentAIController::OnUnPossess()
{
	UnbindPawnAbilitySystemEvents();
	StopForgetTargetTimer();

	Super::OnUnPossess();
}

/// Sets the target when a hostile pawn is first seen; starts the forget timer when the target leaves sight
void AGodsentAIController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !GetBlackboardComponent())
		return;

	AActor* CurrentTarget = GetTargetActor();

	if (Stimulus.WasSuccessfullySensed())
	{
		if (!CurrentTarget && IsValidTargetCandidate(Actor))
			SetTargetActor(Actor);
		else if (Actor == CurrentTarget)
			StopForgetTargetTimer(); // Back in sight before the delay ran out

		return;
	}

	if (bForgetLostTargets && Actor == CurrentTarget)
		StartForgetTargetTimer();
}

bool AGodsentAIController::IsActorCurrentlySeen(AActor* Actor) const
{
	if (!Actor || !EnemyPerceptionComponent)
		return false;

	TArray<AActor*> SeenActors;
	EnemyPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), SeenActors);

	return SeenActors.Contains(Actor);
}

bool AGodsentAIController::IsValidTargetCandidate(const AActor* Actor) const
{
	return IsValid(Actor) &&
		GetTeamAttitudeTowards(*Actor) == ETeamAttitude::Hostile &&
		!UGodsentFunctionLibrary::NativeDoesActorHaveTag(const_cast<AActor*>(Actor), GodsentGameplayTags::Shared_Status_Dead);
}

void AGodsentAIController::StartForgetTargetTimer()
{
	if (!bForgetLostTargets)
		return;

	GetWorldTimerManager().SetTimer(ForgetTargetTimerHandle, this, &ThisClass::OnForgetTargetTimerExpired, FMath::Max(ForgetTargetDelay, 0.01f), false);
}

void AGodsentAIController::StopForgetTargetTimer()
{
	GetWorldTimerManager().ClearTimer(ForgetTargetTimerHandle);
}

void AGodsentAIController::OnForgetTargetTimerExpired()
{
	AActor* CurrentTarget = GetTargetActor();
	const APawn* ControlledPawn = GetPawn();

	if (!CurrentTarget || !ControlledPawn)
		return;

	// Seen again without a new perception event (can happen at the edge of the sight cone)
	if (IsActorCurrentlySeen(CurrentTarget))
		return;

	// Still right next to us: keep fighting and check again later
	if (FVector::Dist(CurrentTarget->GetActorLocation(), ControlledPawn->GetActorLocation()) <= CloseRangeMemoryDistance)
	{
		StartForgetTargetTimer();
		return;
	}

	ClearTargetActor();
}

/// Aggro on damage: the attacker becomes the target if there isn't one yet
void AGodsentAIController::OnPawnDamageTaken(const FGameplayEventData* Payload)
{
	if (!bAggroOnDamage || !Payload || GetTargetActor())
		return;

	AActor* InstigatorActor = const_cast<AActor*>(Payload->Instigator.Get());

	if (!IsValidTargetCandidate(InstigatorActor))
		return;

	SetTargetActor(InstigatorActor);

	// Hit by something it can't see (e.g. from behind): forget it unless the enemy turns and spots it
	if (bForgetLostTargets && !IsActorCurrentlySeen(InstigatorActor))
		StartForgetTargetTimer();
}

/// Stops the tree and movement when the pawn dies, so a dying enemy can't keep walking or attacking
void AGodsentAIController::OnPawnDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0 || !bStopLogicOnDeath)
		return;

	StopForgetTargetTimer();
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);

	if (UBrainComponent* Brain = GetBrainComponent())
		Brain->StopLogic(TEXT("Pawn died"));
}

void AGodsentAIController::BindPawnAbilitySystemEvents(APawn* InPawn)
{
	UnbindPawnAbilitySystemEvents();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InPawn);

	if (!ASC)
		return;

	BoundAbilitySystemComponent = ASC;

	// Sent by the attribute set whenever damage lands (instigator = attacker)
	DamageTakenDelegateHandle = ASC->GenericGameplayEventCallbacks
		.FindOrAdd(GodsentGameplayTags::Shared_Event_DamageTaken)
		.AddUObject(this, &ThisClass::OnPawnDamageTaken);

	DeadTagDelegateHandle = ASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::OnPawnDeadTagChanged);
}

void AGodsentAIController::UnbindPawnAbilitySystemEvents()
{
	UAbilitySystemComponent* ASC = BoundAbilitySystemComponent.Get();

	if (ASC)
	{
		if (FGameplayEventMulticastDelegate* DamageTakenDelegate = ASC->GenericGameplayEventCallbacks.Find(GodsentGameplayTags::Shared_Event_DamageTaken))
			DamageTakenDelegate->Remove(DamageTakenDelegateHandle);

		ASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved)
			.Remove(DeadTagDelegateHandle);
	}

	DamageTakenDelegateHandle.Reset();
	DeadTagDelegateHandle.Reset();
	BoundAbilitySystemComponent.Reset();
}
