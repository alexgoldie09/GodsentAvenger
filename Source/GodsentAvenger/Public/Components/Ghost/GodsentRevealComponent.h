// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GodsentRevealComponent.generated.h"

class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRevealChangedDelegate, bool, bIsRevealed);

/**
 * Fades an enemy between hidden (outline only) and revealed while it has Reveal Tag (Enemy.Status.Revealed,
 * given by the behaviour tree's combat branch). Each frame of the fade it writes the 0-1 "Reveal" value to:
 * - a scalar parameter on every mesh's materials (the ghost material uses it for opacity and glow),
 * - a float user parameter on every Niagara component (the aura brightens),
 * and shows the health bar only while revealed. Stays revealed once the enemy dies, so the death dissolve is visible.
 */
UCLASS(ClassGroup = (Godsent), meta = (BlueprintSpawnableComponent))
class GODSENTAVENGER_API UGodsentRevealComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGodsentRevealComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/// 0 = hidden, 1 = fully revealed
	UFUNCTION(BlueprintPure, Category = "Godsent|Reveal")
	float GetRevealAlpha() const { return CurrentRevealAlpha; }

	UFUNCTION(BlueprintPure, Category = "Godsent|Reveal")
	bool IsRevealed() const { return TargetRevealAlpha > 0.5f; }

	/// Fires when it starts revealing or starts hiding (for sounds or one-off effects)
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Reveal")
	FOnRevealChangedDelegate OnRevealChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/// Revealed while the owner has this tag
	UPROPERTY(EditDefaultsOnly, Category = "Reveal")
	FGameplayTag RevealTag;

	/// Scalar parameter written on the meshes' materials
	UPROPERTY(EditDefaultsOnly, Category = "Reveal")
	FName MaterialParameterName = FName("Reveal");

	/// Float user parameter written on Niagara components (User.Reveal in the system)
	UPROPERTY(EditDefaultsOnly, Category = "Reveal")
	FName NiagaraParameterName = FName("Reveal");

	/// Reveal units per second when appearing (2 = half a second)
	UPROPERTY(EditDefaultsOnly, Category = "Reveal", meta = (ClampMin = "0.01"))
	float RevealSpeed = 2.f;

	/// Units per second when fading back out
	UPROPERTY(EditDefaultsOnly, Category = "Reveal", meta = (ClampMin = "0.01"))
	float HideSpeed = 1.f;

	/// Hide widget components (the health bar) while hidden
	UPROPERTY(EditDefaultsOnly, Category = "Reveal")
	bool bHideWidgetsWhileHidden = true;

private:
	void OnRevealTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);

	void SetTargetRevealAlpha(float NewTarget);
	void ApplyRevealAlpha();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle RevealTagHandle;
	FDelegateHandle DeadTagHandle;

	float CurrentRevealAlpha = 0.f;
	float TargetRevealAlpha = 0.f;
	bool bIsDead = false;
};
