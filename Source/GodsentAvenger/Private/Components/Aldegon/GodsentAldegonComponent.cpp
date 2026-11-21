// Alex Goldberg rights reserved

#include "Components/Aldegon/GodsentAldegonComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/GodsentEnemyCharacter.h"
#include "Components/Flight/GodsentFlightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/Ghost/GodsentHoverComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GodsentGameplayTags.h"
#include "AbilitySystem/GodsentAttributeSet.h"
#include "Abilities/GameplayAbility.h"
#include "Controllers/GodsentAIController.h"
#include "GameplayEffectTypes.h"
#include "GodsentFunctionLibrary.h"
#include "TimerManager.h"

namespace
{
	/// Stats are read at level 1 if there's no enemy level to use
	constexpr float AldegonDefaultStatLevel = 1.f;

	/// Sitting down starts this close to the end of Rise (exactly at the end can count as already finished)
	constexpr float AldegonRiseEndInset = 0.01f;

	/// Health as a percentage
	constexpr float AldegonFullHealthPercent = 100.f;

	/// Ground Trigger falls back to this walk speed if the target has no movement or MoveSpeed to read (the hero's default)
	constexpr float AldegonFallbackTargetWalkSpeed = 400.f;
}

UGodsentAldegonComponent::UGodsentAldegonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// A Soul Beam or Death Wave breaks a claw streak
	ClawAbilityTag = GodsentGameplayTags::Enemy_Ability_Aldegon_Claw;
	SummonAbilityTag = GodsentGameplayTags::Enemy_Ability_Aldegon_Summon;
	CancelOnFightEndTags.AddTag(GodsentGameplayTags::Enemy_Ability_Aldegon);
	LifeSiphonAbilityTag = GodsentGameplayTags::Enemy_Ability_Aldegon_LifeSiphon;
	ClawResetAbilityTags.AddTag(GodsentGameplayTags::Enemy_Ability_Aldegon_SoulBeam);
	ClawResetAbilityTags.AddTag(GodsentGameplayTags::Enemy_Ability_Aldegon_DeathWave);
}

void UGodsentAldegonComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: GodsentAldegonComponent needs a Character owner"), *GetNameSafe(GetOwner()));
		return;
	}

	FlightComponent = OwnerCharacter->FindComponentByClass<UGodsentFlightComponent>();
	HoverComponent = OwnerCharacter->FindComponentByClass<UGodsentHoverComponent>();

	if (!FlightComponent || !HoverComponent)
		UE_LOG(LogTemp, Warning, TEXT("%s: Aldegon needs a Godsent Flight component and a Godsent Hover component"), *GetNameSafe(OwnerCharacter));

	// The throne: where he was placed in the level
	HomeLocation = OwnerCharacter->GetActorLocation();
	HomeRotation = OwnerCharacter->GetActorRotation();

	if (FlightComponent)
		FlightComponent->OnFlightActionFinished.AddUniqueDynamic(this, &ThisClass::OnFlightActionFinished);

	OwnerCharacter->MovementModeChangedDelegate.AddUniqueDynamic(this, &ThisClass::OnMovementModeChanged);

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerCharacter))
	{
		DeadTagDelegateHandle = ASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::OnDeadTagChanged);

		HealthChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGodsentAttributeSet::GetCurrentHealthAttribute())
			.AddUObject(this, &ThisClass::OnHealthChanged);

		AbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnAbilityEnded);
	}

	ApplyStats();

	// Starts sat on the throne: no float
	if (HoverComponent)
		HoverComponent->SetHoverActive(false, true);

	SetState(EGodsentAldegonState::Dormant);
}

void UGodsentAldegonComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RiseTimerHandle);
		World->GetTimerManager().ClearTimer(HoverTimerHandle);
	}

	if (OwnerCharacter)
	{
		OwnerCharacter->MovementModeChangedDelegate.RemoveDynamic(this, &ThisClass::OnMovementModeChanged);

		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerCharacter))
		{
			ASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved).Remove(DeadTagDelegateHandle);
			ASC->GetGameplayAttributeValueChangeDelegate(UGodsentAttributeSet::GetCurrentHealthAttribute()).Remove(HealthChangedDelegateHandle);
			ASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
		}
	}

	if (FlightComponent)
		FlightComponent->OnFlightActionFinished.RemoveDynamic(this, &ThisClass::OnFlightActionFinished);

	Super::EndPlay(EndPlayReason);
}

void UGodsentAldegonComponent::ApplyStats()
{
	const float GroundedHeightValue = ReadStat(GroundedHeight, TEXT("Grounded Height"));

	if (HoverComponent)
		HoverComponent->SetHoverHeight(GroundedHeightValue);

	if (FlightComponent)
	{
		// The float is on top of the capsule's height, so the capsule flies that much lower to put his feet at Airborne Height
		FlightComponent->SetCruiseHeight(ReadStat(AirborneHeight, TEXT("Airborne Height")) - GroundedHeightValue);
		FlightComponent->SetFlightSpeeds(ReadStat(FlySpeed, TEXT("Fly Speed")), ReadStat(ClimbSpeed, TEXT("Climb Speed")),
			ReadStat(DescentSpeed, TEXT("Descent Speed")), ReadStat(FlyTurnRate, TEXT("Fly Turn Rate")));
	}
}

float UGodsentAldegonComponent::ReadStat(const FScalableFloat& Stat, const TCHAR* StatName) const
{
	// Not linked to the curve table: still works (Value on its own) but say so once
	if (!Stat.Curve.CurveTable && !WarnedUnlinkedStats.Contains(StatName))
	{
		WarnedUnlinkedStats.Add(StatName);
		UE_LOG(LogTemp, Warning, TEXT("%s: Aldegon stat '%s' isn't linked to CT_AldegonStats, using %.2f"),
			*GetNameSafe(GetOwner()), StatName, Stat.Value);
	}

	return Stat.GetValueAtLevel(GetStatLevel());
}

float UGodsentAldegonComponent::GetStatLevel() const
{
	if (const AGodsentEnemyCharacter* Enemy = Cast<AGodsentEnemyCharacter>(OwnerCharacter))
		return Enemy->GetAppliedEnemyLevel();

	return AldegonDefaultStatLevel;
}

// ---- Commands ----

bool UGodsentAldegonComponent::Wake()
{
	if (CurrentState != EGodsentAldegonState::Dormant || !OwnerCharacter)
		return false;

	ApplyStats();

	// Sat with no movement physics after a return home: back to normal walking
	if (OwnerCharacter->GetCharacterMovement()->MovementMode == MOVE_None)
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	SetState(EGodsentAldegonState::Waking);

	const float RiseLength = PlayRise(false);

	// No Rise montage: float up and take off straight away
	if (RiseLength <= 0.f)
	{
		if (HoverComponent)
			HoverComponent->SetHoverActive(true);

		OnRiseFinished();
		return true;
	}

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();

	// Start floating partway through standing up, then take off once he's up
	TimerManager.SetTimer(HoverTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (HoverComponent)
			HoverComponent->SetHoverActive(true);
	}), FMath::Max(RiseLength * RiseHoverStart, KINDA_SMALL_NUMBER), false);

	TimerManager.SetTimer(RiseTimerHandle, FTimerDelegate::CreateUObject(this, &ThisClass::OnRiseFinished), RiseLength, false);
	return true;
}

bool UGodsentAldegonComponent::TakeOff()
{
	if (CurrentState != EGodsentAldegonState::Grounded || !FlightComponent)
		return false;

	SetState(EGodsentAldegonState::TakingOff);
	FlightComponent->TakeOff();
	return true;
}

bool UGodsentAldegonComponent::FlyTo(const FVector& Destination)
{
	if (CurrentState != EGodsentAldegonState::Airborne || !FlightComponent)
		return false;

	return FlightComponent->FlyTo(Destination);
}

bool UGodsentAldegonComponent::LandAt(const FVector& Spot)
{
	if (CurrentState != EGodsentAldegonState::Airborne || !FlightComponent)
		return false;

	if (!FlightComponent->LandAt(Spot))
		return false;

	SetState(EGodsentAldegonState::Landing);
	return true;
}

bool UGodsentAldegonComponent::ReturnHome()
{
	const bool bAwake = CurrentState != EGodsentAldegonState::Dormant && CurrentState != EGodsentAldegonState::SittingDown
		&& CurrentState != EGodsentAldegonState::ReturningHome && CurrentState != EGodsentAldegonState::Dead;

	if (!bAwake || !FlightComponent)
		return false;

	// Still getting up: stop that and go straight back
	GetWorld()->GetTimerManager().ClearTimer(RiseTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(HoverTimerHandle);

	if (CurrentState == EGodsentAldegonState::Waking)
	{
		if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
			AnimInstance->Montage_Stop(MontageStopBlendTime, RiseMontage);
	}

	if (HoverComponent)
		HoverComponent->SetHoverActive(true);

	if (!FlightComponent->LandAt(HomeLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Return Home failed, the throne isn't near the navmesh"), *GetNameSafe(OwnerCharacter));
		return false;
	}

	SetState(EGodsentAldegonState::ReturningHome);
	return true;
}

void UGodsentAldegonComponent::SetFacingTarget(AActor* NewFacingTarget)
{
	if (FlightComponent)
		FlightComponent->SetFacingTarget(NewFacingTarget);
}

bool UGodsentAldegonComponent::IsFlying() const
{
	return FlightComponent && FlightComponent->IsFlying();
}

// ---- Throne ----

float UGodsentAldegonComponent::PlayRise(bool bBackwards)
{
	UAnimInstance* AnimInstance = OwnerCharacter && OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;

	if (!RiseMontage || !AnimInstance)
		return 0.f;

	const float PlayRate = bBackwards ? SitDownPlayRate : RisePlayRate;

	if (AnimInstance->Montage_Play(RiseMontage, PlayRate) <= 0.f)
		return 0.f;

	// Backwards: start from the end and run to the start
	if (bBackwards)
	{
		AnimInstance->Montage_SetPosition(RiseMontage, FMath::Max(RiseMontage->GetPlayLength() - AldegonRiseEndInset, 0.f));
		AnimInstance->Montage_SetPlayRate(RiseMontage, -PlayRate);
	}

	// Real seconds it takes (the montage's own Rate Scale counts too)
	const float TotalRate = PlayRate * FMath::Max(RiseMontage->RateScale, KINDA_SMALL_NUMBER);
	return RiseMontage->GetPlayLength() / TotalRate;
}

void UGodsentAldegonComponent::OnRiseFinished()
{
	if (CurrentState != EGodsentAldegonState::Waking)
		return;

	// Up off the throne: now he's really standing (well, floating) on the floor, so take off from there
	SetState(EGodsentAldegonState::Grounded);
	TakeOff();
}

void UGodsentAldegonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bTurningToHome)
		TickTurnToHome(DeltaTime);
	else
		SetComponentTickEnabled(false);
}

void UGodsentAldegonComponent::TickTurnToHome(float DeltaTime)
{
	// Glide onto the throne (the landing spot is the nearest navmesh, a little off the seat) while turning to face out
	const FVector NewLocation = FMath::VInterpConstantTo(OwnerCharacter->GetActorLocation(), HomeLocation, DeltaTime, HomeSlideSpeed);
	const FRotator NewRotation = FMath::RInterpConstantTo(OwnerCharacter->GetActorRotation(), FRotator(0.f, HomeRotation.Yaw, 0.f), DeltaTime, HomeTurnRate);

	OwnerCharacter->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const bool bInPlace = FVector::Dist(NewLocation, HomeLocation) <= HomeArriveDistance;
	const bool bLinedUp = NewRotation.Equals(FRotator(0.f, HomeRotation.Yaw, 0.f), HomeFacingTolerance);

	if (!bInPlace || !bLinedUp)
		return;

	// In place: snap the last bit and sit down
	bTurningToHome = false;
	SetComponentTickEnabled(false);

	OwnerCharacter->SetActorLocationAndRotation(HomeLocation, HomeRotation, false, nullptr, ETeleportType::TeleportPhysics);
	SetState(EGodsentAldegonState::SittingDown);

	const float SitDownLength = PlayRise(true);

	if (SitDownLength <= 0.f)
	{
		if (HoverComponent)
			HoverComponent->SetHoverActive(false);

		OnSitDownFinished();
		return;
	}

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();

	TimerManager.SetTimer(HoverTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (HoverComponent)
			HoverComponent->SetHoverActive(false);
	}), FMath::Max(SitDownLength * SitDownSettleAt, KINDA_SMALL_NUMBER), false);

	TimerManager.SetTimer(RiseTimerHandle, FTimerDelegate::CreateUObject(this, &ThisClass::OnSitDownFinished), SitDownLength, false);
}

void UGodsentAldegonComponent::OnSitDownFinished()
{
	if (CurrentState != EGodsentAldegonState::SittingDown)
		return;

	if (bHealOnReturnHome)
		HealToFull();

	ResetFightCounters();
	SetState(EGodsentAldegonState::Dormant);

	// The player came back in while he was sitting down: straight back up
	if (bFightActive)
		Wake();
}

void UGodsentAldegonComponent::HealToFull()
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerCharacter);

	if (!ASC)
		return;

	// Also updates his health bar (the attribute set broadcasts the change)
	const float MaxHealth = ASC->GetNumericAttribute(UGodsentAttributeSet::GetMaxHealthAttribute());
	ASC->SetNumericAttributeBase(UGodsentAttributeSet::GetCurrentHealthAttribute(), MaxHealth);
}

// ---- Events ----

void UGodsentAldegonComponent::OnFlightActionFinished(EGodsentFlightAction Action, bool bSucceeded)
{
	// Knocked out of the air: On Movement Mode Changed handles it when he lands
	if (!bSucceeded)
		return;

	switch (CurrentState)
	{
	case EGodsentAldegonState::TakingOff:
		if (Action == EGodsentFlightAction::TakeOff)
			SetState(EGodsentAldegonState::Airborne);
		break;

	case EGodsentAldegonState::Landing:
		if (Action == EGodsentFlightAction::Land)
			SetState(EGodsentAldegonState::Grounded);
		break;

	case EGodsentAldegonState::ReturningHome:
		if (Action == EGodsentFlightAction::Land)
		{
			// No movement physics while he lines up and sits (so a raised seat can't drop him off it)
			OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
			OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_None);

			bTurningToHome = true;
			SetComponentTickEnabled(true);
		}
		break;

	default:
		break;
	}
}

void UGodsentAldegonComponent::OnMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (CurrentState == EGodsentAldegonState::Dead || !OwnerCharacter)
		return;

	// Knocked out of the air (e.g. Heal's knockback): he lands wherever he comes down
	const bool bLandedFromFall = PrevMovementMode == MOVE_Falling && OwnerCharacter->GetCharacterMovement()->IsMovingOnGround();
	const bool bWasUp = CurrentState == EGodsentAldegonState::Airborne || CurrentState == EGodsentAldegonState::TakingOff
		|| CurrentState == EGodsentAldegonState::Landing;

	if (bLandedFromFall && bWasUp)
		SetState(EGodsentAldegonState::Grounded);

	// Knocked down on the way home: carry on home
	if (bLandedFromFall && CurrentState == EGodsentAldegonState::ReturningHome && FlightComponent)
		FlightComponent->LandAt(HomeLocation);
}

void UGodsentAldegonComponent::OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0 || CurrentState == EGodsentAldegonState::Dead)
		return;

	GetWorld()->GetTimerManager().ClearTimer(RiseTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(HoverTimerHandle);
	bTurningToHome = false;

	// Falls out of the air and settles onto the floor
	if (FlightComponent)
		FlightComponent->DropToGround();

	if (HoverComponent)
		HoverComponent->SetHoverActive(false);

	SetState(EGodsentAldegonState::Dead);

	// His adds and totems die with him
	KillAllSummons();

	if (bFightActive)
	{
		bFightActive = false;
		OnFightEnded.Broadcast(EGodsentBossFightEnd::BossDefeated);
	}
}

void UGodsentAldegonComponent::SetState(EGodsentAldegonState NewState)
{
	if (NewState == CurrentState && NewState != EGodsentAldegonState::Dormant)
		return;

	const EGodsentAldegonState OldState = CurrentState;
	CurrentState = NewState;
	StateEnteredTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	switch (NewState)
	{
	case EGodsentAldegonState::TakingOff:
		// Going up deals with whatever made him lift off
		bLiftOffPending = false;
		ConsecutiveClaws = 0;
		break;

	case EGodsentAldegonState::Grounded:
		AirborneBandsCrossed = 0;
		break;

	default:
		break;
	}

	// Whatever he was told to face while flying up or down is done with
	if (NewState == EGodsentAldegonState::Airborne || NewState == EGodsentAldegonState::Grounded)
		SetFacingTarget(nullptr);

	if (bLogStateChanges)
	{
		UE_LOG(LogTemp, Log, TEXT("[Aldegon] %s -> %s"),
			*UEnum::GetDisplayValueAsText(OldState).ToString(), *UEnum::GetDisplayValueAsText(NewState).ToString());
	}

	OnAldegonStateChanged.Broadcast(NewState, OldState);
}

// ---- Fight ----

void UGodsentAldegonComponent::StartFight(AActor* NewChallenger)
{
	if (CurrentState == EGodsentAldegonState::Dead || !NewChallenger)
		return;

	const bool bWasActive = bFightActive;

	bFightActive = true;
	Challenger = NewChallenger;

	if (AGodsentAIController* AIController = OwnerCharacter ? Cast<AGodsentAIController>(OwnerCharacter->GetController()) : nullptr)
		AIController->SetTargetActor(NewChallenger);

	if (!bWasActive)
		OnFightStarted.Broadcast(NewChallenger);

	// Sitting down: On Sit Down Finished wakes him once he's settled
	if (CurrentState == EGodsentAldegonState::Dormant)
		Wake();
	else if (CurrentState == EGodsentAldegonState::ReturningHome)
		ResumeFromReturnHome();
}

void UGodsentAldegonComponent::EndFight()
{
	if (!bFightActive)
		return;

	bFightActive = false;
	Challenger.Reset();

	if (AGodsentAIController* AIController = OwnerCharacter ? Cast<AGodsentAIController>(OwnerCharacter->GetController()) : nullptr)
		AIController->ClearTargetActor();

	// Drop whatever move he's in the middle of before heading home
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerCharacter))
	{
		if (!CancelOnFightEndTags.IsEmpty())
			ASC->CancelAbilities(&CancelOnFightEndTags);
	}

	KillAllSummons();
	ReturnHome();

	OnFightEnded.Broadcast(EGodsentBossFightEnd::PlayerLeft);
}

void UGodsentAldegonComponent::ResumeFromReturnHome()
{
	if (CurrentState != EGodsentAldegonState::ReturningHome || !FlightComponent)
		return;

	// Still in the air: just climb back to fighting height
	if (FlightComponent->IsFlying())
	{
		SetState(EGodsentAldegonState::TakingOff);
		FlightComponent->TakeOff();
		return;
	}

	// Already lining up with the throne (no movement physics): stand back up and take off
	bTurningToHome = false;
	SetComponentTickEnabled(false);

	if (OwnerCharacter->GetCharacterMovement()->MovementMode == MOVE_None)
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	SetState(EGodsentAldegonState::Grounded);
	TakeOff();
}

void UGodsentAldegonComponent::ResetFightCounters()
{
	bLiftOffPending = false;
	AirborneBandsCrossed = 0;
	ConsecutiveClaws = 0;
	LastHealthPercent = GetHealthPercent();
	SetFrenzy(false);
}

void UGodsentAldegonComponent::SetFrenzy(bool bNewFrenzied)
{
	if (bFrenzied == bNewFrenzied)
		return;

	bFrenzied = bNewFrenzied;

	if (bFrenzied)
	{
		UGodsentFunctionLibrary::AddGameplayTagToActorIfNone(OwnerCharacter, GodsentGameplayTags::Enemy_Status_Frenzy);
		OnFrenzyStarted.Broadcast();
	}
	else
	{
		UGodsentFunctionLibrary::RemoveGameplayTagFromActorIfFound(OwnerCharacter, GodsentGameplayTags::Enemy_Status_Frenzy);
	}
}

void UGodsentAldegonComponent::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	const float NewPercent = GetHealthPercent();
	const float OldPercent = LastHealthPercent;
	LastHealthPercent = NewPercent;

	// Only counts once he's up and fighting (not the start-up stats filling his health in)
	const bool bAwake = CurrentState == EGodsentAldegonState::Grounded || CurrentState == EGodsentAldegonState::Landing
		|| CurrentState == EGodsentAldegonState::TakingOff || CurrentState == EGodsentAldegonState::Airborne;

	if (!bAwake || NewPercent >= OldPercent)
		return;

	int32 BandsCrossed = 0;

	for (const float Band : LiftOffHealthBands)
	{
		if (OldPercent > Band && NewPercent <= Band)
			++BandsCrossed;
	}

	if (BandsCrossed > 0)
	{
		const bool bOnTheGround = CurrentState == EGodsentAldegonState::Grounded || CurrentState == EGodsentAldegonState::Landing;

		// Grounded: time to go up. Airborne: count them, enough brings him down.
		if (bOnTheGround)
			bLiftOffPending = true;
		else
			AirborneBandsCrossed += BandsCrossed;
	}

	const float FrenzyAt = ReadStat(FrenzyAtHealthPercent, TEXT("Frenzy At Health Percent"));

	if (FrenzyAt > 0.f && NewPercent <= FrenzyAt && NewPercent > 0.f)
		SetFrenzy(true);
}

void UGodsentAldegonComponent::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	const UGameplayAbility* EndedAbility = AbilityEndedData.AbilityThatEnded;

	if (!EndedAbility || AbilityEndedData.bWasCancelled)
		return;

	const FGameplayTagContainer& AbilityTags = EndedAbility->GetAssetTags();

	if (ClawAbilityTag.IsValid() && AbilityTags.HasTag(ClawAbilityTag))
		++ConsecutiveClaws;
	else if (AbilityTags.HasAny(ClawResetAbilityTags))
		ConsecutiveClaws = 0;
}

// ---- What the behaviour tree reads ----

bool UGodsentAldegonComponent::IsForcedLandingPending() const
{
	const int32 BandsToLand = FMath::RoundToInt(ReadStat(AirborneBandsToLand, TEXT("Airborne Bands To Land")));
	return BandsToLand > 0 && AirborneBandsCrossed >= BandsToLand;
}

float UGodsentAldegonComponent::GetTimeInState() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() - StateEnteredTime : 0.f;
}

float UGodsentAldegonComponent::GetHealthPercent() const
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerCharacter);

	if (!ASC)
		return AldegonFullHealthPercent;

	const float MaxHealth = ASC->GetNumericAttribute(UGodsentAttributeSet::GetMaxHealthAttribute());

	if (MaxHealth <= 0.f)
		return AldegonFullHealthPercent;

	return ASC->GetNumericAttribute(UGodsentAttributeSet::GetCurrentHealthAttribute()) / MaxHealth * AldegonFullHealthPercent;
}

float UGodsentAldegonComponent::GetGroundTriggerDistance(const AActor* Target) const
{
	float WalkSpeed = AldegonFallbackTargetWalkSpeed;

	// The target's normal walking speed (MoveSpeed before sprint or buffs), so sprinting doesn't move the line
	if (const UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
	{
		const float BaseMoveSpeed = TargetASC->GetNumericAttributeBase(UGodsentAttributeSet::GetMoveSpeedAttribute());

		if (BaseMoveSpeed > 0.f)
			WalkSpeed = BaseMoveSpeed;
	}
	else if (const ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		WalkSpeed = TargetCharacter->GetCharacterMovement()->MaxWalkSpeed;
	}

	return ReadStat(GroundTriggerWalkSeconds, TEXT("Ground Trigger Walk Seconds")) * WalkSpeed;
}

float UGodsentAldegonComponent::GetResummonDistance() const
{
	return ReadStat(ResummonDistance, TEXT("Resummon Distance"));
}

float UGodsentAldegonComponent::GetMinAirborneTime() const
{
	return ReadStat(MinAirborneTime, TEXT("Min Airborne Time"));
}

int32 UGodsentAldegonComponent::GetMaxConsecutiveClaws() const
{
	return FMath::RoundToInt(ReadStat(MaxConsecutiveClaws, TEXT("Max Consecutive Claws")));
}

float UGodsentAldegonComponent::GetClawRange() const
{
	return ReadStat(ClawRange, TEXT("Claw Range"));
}

// ---- Summons ----

TArray<TWeakObjectPtr<AActor>>& UGodsentAldegonComponent::GetSummonList(EGodsentAldegonSummonType SummonType)
{
	return SummonType == EGodsentAldegonSummonType::Totem ? LivingTotems : LivingAdds;
}

const TArray<TWeakObjectPtr<AActor>>& UGodsentAldegonComponent::GetSummonList(EGodsentAldegonSummonType SummonType) const
{
	return SummonType == EGodsentAldegonSummonType::Totem ? LivingTotems : LivingAdds;
}

void UGodsentAldegonComponent::RegisterSummon(AActor* Summon, EGodsentAldegonSummonType SummonType)
{
	if (!IsValid(Summon))
		return;

	TArray<TWeakObjectPtr<AActor>>& SummonList = GetSummonList(SummonType);

	// Drop the dead while we're here
	SummonList.RemoveAll([](const TWeakObjectPtr<AActor>& Entry)
	{
		return !Entry.IsValid() || UGodsentFunctionLibrary::NativeDoesActorHaveTag(Entry.Get(), GodsentGameplayTags::Shared_Status_Dead);
	});

	SummonList.AddUnique(Summon);
}

int32 UGodsentAldegonComponent::GetLivingSummonCount(EGodsentAldegonSummonType SummonType) const
{
	int32 LivingCount = 0;

	for (const TWeakObjectPtr<AActor>& Entry : GetSummonList(SummonType))
	{
		if (Entry.IsValid() && !UGodsentFunctionLibrary::NativeDoesActorHaveTag(Entry.Get(), GodsentGameplayTags::Shared_Status_Dead))
			++LivingCount;
	}

	return LivingCount;
}

int32 UGodsentAldegonComponent::GetMaxSummons(EGodsentAldegonSummonType SummonType) const
{
	const float MaxValue = SummonType == EGodsentAldegonSummonType::Totem
		? ReadStat(MaxTotems, TEXT("Max Totems"))
		: ReadStat(MaxAdds, TEXT("Max Adds"));

	return FMath::RoundToInt(MaxValue);
}

void UGodsentAldegonComponent::GetLivingSummons(TArray<AActor*>& OutSummons) const
{
	OutSummons.Reset();

	for (const TArray<TWeakObjectPtr<AActor>>* SummonList : { &LivingAdds, &LivingTotems })
	{
		for (const TWeakObjectPtr<AActor>& Entry : *SummonList)
		{
			if (Entry.IsValid() && !UGodsentFunctionLibrary::NativeDoesActorHaveTag(Entry.Get(), GodsentGameplayTags::Shared_Status_Dead))
				OutSummons.Add(Entry.Get());
		}
	}
}

bool UGodsentAldegonComponent::CanSummonNow(EGodsentAldegonSummonType SummonType, FGameplayTag AbilityTag) const
{
	if (GetLivingSummonCount(SummonType) >= GetMaxSummons(SummonType))
		return false;

	if (!AbilityTag.IsValid())
		AbilityTag = SummonType == EGodsentAldegonSummonType::Totem ? LifeSiphonAbilityTag : SummonAbilityTag;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerCharacter);

	if (!ASC || !AbilityTag.IsValid())
		return false;

	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTag.GetSingleTagContainer(), MatchingSpecs);

	// Ready = could activate right now
	for (const FGameplayAbilitySpec* Spec : MatchingSpecs)
	{
		if (Spec && Spec->Ability && !Spec->IsActive() && Spec->Ability->CanActivateAbility(Spec->Handle, ASC->AbilityActorInfo.Get()))
			return true;
	}

	return false;
}

void UGodsentAldegonComponent::KillAllSummons()
{
	TArray<AActor*> Summons;
	GetLivingSummons(Summons);

	// Same as their health running out: their own death plays (dissolve and all)
	for (AActor* Summon : Summons)
		UGodsentFunctionLibrary::AddGameplayTagToActorIfNone(Summon, GodsentGameplayTags::Shared_Status_Dead);

	LivingAdds.Reset();
	LivingTotems.Reset();
}
