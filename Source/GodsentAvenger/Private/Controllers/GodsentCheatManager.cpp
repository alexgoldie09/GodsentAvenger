// Alex Goldberg rights reserved

#include "Controllers/GodsentCheatManager.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "AbilitySystem/GodsentAttributeSet.h"
#include "Components/SkillTree/SkillTreeComponent.h"
#include "Controllers/GodsentHeroController.h"
#include "GameplayTagContainer.h"
#include "GodsentGameplayTags.h"
#include "Subsystems/GodsentLineageSubsystem.h"
#include "Components/Aldegon/GodsentAldegonComponent.h"
#include "Components/Flight/GodsentFlightComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"

namespace
{
	/// Knock Down cheat: a sideways shove with a little lift, like a Heal knockback
	constexpr float AldegonCheatKnockbackStrength = 600.f;
	constexpr float AldegonCheatKnockbackUpForce = 200.f;

	/// Hurt cheat: health as a percentage, and it never kills (leaves him on 1)
	constexpr float AldegonCheatFullHealthPercent = 100.f;
	constexpr float AldegonCheatMinHealth = 1.f;
}

AGodsentHeroController* UGodsentCheatManager::GetHeroController() const
{
	return Cast<AGodsentHeroController>(GetOuterAPlayerController());
}

UGodsentAbilitySystemComponent* UGodsentCheatManager::GetHeroASC() const
{
	const APlayerController* PlayerController = GetOuterAPlayerController();
	return PlayerController ? Cast<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerController->GetPawn())) : nullptr;
}

void UGodsentCheatManager::GodsentNewGame()
{
	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(GetOuterAPlayerController()))
		Lineage->StartNewGameAndOpenLevel(GetOuterAPlayerController());
}

void UGodsentCheatManager::GodsentSetGeneration(int32 NewGeneration)
{
	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(GetOuterAPlayerController()))
	{
		Lineage->DebugSetGeneration(NewGeneration);
		UE_LOG(LogTemp, Warning, TEXT("GodsentSetGeneration: generation %d, enemy tier %d (applies to enemies spawned from now on)"),
			Lineage->GetLineageData().Generation, Lineage->GetEnemyTier());
	}
}

void UGodsentCheatManager::GodsentPrintLineage()
{
	if (const UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(GetOuterAPlayerController()))
		Lineage->LogLineage();
}

void UGodsentCheatManager::GodsentResetRevival()
{
	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(GetOuterAPlayerController()))
		Lineage->DebugResetHeirRevival();

	UE_LOG(LogTemp, Warning, TEXT("GodsentResetRevival: the current heir can revive again"));
}

void UGodsentCheatManager::GodsentGrantSkillPoints(int32 Amount)
{
	const AGodsentHeroController* HeroController = GetHeroController();
	USkillTreeComponent* SkillTree = HeroController ? HeroController->GetSkillTreeComponent() : nullptr;

	if (SkillTree)
		SkillTree->AddSkillPoints(Amount);

	UE_LOG(LogTemp, Warning, TEXT("GodsentGrantSkillPoints: now %d"), SkillTree ? SkillTree->GetSkillPoints() : 0);
}

void UGodsentCheatManager::GodsentUnlockSkill(FName SkillTagName)
{
	const AGodsentHeroController* HeroController = GetHeroController();
	USkillTreeComponent* SkillTree = HeroController ? HeroController->GetSkillTreeComponent() : nullptr;

	const FGameplayTag SkillTag = FGameplayTag::RequestGameplayTag(SkillTagName, false);
	const bool bUnlocked = SkillTag.IsValid() && SkillTree && SkillTree->ForceUnlockSkill(SkillTag);

	UE_LOG(LogTemp, Warning, TEXT("GodsentUnlockSkill: %s %s"), *SkillTagName.ToString(),
		bUnlocked ? TEXT("unlocked") : TEXT("not found, not built yet, or no skill tree data"));
}

void UGodsentCheatManager::GodsentSetAbilityLevel(FName AbilityTagName, int32 NewLevel)
{
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = GetHeroASC();

	if (!AbilityTag.IsValid() || !ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("GodsentSetAbilityLevel: unknown tag %s or no ability system"), *AbilityTagName.ToString());
		return;
	}

	const bool bFound = ASC->SetAbilityLevelByTag(AbilityTag, NewLevel);
	UE_LOG(LogTemp, Warning, TEXT("GodsentSetAbilityLevel: %s -> level %d (%s)"), *AbilityTagName.ToString(), NewLevel, bFound ? TEXT("set") : TEXT("ability not granted"));
}

void UGodsentCheatManager::GodsentGrantAbility(FName AbilityTagName, int32 Level)
{
	const AGodsentHeroController* HeroController = GetHeroController();
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = GetHeroASC();

	if (!HeroController || !AbilityTag.IsValid() || !ASC)
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

	const FGodsentHeroAbilitySet* Entry = HeroController->GetDebugGrantableAbilities().FindByPredicate([&AbilityTag](const FGodsentHeroAbilitySet& AbilitySet)
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
		AbilitySpec.SourceObject = HeroController->GetPawn();
		ASC->GiveAbility(AbilitySpec);
	}

	UE_LOG(LogTemp, Warning, TEXT("GodsentGrantAbility: granted %s at level %d"), *AbilityTagName.ToString(), Level);
}

void UGodsentCheatManager::GodsentRemoveAbility(FName AbilityTagName)
{
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	UGodsentAbilitySystemComponent* ASC = GetHeroASC();

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
// ---- Aldegon ----

UGodsentAldegonComponent* UGodsentCheatManager::FindAldegon(const TCHAR* CommandName) const
{
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
	{
		if (UGodsentAldegonComponent* Aldegon = It->FindComponentByClass<UGodsentAldegonComponent>())
			return Aldegon;
	}

	UE_LOG(LogTemp, Warning, TEXT("%s: no Aldegon in this level"), CommandName);
	return nullptr;
}

void UGodsentCheatManager::LogAldegonCommand(const TCHAR* CommandName, bool bAccepted, const UGodsentAldegonComponent* Aldegon) const
{
	const FString StateName = Aldegon ? UEnum::GetDisplayValueAsText(Aldegon->GetAldegonState()).ToString() : TEXT("?");

	UE_LOG(LogTemp, Warning, TEXT("%s: %s (Aldegon is %s)"), CommandName,
		bAccepted ? TEXT("ok") : TEXT("ignored, wrong state or nowhere to go"), *StateName);
}

void UGodsentCheatManager::GodsentAldegonWake()
{
	if (UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonWake")))
		LogAldegonCommand(TEXT("GodsentAldegonWake"), Aldegon->Wake(), Aldegon);
}

void UGodsentCheatManager::GodsentAldegonTakeOff()
{
	if (UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonTakeOff")))
		LogAldegonCommand(TEXT("GodsentAldegonTakeOff"), Aldegon->TakeOff(), Aldegon);
}

void UGodsentCheatManager::GodsentAldegonFlyToPlayer()
{
	UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonFlyToPlayer"));
	const APawn* PlayerPawn = GetOuterAPlayerController()->GetPawn();

	if (Aldegon && PlayerPawn)
		LogAldegonCommand(TEXT("GodsentAldegonFlyToPlayer"), Aldegon->FlyTo(PlayerPawn->GetActorLocation()), Aldegon);
}

void UGodsentCheatManager::GodsentAldegonFlyRandom(float Radius)
{
	UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonFlyRandom"));
	const APawn* PlayerPawn = GetOuterAPlayerController()->GetPawn();
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!Aldegon || !PlayerPawn || !NavSystem)
		return;

	FNavLocation RandomSpot;
	const bool bFound = NavSystem->GetRandomReachablePointInRadius(PlayerPawn->GetActorLocation(), Radius, RandomSpot);

	LogAldegonCommand(TEXT("GodsentAldegonFlyRandom"), bFound && Aldegon->FlyTo(RandomSpot.Location), Aldegon);
}

void UGodsentCheatManager::GodsentAldegonLand()
{
	if (UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonLand")))
		LogAldegonCommand(TEXT("GodsentAldegonLand"), Aldegon->LandAt(Aldegon->GetOwner()->GetActorLocation()), Aldegon);
}

void UGodsentCheatManager::GodsentAldegonLandNearPlayer(float Distance)
{
	UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonLandNearPlayer"));
	const APawn* PlayerPawn = GetOuterAPlayerController()->GetPawn();

	if (!Aldegon || !PlayerPawn)
		return;

	// On Aldegon's side of the player
	FVector FromPlayer = Aldegon->GetOwner()->GetActorLocation() - PlayerPawn->GetActorLocation();
	FromPlayer.Z = 0.f;

	const FVector Spot = PlayerPawn->GetActorLocation() + FromPlayer.GetSafeNormal() * Distance;
	LogAldegonCommand(TEXT("GodsentAldegonLandNearPlayer"), Aldegon->LandAt(Spot), Aldegon);
}

void UGodsentCheatManager::GodsentAldegonReturnHome()
{
	if (UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonReturnHome")))
		LogAldegonCommand(TEXT("GodsentAldegonReturnHome"), Aldegon->ReturnHome(), Aldegon);
}

void UGodsentCheatManager::GodsentAldegonKnockDown()
{
	UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonKnockDown"));
	ACharacter* AldegonCharacter = Aldegon ? Cast<ACharacter>(Aldegon->GetOwner()) : nullptr;
	const APawn* PlayerPawn = GetOuterAPlayerController()->GetPawn();

	if (!AldegonCharacter || !PlayerPawn)
		return;

	// Away from the player, like Heal's knockback
	FVector Away = AldegonCharacter->GetActorLocation() - PlayerPawn->GetActorLocation();
	Away.Z = 0.f;

	AldegonCharacter->LaunchCharacter(Away.GetSafeNormal() * AldegonCheatKnockbackStrength + FVector(0.f, 0.f, AldegonCheatKnockbackUpForce), true, true);
	LogAldegonCommand(TEXT("GodsentAldegonKnockDown"), true, Aldegon);
}

void UGodsentCheatManager::GodsentAldegonFacePlayer(bool bFacePlayer)
{
	if (UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonFacePlayer")))
	{
		Aldegon->SetFacingTarget(bFacePlayer ? GetOuterAPlayerController()->GetPawn() : nullptr);
		LogAldegonCommand(TEXT("GodsentAldegonFacePlayer"), true, Aldegon);
	}
}

void UGodsentCheatManager::GodsentAldegonState()
{
	UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonState"));

	if (!Aldegon)
		return;

	const UGodsentFlightComponent* Flight = Aldegon->GetOwner()->FindComponentByClass<UGodsentFlightComponent>();

	UE_LOG(LogTemp, Warning, TEXT("GodsentAldegonState: %s, flying %s, flight action %s, capsule %.0f cm off the ground"),
		*UEnum::GetDisplayValueAsText(Aldegon->GetAldegonState()).ToString(),
		Aldegon->IsFlying() ? TEXT("yes") : TEXT("no"),
		Flight ? *UEnum::GetDisplayValueAsText(Flight->GetCurrentAction()).ToString() : TEXT("?"),
		Flight ? Flight->GetHeightAboveGround() : 0.f);

	UE_LOG(LogTemp, Warning, TEXT("    Fight %s, health %.0f%%, lift off pending %s, forced landing %s, claws %d, adds %d/%d, totems %d/%d, frenzy %s, %.1fs in state"),
		Aldegon->IsFightActive() ? TEXT("on") : TEXT("off"),
		Aldegon->GetHealthPercent(),
		Aldegon->IsLiftOffPending() ? TEXT("yes") : TEXT("no"),
		Aldegon->IsForcedLandingPending() ? TEXT("yes") : TEXT("no"),
		Aldegon->GetConsecutiveClaws(),
		Aldegon->GetLivingSummonCount(EGodsentAldegonSummonType::Add), Aldegon->GetMaxSummons(EGodsentAldegonSummonType::Add),
		Aldegon->GetLivingSummonCount(EGodsentAldegonSummonType::Totem), Aldegon->GetMaxSummons(EGodsentAldegonSummonType::Totem),
		Aldegon->IsFrenzied() ? TEXT("yes") : TEXT("no"),
		Aldegon->GetTimeInState());
}

void UGodsentCheatManager::GodsentAldegonStartFight()
{
	if (UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonStartFight")))
	{
		Aldegon->StartFight(GetOuterAPlayerController()->GetPawn());
		LogAldegonCommand(TEXT("GodsentAldegonStartFight"), Aldegon->IsFightActive(), Aldegon);
	}
}

void UGodsentCheatManager::GodsentAldegonEndFight()
{
	if (UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonEndFight")))
	{
		Aldegon->EndFight();
		LogAldegonCommand(TEXT("GodsentAldegonEndFight"), !Aldegon->IsFightActive(), Aldegon);
	}
}

void UGodsentCheatManager::GodsentAldegonHurt(float Percent)
{
	UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonHurt"));
	UAbilitySystemComponent* ASC = Aldegon ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Aldegon->GetOwner()) : nullptr;

	if (!ASC)
		return;

	// Straight off his health (no defence, no hit react), the same way a health drain would
	const float MaxHealth = ASC->GetNumericAttribute(UGodsentAttributeSet::GetMaxHealthAttribute());
	const float CurrentHealth = ASC->GetNumericAttribute(UGodsentAttributeSet::GetCurrentHealthAttribute());
	const float NewHealth = FMath::Max(CurrentHealth - MaxHealth * Percent / AldegonCheatFullHealthPercent, AldegonCheatMinHealth);

	ASC->SetNumericAttributeBase(UGodsentAttributeSet::GetCurrentHealthAttribute(), NewHealth);
	UE_LOG(LogTemp, Warning, TEXT("GodsentAldegonHurt: now %.0f%%"), Aldegon->GetHealthPercent());
}

// ---- Aldegon summons (Stage 20) ----

void UGodsentCheatManager::TryAldegonAbility(const TCHAR* CommandName, const FGameplayTag& AbilityTag) const
{
	UGodsentAldegonComponent* Aldegon = FindAldegon(CommandName);
	UAbilitySystemComponent* ASC = Aldegon ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Aldegon->GetOwner()) : nullptr;

	if (!ASC)
		return;

	const bool bActivated = ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());

	UE_LOG(LogTemp, Warning, TEXT("%s: %s (adds %d/%d, totems %d/%d)"), CommandName,
		bActivated ? TEXT("cast") : TEXT("refused (cooldown, already casting, or blocked)"),
		Aldegon->GetLivingSummonCount(EGodsentAldegonSummonType::Add), Aldegon->GetMaxSummons(EGodsentAldegonSummonType::Add),
		Aldegon->GetLivingSummonCount(EGodsentAldegonSummonType::Totem), Aldegon->GetMaxSummons(EGodsentAldegonSummonType::Totem));
}

void UGodsentCheatManager::GodsentAldegonSummon()
{
	TryAldegonAbility(TEXT("GodsentAldegonSummon"), GodsentGameplayTags::Enemy_Ability_Aldegon_Summon);
}

void UGodsentCheatManager::GodsentAldegonSiphon()
{
	TryAldegonAbility(TEXT("GodsentAldegonSiphon"), GodsentGameplayTags::Enemy_Ability_Aldegon_LifeSiphon);
}

void UGodsentCheatManager::GodsentAldegonKillSummons()
{
	if (UGodsentAldegonComponent* Aldegon = FindAldegon(TEXT("GodsentAldegonKillSummons")))
	{
		Aldegon->KillAllSummons();
		UE_LOG(LogTemp, Warning, TEXT("GodsentAldegonKillSummons: done"));
	}
}
