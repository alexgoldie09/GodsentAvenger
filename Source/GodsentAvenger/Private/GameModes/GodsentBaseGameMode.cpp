// Alex Goldberg rights reserved

#include "GameModes/GodsentBaseGameMode.h"
#include "Controllers/GodsentHeroController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GodsentGameInstance.h"
#include "Subsystems/GodsentLineageSubsystem.h"
#include "TimerManager.h"

AGodsentBaseGameMode* AGodsentBaseGameMode::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;

	return World ? World->GetAuthGameMode<AGodsentBaseGameMode>() : nullptr;
}

/// The controller has started in the level: name the heir (after the start delay) or start play straight away
void AGodsentBaseGameMode::HandleHeroControllerReady(AGodsentHeroController* HeroController)
{
	if (!HeroController)
		return;

	const UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this);

	if (!Lineage || !Lineage->NeedsHeirName())
	{
		StartHeirPlay(HeroController);
		return;
	}

	// The level is visible but the heir waits for its name
	SetRunState(EGodsentRunState::Starting);
	HeroController->SetHeroInputBlocked(true);

	RunAfterDelay(HeroController, HeirNamePromptDelay, [this](AGodsentHeroController* Controller)
	{
		ShowHeirNamePrompt(Controller);
	});
}

void AGodsentBaseGameMode::ShowHeirNamePrompt(AGodsentHeroController* HeroController)
{
	SetRunState(EGodsentRunState::NamingHeir);

	// No widget class set up: play unnamed rather than getting stuck
	if (!HeroController->ShowHeirNamePrompt())
		StartHeirPlay(HeroController);
}

/// The name prompt was confirmed: every heir after the founder spends their new skill point before play
void AGodsentBaseGameMode::HandleHeirNamed(AGodsentHeroController* HeroController)
{
	if (!HeroController || RunState != EGodsentRunState::NamingHeir)
		return;

	const UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this);
	const bool bIsLaterHeir = Lineage && Lineage->GetLineageData().Generation > 1;

	if (bIsLaterHeir)
	{
		SetRunState(EGodsentRunState::ChoosingSkills);

		if (HeroController->ShowRebirthSkillTree())
			return;
	}

	StartHeirPlay(HeroController);
}

/// The rebirth skill tree's Continue button was pressed
void AGodsentBaseGameMode::HandleRebirthSkillsChosen(AGodsentHeroController* HeroController)
{
	if (!HeroController || RunState != EGodsentRunState::ChoosingSkills)
		return;

	StartHeirPlay(HeroController);
}

/// The heir takes control: unpause, give input back, record the level and commit ("loading in")
void AGodsentBaseGameMode::StartHeirPlay(AGodsentHeroController* HeroController)
{
	SetRunState(EGodsentRunState::Playing);

	HeroController->SetHeroInputBlocked(false);
	HeroController->ResumeGameplay();

	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this))
	{
		if (const UGodsentGameInstance* GodsentGameInstance = Cast<UGodsentGameInstance>(GetGameInstance()))
			Lineage->SetLastLevelTag(GodsentGameInstance->FindLevelTagForWorld(GetWorld()));

		Lineage->CommitProgress();
	}
}

/// The heir died: record it on the lineage now (+1 generation, +1 skill point, commit), show the lose screen after the delay
void AGodsentBaseGameMode::HandleHeroDefeated(AGodsentHeroController* HeroController)
{
	if (!HeroController || RunState == EGodsentRunState::Lost || RunState == EGodsentRunState::Won)
		return;

	SetRunState(EGodsentRunState::Lost);

	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this))
		Lineage->RecordHeirDeath();

	RunAfterDelay(HeroController, LoseScreenDelay, [](AGodsentHeroController* Controller)
	{
		Controller->ShowLoseScreen();
	});
}

/// The level was won: show the win screen after the delay
void AGodsentBaseGameMode::HandleRunWon()
{
	if (RunState == EGodsentRunState::Lost || RunState == EGodsentRunState::Won)
		return;

	AGodsentHeroController* HeroController = Cast<AGodsentHeroController>(GetWorld()->GetFirstPlayerController());

	if (!HeroController)
		return;

	SetRunState(EGodsentRunState::Won);

	RunAfterDelay(HeroController, WinScreenDelay, [](AGodsentHeroController* Controller)
	{
		Controller->ShowWinScreen();
	});
}

void AGodsentBaseGameMode::SetRunState(EGodsentRunState NewState)
{
	if (RunState == NewState)
		return;

	RunState = NewState;
	OnRunStateChanged.Broadcast(RunState);
}

/// Runs Action on the controller after Delay seconds (straight away for 0), unless the controller is gone by then.
/// One timer at a time: a new step replaces any step still waiting.
void AGodsentBaseGameMode::RunAfterDelay(AGodsentHeroController* HeroController, float Delay, TFunction<void(AGodsentHeroController*)> Action)
{
	GetWorldTimerManager().ClearTimer(RunTimerHandle);

	if (Delay <= 0.f)
	{
		Action(HeroController);
		return;
	}

	TWeakObjectPtr<AGodsentHeroController> WeakController = HeroController;

	GetWorldTimerManager().SetTimer(RunTimerHandle, FTimerDelegate::CreateWeakLambda(this, [WeakController, Action]()
	{
		if (AGodsentHeroController* Controller = WeakController.Get())
			Action(Controller);
	}), Delay, false);
}
