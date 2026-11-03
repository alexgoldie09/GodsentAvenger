// Alex Goldberg rights reserved

#include "Spawners/GodsentEnemySpawner.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Characters/GodsentEnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GodsentGameplayTags.h"

AGodsentEnemySpawner::AGodsentEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// A spawner streamed in while the player already stands inside its radius should still activate
	bGenerateOverlapEventsDuringLevelStreaming = true;

	SpawnerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnerRoot"));
	SetRootComponent(SpawnerRoot);

	// Only the player's pawn matters: overlap pawns, ignore everything else
	ActivationSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ActivationSphere"));
	ActivationSphere->SetupAttachment(SpawnerRoot);
	ActivationSphere->InitSphereRadius(ActivationRadius);
	ActivationSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Armed in BeginPlay for proximity spawners
	ActivationSphere->SetCollisionObjectType(ECC_WorldDynamic);
	ActivationSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	ActivationSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ActivationSphere->SetGenerateOverlapEvents(true);
	ActivationSphere->SetHiddenInGame(true);
	ActivationSphere->ShapeColor = FColor(255, 200, 40);
	ActivationSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnActivationSphereBeginOverlap);

	SpawnAreaPreview = CreateDefaultSubobject<USphereComponent>(TEXT("SpawnAreaPreview"));
	SpawnAreaPreview->SetupAttachment(SpawnerRoot);
	SpawnAreaPreview->InitSphereRadius(SpawnRadius);
	SpawnAreaPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnAreaPreview->SetGenerateOverlapEvents(false);
	SpawnAreaPreview->SetHiddenInGame(true);
	SpawnAreaPreview->ShapeColor = FColor(220, 40, 40);
}

/// Keeps the editor spheres matching the radii
void AGodsentEnemySpawner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ActivationSphere->SetSphereRadius(ActivationMode == EGodsentSpawnerActivation::PlayerProximity ? ActivationRadius : 0.f);
	SpawnAreaPreview->SetSphereRadius(SpawnRadius);
}

void AGodsentEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	ActivationSphere->SetSphereRadius(ActivationRadius);
	SpawnAreaPreview->SetSphereRadius(SpawnRadius);

	switch (ActivationMode)
	{
	case EGodsentSpawnerActivation::PlayerProximity:
		SetActivationArmed(true);
		ActivateIfPlayerInside();
		break;

	case EGodsentSpawnerActivation::BeginPlay:
		SpawnEnemies();
		break;

	default:
		break;
	}
}

void AGodsentEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(ActivationCheckTimerHandle);

	for (FTrackedSpawnedEnemy& Tracked : TrackedEnemies)
		UnbindTrackedEnemy(Tracked);

	TrackedEnemies.Empty();

	Super::EndPlay(EndPlayReason);
}

/// Spawns this spawner's enemies now (does nothing if it already has)
void AGodsentEnemySpawner::SpawnEnemies()
{
	if (bHasSpawned)
		return;

	bHasSpawned = true;
	bHasBroadcastCleared = false;
	SetActivationArmed(false);

	UsedSpawnLocations.Reset();
	PendingSpawnCount = FMath::RandRange(MinSpawnCount, FMath::Max(MinSpawnCount, MaxSpawnCount));

	if (EnemyPool.IsEmpty())
		UE_LOG(LogTemp, Warning, TEXT("%s: Enemy Pool is empty, nothing to spawn"), *GetName());

	if (SpawnInterval <= 0.f)
	{
		while (PendingSpawnCount > 0)
			SpawnNextEnemy();
	}
	else
	{
		// First one now, the rest one per interval
		SpawnNextEnemy();

		if (PendingSpawnCount > 0)
			GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ThisClass::SpawnNextEnemy, SpawnInterval, true);
	}

	// Covers zero enemies (empty pool, or nowhere to stand)
	CheckCleared();
}

void AGodsentEnemySpawner::SpawnNextEnemy()
{
	if (PendingSpawnCount <= 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		CheckCleared();
		return;
	}

	--PendingSpawnCount;
	SpawnOneEnemy();

	if (PendingSpawnCount <= 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		CheckCleared();
	}
}

AGodsentEnemyCharacter* AGodsentEnemySpawner::SpawnOneEnemy()
{
	UWorld* World = GetWorld();
	const TSubclassOf<AGodsentEnemyCharacter> EnemyClass = PickEnemyClass();

	if (!World || !EnemyClass)
		return nullptr;

	FVector GroundLocation;

	if (!FindSpawnLocation(GroundLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no reachable navmesh point within Spawn Radius, skipped an enemy"), *GetName());
		return nullptr;
	}

	// The navmesh point is on the floor; the actor's origin is the middle of its capsule
	const AGodsentEnemyCharacter* EnemyCDO = EnemyClass->GetDefaultObject<AGodsentEnemyCharacter>();
	const float HalfHeight = EnemyCDO && EnemyCDO->GetCapsuleComponent() ? EnemyCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;

	const FTransform SpawnTransform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), GroundLocation + FVector(0.f, 0.f, HalfHeight + 5.f));

	AGodsentEnemyCharacter* Enemy = World->SpawnActorDeferred<AGodsentEnemyCharacter>(
		EnemyClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!Enemy)
		return nullptr;

	// Before FinishSpawning: possession (and the start-up data, home location and patrol radius) happens inside it
	Enemy->SetEnemyLevel(EnemyLevel);

	if (bOverridePatrolRadius)
		Enemy->SetPatrolRadius(PatrolRadiusOverride);

	Enemy->FinishSpawning(SpawnTransform);

	if (!IsValid(Enemy))
		return nullptr;

	FTrackedSpawnedEnemy& Tracked = TrackedEnemies.AddDefaulted_GetRef();
	Tracked.Enemy = Enemy;

	if (UAbilitySystemComponent* EnemyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy))
	{
		Tracked.DeadTagHandle = EnemyASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::OnTrackedEnemyDeadTagChanged, TWeakObjectPtr<AGodsentEnemyCharacter>(Enemy));
	}

	Enemy->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnTrackedEnemyDestroyed);

	if (bDrawDebugSpawnPoints)
		DrawDebugSphere(World, SpawnTransform.GetLocation(), 40.f, 12, FColor::Red, false, 5.f);

	OnEnemySpawned.Broadcast(this, Enemy);
	return Enemy;
}

/// A random reachable navmesh point within Spawn Radius, at least Min Spacing from the others where possible
bool AGodsentEnemySpawner::FindSpawnLocation(FVector& OutLocation)
{
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!NavSystem)
		return false;

	// The spawner may be placed off the floor; start from the navmesh under it
	FNavLocation Origin;

	if (!NavSystem->ProjectPointToNavigation(GetActorLocation(), Origin, FVector(500.f, 500.f, 1000.f)))
		return false;

	FNavLocation Candidate;
	bool bHasFallback = false;
	FVector Fallback = FVector::ZeroVector;

	for (int32 Attempt = 0; Attempt < MaxPlacementAttempts; ++Attempt)
	{
		if (!NavSystem->GetRandomReachablePointInRadius(Origin.Location, SpawnRadius, Candidate))
			continue;

		if (!bHasFallback)
		{
			Fallback = Candidate.Location;
			bHasFallback = true;
		}

		const bool bTooClose = UsedSpawnLocations.ContainsByPredicate([this, &Candidate](const FVector& Used)
		{
			return FVector::Dist2D(Used, Candidate.Location) < MinSpacing;
		});

		if (!bTooClose)
		{
			OutLocation = Candidate.Location;
			UsedSpawnLocations.Add(OutLocation);
			return true;
		}
	}

	// Crowded area: better a little close than missing
	if (bHasFallback)
	{
		OutLocation = Fallback;
		UsedSpawnLocations.Add(OutLocation);
		return true;
	}

	return false;
}

/// Weighted random pick from the Enemy Pool
TSubclassOf<AGodsentEnemyCharacter> AGodsentEnemySpawner::PickEnemyClass() const
{
	float TotalWeight = 0.f;

	for (const FGodsentEnemySpawnEntry& Entry : EnemyPool)
	{
		if (Entry.EnemyClass && Entry.Weight > 0.f)
			TotalWeight += Entry.Weight;
	}

	if (TotalWeight <= 0.f)
		return nullptr;

	float Roll = FMath::FRandRange(0.f, TotalWeight);

	for (const FGodsentEnemySpawnEntry& Entry : EnemyPool)
	{
		if (!Entry.EnemyClass || Entry.Weight <= 0.f)
			continue;

		if (Roll <= Entry.Weight)
			return Entry.EnemyClass;

		Roll -= Entry.Weight;
	}

	return nullptr;
}

void AGodsentEnemySpawner::OnTrackedEnemyDeadTagChanged(const FGameplayTag Tag, int32 NewCount, TWeakObjectPtr<AGodsentEnemyCharacter> WeakEnemy)
{
	if (NewCount > 0)
		MarkEnemyDefeated(WeakEnemy.Get());
}

void AGodsentEnemySpawner::OnTrackedEnemyDestroyed(AActor* DestroyedActor)
{
	if (!bIsResetting)
		MarkEnemyDefeated(Cast<AGodsentEnemyCharacter>(DestroyedActor));
}

void AGodsentEnemySpawner::MarkEnemyDefeated(AGodsentEnemyCharacter* Enemy)
{
	if (!Enemy)
		return;

	FTrackedSpawnedEnemy* Tracked = TrackedEnemies.FindByPredicate([Enemy](const FTrackedSpawnedEnemy& Entry)
	{
		return Entry.Enemy.Get() == Enemy;
	});

	// Already counted (it died, then its body was destroyed after the dissolve)
	if (!Tracked || Tracked->bDefeated)
		return;

	Tracked->bDefeated = true;

	// Stop listening for its death; keep the destroyed binding so nothing else fires for it
	if (UAbilitySystemComponent* EnemyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy))
		EnemyASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved).Remove(Tracked->DeadTagHandle);

	Tracked->DeadTagHandle.Reset();

	OnEnemyDefeated.Broadcast(this, Enemy, GetAliveEnemyCount());
	CheckCleared();
}

void AGodsentEnemySpawner::UnbindTrackedEnemy(FTrackedSpawnedEnemy& Tracked)
{
	AGodsentEnemyCharacter* Enemy = Tracked.Enemy.Get();

	if (!Enemy)
		return;

	if (Tracked.DeadTagHandle.IsValid())
	{
		if (UAbilitySystemComponent* EnemyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy))
			EnemyASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved).Remove(Tracked.DeadTagHandle);

		Tracked.DeadTagHandle.Reset();
	}

	Enemy->OnDestroyed.RemoveDynamic(this, &ThisClass::OnTrackedEnemyDestroyed);
}

void AGodsentEnemySpawner::CheckCleared()
{
	if (!bHasSpawned || bHasBroadcastCleared || PendingSpawnCount > 0 || GetAliveEnemyCount() > 0)
		return;

	bHasBroadcastCleared = true;
	OnSpawnerCleared.Broadcast(this);
}

int32 AGodsentEnemySpawner::GetAliveEnemyCount() const
{
	int32 AliveCount = 0;

	for (const FTrackedSpawnedEnemy& Tracked : TrackedEnemies)
	{
		if (!Tracked.bDefeated && Tracked.Enemy.IsValid())
			++AliveCount;
	}

	return AliveCount;
}

TArray<AGodsentEnemyCharacter*> AGodsentEnemySpawner::GetAliveEnemies() const
{
	TArray<AGodsentEnemyCharacter*> AliveEnemies;

	for (const FTrackedSpawnedEnemy& Tracked : TrackedEnemies)
	{
		if (!Tracked.bDefeated && Tracked.Enemy.IsValid())
			AliveEnemies.Add(Tracked.Enemy.Get());
	}

	return AliveEnemies;
}

/// Forgets this run's enemies and re-arms the spawner for a fresh random set
void AGodsentEnemySpawner::ResetSpawner(bool bDestroySpawnedEnemies)
{
	bIsResetting = true;

	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

	for (FTrackedSpawnedEnemy& Tracked : TrackedEnemies)
	{
		UnbindTrackedEnemy(Tracked);

		// Also clears out bodies that are still dissolving
		if (bDestroySpawnedEnemies && Tracked.Enemy.IsValid())
			Tracked.Enemy->Destroy();
	}

	TrackedEnemies.Empty();
	UsedSpawnLocations.Reset();
	PendingSpawnCount = 0;
	bHasSpawned = false;
	bHasBroadcastCleared = false;

	bIsResetting = false;

	switch (ActivationMode)
	{
	case EGodsentSpawnerActivation::PlayerProximity:
		SetActivationArmed(true);
		ActivateIfPlayerInside(); // A new heir may already be standing inside
		break;

	case EGodsentSpawnerActivation::BeginPlay:
		SpawnEnemies();
		break;

	default:
		break;
	}
}

/// Resets every spawner in the world (the rebirth loop calls this)
void AGodsentEnemySpawner::ResetAllSpawners(const UObject* WorldContextObject, int32 NewEnemyLevel, bool bDestroySpawnedEnemies)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;

	if (!World)
		return;

	for (TActorIterator<AGodsentEnemySpawner> It(World); It; ++It)
	{
		if (NewEnemyLevel > 0)
			It->SetEnemyLevel(NewEnemyLevel);

		It->ResetSpawner(bDestroySpawnedEnemies);
	}
}

void AGodsentEnemySpawner::OnActivationSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsPlayerPawn(OtherActor))
		SpawnEnemies();
}

void AGodsentEnemySpawner::SetActivationArmed(bool bArmed)
{
	ActivationSphere->SetCollisionEnabled(bArmed ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	if (bArmed)
		GetWorldTimerManager().SetTimer(ActivationCheckTimerHandle, this, &ThisClass::ActivateIfPlayerInside, 0.5f, true);
	else
		GetWorldTimerManager().ClearTimer(ActivationCheckTimerHandle);
}

/// Begin Overlap only fires when an overlap starts, so a player already inside (e.g. spawned there, or a new heir
/// standing inside after a reset) is found by checking the current overlaps instead
void AGodsentEnemySpawner::ActivateIfPlayerInside()
{
	if (bHasSpawned)
		return;

	TArray<AActor*> OverlappingActors;
	ActivationSphere->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	for (const AActor* Actor : OverlappingActors)
	{
		if (IsPlayerPawn(Actor))
		{
			SpawnEnemies();
			return;
		}
	}
}

bool AGodsentEnemySpawner::IsPlayerPawn(const AActor* Actor)
{
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && Pawn->IsPlayerControlled();
}
