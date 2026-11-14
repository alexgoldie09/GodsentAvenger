// Alex Goldberg rights reserved

#include "Components/UI/HeroUIComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GodsentAttributeSet.h"
#include "GodsentGameplayTags.h"

void UHeroUIComponent::RefreshStatusBars()
{
	Super::RefreshStatusBars();

	const UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();

	if (!ASC)
		return;

	const float MaxStamina = ASC->GetNumericAttribute(UGodsentAttributeSet::GetMaxStaminaAttribute());
	const float CurrentStamina = ASC->GetNumericAttribute(UGodsentAttributeSet::GetCurrentStaminaAttribute());

	OnMaxStaminaChanged.Broadcast(MaxStamina);

	if (MaxStamina > 0.f)
		OnCurrentStaminaChanged.Broadcast(CurrentStamina / MaxStamina);

	if (HasInfiniteStamina())
		OnInfiniteStaminaGranted.Broadcast();
}

bool UHeroUIComponent::HasInfiniteStamina() const
{
	const UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(GodsentGameplayTags::Player_Passive_ImmortalVigor);
}
