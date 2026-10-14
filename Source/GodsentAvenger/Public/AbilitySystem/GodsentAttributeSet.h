// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "UObject/WeakInterfacePtr.h"
#include "GodsentAttributeSet.generated.h"

/// Generates the getter, setter and initialiser functions for an attribute
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

class IPawnUIInterface;

/**
 * Attributes shared by the hero and enemies.
 *
 * Where each kind of change is handled:
 *  - PreAttributeChange:       clamps every current-value change (including Infinite passive effects)
 *  - PostAttributeChange:      lowers current values when a max drops, and refreshes the health/stamina bars
 *  - PostGameplayEffectExecute: clamps base values after Instant/periodic effects, applies DamageTaken, adds the Dead tag
 */
UCLASS()
class GODSENTAVENGER_API UGodsentAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGodsentAttributeSet();

	//~ Begin UAttributeSet Interface
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	//~ End UAttributeSet Interface

	/** Health **/
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData CurrentHealth;
	ATTRIBUTE_ACCESSORS(UGodsentAttributeSet, CurrentHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UGodsentAttributeSet, MaxHealth)

	/** Stamina **/
	UPROPERTY(BlueprintReadOnly, Category = "Stamina")
	FGameplayAttributeData CurrentStamina;
	ATTRIBUTE_ACCESSORS(UGodsentAttributeSet, CurrentStamina)

	UPROPERTY(BlueprintReadOnly, Category = "Stamina")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UGodsentAttributeSet, MaxStamina)

	/// Stamina regained per second (read by GE_Hero_StaminaRegen)
	UPROPERTY(BlueprintReadOnly, Category = "Stamina")
	FGameplayAttributeData StaminaRegen;
	ATTRIBUTE_ACCESSORS(UGodsentAttributeSet, StaminaRegen)

	/** Combat **/
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UGodsentAttributeSet, AttackPower)

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData DefensePower;
	ATTRIBUTE_ACCESSORS(UGodsentAttributeSet, DefensePower)

	/// Montage play-rate multiplier for attacks (1 = normal speed)
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UGodsentAttributeSet, AttackSpeed)

	/** Movement **/
	/// Kept in sync with the movement component's Max Walk Speed by AGodsentBaseCharacter
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UGodsentAttributeSet, MoveSpeed)

	/** Meta **/
	/// Temporary value written by the damage calculation, then subtracted from CurrentHealth
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FGameplayAttributeData DamageTaken;
	ATTRIBUTE_ACCESSORS(UGodsentAttributeSet, DamageTaken)

private:
	/// Finds (and caches) the owning pawn's UI interface, or nullptr if it has none
	IPawnUIInterface* GetPawnUIInterface();

	/// Sends the current health percentage to the pawn's UI component
	void BroadcastHealthPercent();

	/// Sends the current stamina percentage to the hero's UI component
	void BroadcastStaminaPercent();

	/// Cached UI interface of the owning pawn
	TWeakInterfacePtr<IPawnUIInterface> CachedPawnUIInterface;
};
