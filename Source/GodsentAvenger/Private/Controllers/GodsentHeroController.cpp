// Alex Goldberg rights reserved

#include "Controllers/GodsentHeroController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "Components/SkillTree/SkillTreeComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameModes/GodsentBaseGameMode.h"
#include "GodsentGameplayTags.h"
#include "Subsystems/GodsentLineageSubsystem.h"

AGodsentHeroController::AGodsentHeroController()
{
	HeroTeamId = FGenericTeamId(0);

	SkillTreeComponent = CreateDefaultSubobject<USkillTreeComponent>(TEXT("SkillTreeComponent"));
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

/// Debug: wipes the lineage and reloads the map
void AGodsentHeroController::GodsentNewGame()
{
	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this))
		Lineage->StartNewGameAndOpenLevel(this);
}

/// Debug: sets the generation
void AGodsentHeroController::GodsentSetGeneration(int32 NewGeneration)
{
	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this))
	{
		Lineage->DebugSetGeneration(NewGeneration);
		UE_LOG(LogTemp, Warning, TEXT("GodsentSetGeneration: generation %d, enemy tier %d (applies to enemies spawned from now on)"),
			Lineage->GetLineageData().Generation, Lineage->GetEnemyTier());
	}
}

/// Debug: prints the lineage data
void AGodsentHeroController::GodsentPrintLineage()
{
	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this))
		Lineage->LogLineage(); // Prints the whole struct without saving
}

/// Debug: adds skill points
void AGodsentHeroController::GodsentGrantSkillPoints(int32 Amount)
{
	if (SkillTreeComponent)
		SkillTreeComponent->AddSkillPoints(Amount);

	UE_LOG(LogTemp, Warning, TEXT("GodsentGrantSkillPoints: now %d"), SkillTreeComponent ? SkillTreeComponent->GetSkillPoints() : 0);
}

/// Debug: unlocks a skill and its earlier tiers for free
void AGodsentHeroController::GodsentUnlockSkill(FName SkillTagName)
{
	const FGameplayTag SkillTag = FGameplayTag::RequestGameplayTag(SkillTagName, false);
	const bool bUnlocked = SkillTag.IsValid() && SkillTreeComponent && SkillTreeComponent->ForceUnlockSkill(SkillTag);

	UE_LOG(LogTemp, Warning, TEXT("GodsentUnlockSkill: %s %s"), *SkillTagName.ToString(),
		bUnlocked ? TEXT("unlocked") : TEXT("not found, not built yet, or no skill tree data"));
}

FGenericTeamId AGodsentHeroController::GetGenericTeamId() const
{
	return HeroTeamId;
}

/// Console command for testing skill tiers until the skill tree exists
void AGodsentHeroController::GodsentSetAbilityLevel(FName AbilityTagName, int32 NewLevel)
{
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = Cast<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()));

	if (!AbilityTag.IsValid() || !ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("GodsentSetAbilityLevel: unknown tag %s or no ability system"), *AbilityTagName.ToString());
		return;
	}

	const bool bFound = ASC->SetAbilityLevelByTag(AbilityTag, NewLevel);
	UE_LOG(LogTemp, Warning, TEXT("GodsentSetAbilityLevel: %s -> level %d (%s)"), *AbilityTagName.ToString(), NewLevel, bFound ? TEXT("set") : TEXT("ability not granted"));
}

/// Grants an ability from DebugGrantableAbilities by its Asset Tag, or sets its level if it's already granted
void AGodsentHeroController::GodsentGrantAbility(FName AbilityTagName, int32 Level)
{
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = Cast<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()));

	if (!AbilityTag.IsValid() || !ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("GodsentGrantAbility: unknown tag %s or no ability system"), *AbilityTagName.ToString());
		return;
	}

	Level = FMath::Max(Level, 1);

	// Already granted: just change the tier
	if (ASC->GetAbilityLevelByTag(AbilityTag) > 0)
	{
		ASC->SetAbilityLevelByTag(AbilityTag, Level);
		UE_LOG(LogTemp, Warning, TEXT("GodsentGrantAbility: %s already granted, level set to %d"), *AbilityTagName.ToString(), Level);
		return;
	}

	const FGodsentHeroAbilitySet* Entry = DebugGrantableAbilities.FindByPredicate([&AbilityTag](const FGodsentHeroAbilitySet& AbilitySet)
	{
		return AbilitySet.AbilityToGrant && AbilitySet.AbilityToGrant->GetDefaultObject<UGameplayAbility>()->GetAssetTags().HasTagExact(AbilityTag);
	});

	if (!Entry)
	{
		UE_LOG(LogTemp, Warning, TEXT("GodsentGrantAbility: %s isn't in BP_HeroController's Debug Grantable Abilities"), *AbilityTagName.ToString());
		return;
	}

	if (Entry->InputTag.IsValid())
	{
		ASC->GrantHeroAbility(*Entry, Level); // Input-bound (Heal, Dodge, Block...)
	}
	else
	{
		// No input: passive / OnGiven abilities (Light activates itself as soon as it's given)
		FGameplayAbilitySpec AbilitySpec(Entry->AbilityToGrant, Level);
		AbilitySpec.SourceObject = GetPawn();
		ASC->GiveAbility(AbilitySpec);
	}

	UE_LOG(LogTemp, Warning, TEXT("GodsentGrantAbility: granted %s at level %d"), *AbilityTagName.ToString(), Level);
}

/// Removes every granted ability with this Asset Tag
void AGodsentHeroController::GodsentRemoveAbility(FName AbilityTagName)
{
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = Cast<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()));

	if (!AbilityTag.IsValid() || !ASC)
		return;

	TArray<FGameplayAbilitySpecHandle> HandlesToRemove;

	for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.Ability->GetAssetTags().HasTagExact(AbilityTag))
			HandlesToRemove.Add(AbilitySpec.Handle);
	}

	for (const FGameplayAbilitySpecHandle& Handle : HandlesToRemove)
		ASC->ClearAbility(Handle); // Active ones (Light) are ended first, so OnEndAbility still runs

	UE_LOG(LogTemp, Warning, TEXT("GodsentRemoveAbility: removed %d ability(s) with %s"), HandlesToRemove.Num(), *AbilityTagName.ToString());
}
