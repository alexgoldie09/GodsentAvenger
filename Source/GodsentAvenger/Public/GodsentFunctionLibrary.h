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

/**
 * Shared helper functions for C++ and Blueprints.
 * The combat component helpers (Get Pawn Combat Component From Actor) come back in stage 2.
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
