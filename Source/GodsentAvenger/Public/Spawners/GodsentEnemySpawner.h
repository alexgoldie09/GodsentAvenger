// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "GodsentEnemySpawner.generated.h"

class AGodsentEnemyCharacter;
class AGodsentEnemySpawner;
class USphereComponent;

/// One possible enemy in a spawner's pool
USTRUCT(BlueprintType)
struct FGodsentEnemySpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TSubclassOf<AGodsentEnemyCharacter> EnemyClass;

	/// How likely this enemy is compared with the others (Zombie 3 + Ghost 1 = 75% zombies). 0 = never.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float Weight = 1.f;
};

/// When the spawner spawns its enemies
UENUM(BlueprintType)
enum class EGodsentSpawnerActivation : uint8
{
	PlayerProximity,	// When the player enters the Activation Radius (default)
	BeginPlay,			// As soon as the level starts
	Manual				// Only when Spawn Enemies is called (e.g. from a trigger or a script)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpawnerEnemySpawnedDelegate, AGodsentEnemySpawner*, Spawner, AGodsentEnemyCharacter*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSpawnerEnemyDefeatedDelegate, AGodsentEnemySpawner*, Spawner, AGodsentEnemyCharacter*, Enemy, int32, RemainingEnemies);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnerClearedDelegate, AGodsentEnemySpawner*, Spawner);

/**
 * Spawns a random set of enemies within Spawn Radius once the player comes near, and tracks them.
 * - picks Min-Max Spawn Count enemies from the weighted Enemy Pool (different every run);
 * - places each on a random reachable navmesh point, at least Min Spacing apart, facing a random way;
 * - spawned enemies patrol around where they appeared (the AI controller records it as home);
 * - counts an enemy as defeated the moment it gets Shared.Status.Dead (or if it's destroyed some other way, e.g. falling out of the world)
 *   and fires On Enemy Defeated / On Spawner Cleared, for gates, objectives or UI to listen to;
 * - never respawns on its own: Reset Spawner (or Reset All Spawners, for the rebirth loop) clears it and re-arms it for a fresh roll.
 *
 * World Partition: untick "Is Spatially Loaded" on spawners, so one doesn't stream out and back in and spawn a second wave.
 */
UCLASS(Blueprintable)
class GODSENTAVENGER_API AGodsentEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AGodsentEnemySpawner();

	virtual void OnConstruction(const FTransform& Transform) override;

	/// Spawns this spawner's enemies now (does nothing if it already has)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Spawner")
	void SpawnEnemies();

	/// Forgets this run's enemies (destroying any still in the level) and re-arms the spawner for a fresh random set
	UFUNCTION(BlueprintCallable, Category = "Godsent|Spawner")
	void ResetSpawner(bool bDestroySpawnedEnemies = true);

	/// Resets every spawner in the world, e.g. when a new heir is reborn. New Enemy Level 0 keeps each spawner's current level.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Spawner", meta = (WorldContext = "WorldContextObject"))
	static void ResetAllSpawners(const UObject* WorldContextObject, int32 NewEnemyLevel = 0, bool bDestroySpawnedEnemies = true);

	/// Level the next enemies are spawned at (their curve table rows are read at this level)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Spawner")
	void SetEnemyLevel(int32 NewEnemyLevel) { EnemyLevel = FMath::Max(NewEnemyLevel, 1); }

	UFUNCTION(BlueprintPure, Category = "Godsent|Spawner")
	int32 GetEnemyLevel() const { return EnemyLevel; }

	UFUNCTION(BlueprintPure, Category = "Godsent|Spawner")
	bool HasSpawned() const { return bHasSpawned; }

	/// True once it has spawned and every one of its enemies is defeated
	UFUNCTION(BlueprintPure, Category = "Godsent|Spawner")
	bool IsCleared() const { return bHasBroadcastCleared; }

	UFUNCTION(BlueprintPure, Category = "Godsent|Spawner")
	int32 GetAliveEnemyCount() const;

	/// Enemies from this spawner that are still alive
	UFUNCTION(BlueprintPure, Category = "Godsent|Spawner")
	TArray<AGodsentEnemyCharacter*> GetAliveEnemies() const;

	/// Fires for each enemy as it's spawned
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Spawner")
	FOnSpawnerEnemySpawnedDelegate OnEnemySpawned;

	/// Fires when one of its enemies dies, with how many are left
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Spawner")
	FOnSpawnerEnemyDefeatedDelegate OnEnemyDefeated;

	/// Fires once all of its enemies are dead (hook the Aldegon barrier to this later)
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Spawner")
	FOnSpawnerClearedDelegate OnSpawnerCleared;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<USceneComponent> SpawnerRoot;

	/// Player entering this sphere activates the spawner (shown in the editor only)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<USphereComponent> ActivationSphere;

	/// Shows the spawn area in the editor (no collision)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<USphereComponent> SpawnAreaPreview;

	// ---- Activation ----

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Activation")
	EGodsentSpawnerActivation ActivationMode = EGodsentSpawnerActivation::PlayerProximity;

	/// Keep this larger than the enemies' sight radius, so they're already standing there when the player could first notice them
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Activation", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "ActivationMode == EGodsentSpawnerActivation::PlayerProximity"))
	float ActivationRadius = 3500.f;

	// ---- What to spawn ----

	/// Enemies to pick from (weighted)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Enemies")
	TArray<FGodsentEnemySpawnEntry> EnemyPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Enemies", meta = (ClampMin = "0"))
	int32 MinSpawnCount = 2;

	/// Each activation spawns a random number between Min and Max (set both the same for a fixed count)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Enemies", meta = (ClampMin = "0"))
	int32 MaxSpawnCount = 4;

	/// Base level its enemies are spawned at. The lineage's enemy tier is added on top
	/// (applied level = Enemy Level + tier - 1) unless Scale With Lineage Tier is off.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Enemies", meta = (ClampMin = "1"))
	int32 EnemyLevel = 1;

	/// Off: this spawner's enemies always use Enemy Level as it is, whatever the generation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Enemies")
	bool bScaleWithLineageTier = true;

	/// On: every enemy from this spawner patrols Patrol Radius Override instead of its own Patrol Radius
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Enemies")
	bool bOverridePatrolRadius = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Enemies", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bOverridePatrolRadius"))
	float PatrolRadiusOverride = 800.f;

	// ---- Where to spawn ----

	/// Enemies appear on the navmesh within this distance of the spawner
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float SpawnRadius = 1000.f;

	/// No two enemies from this spawner appear closer together than this
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float MinSpacing = 250.f;

	/// Tries per enemy to find a point that respects Min Spacing (after that it takes any reachable point)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Placement", meta = (ClampMin = "1"))
	int32 MaxPlacementAttempts = 12;

	/// Seconds between each enemy appearing (0 = all at once)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Placement", meta = (ClampMin = "0.0", Units = "s"))
	float SpawnInterval = 0.2f;

	/// Draws each spawn point for a few seconds
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Debug")
	bool bDrawDebugSpawnPoints = false;

private:
	struct FTrackedSpawnedEnemy
	{
		TWeakObjectPtr<AGodsentEnemyCharacter> Enemy;
		FDelegateHandle DeadTagHandle;
		bool bDefeated = false;
	};

	UFUNCTION()
	void OnActivationSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/// Destroyed without dying first (fell out of the world, cleaned up by a script): still counts as defeated
	UFUNCTION()
	void OnTrackedEnemyDestroyed(AActor* DestroyedActor);

	void OnTrackedEnemyDeadTagChanged(const FGameplayTag Tag, int32 NewCount, TWeakObjectPtr<AGodsentEnemyCharacter> WeakEnemy);

	void SpawnNextEnemy();
	AGodsentEnemyCharacter* SpawnOneEnemy();
	bool FindSpawnLocation(FVector& OutLocation);
	TSubclassOf<AGodsentEnemyCharacter> PickEnemyClass() const;

	void MarkEnemyDefeated(AGodsentEnemyCharacter* Enemy);
	void UnbindTrackedEnemy(FTrackedSpawnedEnemy& Tracked);
	void CheckCleared();

	void SetActivationArmed(bool bArmed);
	void ActivateIfPlayerInside();
	static bool IsPlayerPawn(const AActor* Actor);

	TArray<FTrackedSpawnedEnemy> TrackedEnemies;
	TArray<FVector> UsedSpawnLocations;
	FTimerHandle SpawnTimerHandle;

	/// While armed, re-checks for the player every half second (catches a player who was already inside before they were possessed)
	FTimerHandle ActivationCheckTimerHandle;

	int32 PendingSpawnCount = 0;
	bool bHasSpawned = false;
	bool bHasBroadcastCleared = false;
	bool bIsResetting = false;
};
