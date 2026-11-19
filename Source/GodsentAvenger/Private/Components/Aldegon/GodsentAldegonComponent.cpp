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
#include "TimerManager.h"

namespace
{
	/// Stats are read at level 1 if there's no enemy level to use
	constexpr float AldegonDefaultStatLevel = 1.f;

	/// Sitting down starts this close to the end of Rise (exactly at the end can count as already finished)
	constexpr float AldegonRiseEndInset = 0.01f;
}

UGodsentAldegonComponent::UGodsentAldegonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
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
			ASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved).Remove(DeadTagDelegateHandle);
	}

	if (FlightComponent)
		FlightComponent->OnFlightActionFinished.RemoveDynamic(this, &ThisClass::OnFlightActionFinished);

	Super::EndPlay(EndPlayReason);
}

void UGodsentAldegonComponent::ApplyStats()
{
	const float Level = GetStatLevel();
	const float GroundedHeightValue = GroundedHeight.GetValueAtLevel(Level);

	if (HoverComponent)
		HoverComponent->SetHoverHeight(GroundedHeightValue);

	if (FlightComponent)
	{
		// The float is on top of the capsule's height, so the capsule flies that much lower to put his feet at Airborne Height
		FlightComponent->SetCruiseHeight(AirborneHeight.GetValueAtLevel(Level) - GroundedHeightValue);
		FlightComponent->SetFlightSpeeds(FlySpeed.GetValueAtLevel(Level), ClimbSpeed.GetValueAtLevel(Level),
			DescentSpeed.GetValueAtLevel(Level), FlyTurnRate.GetValueAtLevel(Level));
	}
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

	SetState(EGodsentAldegonState::Dormant);
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
}

void UGodsentAldegonComponent::SetState(EGodsentAldegonState NewState)
{
	if (NewState == CurrentState && NewState != EGodsentAldegonState::Dormant)
		return;

	const EGodsentAldegonState OldState = CurrentState;
	CurrentState = NewState;

	if (bLogStateChanges)
	{
		UE_LOG(LogTemp, Log, TEXT("[Aldegon] %s -> %s"),
			*UEnum::GetDisplayValueAsText(OldState).ToString(), *UEnum::GetDisplayValueAsText(NewState).ToString());
	}

	OnAldegonStateChanged.Broadcast(NewState, OldState);
}
