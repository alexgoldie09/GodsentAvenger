// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GodsentHeroGameplayAbility_TargetLock.generated.h"

class UInputMappingContext;
class UGodsentWidgetBase;

/**
 * Camera-only lock-on (no strafe animations):
 * - the camera tracks the target; the hero keeps turning towards its movement so the walk cycle always matches
 * - the hero only turns to face the target while blocking (here) and at the start of attacks (Face Locked On Target)
 * - the slowdown is a MoveSpeed GE gated by Player.Status.TargetLock (this ability's Activation Owned Tag)
 * The Blueprint child runs Execute Task On Tick -> On Target Lock Tick, and Wait Gameplay Event (Player.Event.SwitchTarget) -> Switch Target.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentHeroGameplayAbility_TargetLock : public UGodsentHeroGameplayAbility
{
	GENERATED_BODY()

protected:
	//~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility Interface

	/// Keeps the camera on the target, moves the reticle, turns the hero while blocking, and drops the lock when it's no longer valid
	UFUNCTION(BlueprintCallable, Category = "Godsent|TargetLock")
	void OnTargetLockTick(float DeltaTime);

	/// Moves the lock to the nearest valid target on the left or right (Player.Event.SwitchTarget.Left / .Right)
	UFUNCTION(BlueprintCallable, Category = "Godsent|TargetLock")
	void SwitchTarget(const FGameplayTag& InSwitchDirectionTag);

private:
#pragma region Locking
	/// Finds targets and locks onto the nearest; cancels the ability if there are none
	void TryLockOnTarget();

	/// Box-traces along the camera's facing and fills AvailableTargets with living hostile pawns
	void GetAvailableActorsToLock();

	/// The nearest actor in the list, or nullptr
	AActor* GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors);

	/// Splits AvailableTargets into those left and right of the current target, as seen from the hero
	void GetAvailableActorsAroundTarget(TArray<AActor*>& OutActorsOnLeft, TArray<AActor*>& OutActorsOnRight);

	/// Changes the locked target and tells the combat component (attacks read it to face the target)
	void SetCurrentTarget(AActor* NewTarget);

	/// True while the target can still be locked onto (alive, in range)
	bool IsCurrentTargetStillValid();
#pragma endregion

#pragma region Widget
	/// Creates the reticle widget
	void DrawTargetLockWidget();

	/// Moves the reticle over the target
	void SetTargetLockWidgetPosition();
#pragma endregion

#pragma region Setup and cleanup
	/// Adds the mapping context that turns look input into target switching
	void InitTargetLockMappingContext();

	/// Removes the target lock mapping context
	void ResetTargetLockMappingContext();

	/// Gives turning back to the movement component
	void ResetTargetLockMovement();

	/// Ends the ability from inside it
	void CancelTargetLockAbility();

	/// Clears targets and removes the reticle
	void Cleanup();
#pragma endregion

#pragma region Settings
	/// How far ahead (along the camera) to look for targets
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float BoxTraceDistance = 5000.f;

	/// Size of the search box
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	FVector TraceBoxSize = FVector(5000.f, 5000.f, 300.f);

	/// Object types the search hits (set to Pawn)
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TArray<TEnumAsByte<EObjectTypeQuery>> BoxTraceChannel;

	/// Draws the search box for debugging
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	bool bShowPersistentDebugShape = false;

	/// The lock breaks if the target gets further away than this
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float MaxLockDistance = 3000.f;

	/// Reticle widget drawn over the target (needs a Size Box so it can be centred)
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TSubclassOf<UGodsentWidgetBase> TargetLockWidgetClass;

	/// How quickly the camera swings onto the target
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float CameraRotationInterpSpeed = 5.f;

	/// Tilts the camera down by this many degrees so the target isn't dead centre
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float CameraPitchOffset = 20.f;

	/// How quickly the hero turns to face the target while blocking
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float BlockFacingInterpSpeed = 10.f;

	/// Mapping context that routes mouse / right stick to IA_SwitchTarget while locked
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TObjectPtr<UInputMappingContext> TargetLockMappingContext;

	/// Must be higher than the default and weapon contexts so it overrides Look
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	int32 TargetLockMappingContextPriority = 3;
#pragma endregion

#pragma region State
	UPROPERTY()
	TArray<AActor*> AvailableTargets;

	UPROPERTY()
	TObjectPtr<AActor> CurrentLockedOnTargetActor;

	UPROPERTY()
	TObjectPtr<UGodsentWidgetBase> DrawnTargetLockWidget;

	FVector2D TargetLockWidgetSize = FVector2D::ZeroVector;
#pragma endregion
};
