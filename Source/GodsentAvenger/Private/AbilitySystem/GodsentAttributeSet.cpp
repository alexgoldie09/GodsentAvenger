// Alex Goldberg rights reserved

#include "AbilitySystem/GodsentAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "Interfaces/PawnUIInterface.h"
#include "Components/UI/PawnUIComponent.h"
#include "Components/UI/HeroUIComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Characters/GodsentBaseCharacter.h"
#include "GodsentDebugHelper.h"
#include "Subsystems/GodsentLineageSubsystem.h"

UGodsentAttributeSet::UGodsentAttributeSet()
{
	InitCurrentHealth(1.f);
	InitMaxHealth(1.f);
	InitCurrentStamina(1.f);
	InitMaxStamina(1.f);
	InitStaminaRegen(0.f);
	InitStaminaRegenDelay(0.f); // No delay until GE_Hero_StartUpStats sets it from the curve table
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
		// Immortal Vigor: stamina never goes down
		NewValue = OwnerHasTag(GodsentGameplayTags::Player_Passive_ImmortalVigor) ? GetMaxStamina() : FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxStaminaAttribute())
		NewValue = FMath::Max(NewValue, 1.f);
	else if (Attribute == GetStaminaRegenAttribute() || Attribute == GetStaminaRegenDelayAttribute() || Attribute == GetMoveSpeedAttribute())
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
	else if (Attribute == GetMaxStaminaAttribute() && OwnerHasTag(GodsentGameplayTags::Player_Passive_ImmortalVigor))
		SetCurrentStamina(NewValue); // Immortal Vigor: a bigger max fills straight up

	if (Attribute == GetCurrentHealthAttribute() || Attribute == GetMaxHealthAttribute())
		BroadcastHealthPercent();
	else if (Attribute == GetCurrentStaminaAttribute() || Attribute == GetMaxStaminaAttribute())
		BroadcastStaminaPercent();

	// The hero's bars change width with the max (WBP_HeroOverlay)
	if (OldValue != NewValue)
	{
		if (IPawnUIInterface* PawnUIInterface = GetPawnUIInterface())
		{
			if (Attribute == GetMaxHealthAttribute())
			{
				if (UPawnUIComponent* PawnUIComponent = PawnUIInterface->GetPawnUIComponent())
					PawnUIComponent->OnMaxHealthChanged.Broadcast(NewValue);
			}
			else if (Attribute == GetMaxStaminaAttribute())
			{
				if (UHeroUIComponent* HeroUIComponent = PawnUIInterface->GetHeroUIComponent())
					HeroUIComponent->OnMaxStaminaChanged.Broadcast(NewValue);
			}
		}
	}
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

		if (DamageDone > 0.f)
			HandleDamageReactions(Data, DamageDone);
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
		AActor* AvatarActor = Data.Target.GetAvatarActor();

		// Saintly Revival gets one go before the real death (not for the kill volume: GE_KillVolume has Shared.Effect.IgnoreRevival)
		FGameplayTagContainer KillingEffectTags;
		Data.EffectSpec.GetAllAssetTags(KillingEffectTags);

		const bool bCanRevive = !KillingEffectTags.HasTagExact(GodsentGameplayTags::Shared_Effect_IgnoreRevival);

		if (!bCanRevive || !TryStartRevival(AvatarActor))
			UGodsentFunctionLibrary::AddGameplayTagToActorIfNone(AvatarActor, GodsentGameplayTags::Shared_Status_Dead);
	}
}

bool UGodsentAttributeSet::OwnerHasTag(const FGameplayTag& Tag) const
{
	const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(Tag);
}

/// Saintly Revival: once per heir, health hitting 0 plays the revive ability (GA_Hero_Revive) instead of the death.
/// The Dead tag is never added, so the death ability, the lose screen and the lineage's death record don't run.
bool UGodsentAttributeSet::TryStartRevival(AActor* AvatarActor)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();

	if (!ASC || !AvatarActor)
		return false;

	// Already kneeling: hits that get through can't kill
	if (ASC->HasMatchingGameplayTag(GodsentGameplayTags::Player_Status_Reviving))
		return true;

	if (!ASC->HasMatchingGameplayTag(GodsentGameplayTags::Player_Passive_SaintlyRevival) ||
		ASC->HasMatchingGameplayTag(GodsentGameplayTags::Shared_Status_Dead))
		return false;

	UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(AvatarActor);

	if (!Lineage || Lineage->HasHeirUsedRevival())
		return false;

	ASC->AddLooseGameplayTag(GodsentGameplayTags::Player_Status_Reviving);

	FGameplayEventData EventData;
	EventData.Instigator = AvatarActor;
	EventData.Target = AvatarActor;

	// Nothing listening (the revive ability isn't granted): die as normal
	if (ASC->HandleGameplayEvent(GodsentGameplayTags::Player_Event_Revive, &EventData) <= 0)
	{
		ASC->RemoveLooseGameplayTag(GodsentGameplayTags::Player_Status_Reviving);
		UE_LOG(LogTemp, Warning, TEXT("[Revival] %s has Saintly Revival but no ability triggered by Player.Event.Revive"), *GetNameSafe(AvatarActor));
		return false;
	}

	// Saved straight away so quitting and continuing doesn't give it back
	Lineage->MarkHeirRevivalUsed();
	return true;
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

/// Plays damage feedback on every damaging hit, and staggers the target only when the hit carries hit stun
void UGodsentAttributeSet::HandleDamageReactions(const FGameplayEffectModCallbackData& Data, float DamageDone)
{
	AActor* TargetActor = Data.Target.GetAvatarActor();
	AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();

	FGameplayTagContainer SpecAssetTags;
	Data.EffectSpec.GetAllAssetTags(SpecAssetTags);

	// Feedback always plays, staggered or not
	if (AGodsentBaseCharacter* TargetCharacter = Cast<AGodsentBaseCharacter>(TargetActor))
		TargetCharacter->BP_OnDamageTaken(DamageDone, InstigatorActor);

	// Lets passive abilities react to being hit (Holy Retribution's thorns); EventMagnitude is the damage taken
	FGameplayEventData DamageTakenEventData;
	DamageTakenEventData.Instigator = InstigatorActor;
	DamageTakenEventData.Target = TargetActor;
	DamageTakenEventData.EventMagnitude = DamageDone;
	DamageTakenEventData.InstigatorTags = SpecAssetTags;   // the damage effect's asset tags (e.g. Shared.Effect.Melee)

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, GodsentGameplayTags::Shared_Event_DamageTaken, DamageTakenEventData);

	// A killing blow plays the death ability instead
	if (GetCurrentHealth() <= 0.f)
		return;
	
	if (!SpecAssetTags.HasTagExact(GodsentGameplayTags::Shared_Effect_HitStun))
		return;

	FGameplayEventData EventData;
	EventData.Instigator = InstigatorActor;
	EventData.Target = TargetActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, GodsentGameplayTags::Shared_Event_HitReact, EventData);
}
