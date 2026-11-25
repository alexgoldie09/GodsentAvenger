// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/Aldegon/GodsentAldegonAbility_Summon.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GodsentEnemyCharacter.h"
#include "Components/Aldegon/GodsentAldegonComponent.h"
#include "Components/CapsuleComponent.h"
#include "Controllers/GodsentAIController.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Pawn.h"
#include "GodsentGameplayTags.h"

namespace
{
	/// Summons appear this far above the floor so they don't start inside it (totems can't fall, so keep it small)
	constexpr float SummonFloorClearance = 5.f;

	/// The room check uses a capsule this much smaller, so small bumps in the floor don't rule a spot out
	constexpr float SummonRoomCheckTolerance = 10.f;

	/// Smallest capsule the room check will shrink to
	constexpr float SummonMinRoomCheckSize = 1.f;

	/// Capsule half height used if the summon's class has no capsule to read
	constexpr float SummonFallbackHalfHeight = 90.f;

	/// How long the debug drawing stays up (s)
	constexpr float SummonDebugDrawTime = 5.f;
	constexpr float SummonDebugSpotRadius = 40.f;
	constexpr int32 SummonDebugSegments = 16;
	constexpr int32 SummonDebugCircleSegments = 32;
	constexpr float SummonDebugLineThickness = 2.f;

	constexpr float SummonFullTurnDegrees = 360.f;

	/// Looks this far above and below a navmesh point for the real floor (navmesh only roughly follows a bumpy floor)
	constexpr float SummonFloorSearchUp = 100.f;
	constexpr float SummonFloorSearchDown = 200.f;

	/// Capsule radius used if the summon's class has no capsule to read
	constexpr float SummonFallbackRadius = 40.f;

	const FName SummonFloorTraceTag = TEXT("GodsentSummonFloor");
}

UGodsentAldegonAbility_Summon::UGodsentAldegonAbility_Summon()
{
	SummonEventTag = GodsentGameplayTags::Enemy_Event_SummonEnemies;
}

void UGodsentAldegonAbility_Summon::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// Load the pool in the background now, so it's ready long before he first summons
	const TArray<FSoftObjectPath> ClassPaths = FGodsentEnemySpawnEntry::GetClassPaths(SummonPool);

	if (!ClassPaths.IsEmpty() && !PoolLoadHandle.IsValid())
		PoolLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ClassPaths);
}

void UGodsentAldegonAbility_Summon::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (PoolLoadHandle.IsValid())
	{
		PoolLoadHandle->ReleaseHandle();
		PoolLoadHandle.Reset();
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

void UGodsentAldegonAbility_Summon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bSummonedThisCast = false;

	// No montage: summon straight away so it still works while testing
	if (!SummonMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no Summon Montage, summoning without one"), *GetNameSafe(this));
		SpawnSummons();
		FinishMove(bSummonedThisCast);
		return;
	}

	UAbilityTask_WaitGameplayEvent* WaitSummon = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SummonEventTag, nullptr, true);
	WaitSummon->EventReceived.AddDynamic(this, &ThisClass::OnSummonEvent);
	WaitSummon->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, SummonMontage, GetMoveMontageRate());

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageDone);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageDone);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGodsentAldegonAbility_Summon::OnSummonEvent(FGameplayEventData Payload)
{
	SpawnSummons();
}

void UGodsentAldegonAbility_Summon::OnMontageDone()
{
	// The notify is missing: summon at the end rather than not at all
	if (!bSummonedThisCast && IsActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Summon Montage has no %s notify, summoned at the end instead"),
			*GetNameSafe(this), *SummonEventTag.ToString());
		SpawnSummons();
	}

	FinishMove(bSummonedThisCast);
}

void UGodsentAldegonAbility_Summon::OnMontageInterrupted()
{
	// Cut off by something else: no free summons, and no cooldown if nothing came out
	FinishMove(bSummonedThisCast);
}

void UGodsentAldegonAbility_Summon::OnMontageCancelled()
{
	FinishMove(false);
}

void UGodsentAldegonAbility_Summon::SpawnSummons()
{
	if (bSummonedThisCast)
		return;

	bSummonedThisCast = true;

	UGodsentAldegonComponent* Aldegon = GetAldegonComponent();

	if (!Aldegon)
		return;

	// Random per cast, never past his cap
	const float Level = GetAbilityLevel();
	const int32 MinCount = FMath::Max(FMath::RoundToInt(MinPerCast.GetValueAtLevel(Level)), 0);
	const int32 MaxCount = FMath::Max(FMath::RoundToInt(MaxPerCast.GetValueAtLevel(Level)), MinCount);
	const int32 Room = FMath::Max(Aldegon->GetMaxSummons(SummonType) - Aldegon->GetLivingSummonCount(SummonType), 0);
	const int32 SummonCount = FMath::Min(FMath::RandRange(MinCount, MaxCount), Room);

	if (SummonCount <= 0)
		return;

	FVector Centre = FVector::ZeroVector;

	if (!GetSummonCentre(Centre))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no navmesh near the summon spot, nothing summoned"), *GetNameSafe(this));
		return;
	}

	const float Spread = SpreadRadius.GetValueAtLevel(Level);
	const float Spacing = MinSpacing.GetValueAtLevel(Level);

	// Keep clear of what he already has out
	TArray<FVector> TakenSpots;
	TArray<AActor*> LivingSummons;
	Aldegon->GetLivingSummons(LivingSummons);

	for (const AActor* Summon : LivingSummons)
		TakenSpots.Add(Summon->GetActorLocation());

	if (bDrawDebug)
		DrawDebugCircle(GetWorld(), Centre, Spread, SummonDebugCircleSegments, FColor::Purple, false, SummonDebugDrawTime, 0,
			SummonDebugLineThickness, FVector::ForwardVector, FVector::RightVector, false);

	AActor* Target = GetCombatTarget();

	for (int32 Index = 0; Index < SummonCount; ++Index)
	{
		const FGodsentEnemySpawnEntry* PickedEntry = FGodsentEnemySpawnEntry::PickWeighted(SummonPool);
		const TSubclassOf<AGodsentEnemyCharacter> SummonClass = PickedEntry ? PickedEntry->ResolveClass() : nullptr;

		if (!SummonClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: Summon Pool is empty (or all weights are 0)"), *GetNameSafe(this));
			return;
		}

		// Its own capsule decides whether a spot has room
		const AGodsentEnemyCharacter* SummonCDO = SummonClass->GetDefaultObject<AGodsentEnemyCharacter>();
		const UCapsuleComponent* SummonCapsule = SummonCDO ? SummonCDO->GetCapsuleComponent() : nullptr;
		const float CapsuleRadius = SummonCapsule ? SummonCapsule->GetScaledCapsuleRadius() : SummonFallbackRadius;
		const float CapsuleHalfHeight = SummonCapsule ? SummonCapsule->GetScaledCapsuleHalfHeight() : SummonFallbackHalfHeight;

		FVector Spot = FVector::ZeroVector;

		if (!FindSummonSpot(Centre, Spread, Spacing, CapsuleRadius, CapsuleHalfHeight, TakenSpots, Spot))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: no spot with room to stand near the summon spot, skipped one"), *GetNameSafe(this));
			continue;
		}

		if (AGodsentEnemyCharacter* Summon = SpawnOneSummon(SummonClass, Spot, Target))
			Aldegon->RegisterSummon(Summon, SummonType);
	}
}

bool UGodsentAldegonAbility_Summon::GetSummonCentre(FVector& OutCentre) const
{
	const APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!AvatarPawn || !NavSystem)
		return false;

	FVector WantedCentre = AvatarPawn->GetActorLocation();

	// The tree's pick (EQS_Aldegon_SummonSpot), if it set one
	const AAIController* AIController = Cast<AAIController>(AvatarPawn->GetController());
	const UBlackboardComponent* Blackboard = AIController ? AIController->GetBlackboardComponent() : nullptr;

	if (Blackboard && Blackboard->GetKeyID(SummonLocationKeyName) != FBlackboard::InvalidKey && Blackboard->IsVectorValueSet(SummonLocationKeyName))
		WantedCentre = Blackboard->GetValueAsVector(SummonLocationKeyName);

	FNavLocation NavCentre;

	if (!NavSystem->ProjectPointToNavigation(WantedCentre, NavCentre, NavSearchExtent))
		return false;

	OutCentre = NavCentre.Location;
	return true;
}

bool UGodsentAldegonAbility_Summon::FindSummonSpot(const FVector& Centre, float Spread, float Spacing, float CapsuleRadius,
	float CapsuleHalfHeight, TArray<FVector>& Taken, FVector& OutSpot) const
{
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!NavSystem)
		return false;

	auto IsTooClose = [&Taken, Spacing](const FVector& Candidate)
	{
		return Taken.ContainsByPredicate([&Candidate, Spacing](const FVector& Used)
		{
			return FVector::Dist2D(Used, Candidate) < Spacing;
		});
	};

	bool bHasFallback = false;
	FVector Fallback = Centre;

	for (int32 Attempt = 0; Attempt < MaxPlacementAttempts; ++Attempt)
	{
		FVector NavPoint = Centre;

		// No spread: right on the spot (later ones still try to keep their spacing)
		if (Spread > 0.f || Attempt > 0)
		{
			FNavLocation RandomPoint;

			if (!NavSystem->GetRandomReachablePointInRadius(Centre, FMath::Max(Spread, Spacing), RandomPoint))
				continue;

			NavPoint = RandomPoint.Location;
		}

		// No real floor there, or no room for the capsule (bumpy floor, under a ledge): try somewhere else
		FVector FloorSpot = FVector::ZeroVector;

		if (!FitsOnFloor(NavPoint, CapsuleRadius, CapsuleHalfHeight, FloorSpot))
			continue;

		if (!bHasFallback)
		{
			Fallback = FloorSpot;
			bHasFallback = true;
		}

		if (!IsTooClose(FloorSpot))
		{
			OutSpot = FloorSpot;
			Taken.Add(OutSpot);
			return true;
		}
	}

	// Crowded: a little close is better than missing one (but never somewhere it would fall through)
	if (!bHasFallback)
		return false;

	OutSpot = Fallback;
	Taken.Add(OutSpot);
	return true;
}

bool UGodsentAldegonAbility_Summon::FitsOnFloor(const FVector& NavPoint, float CapsuleRadius, float CapsuleHalfHeight, FVector& OutFloorSpot) const
{
	UWorld* World = GetWorld();

	if (!World)
		return false;

	FCollisionQueryParams Params(SummonFloorTraceTag, false, GetAvatarActorFromActorInfo());

	// The floor a character would actually stand on (anything that blocks pawns, but not the pawns themselves)
	FCollisionResponseParams FloorResponse;
	FloorResponse.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);

	FHitResult FloorHit;
	const FVector TraceStart = NavPoint + FVector(0.f, 0.f, SummonFloorSearchUp);
	const FVector TraceEnd = NavPoint - FVector(0.f, 0.f, SummonFloorSearchDown);

	if (!World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Pawn, Params, FloorResponse))
		return false;

	// Room to stand there, a little above the floor (pawns count here: not inside the player or another summon)
	const FVector CapsuleCentre = FloorHit.ImpactPoint + FVector(0.f, 0.f, CapsuleHalfHeight + SummonFloorClearance);
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
		FMath::Max(CapsuleRadius - SummonRoomCheckTolerance, SummonMinRoomCheckSize),
		FMath::Max(CapsuleHalfHeight - SummonRoomCheckTolerance, SummonMinRoomCheckSize));

	if (World->OverlapBlockingTestByChannel(CapsuleCentre, FQuat::Identity, ECC_Pawn, CapsuleShape, Params))
		return false;

	OutFloorSpot = FloorHit.ImpactPoint;
	return true;
}

AGodsentEnemyCharacter* UGodsentAldegonAbility_Summon::SpawnOneSummon(TSubclassOf<AGodsentEnemyCharacter> SummonClass, const FVector& FloorSpot, AActor* Target)
{
	APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	UWorld* World = GetWorld();

	if (!AvatarPawn || !World || !SummonClass)
		return nullptr;

	// The navmesh point is on the floor; the actor's origin is the middle of its capsule
	const AGodsentEnemyCharacter* SummonCDO = SummonClass->GetDefaultObject<AGodsentEnemyCharacter>();
	const float HalfHeight = SummonCDO && SummonCDO->GetCapsuleComponent()
		? SummonCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: SummonFallbackHalfHeight;

	float Yaw = FMath::FRandRange(0.f, SummonFullTurnDegrees);

	if (bFaceTarget && Target)
		Yaw = (Target->GetActorLocation() - FloorSpot).Rotation().Yaw;

	const FTransform SpawnTransform(FRotator(0.f, Yaw, 0.f), FloorSpot + FVector(0.f, 0.f, HalfHeight + SummonFloorClearance));

	// He's their Instigator (a totem finds who to heal through it; Owner becomes their AI controller on possess)
	AGodsentEnemyCharacter* Summon = World->SpawnActorDeferred<AGodsentEnemyCharacter>(
		SummonClass, SpawnTransform, AvatarPawn, AvatarPawn, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!Summon)
		return nullptr;

	// His level, which already has the lineage tier in it
	if (const AGodsentEnemyCharacter* AldegonCharacter = Cast<AGodsentEnemyCharacter>(AvatarPawn))
		Summon->SetEnemyLevel(AldegonCharacter->GetAppliedEnemyLevel());

	Summon->SetScaleWithLineageTier(false);
	Summon->FinishSpawning(SpawnTransform);

	if (!IsValid(Summon))
		return nullptr;

	// Straight for the player, no patrolling first
	if (AGodsentAIController* SummonController = Cast<AGodsentAIController>(Summon->GetController()))
	{
		if (Target)
			SummonController->SetTargetActor(Target);
	}

	if (SpawnCueTag.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayCueParameters CueParameters;
			CueParameters.Location = FloorSpot;
			CueParameters.Instigator = AvatarPawn;
			CueParameters.EffectCauser = Summon;
			ASC->ExecuteGameplayCue(SpawnCueTag, CueParameters);
		}
	}

	if (bDrawDebug)
		DrawDebugSphere(World, FloorSpot, SummonDebugSpotRadius, SummonDebugSegments, FColor::Purple, false, SummonDebugDrawTime);

	return Summon;
}
