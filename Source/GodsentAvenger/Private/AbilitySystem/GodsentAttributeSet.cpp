// Alex Goldberg rights reserved

#include "AbilitySystem/GodsentAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "Interfaces/PawnUIInterface.h"
#include "Components/UI/PawnUIComponent.h"
#include "Components/UI/HeroUIComponent.h"

UGodsentAttributeSet::UGodsentAttributeSet()
{
	InitCurrentHealth(1.f);
	InitMaxHealth(1.f);
	InitCurrentStamina(1.f);
	InitMaxStamina(1.f);
	InitStaminaRegen(0.f);
	InitAttackPower(1.f);
	InitDefensePower(1.f);
	InitAttackSpeed(1.f);
	InitMoveSpeed(0.f); // Seeded from the movement component when the character is possessed
	InitDamageTaken(0.f);
}

/// Clamps every current-value change, including ones from Infinite passive effects
void UGodsentAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentHealthAttribute())
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	else if (Attribute == GetCurrentStaminaAttribute())
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxStaminaAttribute())
		NewValue = FMath::Max(NewValue, 1.f);
	else if (Attribute == GetStaminaRegenAttribute() || Attribute == GetMoveSpeedAttribute())
		NewValue = FMath::Max(NewValue, 0.f);
	else if (Attribute == GetAttackSpeedAttribute())
		NewValue = FMath::Max(NewValue, 0.1f);
}

/// Lowers current values when a max drops, and refreshes the bars on any health or stamina change
void UGodsentAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxHealthAttribute() && GetCurrentHealth() > NewValue)
		SetCurrentHealth(NewValue);
	else if (Attribute == GetMaxStaminaAttribute() && GetCurrentStamina() > NewValue)
		SetCurrentStamina(NewValue);

	if (Attribute == GetCurrentHealthAttribute() || Attribute == GetMaxHealthAttribute())
		BroadcastHealthPercent();
	else if (Attribute == GetCurrentStaminaAttribute() || Attribute == GetMaxStaminaAttribute())
		BroadcastStaminaPercent();
}

/// Clamps base values after Instant/periodic effects, applies DamageTaken, and adds the Dead tag
void UGodsentAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& Attribute = Data.EvaluatedData.Attribute;

	if (Attribute == GetDamageTakenAttribute())
	{
		const float DamageDone = GetDamageTaken();
		SetDamageTaken(0.f);
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth() - DamageDone, 0.f, GetMaxHealth()));
	}
	else if (Attribute == GetCurrentHealthAttribute())
	{
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.f, GetMaxHealth()));
	}
	else if (Attribute == GetCurrentStaminaAttribute())
	{
		SetCurrentStamina(FMath::Clamp(GetCurrentStamina(), 0.f, GetMaxStamina()));
	}

	// Any effect that lowers health (damage or a direct drain) can kill
	const bool bHealthChanged = Attribute == GetDamageTakenAttribute() || Attribute == GetCurrentHealthAttribute();

	if (bHealthChanged && GetCurrentHealth() <= 0.f)
	{
		UGodsentFunctionLibrary::AddGameplayTagToActorIfNone(
			Data.Target.GetAvatarActor(),
			GodsentGameplayTags::Shared_Status_Dead);
	}
}

/// Finds (and caches) the owning pawn's UI interface, or nullptr if it has none
IPawnUIInterface* UGodsentAttributeSet::GetPawnUIInterface()
{
	if (!CachedPawnUIInterface.IsValid())
	{
		const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		AActor* AvatarActor = ASC ? ASC->GetAvatarActor() : nullptr;

		if (!AvatarActor)
			return nullptr;

		CachedPawnUIInterface = TWeakInterfacePtr<IPawnUIInterface>(AvatarActor);
	}

	return CachedPawnUIInterface.IsValid() ? CachedPawnUIInterface.Get() : nullptr;
}

/// Sends the current health percentage to the pawn's UI component
void UGodsentAttributeSet::BroadcastHealthPercent()
{
	if (IPawnUIInterface* PawnUIInterface = GetPawnUIInterface())
		if (UPawnUIComponent* PawnUIComponent = PawnUIInterface->GetPawnUIComponent())
			PawnUIComponent->OnCurrentHealthChanged.Broadcast(GetCurrentHealth() / GetMaxHealth());
}

/// Sends the current stamina percentage to the hero's UI component
void UGodsentAttributeSet::BroadcastStaminaPercent()
{
	if (IPawnUIInterface* PawnUIInterface = GetPawnUIInterface())
		if (UHeroUIComponent* HeroUIComponent = PawnUIInterface->GetHeroUIComponent())
			HeroUIComponent->OnCurrentStaminaChanged.Broadcast(GetCurrentStamina() / GetMaxStamina());
}
