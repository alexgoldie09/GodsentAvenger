// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility_TargetLock.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/GodsentHeroCharacter.h"
#include "Components/Combat/HeroCombatComponent.h"
#include "Components/SizeBox.h"
#include "Controllers/GodsentHeroController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Widgets/GodsentWidgetBase.h"

/// Locks onto the nearest target in front of the camera, then hands over to the Blueprint (tick and switch events)
void UGodsentHeroGameplayAbility_TargetLock::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	TryLockOnTarget();

	// Nothing to lock onto: TryLockOnTarget already cancelled the ability, so don't add input or start the Blueprint
	if (!CurrentLockedOnTargetActor)
		return;

	InitTargetLockMappingContext();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

/// Restores turning and input and removes the reticle
void UGodsentHeroGameplayAbility_TargetLock::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	ResetTargetLockMovement();
	ResetTargetLockMappingContext();
	Cleanup();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

/// Keeps the camera on the target, moves the reticle, turns the hero while blocking, and drops the lock when it's no longer valid
void UGodsentHeroGameplayAbility_TargetLock::OnTargetLockTick(float DeltaTime)
{
	AGodsentHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();
	AGodsentHeroController* HeroController = GetHeroControllerFromActorInfo();

	if (!HeroCharacter || !HeroController || !IsCurrentTargetStillValid() ||
		UGodsentFunctionLibrary::NativeDoesActorHaveTag(HeroCharacter, GodsentGameplayTags::Shared_Status_Dead))
	{
		CancelTargetLockAbility();
		return;
	}

	SetTargetLockWidgetPosition();

	// Camera: always swing towards the target (tilted down slightly so the hero stays in frame)
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(HeroCharacter->GetActorLocation(), CurrentLockedOnTargetActor->GetActorLocation());
	LookAtRotation.Pitch -= CameraPitchOffset;

	const FRotator NewControlRotation = FMath::RInterpTo(HeroController->GetControlRotation(), LookAtRotation, DeltaTime, CameraRotationInterpSpeed);
	HeroController->SetControlRotation(FRotator(NewControlRotation.Pitch, NewControlRotation.Yaw, 0.f));

	// Body: only face the target while blocking (the block facing check needs it) and never mid-roll or mid-cast.
	// Otherwise the hero keeps turning towards movement, so the walk cycle always matches the direction (no strafe anims).
	const bool bShouldFaceTarget =
		UGodsentFunctionLibrary::NativeDoesActorHaveTag(HeroCharacter, GodsentGameplayTags::Player_Status_Blocking) &&
		!UGodsentFunctionLibrary::NativeDoesActorHaveTag(HeroCharacter, GodsentGameplayTags::Player_Status_MovementLocked);

	if (UCharacterMovementComponent* MovementComponent = HeroCharacter->GetCharacterMovement())
		MovementComponent->bOrientRotationToMovement = !bShouldFaceTarget;

	if (bShouldFaceTarget)
	{
		const FVector ToTarget = (CurrentLockedOnTargetActor->GetActorLocation() - HeroCharacter->GetActorLocation()).GetSafeNormal2D();

		if (!ToTarget.IsNearlyZero())
		{
			const FRotator FacingRotation(0.f, ToTarget.Rotation().Yaw, 0.f);
			HeroCharacter->SetActorRotation(FMath::RInterpTo(HeroCharacter->GetActorRotation(), FacingRotation, DeltaTime, BlockFacingInterpSpeed));
		}
	}
}

/// Moves the lock to the nearest valid target on the left or right
void UGodsentHeroGameplayAbility_TargetLock::SwitchTarget(const FGameplayTag& InSwitchDirectionTag)
{
	GetAvailableActorsToLock();

	TArray<AActor*> ActorsOnLeft;
	TArray<AActor*> ActorsOnRight;
	GetAvailableActorsAroundTarget(ActorsOnLeft, ActorsOnRight);

	AActor* NewTarget = InSwitchDirectionTag.MatchesTagExact(GodsentGameplayTags::Player_Event_SwitchTarget_Left)
		? GetNearestTargetFromAvailableActors(ActorsOnLeft)
		: GetNearestTargetFromAvailableActors(ActorsOnRight);

	// No one on that side: keep the current target
	if (NewTarget)
		SetCurrentTarget(NewTarget);
}

/// Finds targets and locks onto the nearest; cancels the ability if there are none
void UGodsentHeroGameplayAbility_TargetLock::TryLockOnTarget()
{
	GetAvailableActorsToLock();

	AActor* NearestTarget = GetNearestTargetFromAvailableActors(AvailableTargets);

	if (!NearestTarget)
	{
		CancelTargetLockAbility();
		return;
	}

	SetCurrentTarget(NearestTarget);
	DrawTargetLockWidget();
	SetTargetLockWidgetPosition();
}

/// Box-traces along the camera's facing and fills AvailableTargets with living hostile pawns
void UGodsentHeroGameplayAbility_TargetLock::GetAvailableActorsToLock()
{
	AvailableTargets.Empty();

	AGodsentHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();

	if (!HeroCharacter)
		return;

	// Search where the camera is looking rather than where the hero is facing, so you lock onto what you can see
	const AGodsentHeroController* HeroController = GetHeroControllerFromActorInfo();
	const FVector SearchDirection = HeroController
		? FRotator(0.f, HeroController->GetControlRotation().Yaw, 0.f).Vector()
		: HeroCharacter->GetActorForwardVector();

	TArray<FHitResult> BoxTraceHits;

	UKismetSystemLibrary::BoxTraceMultiForObjects(
		HeroCharacter,
		HeroCharacter->GetActorLocation(),
		HeroCharacter->GetActorLocation() + SearchDirection * BoxTraceDistance,
		TraceBoxSize / 2.f,
		SearchDirection.ToOrientationRotator(),
		BoxTraceChannel,
		false,
		TArray<AActor*>(),
		bShowPersistentDebugShape ? EDrawDebugTrace::Persistent : EDrawDebugTrace::None,
		BoxTraceHits,
		true);

	for (const FHitResult& Hit : BoxTraceHits)
	{
		APawn* HitPawn = Cast<APawn>(Hit.GetActor());

		if (!HitPawn || HitPawn == HeroCharacter || AvailableTargets.Contains(HitPawn))
			continue;

		// Only things with a Godsent ability system (the tag checks assert otherwise), on the other team, and alive
		if (!Cast<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitPawn)))
			continue;

		if (!UGodsentFunctionLibrary::IsTargetPawnHostile(HeroCharacter, HitPawn))
			continue;

		if (UGodsentFunctionLibrary::NativeDoesActorHaveTag(HitPawn, GodsentGameplayTags::Shared_Status_Dead))
			continue;

		AvailableTargets.Add(HitPawn);
	}
}

/// The nearest actor in the list, or nullptr
AActor* UGodsentHeroGameplayAbility_TargetLock::GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors)
{
	const AGodsentHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();

	if (!HeroCharacter || InAvailableActors.IsEmpty())
		return nullptr;

	float ClosestDistance = 0.f;
	return UGameplayStatics::FindNearestActor(HeroCharacter->GetActorLocation(), InAvailableActors, ClosestDistance);
}

/// Splits AvailableTargets into those left and right of the current target, as seen from the hero
void UGodsentHeroGameplayAbility_TargetLock::GetAvailableActorsAroundTarget(TArray<AActor*>& OutActorsOnLeft, TArray<AActor*>& OutActorsOnRight)
{
	const AGodsentHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();

	if (!HeroCharacter || !CurrentLockedOnTargetActor)
		return;

	const FVector HeroLocation = HeroCharacter->GetActorLocation();
	const FVector HeroToCurrentNorm = (CurrentLockedOnTargetActor->GetActorLocation() - HeroLocation).GetSafeNormal2D();

	for (AActor* AvailableActor : AvailableTargets)
	{
		if (!AvailableActor || AvailableActor == CurrentLockedOnTargetActor)
			continue;

		const FVector HeroToAvailableNorm = (AvailableActor->GetActorLocation() - HeroLocation).GetSafeNormal2D();
		const FVector CrossResult = FVector::CrossProduct(HeroToCurrentNorm, HeroToAvailableNorm);

		if (CrossResult.Z > 0.f)
			OutActorsOnRight.AddUnique(AvailableActor);
		else
			OutActorsOnLeft.AddUnique(AvailableActor);
	}
}

/// Changes the locked target and tells the combat component (attacks read it to face the target)
void UGodsentHeroGameplayAbility_TargetLock::SetCurrentTarget(AActor* NewTarget)
{
	CurrentLockedOnTargetActor = NewTarget;

	if (UHeroCombatComponent* HeroCombatComponent = GetHeroCombatComponentFromActorInfo())
		HeroCombatComponent->SetLockedOnTarget(NewTarget);
}

/// True while the target can still be locked onto (alive, in range)
bool UGodsentHeroGameplayAbility_TargetLock::IsCurrentTargetStillValid()
{
	const AGodsentHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();

	if (!HeroCharacter || !IsValid(CurrentLockedOnTargetActor))
		return false;

	if (UGodsentFunctionLibrary::NativeDoesActorHaveTag(CurrentLockedOnTargetActor, GodsentGameplayTags::Shared_Status_Dead))
		return false;

	return FVector::DistSquared(HeroCharacter->GetActorLocation(), CurrentLockedOnTargetActor->GetActorLocation()) <= FMath::Square(MaxLockDistance);
}

/// Creates the reticle widget
void UGodsentHeroGameplayAbility_TargetLock::DrawTargetLockWidget()
{
	if (DrawnTargetLockWidget)
		return;

	checkf(TargetLockWidgetClass, TEXT("GA_Hero_TargetLock: set Target Lock Widget Class in Class Defaults."));

	AGodsentHeroController* HeroController = GetHeroControllerFromActorInfo();

	if (!HeroController)
		return;

	DrawnTargetLockWidget = CreateWidget<UGodsentWidgetBase>(HeroController, TargetLockWidgetClass);

	if (DrawnTargetLockWidget)
		DrawnTargetLockWidget->AddToViewport();
}

/// Moves the reticle over the target
void UGodsentHeroGameplayAbility_TargetLock::SetTargetLockWidgetPosition()
{
	AGodsentHeroController* HeroController = GetHeroControllerFromActorInfo();

	if (!DrawnTargetLockWidget || !CurrentLockedOnTargetActor || !HeroController)
		return;

	FVector2D ScreenPosition;
	UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(HeroController, CurrentLockedOnTargetActor->GetActorLocation(), ScreenPosition, true);

	// Find the reticle's size once from its Size Box, so it can be centred on the target
	if (TargetLockWidgetSize.IsZero() && DrawnTargetLockWidget->WidgetTree)
	{
		DrawnTargetLockWidget->WidgetTree->ForEachWidget([this](UWidget* FoundWidget)
		{
			if (const USizeBox* FoundSizeBox = Cast<USizeBox>(FoundWidget))
			{
				TargetLockWidgetSize.X = FoundSizeBox->GetWidthOverride();
				TargetLockWidgetSize.Y = FoundSizeBox->GetHeightOverride();
			}
		});
	}

	DrawnTargetLockWidget->SetPositionInViewport(ScreenPosition - TargetLockWidgetSize / 2.f, false);
}

/// Adds the mapping context that turns look input into target switching
void UGodsentHeroGameplayAbility_TargetLock::InitTargetLockMappingContext()
{
	const AGodsentHeroController* HeroController = GetHeroControllerFromActorInfo();

	if (!HeroController || !TargetLockMappingContext)
		return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(HeroController->GetLocalPlayer()))
		Subsystem->AddMappingContext(TargetLockMappingContext, TargetLockMappingContextPriority);
}

/// Removes the target lock mapping context
void UGodsentHeroGameplayAbility_TargetLock::ResetTargetLockMappingContext()
{
	const AGodsentHeroController* HeroController = GetHeroControllerFromActorInfo();

	if (!HeroController || !TargetLockMappingContext)
		return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(HeroController->GetLocalPlayer()))
		Subsystem->RemoveMappingContext(TargetLockMappingContext);
}

/// Gives turning back to the movement component
void UGodsentHeroGameplayAbility_TargetLock::ResetTargetLockMovement()
{
	if (AGodsentHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo())
		if (UCharacterMovementComponent* MovementComponent = HeroCharacter->GetCharacterMovement())
			MovementComponent->bOrientRotationToMovement = true;
}

/// Ends the ability from inside it
void UGodsentHeroGameplayAbility_TargetLock::CancelTargetLockAbility()
{
	CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
}

/// Clears targets and removes the reticle
void UGodsentHeroGameplayAbility_TargetLock::Cleanup()
{
	AvailableTargets.Empty();
	SetCurrentTarget(nullptr);

	if (DrawnTargetLockWidget)
		DrawnTargetLockWidget->RemoveFromParent();

	DrawnTargetLockWidget = nullptr;
	TargetLockWidgetSize = FVector2D::ZeroVector;
}
