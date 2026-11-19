// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentAldegonComponent.generated.h"

class ACharacter;
class UAnimMontage;
class UGodsentFlightComponent;
class UGodsentHoverComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAldegonStateChanged, EGodsentAldegonState, NewState, EGodsentAldegonState, OldState);

/**
 * Aldegon's body: which state he's in and how he moves between them (BP_Aldegon).
 * Needs a Godsent Flight component and a Godsent Hover component on the same actor (the Mesh tagged "Hover").
 *
 * Dormant (sitting) → Wake → Rise montage → float up → Take Off → Airborne.
 * Airborne ↔ Grounded through Land At / Take Off. Fly To moves him around while airborne.
 * Return Home (player left the arena): fly back, land, turn to the throne, Rise backwards, heal to full → Dormant.
 * Knocked out of the air (a knockback launch): he lands wherever he falls and is Grounded.
 *
 * Every tuning number is a Scalable Float, so it can point at a row in CT_Aldegon.
 */
UCLASS(ClassGroup = (Godsent), meta = (BlueprintSpawnableComponent))
class GODSENTAVENGER_API UGodsentAldegonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGodsentAldegonComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/// Dormant only: gets up off the throne, then takes off
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	bool Wake();

	/// Grounded only: climbs up to Airborne Height
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	bool TakeOff();

	/// Airborne only: flies to a spot and stays airborne
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	bool FlyTo(const FVector& Destination);

	/// Airborne only: flies to a spot on the navmesh and lands there
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	bool LandAt(const FVector& Spot);

	/// Any awake state: flies back to the throne, sits down and heals to full
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	bool ReturnHome();

	/// Faces this actor while flying (e.g. the player). Null = face the way he's flying.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	void SetFacingTarget(AActor* NewFacingTarget);

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon")
	EGodsentAldegonState GetAldegonState() const { return CurrentState; }

	/// Flying for any reason (airborne, taking off, landing, flying home)
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon")
	bool IsFlying() const;

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon")
	FVector GetHomeLocation() const { return HomeLocation; }

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon")
	FRotator GetHomeRotation() const { return HomeRotation; }

	/// Re-reads every stat from the curve table (at the enemy's applied level) and pushes them to flight and hover
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	void ApplyStats();

	UPROPERTY(BlueprintAssignable, Category = "Godsent|Aldegon")
	FOnAldegonStateChanged OnAldegonStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ---- Stats (point these at CT_Aldegon rows) ----

	/// How high his feet are when airborne (what you see, Grounded Height float included). "A head above the player."
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat AirborneHeight = 250.f;

	/// How far his feet float off the floor (always on while he's awake, airborne or grounded). "Inches."
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat GroundedHeight = 15.f;

	/// Top flying speed (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat FlySpeed = 350.f;

	/// Top climbing speed (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat ClimbSpeed = 200.f;

	/// Top coming-down speed (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat DescentSpeed = 250.f;

	/// Turning speed while flying (degrees/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat FlyTurnRate = 120.f;

	// ---- Throne ----

	/// Getting out of the throne. Played backwards to sit back down.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne")
	TObjectPtr<UAnimMontage> RiseMontage;

	/// Play rate for Rise when getting up
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.1"))
	float RisePlayRate = 1.f;

	/// Play rate for Rise backwards when sitting down (positive; it's flipped for you)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.1"))
	float SitDownPlayRate = 1.f;

	/// How far through Rise (0-1) he starts floating, so he drifts up as he finishes standing
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RiseHoverStart = 0.7f;

	/// How far through sitting down (0-1) the float settles, so he's resting before he touches the seat
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SitDownSettleAt = 0.f;

	/// Speed he glides onto the throne after landing beside it (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "1.0", Units = "cm/s"))
	float HomeSlideSpeed = 150.f;

	/// Counts as back on the throne this close to where he was placed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.0", Units = "cm"))
	float HomeArriveDistance = 2.f;

	/// Blend out time if Rise is stopped part-way (sent home while still getting up)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.0", Units = "s"))
	float MontageStopBlendTime = 0.25f;

	/// Turning speed when lining up with the throne after landing at home (degrees/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "1.0"))
	float HomeTurnRate = 180.f;

	/// Counts as lined up with the throne within this many degrees
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.0"))
	float HomeFacingTolerance = 2.f;

	/// Back to full health once he's sat back down
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne")
	bool bHealOnReturnHome = true;

	/// Logs every state change to the output log
	UPROPERTY(EditDefaultsOnly, Category = "Aldegon|Debug")
	bool bLogStateChanges = true;

private:
	void SetState(EGodsentAldegonState NewState);

	/// Enemy level the stats are read at
	float GetStatLevel() const;

	/// Plays Rise forwards (getting up) or backwards (sitting down). Returns the montage length in seconds (0 = didn't play).
	float PlayRise(bool bBackwards);

	void OnRiseFinished();
	void OnSitDownFinished();

	/// Lines up with the throne after landing at home, then sits
	void TickTurnToHome(float DeltaTime);

	void HealToFull();

	UFUNCTION()
	void OnFlightActionFinished(EGodsentFlightAction Action, bool bSucceeded);

	UFUNCTION()
	void OnMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	void OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UGodsentFlightComponent> FlightComponent;

	UPROPERTY()
	TObjectPtr<UGodsentHoverComponent> HoverComponent;

	EGodsentAldegonState CurrentState = EGodsentAldegonState::Dormant;

	FVector HomeLocation = FVector::ZeroVector;
	FRotator HomeRotation = FRotator::ZeroRotator;

	/// Landed at home and turning to face the throne
	bool bTurningToHome = false;

	FTimerHandle RiseTimerHandle;
	FTimerHandle HoverTimerHandle;

	FDelegateHandle DeadTagDelegateHandle;
};
