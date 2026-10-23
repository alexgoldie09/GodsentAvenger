// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentGameplayAbility.generated.h"

class UGodsentAbilitySystemComponent;
class UPawnCombatComponent;

UENUM(BlueprintType)
enum class EGodsentAbilityActivationPolicy : uint8
{
	OnTriggered,
	OnGiven
};

/**
 * Base class for every Godsent ability (hero and enemy).
 */
UCLASS()
class GODSENTAVENGER_API UGodsentGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

protected:
	//~ Begin UGameplayAbility Interface.
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility Interface.

	/// OnGiven abilities activate as soon as they're granted and are removed when they end
	UPROPERTY(EditDefaultsOnly, Category = "GodsentAbility")
	EGodsentAbilityActivationPolicy AbilityActivationPolicy = EGodsentAbilityActivationPolicy::OnTriggered;

	/// Gets the owner's combat component
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	UPawnCombatComponent* GetPawnCombatComponentFromActorInfo() const;

	/// Gets the owner's Godsent Ability System Component
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	UGodsentAbilitySystemComponent* GetGodsentAbilitySystemComponentFromActorInfo() const;

	/// Applies an effect spec to a target, with Successful and Failed exec pins
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability", meta = (DisplayName = "Apply Gameplay Effect Spec Handle to Target Actor", ExpandEnumAsExecs = "OutSuccessType"))
	FActiveGameplayEffectHandle BP_ApplyEffectSpecHandleToTargetActor(AActor* TargetActor, const FGameplayEffectSpecHandle& InSpecHandle, EGodsentSuccessType& OutSuccessType);

	/// Applies an effect spec to every hostile pawn in a set of hit results and sends each one a hit react event (AOE attacks)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability")
	void ApplyGameplayEffectSpecHandleToHitResults(const FGameplayEffectSpecHandle& InSpecHandle, const TArray<FHitResult>& HitResults);

	/// Applies an effect spec to every living hostile pawn within Radius of Origin (heal release, light aura, later enemy AOEs).
	/// bCausesHitStun adds Shared.Effect.HitStun to the spec so the hits stagger.
	/// RequiredTargetTag (optional) limits it to targets carrying that tag, e.g. Enemy.Type.Undead for Greater Light.
	/// Returns how many targets were hit. Make the spec once (store it in a local variable) before passing it in.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability", meta = (AdvancedDisplay = "bDrawDebug"))
	int32 ApplyEffectSpecHandleToHostilesInRadius(const FGameplayEffectSpecHandle& InSpecHandle, FVector Origin, float Radius, bool bCausesHitStun, FGameplayTag RequiredTargetTag, bool bDrawDebug = false);

	/// Applies an effect spec to a target and returns the active effect handle
	FActiveGameplayEffectHandle NativeApplyEffectSpecHandleToTarget(AActor* TargetActor, const FGameplayEffectSpecHandle& InSpecHandle);
};
