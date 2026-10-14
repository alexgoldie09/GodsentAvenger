// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentGameplayAbility.generated.h"

class UGodsentAbilitySystemComponent;

UENUM(BlueprintType)
enum class EGodsentAbilityActivationPolicy : uint8
{
	OnTriggered,
	OnGiven
};

/**
 * Base class for every Godsent ability (hero and enemy).
 * GetPawnCombatComponentFromActorInfo comes back in stage 2 with the combat components.
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

	/// Gets the owner's Godsent Ability System Component
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	UGodsentAbilitySystemComponent* GetGodsentAbilitySystemComponentFromActorInfo() const;

	/// Applies an effect spec to a target, with Successful and Failed exec pins
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability", meta = (DisplayName = "Apply Gameplay Effect Spec Handle to Target Actor", ExpandEnumAsExecs = "OutSuccessType"))
	FActiveGameplayEffectHandle BP_ApplyEffectSpecHandleToTargetActor(AActor* TargetActor, const FGameplayEffectSpecHandle& InSpecHandle, EGodsentSuccessType& OutSuccessType);

	/// Applies an effect spec to every hostile pawn in a set of hit results and sends each one a hit react event (AOE attacks)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability")
	void ApplyGameplayEffectSpecHandleToHitResults(const FGameplayEffectSpecHandle& InSpecHandle, const TArray<FHitResult>& HitResults);

	/// Applies an effect spec to a target and returns the active effect handle
	FActiveGameplayEffectHandle NativeApplyEffectSpecHandleToTarget(AActor* TargetActor, const FGameplayEffectSpecHandle& InSpecHandle);
};
