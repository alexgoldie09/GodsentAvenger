// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentFlightComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGodsentFlightActionFinished, EGodsentFlightAction, Action, bool, bSucceeded);

/**
 * Real flight for a character: the capsule leaves the ground (Flying movement mode), unlike the Hover component
 * which only lifts the visuals.
 *
 * - Take Off: climbs straight up to Cruise Height (measured from the ground under the capsule).
 * - Fly To: asks the navmesh for a route and follows it at Cruise Height, so it goes around walls and pillars.
 * - Land At: flies over the spot (on the navmesh), comes down, then switches back to walking.
 *
 * While flying it turns to face its Facing Target if it has one, otherwise the way it's moving.
 * If something else takes it out of flying (a knockback launch, death), the current action fails.
 */
UCLASS(ClassGroup = (Godsent), meta = (BlueprintSpawnableComponent))
class GODSENTAVENGER_API UGodsentFlightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGodsentFlightComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/// Climbs to Cruise Height and holds there
	UFUNCTION(BlueprintCallable, Category = "Godsent|Flight")
	void TakeOff();

	/// Flies to a spot at Cruise Height (takes off first if needed). False if there's no character to move.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Flight")
	bool FlyTo(const FVector& Destination);

	/// Flies over a spot on the navmesh and lands on it (takes off first if needed). False if the spot isn't near the navmesh.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Flight")
	bool LandAt(const FVector& Spot);

	/// Drops the current action quietly and holds position (still flying if it was)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Flight")
	void StopFlightAction();

	/// Stops flying and lets gravity take it (e.g. on death)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Flight")
	void DropToGround();

	/// Turns to face this actor while flying. Null = face the way it's moving.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Flight")
	void SetFacingTarget(AActor* NewFacingTarget);

	UFUNCTION(BlueprintCallable, Category = "Godsent|Flight")
	void SetCruiseHeight(float NewCruiseHeight);

	/// Pushes speeds into the movement component (fly speed) and this component (climb, descent, turn)
	void SetFlightSpeeds(float NewFlySpeed, float NewClimbSpeed, float NewDescentSpeed, float NewTurnRate);

	UFUNCTION(BlueprintPure, Category = "Godsent|Flight")
	bool IsFlying() const;

	UFUNCTION(BlueprintPure, Category = "Godsent|Flight")
	EGodsentFlightAction GetCurrentAction() const { return CurrentAction; }

	/// Gap between the bottom of the capsule and the ground below it (-1 = no ground found)
	UFUNCTION(BlueprintPure, Category = "Godsent|Flight")
	float GetHeightAboveGround() const;

	/// Fires when Take Off, Fly To or Land At finishes (Succeeded = false if it was knocked out of the air).
	/// Not fired when a new command replaces the old one, or for Stop Flight Action.
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Flight")
	FOnGodsentFlightActionFinished OnFlightActionFinished;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/// Top flying speed (pushed into the movement component's Max Fly Speed)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm/s"))
	float FlySpeed = 350.f;

	/// How high the bottom of the capsule flies above the ground
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm"))
	float CruiseHeight = 300.f;

	/// Top speed when climbing
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm/s"))
	float ClimbSpeed = 200.f;

	/// Top speed when coming down
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm/s"))
	float DescentSpeed = 250.f;

	/// How hard it corrects its height (higher = snappier, lower = floatier)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.1"))
	float HeightResponse = 3.f;

	/// Turning speed while flying
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "deg"))
	float TurnRate = 120.f;

	/// Below this speed it stops turning to face where it's going (so it doesn't spin on the spot)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MinFacingSpeed = 10.f;

	/// How quickly it stops when there's nowhere to go (the movement component's default for flying is 0, i.e. drift forever)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm/s"))
	float FlyBrakingDeceleration = 1500.f;

	/// Starts slowing down this far from the destination
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm"))
	float SlowDownDistance = 300.f;

	/// Counts as arrived this close to the destination (flat distance)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "1.0", Units = "cm"))
	float ArriveDistance = 50.f;

	/// Moves on to the next route point this close to the current one
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "1.0", Units = "cm"))
	float RoutePointReachDistance = 120.f;

	/// Take Off is done once within this much of Cruise Height
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm"))
	float CruiseHeightTolerance = 15.f;

	/// Counts as touched down this close to the ground
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm"))
	float TouchDownHeight = 5.f;

	/// Coming down but blocked this long (by something he can't stand on): lands where he is instead of hanging there
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "s"))
	float DescentStallTime = 0.5f;

	/// How far down it looks for the ground
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm"))
	float GroundTraceDistance = 5000.f;

	/// What counts as ground (WorldStatic). Avoid WorldDynamic: weapon and trigger boxes are often that type.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	TArray<TEnumAsByte<EObjectTypeQuery>> GroundObjectTypes = { ObjectTypeQuery1 };

	/// Draws the ground trace while flying (green = found ground, red = didn't)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight|Debug")
	bool bDrawGroundTrace = false;

	/// How far up and down a spot can be from the navmesh and still be found on it
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm"))
	float NavSearchHeight = 2000.f;

	/// How far sideways a spot can be from the navmesh and still be found on it
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0.0", Units = "cm"))
	float NavSearchRadius = 200.f;

private:
	void BeginFlying();
	void StopFlying();

	/// Builds the route to Destination (falls back to a straight line if the navmesh has none)
	void BuildRoute(const FVector& Destination);

	/// Forgets the current action without telling anyone
	void ClearAction();

	/// Coming down: close to the traced ground, on a floor his capsule can stand on, or stuck on the way down
	bool HasTouchedDown(float HeightAboveGround, float DeltaTime);

	/// Finishes the current action and tells listeners
	void FinishAction(bool bSucceeded);

	/// Flat velocity towards the next route point (zero once arrived)
	FVector GetRouteVelocity(bool& bOutArrived);

	void FaceTowards(float DeltaTime, const FVector& Velocity);

	UFUNCTION()
	void OnMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	bool ProjectToNavmesh(const FVector& Point, FVector& OutNavPoint) const;

	/// Finds the ground under the capsule. False if there's nothing there.
	bool TraceGround(float& OutGroundZ, bool bDrawDebug = false) const;

	float GetCapsuleBottomZ() const;

	/// Last ground height found (used if a trace misses, so he still knows roughly how high he is)
	float LastGroundZ = 0.f;
	bool bHasLastGroundZ = false;

	/// Only warn once per flight about missing ground
	bool bWarnedNoGround = false;

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	TWeakObjectPtr<AActor> FacingTarget;

	EGodsentFlightAction CurrentAction = EGodsentFlightAction::None;

	/// Route points still to visit (the last one is the destination)
	TArray<FVector> Route;
	int32 RouteIndex = 0;

	/// Land At: over the spot and coming down
	bool bDescending = false;

	/// Coming down: lowest the capsule has got, and how long it's been stuck there
	float LastDescentZ = 0.f;
	float DescentStallTimer = 0.f;

	/// Set while this component is the one changing the movement mode (so it doesn't fail its own action)
	bool bChangingModeItself = false;

	/// The movement component's own setting, put back after landing
	bool bOrientedToMovementBeforeFlying = true;
};
