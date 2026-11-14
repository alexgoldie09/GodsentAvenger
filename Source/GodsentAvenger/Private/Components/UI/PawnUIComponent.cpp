// Alex Goldberg rights reserved

#include "Components/UI/PawnUIComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GodsentAttributeSet.h"

void UPawnUIComponent::RefreshStatusBars()
{
	const UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();

	if (!ASC)
		return;

	const float MaxHealth = ASC->GetNumericAttribute(UGodsentAttributeSet::GetMaxHealthAttribute());
	const float CurrentHealth = ASC->GetNumericAttribute(UGodsentAttributeSet::GetCurrentHealthAttribute());

	OnMaxHealthChanged.Broadcast(MaxHealth);

	if (MaxHealth > 0.f)
		OnCurrentHealthChanged.Broadcast(CurrentHealth / MaxHealth);
}

UAbilitySystemComponent* UPawnUIComponent::GetOwnerAbilitySystemComponent() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}
