// Alex Goldberg rights reserved

#include "Controllers/GodsentHeroController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "Components/SkillTree/SkillTreeComponent.h"
#include "Controllers/GodsentCheatManager.h"
#include "Blueprint/UserWidget.h"
#include "GameModes/GodsentBaseGameMode.h"
#include "GodsentGameplayTags.h"
#include "Subsystems/GodsentLineageSubsystem.h"

AGodsentHeroController::AGodsentHeroController()
{
	HeroTeamId = FGenericTeamId(0);

	SkillTreeComponent = CreateDefaultSubobject<USkillTreeComponent>(TEXT("SkillTreeComponent"));

	// Debug console commands (never created in Shipping)
	CheatClass = UGodsentCheatManager::StaticClass();
}

/// Reports to the game mode, which decides whether to show the name prompt or start play
void AGodsentHeroController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
		return;

	if (AGodsentBaseGameMode* GameMode = AGodsentBaseGameMode::Get(this))
	{
		GameMode->HandleHeroControllerReady(this);
	}
	else
	{
		// A level with another game mode: no rebirth loop, just play
		UE_LOG(LogTemp, Warning, TEXT("[Lineage] This level's game mode isn't a GodsentBaseGameMode, so there's no name prompt or lose screen"));
		ResumeGameplay();
	}
}

/// Possesses the heir (its startup data is granted inside Super), loads the lineage's skills and re-applies them on top
void AGodsentHeroController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (SkillTreeComponent)
	{
		SkillTreeComponent->LoadFromLineage();
		SkillTreeComponent->ApplyAllUnlockedSkills();
	}

	UnbindHeroDeath();

	if (UAbilitySystemComponent* HeroASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InPawn))
	{
		BoundHeroASC = HeroASC;
		HeroDeadTagHandle = HeroASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::OnHeroDeadTagChanged);
	}
}

void AGodsentHeroController::OnUnPossess()
{
	UnbindHeroDeath();

	Super::OnUnPossess();
}

/// The heir died: the game mode records it on the lineage (+1 generation, +1 skill point, commit) and brings up the lose screen
void AGodsentHeroController::OnHeroDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
		return;

	// Once per heir
	UnbindHeroDeath();

	if (AGodsentBaseGameMode* GameMode = AGodsentBaseGameMode::Get(this))
		GameMode->HandleHeroDefeated(this);

	// Keep the skill tree's copy in step with the lineage
	if (SkillTreeComponent)
		SkillTreeComponent->LoadFromLineage();
}

void AGodsentHeroController::UnbindHeroDeath()
{
	if (UAbilitySystemComponent* HeroASC = BoundHeroASC.Get())
		HeroASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved).Remove(HeroDeadTagHandle);

	HeroDeadTagHandle.Reset();
	BoundHeroASC.Reset();
}

/// Opens the name prompt (paused). False if no Heir Name Widget Class is set.
bool AGodsentHeroController::ShowHeirNamePrompt()
{
	if (!HeirNameWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Lineage] BP_HeroController has no Heir Name Widget Class, skipping the name prompt"));
		return false;
	}

	RemoveWidget(HeirNameWidget);
	HeirNameWidget = CreateWidget<UUserWidget>(this, HeirNameWidgetClass);

	if (!HeirNameWidget)
		return false;

	ShowMenuWidget(HeirNameWidget, true);
	return true;
}

/// Called by the name widget's confirm button
bool AGodsentHeroController::SubmitHeirName(const FString& ProposedName, FText& OutReason)
{
	UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this);

	if (!Lineage || !Lineage->SetHeirName(ProposedName, OutReason))
		return false;

	RemoveWidget(HeirNameWidget);

	if (AGodsentBaseGameMode* GameMode = AGodsentBaseGameMode::Get(this))
		GameMode->HandleHeirNamed(this);
	else
		ResumeGameplay();

	return true;
}

/// Opens the rebirth skill tree (paused). False if no Rebirth Skill Tree Widget Class is set.
bool AGodsentHeroController::ShowRebirthSkillTree()
{
	if (!RebirthSkillTreeWidgetClass)
		return false;

	RemoveWidget(RebirthSkillTreeWidget);
	RebirthSkillTreeWidget = CreateWidget<UUserWidget>(this, RebirthSkillTreeWidgetClass);

	if (!RebirthSkillTreeWidget)
		return false;

	ShowMenuWidget(RebirthSkillTreeWidget, true);
	return true;
}

/// Called by the rebirth skill tree's Continue button
void AGodsentHeroController::CloseRebirthSkillTree()
{
	RemoveWidget(RebirthSkillTreeWidget);

	if (AGodsentBaseGameMode* GameMode = AGodsentBaseGameMode::Get(this))
		GameMode->HandleRebirthSkillsChosen(this);
	else
		ResumeGameplay();
}

/// Shows the lose screen (not paused, so the world carries on behind it)
void AGodsentHeroController::ShowLoseScreen()
{
	if (!LoseScreenWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Lineage] BP_HeroController has no Lose Screen Widget Class"));
		return;
	}

	RemoveWidget(EndScreenWidget);
	EndScreenWidget = CreateWidget<UUserWidget>(this, LoseScreenWidgetClass);

	if (EndScreenWidget)
		ShowMenuWidget(EndScreenWidget, false);
}

/// Shows the win screen (not paused)
void AGodsentHeroController::ShowWinScreen()
{
	if (!WinScreenWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Lineage] BP_HeroController has no Win Screen Widget Class"));
		return;
	}

	RemoveWidget(EndScreenWidget);
	EndScreenWidget = CreateWidget<UUserWidget>(this, WinScreenWidgetClass);

	if (EndScreenWidget)
		ShowMenuWidget(EndScreenWidget, false);
}

/// Adds a widget and gives it the mouse; pauses the game clock too if bPause (not the pause menu)
void AGodsentHeroController::ShowMenuWidget(UUserWidget* Widget, bool bPause)
{
	Widget->AddToViewport(RebirthWidgetZOrder);

	if (bPause)
		SetPause(true);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Widget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(false); // Console rule: the cursor is never shown (UI-03)
}

void AGodsentHeroController::RemoveWidget(TObjectPtr<UUserWidget>& Widget)
{
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
}

/// Unpauses and gives input back to the game
void AGodsentHeroController::ResumeGameplay()
{
	SetPause(false);
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}

/// Stops (or restores) the heir's movement, camera and ability input (all bound on the hero's input component)
void AGodsentHeroController::SetHeroInputBlocked(bool bBlocked)
{
	APawn* HeroPawn = GetPawn();

	if (!HeroPawn)
		return;

	if (bBlocked)
		HeroPawn->DisableInput(this);
	else
		HeroPawn->EnableInput(this);
}

FGenericTeamId AGodsentHeroController::GetGenericTeamId() const
{
	return HeroTeamId;
}
