// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Engine/LatentActionManager.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GodsentFunctionLibrary.generated.h"

class UGodsentAbilitySystemComponent;
class UPawnCombatComponent;

/**
 * Shared helper functions for C++ and Blueprints.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/// Gets the Godsent Ability System Component from an actor (asserts if the actor has none)
	static UGodsentAbilitySystemComponent* NativeGetGodsentASCFromActor(AActor* InActor);

	/// Adds a loose gameplay tag to an actor if it doesn't already have it
	UFUNCTION(BlueprintCallable, Category = "Godsent|FunctionLibrary")
	static void AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd);

	/// Removes a loose gameplay tag from an actor if it has it
	UFUNCTION(BlueprintCallable, Category = "Godsent|FunctionLibrary")
	static void RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove);

	/// Checks whether an actor has a gameplay tag
	static bool NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck);

	/// Checks whether an actor has a gameplay tag, with Yes and No exec pins
	UFUNCTION(BlueprintCallable, Category = "Godsent|FunctionLibrary", meta = (DisplayName = "Does Actor Have Tag", ExpandEnumAsExecs = "OutConfirmType"))
	static void BP_DoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck, EGodsentConfirmType& OutConfirmType);

	/// Gets an actor's combat component through IPawnCombatInterface, or nullptr
	static UPawnCombatComponent* NativeGetPawnCombatComponentFromActor(AActor* InActor);

	/// Gets an actor's combat component, with Valid and Invalid exec pins
	UFUNCTION(BlueprintCallable, Category = "Godsent|FunctionLibrary", meta = (DisplayName = "Get Pawn Combat Component From Actor", ExpandEnumAsExecs = "OutValidType"))
	static UPawnCombatComponent* BP_GetPawnCombatComponentFromActor(AActor* InActor, EGodsentValidType& OutValidType);

	/// Checks whether TargetPawn is on a different team to QueryPawn
	UFUNCTION(BlueprintPure, Category = "Godsent|FunctionLibrary")
	static bool IsTargetPawnHostile(APawn* QueryPawn, APawn* TargetPawn);

	/// Gets the value of a Scalable Float at a level
	UFUNCTION(BlueprintPure, Category = "Godsent|FunctionLibrary", meta = (CompactNodeTitle = "Get Value At Level"))
	static float GetScalableFloatValueAtLevel(const FScalableFloat& InScalableFloat, float InLevel = 1.f);

	/// Works out which direction a hit came from, relative to the victim
	UFUNCTION(BlueprintPure, Category = "Godsent|FunctionLibrary")
	static FGameplayTag ComputeHitReactDirectionTag(AActor* InAttacker, AActor* InVictim, float& OutAngleDifference);

	/// Checks whether the defender is facing the attacker closely enough for a block to count
	UFUNCTION(BlueprintPure, Category = "Godsent|FunctionLibrary")
	static bool IsValidBlock(AActor* InAttacker, AActor* InDefender, float Threshold = 0.f);

	/// Works out how a hit on InDefender is blocked:
	/// not blocking, not facing the attacker, or an unblockable attack -> None;
	/// inside the parry window -> Parried; with the full-block skill -> FullBlocked; otherwise -> Blocked
	static EGodsentBlockResult NativeResolveBlock(AActor* InAttacker, AActor* InDefender, bool bAttackIsUnblockable, float FacingThreshold = 0.f);

	/// Sends the block or parry events for a resolved hit.
	/// Returns true when the hit should deal no damage (FullBlocked or Parried).
	static bool NativeHandleBlockResult(AActor* InAttacker, AActor* InDefender, EGodsentBlockResult BlockResult);

	/// Applies a gameplay effect spec to a target actor using the instigator's ASC
	UFUNCTION(BlueprintCallable, Category = "Godsent|FunctionLibrary")
	static bool ApplyGameplayEffectSpecHandleToTargetActor(AActor* InInstigator, AActor* InTargetActor, const FGameplayEffectSpecHandle& InGameplayEffectSpecHandle);

	/// Counts down over TotalTime, firing Updated every UpdateInterval (used for cooldown UI)
	UFUNCTION(BlueprintCallable, Category = "Godsent|FunctionLibrary", meta = (Latent, WorldContext = "WorldContextObject", LatentInfo = "LatentInfo", ExpandEnumAsExecs = "CountDownInput|CountDownOutput", TotalTime = "1.0", UpdateInterval = "0.1"))
	static void CountDown(const UObject* WorldContextObject,
		float TotalTime,
		float UpdateInterval,
		float& OutRemainingTime,
		EGodsentCountDownActionInput CountDownInput,
		UPARAM(DisplayName = "Output") EGodsentCountDownActionOutput& CountDownOutput,
		FLatentActionInfo LatentInfo);
};
