// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/Aldegon/GodsentTotemAbility_Siphon.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/GodsentAttributeSet.h"
#include "Components/Shake/GodsentShakeComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"

namespace
{
	/// Shortest gap between ticks, so a 0 in the table can't tick every frame
	constexpr float TotemMinSiphonInterval = 0.1f;

	/// Heal Percent is a percentage
	constexpr float TotemPercentDivisor = 100.f;

	constexpr int32 TotemDebugSegments = 24;
	constexpr float TotemDebugLineThickness = 2.f;
}

UGodsentTotemAbility_Siphon::UGodsentTotemAbility_Siphon()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Runs from the moment the totem gets it (DA_BoneTotem's Activate On Given Abilities)
	AbilityActivationPolicy = EGodsentAbilityActivationPolicy::OnGiven;
}

void UGodsentTotemAbility_Siphon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!DamageEffect)
		UE_LOG(LogTemp, Warning, TEXT("%s: no Damage Effect set (use GE_Aldegon_RangedDamage), it will only shake"), *GetNameSafe(this));

	WaitForNextTick();
}

void UGodsentTotemAbility_Siphon::WaitForNextTick()
{
	const float Interval = FMath::Max(SiphonInterval.GetValueAtLevel(GetAbilityLevel()), TotemMinSiphonInterval);

	UAbilityTask_WaitDelay* WaitTick = UAbilityTask_WaitDelay::WaitDelay(this, Interval);
	WaitTick->OnFinish.AddDynamic(this, &ThisClass::OnTickDelayFinished);
	WaitTick->ReadyForActivation();
}

void UGodsentTotemAbility_Siphon::OnTickDelayFinished()
{
	if (!IsActive())
		return;

	AActor* HitTarget = DrainTargets();

	if (HitTarget)
		HealSummoner();

	if (HitTarget || bPulseWithoutTarget)
		PlayPulse(HitTarget);

	WaitForNextTick();
}

AActor* UGodsentTotemAbility_Siphon::DrainTargets()
{
	APawn* TotemPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!TotemPawn || !ASC)
		return nullptr;

	const float Level = GetAbilityLevel();
	const float Radius = SiphonRadius.GetValueAtLevel(Level);

	// Living, hittable players in range (dodging = Invincible, skipped)
	TArray<APawn*> Targets;
	UGodsentFunctionLibrary::GetHostilePawnsInRadius(TotemPawn, TotemPawn->GetActorLocation(), Radius, Targets);

	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), TotemPawn->GetActorLocation(), Radius, TotemDebugSegments,
			Targets.IsEmpty() ? FColor::Silver : FColor::Green, false, SiphonInterval.GetValueAtLevel(Level), 0, TotemDebugLineThickness);
	}

	if (Targets.IsEmpty())
		return nullptr;

	if (DamageEffect)
	{
		FGameplayEffectSpecHandle SpecHandle = CreateEnemyDamageEffectSpecHandle(DamageEffect, SiphonDamage);

		if (SpecHandle.IsValid())
		{
			// Magic: straight through defence
			if (bMagicDamage)
				SpecHandle.Data->AddDynamicAssetTag(GodsentGameplayTags::Shared_Effect_IgnoreDefense);

			if (bCausesHitStun)
				SpecHandle.Data->AddDynamicAssetTag(GodsentGameplayTags::Shared_Effect_HitStun);

			for (APawn* Target : Targets)
			{
				if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
					ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}
	}

	return Targets[0];
}

AActor* UGodsentTotemAbility_Siphon::GetSummoner() const
{
	// Aldegon is the totem's Instigator (its Owner becomes its AI controller once it's possessed)
	const AActor* TotemActor = GetAvatarActorFromActorInfo();
	return TotemActor ? TotemActor->GetInstigator() : nullptr;
}

void UGodsentTotemAbility_Siphon::HealSummoner()
{
	// A totem placed by hand has no one to heal
	AActor* Summoner = GetSummoner();
	UAbilitySystemComponent* SummonerASC = Summoner ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Summoner) : nullptr;

	if (!SummonerASC || SummonerASC->HasMatchingGameplayTag(GodsentGameplayTags::Shared_Status_Dead))
		return;

	const float MaxHealth = SummonerASC->GetNumericAttribute(UGodsentAttributeSet::GetMaxHealthAttribute());
	const float CurrentHealth = SummonerASC->GetNumericAttribute(UGodsentAttributeSet::GetCurrentHealthAttribute());
	const float WantedHeal = MaxHealth * HealPercent.GetValueAtLevel(GetAbilityLevel()) / TotemPercentDivisor;

	// Never past full
	const float HealAmount = FMath::Min(WantedHeal, MaxHealth - CurrentHealth);

	// Same as his heal on the throne: the attribute set updates his boss bar
	if (HealAmount > 0.f)
		SummonerASC->SetNumericAttributeBase(UGodsentAttributeSet::GetCurrentHealthAttribute(), CurrentHealth + HealAmount);
}

void UGodsentTotemAbility_Siphon::PlayPulse(AActor* HitTarget)
{
	AActor* TotemActor = GetAvatarActorFromActorInfo();

	if (!TotemActor)
		return;

	if (UGodsentShakeComponent* Shaker = TotemActor->FindComponentByClass<UGodsentShakeComponent>())
		Shaker->Shake(ShakeStrength);

	if (!PulseCueTag.IsValid())
		return;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayCueParameters CueParameters;

		// The floor under it, for ground effects (tethers use the totem itself: the cue's target)
		FVector FloorLocation = TotemActor->GetActorLocation();

		if (const ACharacter* TotemCharacter = Cast<ACharacter>(TotemActor))
			FloorLocation.Z -= TotemCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		CueParameters.Location = FloorLocation;
		CueParameters.RawMagnitude = SiphonRadius.GetValueAtLevel(GetAbilityLevel());
		CueParameters.EffectCauser = HitTarget;
		CueParameters.Instigator = GetSummoner();
		ASC->ExecuteGameplayCue(PulseCueTag, CueParameters);
	}
}
