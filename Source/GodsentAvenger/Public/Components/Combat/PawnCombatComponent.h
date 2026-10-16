// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/PawnExtensionComponentBase.h"
#include "PawnCombatComponent.generated.h"

class AGodsentWeaponBase;

UENUM(BlueprintType)
enum class EToggleDamageType : uint8
{
	CurrentEquippedWeapon,
	LeftHand,
	RightHand
};

/**
 * Tracks the weapons a pawn carries and switches their hit collision on and off during attacks.
 */
UCLASS()
class GODSENTAVENGER_API UPawnCombatComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()

public:
	/// Registers a spawned weapon under a tag, optionally as the equipped weapon
	UFUNCTION(BlueprintCallable, Category = "Godsent|Combat")
	void RegisterSpawnedWeapon(FGameplayTag InWeaponTagToRegister, AGodsentWeaponBase* InWeaponToRegister, bool bRegisterAsEquippedWeapon = false);

	/// Gets a carried weapon by its tag
	UFUNCTION(BlueprintCallable, Category = "Godsent|Combat")
	AGodsentWeaponBase* GetCharacterCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const;

	/// Gets the equipped weapon, or nullptr
	UFUNCTION(BlueprintCallable, Category = "Godsent|Combat")
	AGodsentWeaponBase* GetCharacterCurrentEquippedWeapon() const;

	/// Turns hit collision on or off for the equipped weapon or one of the body (hand) boxes
	UFUNCTION(BlueprintCallable, Category = "Godsent|Combat")
	void ToggleWeaponCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType = EToggleDamageType::CurrentEquippedWeapon);

	/// Called when the weapon or a body box hits a hostile pawn
	virtual void OnHitTargetActor(AActor* HitActor);

	/// Called when the weapon stops overlapping a hostile pawn
	virtual void OnWeaponPulledFromTargetActor(AActor* InteractedActor);

	/// Tag of the equipped weapon
	UPROPERTY(BlueprintReadWrite, Category = "Godsent|Combat")
	FGameplayTag CurrentEquippedWeaponTag;

protected:
	virtual void ToggleCurrentEquippedWeaponCollision(bool bShouldEnable);

	virtual void ToggleBodyCollisionBoxCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType);

	/// Actors already hit by the current swing, so one swing only hits each target once
	UPROPERTY()
	TArray<TObjectPtr<AActor>> OverlappedActors;

private:
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<AGodsentWeaponBase>> CharacterCarriedWeaponMap;
};
