// Alex Goldberg rights reserved

#include "Components/Flight/GodsentFlightComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

namespace
{
	/// No ground found
	constexpr float FlightNoGround = -1.f;

	/// Coming down slower than this much per check counts as stuck (cm)
	constexpr float FlightDescentStallTolerance = 1.f;

	const FName FlightGroundTraceTag = TEXT("GodsentFlightGround");
}

UGodsentFlightComponent::UGodsentFlightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGodsentFlightComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	MovementComponent = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;

	if (!OwnerCharacter || !MovementComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: GodsentFlightComponent needs a Character owner"), *GetNameSafe(GetOwner()));
		return;
	}

	MovementComponent->BrakingDecelerationFlying = FlyBrakingDeceleration;
	MovementComponent->MaxFlySpeed = FlySpeed;

	// Our move request has to be in before the movement component uses it this frame
	MovementComponent->PrimaryComponentTick.AddPrerequisite(this, PrimaryComponentTick);

	OwnerCharacter->MovementModeChangedDelegate.AddUniqueDynamic(this, &ThisClass::OnMovementModeChanged);
}

void UGodsentFlightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerCharacter)
		OwnerCharacter->MovementModeChangedDelegate.RemoveDynamic(this, &ThisClass::OnMovementModeChanged);

	Super::EndPlay(EndPlayReason);
}

void UGodsentFlightComponent::TakeOff()
{
	if (!MovementComponent)
		return;

	ClearAction();

	CurrentAction = EGodsentFlightAction::TakeOff;
	Route.Reset();
	bDescending = false;

	BeginFlying();
}

bool UGodsentFlightComponent::FlyTo(const FVector& Destination)
{
	if (!MovementComponent)
		return false;

	ClearAction();

	CurrentAction = EGodsentFlightAction::FlyTo;
	bDescending = false;
	BuildRoute(Destination);

	BeginFlying();
	return true;
}

bool UGodsentFlightComponent::LandAt(const FVector& Spot)
{
	FVector NavSpot;

	if (!MovementComponent || !ProjectToNavmesh(Spot, NavSpot))
		return false;

	ClearAction();

	CurrentAction = EGodsentFlightAction::Land;
	bDescending = false;
	BuildRoute(NavSpot);

	BeginFlying();
	return true;
}

void UGodsentFlightComponent::StopFlightAction()
{
	ClearAction();
}

void UGodsentFlightComponent::DropToGround()
{
	ClearAction();

	if (IsFlying())
	{
		bChangingModeItself = true;
		MovementComponent->SetMovementMode(MOVE_Falling);
		bChangingModeItself = false;
	}

	StopFlying();
}

void UGodsentFlightComponent::SetFacingTarget(AActor* NewFacingTarget)
{
	FacingTarget = NewFacingTarget;
}

void UGodsentFlightComponent::SetCruiseHeight(float NewCruiseHeight)
{
	CruiseHeight = FMath::Max(NewCruiseHeight, 0.f);
}

void UGodsentFlightComponent::SetFlightSpeeds(float NewFlySpeed, float NewClimbSpeed, float NewDescentSpeed, float NewTurnRate)
{
	// Speeds can arrive before BeginPlay has found the movement component, so keep them and apply there too
	FlySpeed = FMath::Max(NewFlySpeed, 0.f);

	if (MovementComponent)
		MovementComponent->MaxFlySpeed = FlySpeed;

	ClimbSpeed = FMath::Max(NewClimbSpeed, 0.f);
	DescentSpeed = FMath::Max(NewDescentSpeed, 0.f);
	TurnRate = FMath::Max(NewTurnRate, 0.f);
}

bool UGodsentFlightComponent::IsFlying() const
{
	return MovementComponent && MovementComponent->MovementMode == MOVE_Flying;
}

float UGodsentFlightComponent::GetCapsuleBottomZ() const
{
	return OwnerCharacter->GetActorLocation().Z - OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

bool UGodsentFlightComponent::TraceGround(float& OutGroundZ, bool bDrawDebug) const
{
	if (!OwnerCharacter || GroundObjectTypes.IsEmpty())
		return false;

	// From the middle of the capsule, so the start is never already under the floor
	const FVector TraceStart = OwnerCharacter->GetActorLocation();
	const FVector TraceEnd(TraceStart.X, TraceStart.Y, GetCapsuleBottomZ() - GroundTraceDistance);

	// Complex: the real floor triangles (matches Use Complex As Simple floors exactly)
	FCollisionQueryParams Params(FlightGroundTraceTag, true, OwnerCharacter.Get());
	FHitResult Hit;

	const bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, FCollisionObjectQueryParams(GroundObjectTypes), Params);

	if (bDrawDebug)
		DrawDebugLine(GetWorld(), TraceStart, bHit ? Hit.ImpactPoint : TraceEnd, bHit ? FColor::Green : FColor::Red);

	if (bHit)
		OutGroundZ = Hit.ImpactPoint.Z;

	return bHit;
}

float UGodsentFlightComponent::GetHeightAboveGround() const
{
	if (!OwnerCharacter)
		return FlightNoGround;

	float GroundZ = 0.f;

	if (!TraceGround(GroundZ))
	{
		if (!bHasLastGroundZ)
			return FlightNoGround;

		GroundZ = LastGroundZ;
	}

	return FMath::Max(GetCapsuleBottomZ() - GroundZ, 0.f);
}

void UGodsentFlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsFlying() || DeltaTime <= 0.f)
		return;

	// Flat movement along the route
	bool bArrived = false;
	FVector DesiredVelocity = FVector::ZeroVector;

	const bool bTravelling = (CurrentAction == EGodsentFlightAction::FlyTo || CurrentAction == EGodsentFlightAction::Land) && !bDescending;

	if (bTravelling)
		DesiredVelocity = GetRouteVelocity(bArrived);

	if (bArrived)
	{
		if (CurrentAction == EGodsentFlightAction::FlyTo)
		{
			FinishAction(true);
		}
		else
		{
			bDescending = true;
			LastDescentZ = GetCapsuleBottomZ();
			DescentStallTimer = 0.f;
		}
	}

	// Remember the ground so a missed trace (a gap, odd collision) doesn't leave him not knowing his height
	float GroundZ = 0.f;

	if (TraceGround(GroundZ, bDrawGroundTrace))
	{
		LastGroundZ = GroundZ;
		bHasLastGroundZ = true;
	}
	else if (!bWarnedNoGround)
	{
		bWarnedNoGround = true;
		UE_LOG(LogTemp, Warning, TEXT("%s: Flight can't find ground below (check Ground Object Types). Using the last ground height."), *GetNameSafe(OwnerCharacter));
	}

	// Height: hold Cruise Height, or come down once over the landing spot
	const float HeightAboveGround = bHasLastGroundZ ? FMath::Max(GetCapsuleBottomZ() - LastGroundZ, 0.f) : FlightNoGround;
	const float TargetHeight = bDescending ? 0.f : CruiseHeight;

	if (HeightAboveGround != FlightNoGround)
		DesiredVelocity.Z = FMath::Clamp((TargetHeight - HeightAboveGround) * HeightResponse, -DescentSpeed, ClimbSpeed);

	MovementComponent->RequestDirectMove(DesiredVelocity, false);
	FaceTowards(DeltaTime, DesiredVelocity);

	// Done climbing
	if (CurrentAction == EGodsentFlightAction::TakeOff && HeightAboveGround != FlightNoGround && FMath::Abs(HeightAboveGround - CruiseHeight) <= CruiseHeightTolerance)
	{
		FinishAction(true);
		return;
	}

	// Touched down: back to walking
	if (bDescending && HasTouchedDown(HeightAboveGround, DeltaTime))
	{
		StopFlying();
		FinishAction(true);
	}
}

void UGodsentFlightComponent::BeginFlying()
{
	SetComponentTickEnabled(true);

	if (IsFlying())
		return;

	// He's standing on the ground right now: a fallback if the trace ever misses
	LastGroundZ = GetCapsuleBottomZ();
	bHasLastGroundZ = true;
	bWarnedNoGround = false;

	// We do the turning while flying (orient-to-movement would pitch the whole body when climbing)
	bOrientedToMovementBeforeFlying = MovementComponent->bOrientRotationToMovement;
	MovementComponent->bOrientRotationToMovement = false;

	bChangingModeItself = true;
	MovementComponent->SetMovementMode(MOVE_Flying);
	bChangingModeItself = false;
}

void UGodsentFlightComponent::StopFlying()
{
	if (!MovementComponent)
		return;

	if (IsFlying())
	{
		bChangingModeItself = true;
		MovementComponent->SetMovementMode(MOVE_Walking);
		bChangingModeItself = false;
	}

	MovementComponent->bOrientRotationToMovement = bOrientedToMovementBeforeFlying;
	SetComponentTickEnabled(false);
}

void UGodsentFlightComponent::BuildRoute(const FVector& Destination)
{
	Route.Reset();
	RouteIndex = 0;

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FVector NavStart;
	FVector NavEnd;

	// The route is worked out on the ground under us; we just fly it higher up
	if (NavSystem && ProjectToNavmesh(OwnerCharacter->GetActorLocation(), NavStart) && ProjectToNavmesh(Destination, NavEnd))
	{
		if (const UNavigationPath* Path = NavSystem->FindPathToLocationSynchronously(GetWorld(), NavStart, NavEnd, OwnerCharacter.Get()))
		{
			if (Path->IsValid() && Path->PathPoints.Num() > 1)
			{
				// Skip the first point (where we already are)
				for (int32 Index = 1; Index < Path->PathPoints.Num(); ++Index)
					Route.Add(Path->PathPoints[Index]);
			}
		}
	}

	// No navmesh route: straight line
	if (Route.IsEmpty())
		Route.Add(Destination);
}

FVector UGodsentFlightComponent::GetRouteVelocity(bool& bOutArrived)
{
	bOutArrived = false;

	if (!Route.IsValidIndex(RouteIndex))
	{
		bOutArrived = true;
		return FVector::ZeroVector;
	}

	const FVector Location = OwnerCharacter->GetActorLocation();
	const bool bLastPoint = RouteIndex == Route.Num() - 1;

	FVector ToPoint = Route[RouteIndex] - Location;
	ToPoint.Z = 0.f;
	float Distance = ToPoint.Size();

	// Passed a corner: on to the next one
	if (!bLastPoint && Distance <= RoutePointReachDistance)
	{
		++RouteIndex;
		return GetRouteVelocity(bOutArrived);
	}

	if (bLastPoint && Distance <= ArriveDistance)
	{
		bOutArrived = true;
		return FVector::ZeroVector;
	}

	// Ease in to the destination rather than overshoot it
	float Speed = MovementComponent->MaxFlySpeed;

	if (bLastPoint && SlowDownDistance > 0.f)
		Speed *= FMath::Clamp(Distance / SlowDownDistance, 0.f, 1.f);

	return ToPoint.GetSafeNormal() * Speed;
}

void UGodsentFlightComponent::FaceTowards(float DeltaTime, const FVector& Velocity)
{
	FVector FacingDirection = FVector::ZeroVector;

	if (const AActor* Target = FacingTarget.Get())
		FacingDirection = Target->GetActorLocation() - OwnerCharacter->GetActorLocation();
	else if (Velocity.Size2D() > MinFacingSpeed)
		FacingDirection = Velocity;

	FacingDirection.Z = 0.f;

	if (FacingDirection.IsNearlyZero())
		return;

	// Yaw only: he stays upright
	const FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
	const FRotator TargetRotation(0.f, FacingDirection.Rotation().Yaw, 0.f);

	OwnerCharacter->SetActorRotation(FMath::RInterpConstantTo(CurrentRotation, TargetRotation, DeltaTime, TurnRate));
}

void UGodsentFlightComponent::ClearAction()
{
	CurrentAction = EGodsentFlightAction::None;
	Route.Reset();
	RouteIndex = 0;
	bDescending = false;
	DescentStallTimer = 0.f;
}

bool UGodsentFlightComponent::HasTouchedDown(float HeightAboveGround, float DeltaTime)
{
	// Close to the ground the trace sees
	if (HeightAboveGround != FlightNoGround && HeightAboveGround <= TouchDownHeight)
		return true;

	// The floor his capsule would actually stand on: a step, or floor collision sitting above what the trace sees
	FFindFloorResult FloorResult;
	MovementComponent->FindFloor(MovementComponent->UpdatedComponent->GetComponentLocation(), FloorResult, false);

	if (FloorResult.IsWalkableFloor() && FloorResult.GetDistanceToFloor() <= TouchDownHeight)
		return true;

	// Still dropping: reset the stuck timer
	const float CapsuleBottomZ = GetCapsuleBottomZ();

	if (LastDescentZ - CapsuleBottomZ > FlightDescentStallTolerance)
	{
		LastDescentZ = CapsuleBottomZ;
		DescentStallTimer = 0.f;
		return false;
	}

	// Blocked by something he can't stand on: land where he is rather than hang in the air
	DescentStallTimer += DeltaTime;

	if (DescentStallTimer < DescentStallTime)
		return false;

	UE_LOG(LogTemp, Warning, TEXT("%s: Flight landing was blocked %.0f cm above the ground trace, landed where he is (check the floor's collision under him)"),
		*GetNameSafe(OwnerCharacter), HeightAboveGround);
	return true;
}

void UGodsentFlightComponent::FinishAction(bool bSucceeded)
{
	if (CurrentAction == EGodsentFlightAction::None)
		return;

	const EGodsentFlightAction FinishedAction = CurrentAction;
	ClearAction();

	OnFlightActionFinished.Broadcast(FinishedAction, bSucceeded);
}

void UGodsentFlightComponent::OnMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (bChangingModeItself || PrevMovementMode != MOVE_Flying || IsFlying())
		return;

	// Knocked out of the air (launch, death): whatever we were doing is over
	MovementComponent->bOrientRotationToMovement = bOrientedToMovementBeforeFlying;
	SetComponentTickEnabled(false);
	FinishAction(false);
}

bool UGodsentFlightComponent::ProjectToNavmesh(const FVector& Point, FVector& OutNavPoint) const
{
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!NavSystem)
		return false;

	FNavLocation NavLocation;

	if (!NavSystem->ProjectPointToNavigation(Point, NavLocation, FVector(NavSearchRadius, NavSearchRadius, NavSearchHeight)))
		return false;

	OutNavPoint = NavLocation.Location;
	return true;
}
