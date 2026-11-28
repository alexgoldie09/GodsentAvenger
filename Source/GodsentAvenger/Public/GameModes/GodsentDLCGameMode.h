// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameModes/GodsentBaseGameMode.h"
#include "GameplayTagContainer.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "GodsentDLCGameMode.generated.h"

class AGodsentEnemySpawner;
class AGodsentEnemyCharacter;
class UAbilitySystemComponent;
class UUserWidget;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDLCWaveChangedDelegate, int32, Wave, int32, WaveCount);

/**
 * Game mode for the DLC map: a wave survival mode, completely separate from the main game.
 * - Skills start empty every time, and nothing (skills, points, deaths, the boss star) is saved to the family.
 * - The spawners in the level are taken over: each wave they all send Enemies Per Spawner from that wave's pool.
 *   The waves' enemies load in the background when the map opens.
 * - Once every spawner is cleared (and the wave's boss is dead), the wave is won: Skill Points Per Wave are given and
 *   the skill tree opens (paused) until they're spent, then the next wave starts after Wave Break Delay.
 * - Beating the last wave wins (the win screen). Dying is the usual lose screen; Try Again restarts from wave 1.
 * - A wave with a Boss Class spawns it at the actor tagged Boss Spawn Point Tag and sends it straight at the player.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentDLCGameMode : public AGodsentBaseGameMode
{
	GENERATED_BODY()

public:
	AGodsentDLCGameMode();

	/// Closing the skill tree between waves starts the next wave (otherwise as normal)
	virtual void HandleRebirthSkillsChosen(AGodsentHeroController* HeroController) override;

	/// The wave being fought (1 = the first). 0 before the first wave starts.
	UFUNCTION(BlueprintPure, Category = "Godsent|Waves")
	int32 GetCurrentWave() const { return CurrentWaveIndex == INDEX_NONE ? 0 : CurrentWaveIndex + 1; }

	UFUNCTION(BlueprintPure, Category = "Godsent|Waves")
	int32 GetWaveCount() const { return Waves.Num(); }

	/// A new wave started: update the wave counter ("Wave: {Wave}")
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Waves")
	FOnDLCWaveChangedDelegate OnWaveChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void NativeOnHeirTookControl(APawn* Hero, bool bIsFounder) override;

	/// Every wave in order. The last one won wins the level.
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (TitleProperty = "EnemiesPerSpawner"))
	TArray<FGodsentDLCWave> Waves;

	/// Given after every wave but the last (the skill tree opens to spend them)
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0"))
	int32 SkillPointsPerWave = 2;

	/// Points to start with. Above 0: the skill tree opens before wave 1 to spend them.
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0"))
	int32 StartingSkillPoints = 0;

	/// Seconds before the skill tree opens: after the last enemy of a wave dies, and before wave 1 (Starting Skill Points)
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0.0", Units = "s"))
	float SkillTreeOpenDelay = 1.5f;

	/// Seconds after the player gets control before wave 1
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0.0", Units = "s"))
	float FirstWaveDelay = 3.f;

	/// Seconds of breather between waves (after the skill tree closes)
	UPROPERTY(EditDefaultsOnly, Category = "Waves", meta = (ClampMin = "0.0", Units = "s"))
	float WaveBreakDelay = 4.f;

	/// A wave's boss appears at the actor with this tag (e.g. a Target Point)
	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	FName BossSpawnPointTag = TEXT("DLCBossSpawn");

	/// The "Wave: 1" counter at the top of the screen (listens to On Wave Changed)
	UPROPERTY(EditDefaultsOnly, Category = "Waves|UI")
	TSubclassOf<UUserWidget> WaveCounterWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Waves|UI")
	int32 WaveCounterZOrder = 10;

private:
	/// Loads every wave's enemies (and bosses) in the background
	void LoadWaveEnemies();
	void OnWaveEnemiesLoaded();

	/// Wave 1 after First Wave Delay, once the enemies are loaded and the player has control
	void TryStartFirstWave();

	void StartWave(int32 WaveIndex);
	void StartNextWaveAfterBreak();

	/// Opens the skill tree if there's something to buy, otherwise carries straight on
	void OpenSkillTreeOrContinue();

	/// After the skill tree (or none): wave 1 if the waves haven't started, otherwise the next wave after the break
	void ContinueAfterSkills();

	UFUNCTION()
	void OnSpawnerCleared(AGodsentEnemySpawner* Spawner);

	void SpawnBoss(const FGodsentDLCWave& Wave);
	void OnBossDeadTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void OnBossDestroyed(AActor* DestroyedActor);

	void BossDefeated();
	void UnbindBoss();

	/// Every spawner cleared and the boss (if any) dead: the wave is won
	void CheckWaveComplete();
	void CompleteWave();

	/// Lost or won: no more waves
	bool IsRunOver() const;

	AGodsentHeroController* GetHeroController() const;

	TArray<TWeakObjectPtr<AGodsentEnemySpawner>> WaveSpawners;

	TSharedPtr<FStreamableHandle> WaveEnemiesLoadHandle;

	TWeakObjectPtr<AGodsentEnemyCharacter> Boss;
	TWeakObjectPtr<UAbilitySystemComponent> BossASC;
	FDelegateHandle BossDeadTagHandle;

	UPROPERTY()
	TObjectPtr<UUserWidget> WaveCounterWidget;

	FTimerHandle WaveTimerHandle;

	int32 CurrentWaveIndex = INDEX_NONE;

	bool bWaveEnemiesLoaded = false;
	bool bHeroInControl = false;
	bool bFirstWaveQueued = false;
	bool bWaveInProgress = false;

	/// Spawners are being reset for a new wave: their "cleared" calls don't count yet
	bool bStartingWave = false;

	bool bBossAlive = false;

	/// The skill tree is open between waves (or before wave 1)
	bool bChoosingWaveSkills = false;

	/// Waiting to open the skill tree for the Starting Skill Points (wave 1 waits for it)
	bool bStartingSkillsPending = false;
};
