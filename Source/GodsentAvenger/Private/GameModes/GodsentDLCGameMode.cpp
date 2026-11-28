// Alex Goldberg rights reserved

#include "GameModes/GodsentDLCGameMode.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Characters/GodsentEnemyCharacter.h"
#include "Components/Aldegon/GodsentAldegonComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkillTree/SkillTreeComponent.h"
#include "Controllers/GodsentAIController.h"
#include "Controllers/GodsentHeroController.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GodsentGameplayTags.h"
#include "Spawners/GodsentEnemySpawner.h"
#include "TimerManager.h"

namespace
{
	/// The enemies the default waves use (Andre changes the waves on BP_DLCGameMode)
	const TCHAR* const DLCZombieClassPath = TEXT("/Game/EnemyCharacters/Zombie/BP_NewZombie.BP_NewZombie_C");
	const TCHAR* const DLCGhostClassPath = TEXT("/Game/EnemyCharacters/Ghost/BP_Ghost.BP_Ghost_C");
	const TCHAR* const DLCAldegonClassPath = TEXT("/Game/EnemyCharacters/Aldegon/BP_Aldegon.BP_Aldegon_C");

	/// One row of the default ramp: zombies only, then ghosts mixed in, then Aldegon alone
	struct FDLCDefaultWave
	{
		int32 EnemiesPerSpawner;
		int32 EnemyLevel;
		float ZombieWeight;
		float GhostWeight;
		bool bAldegon;
	};

	constexpr FDLCDefaultWave DLCDefaultWaves[] =
	{
		{ 2, 1, 1.f, 0.f, false },
		{ 3, 1, 1.f, 0.f, false },
		{ 3, 2, 1.f, 0.f, false },
		{ 4, 2, 1.f, 0.f, false },
		{ 3, 3, 3.f, 1.f, false },
		{ 4, 3, 2.f, 1.f, false },
		{ 4, 4, 1.f, 1.f, false },
		{ 5, 4, 1.f, 1.f, false },
		{ 5, 5, 1.f, 2.f, false },
		{ 6, 5, 1.f, 2.f, false },
		{ 6, 6, 1.f, 3.f, false },
		{ 0, 6, 0.f, 0.f, true },
	};

	/// The boss stands this far above the marker's floor point (so the capsule doesn't start in the ground)
	constexpr float DLCBossSpawnFloorClearance = 5.f;

	FGodsentEnemySpawnEntry MakeDLCSpawnEntry(const TCHAR* ClassPath, float Weight)
	{
		FGodsentEnemySpawnEntry Entry;
		Entry.EnemyClass = TSoftClassPtr<AGodsentEnemyCharacter>(FSoftObjectPath(ClassPath));
		Entry.Weight = Weight;
		return Entry;
	}
}

AGodsentDLCGameMode::AGodsentDLCGameMode()
{
	// A side mode with its own skills: nothing here touches the family
	bTracksLineage = false;
	bUsesFamilySkills = false;

	for (const FDLCDefaultWave& DefaultWave : DLCDefaultWaves)
	{
		FGodsentDLCWave& Wave = Waves.AddDefaulted_GetRef();
		Wave.EnemiesPerSpawner = DefaultWave.EnemiesPerSpawner;
		Wave.EnemyLevel = DefaultWave.EnemyLevel;

		if (DefaultWave.ZombieWeight > 0.f)
			Wave.EnemyPool.Add(MakeDLCSpawnEntry(DLCZombieClassPath, DefaultWave.ZombieWeight));

		if (DefaultWave.GhostWeight > 0.f)
			Wave.EnemyPool.Add(MakeDLCSpawnEntry(DLCGhostClassPath, DefaultWave.GhostWeight));

		if (DefaultWave.bAldegon)
			Wave.BossClass = TSoftClassPtr<AGodsentEnemyCharacter>(FSoftObjectPath(DLCAldegonClassPath));
	}
}

void AGodsentDLCGameMode::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<AGodsentEnemySpawner> It(GetWorld()); It; ++It)
	{
		WaveSpawners.Add(*It);
		It->OnSpawnerCleared.AddUniqueDynamic(this, &ThisClass::OnSpawnerCleared);
	}

	if (WaveSpawners.IsEmpty())
		UE_LOG(LogTemp, Warning, TEXT("[Waves] No enemy spawners in the level: waves will only have their bosses"));

	if (Waves.IsEmpty())
		UE_LOG(LogTemp, Warning, TEXT("[Waves] BP_DLCGameMode has no Waves"));

	LoadWaveEnemies();
}

void AGodsentDLCGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);

	if (WaveEnemiesLoadHandle.IsValid() && WaveEnemiesLoadHandle->IsLoadingInProgress())
		WaveEnemiesLoadHandle->CancelHandle();

	WaveEnemiesLoadHandle.Reset();
	UnbindBoss();

	Super::EndPlay(EndPlayReason);
}

void AGodsentDLCGameMode::LoadWaveEnemies()
{
	TArray<FSoftObjectPath> ClassPaths;

	for (const FGodsentDLCWave& Wave : Waves)
	{
		for (const FSoftObjectPath& ClassPath : FGodsentEnemySpawnEntry::GetClassPaths(Wave.EnemyPool))
			ClassPaths.AddUnique(ClassPath);

		if (!Wave.BossClass.IsNull())
			ClassPaths.AddUnique(Wave.BossClass.ToSoftObjectPath());
	}

	if (!ClassPaths.IsEmpty())
	{
		WaveEnemiesLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ClassPaths,
			FStreamableDelegate::CreateUObject(this, &ThisClass::OnWaveEnemiesLoaded));
	}

	// Nothing to load, or already in memory (the handle calls back straight away then, but be safe)
	if (!WaveEnemiesLoadHandle.IsValid())
		OnWaveEnemiesLoaded();
}

void AGodsentDLCGameMode::OnWaveEnemiesLoaded()
{
	bWaveEnemiesLoaded = true;
	TryStartFirstWave();
}

void AGodsentDLCGameMode::NativeOnHeirTookControl(APawn* Hero, bool bIsFounder)
{
	Super::NativeOnHeirTookControl(Hero, bIsFounder);

	if (bHeroInControl)
		return;

	bHeroInControl = true;

	// The waves run the spawners from now on (and clear anything they spawned by themselves)
	for (const TWeakObjectPtr<AGodsentEnemySpawner>& Spawner : WaveSpawners)
	{
		if (Spawner.IsValid())
			Spawner->DisableAutoActivation();
	}

	AGodsentHeroController* HeroController = GetHeroController();

	if (HeroController && WaveCounterWidgetClass && !WaveCounterWidget)
	{
		WaveCounterWidget = CreateWidget<UUserWidget>(HeroController, WaveCounterWidgetClass);

		if (WaveCounterWidget)
			WaveCounterWidget->AddToViewport(WaveCounterZOrder);
	}

	// Starting points: the skill tree opens first, and wave 1 waits for it to close
	if (StartingSkillPoints > 0 && HeroController && HeroController->GetSkillTreeComponent())
	{
		HeroController->GetSkillTreeComponent()->AddSkillPoints(StartingSkillPoints);
		bStartingSkillsPending = true;

		if (SkillTreeOpenDelay > 0.f)
			GetWorldTimerManager().SetTimer(WaveTimerHandle, this, &ThisClass::OpenSkillTreeOrContinue, SkillTreeOpenDelay, false);
		else
			OpenSkillTreeOrContinue();

		return;
	}

	TryStartFirstWave();
}

void AGodsentDLCGameMode::TryStartFirstWave()
{
	if (!bWaveEnemiesLoaded || !bHeroInControl || bFirstWaveQueued || Waves.IsEmpty())
		return;

	// Spending the starting points first
	if (bStartingSkillsPending || bChoosingWaveSkills)
		return;

	bFirstWaveQueued = true;

	if (FirstWaveDelay > 0.f)
		GetWorldTimerManager().SetTimer(WaveTimerHandle, FTimerDelegate::CreateUObject(this, &ThisClass::StartWave, 0), FirstWaveDelay, false);
	else
		StartWave(0);
}

void AGodsentDLCGameMode::StartWave(int32 WaveIndex)
{
	if (IsRunOver() || !Waves.IsValidIndex(WaveIndex))
		return;

	CurrentWaveIndex = WaveIndex;
	bWaveInProgress = true;

	const FGodsentDLCWave& Wave = Waves[WaveIndex];
	UE_LOG(LogTemp, Display, TEXT("[Waves] Wave %d of %d"), WaveIndex + 1, Waves.Num());

	OnWaveChanged.Broadcast(WaveIndex + 1, Waves.Num());

	// Spawners clearing straight away (0 enemies) mustn't end the wave before everything is out
	bStartingWave = true;

	SpawnBoss(Wave);

	for (const TWeakObjectPtr<AGodsentEnemySpawner>& Spawner : WaveSpawners)
	{
		if (Spawner.IsValid())
			Spawner->SpawnWave(Wave.EnemyPool, Wave.EnemiesPerSpawner, Wave.EnemyLevel);
	}

	bStartingWave = false;
	CheckWaveComplete();
}

void AGodsentDLCGameMode::OnSpawnerCleared(AGodsentEnemySpawner* Spawner)
{
	CheckWaveComplete();
}

void AGodsentDLCGameMode::CheckWaveComplete()
{
	if (!bWaveInProgress || bStartingWave || bBossAlive)
		return;

	for (const TWeakObjectPtr<AGodsentEnemySpawner>& Spawner : WaveSpawners)
	{
		if (Spawner.IsValid() && !Spawner->IsCleared())
			return;
	}

	CompleteWave();
}

void AGodsentDLCGameMode::CompleteWave()
{
	bWaveInProgress = false;

	if (IsRunOver())
		return;

	// The last wave: the level is won (Aldegon's death may have done this already)
	if (CurrentWaveIndex >= Waves.Num() - 1)
	{
		HandleRunWon();
		return;
	}

	AGodsentHeroController* HeroController = GetHeroController();
	USkillTreeComponent* SkillTree = HeroController ? HeroController->GetSkillTreeComponent() : nullptr;

	if (SkillTree && SkillPointsPerWave > 0)
		SkillTree->AddSkillPoints(SkillPointsPerWave);

	// A beat after the last kill, so the skill tree doesn't snap open the instant it dies
	if (SkillTreeOpenDelay > 0.f)
		GetWorldTimerManager().SetTimer(WaveTimerHandle, this, &ThisClass::OpenSkillTreeOrContinue, SkillTreeOpenDelay, false);
	else
		OpenSkillTreeOrContinue();
}

void AGodsentDLCGameMode::OpenSkillTreeOrContinue()
{
	if (IsRunOver())
		return;

	bStartingSkillsPending = false;

	// Spend them now (paused); it closes once nothing's left to buy. Nothing to buy: straight on.
	AGodsentHeroController* HeroController = GetHeroController();

	if (HeroController && HeroController->ShowRebirthSkillTree())
	{
		bChoosingWaveSkills = true;
		SetRunState(EGodsentRunState::ChoosingSkills);
		return;
	}

	ContinueAfterSkills();
}

void AGodsentDLCGameMode::ContinueAfterSkills()
{
	// The starting points were spent: now wave 1 (after First Wave Delay)
	if (CurrentWaveIndex == INDEX_NONE)
	{
		TryStartFirstWave();
		return;
	}

	StartNextWaveAfterBreak();
}

void AGodsentDLCGameMode::HandleRebirthSkillsChosen(AGodsentHeroController* HeroController)
{
	if (!bChoosingWaveSkills)
	{
		Super::HandleRebirthSkillsChosen(HeroController);
		return;
	}

	bChoosingWaveSkills = false;
	SetRunState(EGodsentRunState::Playing);

	if (HeroController)
		HeroController->ResumeGameplay();

	ContinueAfterSkills();
}

void AGodsentDLCGameMode::StartNextWaveAfterBreak()
{
	const int32 NextWaveIndex = CurrentWaveIndex + 1;

	if (WaveBreakDelay > 0.f)
		GetWorldTimerManager().SetTimer(WaveTimerHandle, FTimerDelegate::CreateUObject(this, &ThisClass::StartWave, NextWaveIndex), WaveBreakDelay, false);
	else
		StartWave(NextWaveIndex);
}

void AGodsentDLCGameMode::SpawnBoss(const FGodsentDLCWave& Wave)
{
	if (Wave.BossClass.IsNull())
		return;

	const TSubclassOf<AGodsentEnemyCharacter> BossClass = Wave.BossClass.LoadSynchronous();
	AActor* SpawnPoint = nullptr;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(BossSpawnPointTag))
		{
			SpawnPoint = *It;
			break;
		}
	}

	if (!BossClass || !SpawnPoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Waves] Wave %d's boss can't spawn: no Boss Class, or no actor tagged %s"),
			CurrentWaveIndex + 1, *BossSpawnPointTag.ToString());
		return;
	}

	// The marker sits on the floor; the boss's origin is the middle of its capsule
	const AGodsentEnemyCharacter* BossCDO = BossClass->GetDefaultObject<AGodsentEnemyCharacter>();
	const float HalfHeight = BossCDO && BossCDO->GetCapsuleComponent() ? BossCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
	const FTransform SpawnTransform(FRotator(0.f, SpawnPoint->GetActorRotation().Yaw, 0.f),
		SpawnPoint->GetActorLocation() + FVector(0.f, 0.f, HalfHeight + DLCBossSpawnFloorClearance));

	AGodsentEnemyCharacter* NewBoss = GetWorld()->SpawnActorDeferred<AGodsentEnemyCharacter>(BossClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!NewBoss)
		return;

	NewBoss->SetEnemyLevel(Wave.EnemyLevel);
	NewBoss->SetScaleWithLineageTier(false);
	NewBoss->FinishSpawning(SpawnTransform);

	if (!IsValid(NewBoss))
		return;

	Boss = NewBoss;
	bBossAlive = true;

	if (UAbilitySystemComponent* NewBossASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewBoss))
	{
		BossASC = NewBossASC;
		BossDeadTagHandle = NewBossASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::OnBossDeadTagChanged);
	}

	NewBoss->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnBossDestroyed);

	// Straight at the player: Aldegon wakes and fights (no arena here), anything else just targets them
	const AGodsentHeroController* HeroController = GetHeroController();
	APawn* Hero = HeroController ? HeroController->GetPawn() : nullptr;

	if (UGodsentAldegonComponent* AldegonComponent = NewBoss->FindComponentByClass<UGodsentAldegonComponent>())
		AldegonComponent->StartFight(Hero);
	else if (AGodsentAIController* BossController = Cast<AGodsentAIController>(NewBoss->GetController()))
		BossController->SetTargetActor(Hero);
}

void AGodsentDLCGameMode::OnBossDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
		BossDefeated();
}

void AGodsentDLCGameMode::OnBossDestroyed(AActor* DestroyedActor)
{
	BossDefeated();
}

void AGodsentDLCGameMode::BossDefeated()
{
	if (!bBossAlive)
		return;

	bBossAlive = false;
	UnbindBoss();
	CheckWaveComplete();
}

void AGodsentDLCGameMode::UnbindBoss()
{
	if (UAbilitySystemComponent* CurrentBossASC = BossASC.Get())
		CurrentBossASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved).Remove(BossDeadTagHandle);

	if (AGodsentEnemyCharacter* CurrentBoss = Boss.Get())
		CurrentBoss->OnDestroyed.RemoveDynamic(this, &ThisClass::OnBossDestroyed);

	BossASC.Reset();
	BossDeadTagHandle.Reset();
	Boss.Reset();
}

bool AGodsentDLCGameMode::IsRunOver() const
{
	return GetRunState() == EGodsentRunState::Lost || GetRunState() == EGodsentRunState::Won;
}

AGodsentHeroController* AGodsentDLCGameMode::GetHeroController() const
{
	return Cast<AGodsentHeroController>(GetWorld()->GetFirstPlayerController());
}
