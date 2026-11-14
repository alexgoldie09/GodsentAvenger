// Alex Goldberg rights reserved

#include "Components/Stamina/HeroStaminaComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GodsentAttributeSet.h"
#include "Components/UI/HeroUIComponent.h"
#include "GodsentGameplayTags.h"
#include "Interfaces/PawnUIInterface.h"
#include "TimerManager.h"

void UHeroStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());

	if (!ASC)
		return;

	BoundASC = ASC;

	StaminaChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGodsentAttributeSet::GetCurrentStaminaAttribute())
		.AddUObject(this, &ThisClass::OnCurrentStaminaChanged);

	RegenBlockedTagHandle = ASC->RegisterGameplayTagEvent(GodsentGameplayTags::Player_Status_StaminaRegenBlocked, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::OnRegenBlockedTagChanged);

	ImmortalVigorTagHandle = ASC->RegisterGameplayTagEvent(GodsentGameplayTags::Player_Passive_ImmortalVigor, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::OnImmortalVigorTagChanged);

	// The skill may already be on (applied before BeginPlay)
	if (ASC->HasMatchingGameplayTag(GodsentGameplayTags::Player_Passive_ImmortalVigor))
		OnImmortalVigorTagChanged(GodsentGameplayTags::Player_Passive_ImmortalVigor, 1);
}

void UHeroStaminaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(RegenDelayTimerHandle);

	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UGodsentAttributeSet::GetCurrentStaminaAttribute()).Remove(StaminaChangedHandle);
		ASC->RegisterGameplayTagEvent(GodsentGameplayTags::Player_Status_StaminaRegenBlocked, EGameplayTagEventType::NewOrRemoved).Remove(RegenBlockedTagHandle);
		ASC->RegisterGameplayTagEvent(GodsentGameplayTags::Player_Passive_ImmortalVigor, EGameplayTagEventType::NewOrRemoved).Remove(ImmortalVigorTagHandle);
	}

	BoundASC.Reset();
	Super::EndPlay(EndPlayReason);
}

/// Any spend restarts the wait
void UHeroStaminaComponent::OnCurrentStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < Data.OldValue)
		StartRegenDelay();
}

/// An ability that blocked regen just finished: wait before regen starts, not straight away
void UHeroStaminaComponent::OnRegenBlockedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const UAbilitySystemComponent* ASC = BoundASC.Get();

	if (NewCount > 0 || !ASC)
		return;

	const float CurrentStamina = ASC->GetNumericAttribute(UGodsentAttributeSet::GetCurrentStaminaAttribute());
	const float MaxStamina = ASC->GetNumericAttribute(UGodsentAttributeSet::GetMaxStaminaAttribute());

	if (CurrentStamina < MaxStamina)
		StartRegenDelay();
}

/// Immortal Vigor: fill up now and let the HUD send the stamina bar off
void UHeroStaminaComponent::OnImmortalVigorTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	UAbilitySystemComponent* ASC = BoundASC.Get();

	if (NewCount <= 0 || !ASC)
		return;

	EndRegenDelay();
	ASC->SetNumericAttributeBase(UGodsentAttributeSet::GetCurrentStaminaAttribute(),
		ASC->GetNumericAttribute(UGodsentAttributeSet::GetMaxStaminaAttribute()));

	if (const IPawnUIInterface* PawnUIInterface = Cast<IPawnUIInterface>(GetOwner()))
		if (UHeroUIComponent* HeroUIComponent = PawnUIInterface->GetHeroUIComponent())
			HeroUIComponent->OnInfiniteStaminaGranted.Broadcast();
}

void UHeroStaminaComponent::StartRegenDelay()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	UWorld* World = GetWorld();

	if (!ASC || !World)
		return;

	const float Delay = ASC->GetNumericAttribute(UGodsentAttributeSet::GetStaminaRegenDelayAttribute());

	// No delay set: regen as before
	if (Delay <= 0.f || ASC->HasMatchingGameplayTag(GodsentGameplayTags::Player_Passive_ImmortalVigor))
	{
		EndRegenDelay();
		return;
	}

	if (!bRegenDelayed)
	{
		ASC->AddLooseGameplayTag(GodsentGameplayTags::Player_Status_StaminaRegenDelayed);
		bRegenDelayed = true;
	}

	World->GetTimerManager().SetTimer(RegenDelayTimerHandle, this, &ThisClass::EndRegenDelay, Delay, false);
}

void UHeroStaminaComponent::EndRegenDelay()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(RegenDelayTimerHandle);

	if (!bRegenDelayed)
		return;

	bRegenDelayed = false;

	if (UAbilitySystemComponent* ASC = BoundASC.Get())
		ASC->RemoveLooseGameplayTag(GodsentGameplayTags::Player_Status_StaminaRegenDelayed);
}
