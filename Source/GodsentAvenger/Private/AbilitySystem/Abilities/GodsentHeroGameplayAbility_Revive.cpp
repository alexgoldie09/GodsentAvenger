// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility_Revive.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GodsentAttributeSet.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Pawn.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "TimerManager.h"

namespace
{
	/// Normal speed for the revive montage
	constexpr float ReviveMontageRate = 1.f;
}

UGodsentHeroGameplayAbility_Revive::UGodsentHeroGameplayAbility_Revive()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Started by the attribute set when the heir would die
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = GodsentGameplayTags::Player_Event_Revive;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void UGodsentHeroGameplayAbility_Revive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bHasRisen = false;

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	if (!ASC || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// A grace timer from an earlier revive can't be running (once per heir), but clear it to be safe
	if (const UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(GraceTimerHandle);

	if (!bAddedInvincible)
	{
		ASC->AddLooseGameplayTag(GodsentGameplayTags::Shared_Status_Invincible);
		bAddedInvincible = true;
		InvincibleASC = ASC;
	}

	UAbilityTask_WaitGameplayEvent* WaitRise = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GodsentGameplayTags::Player_Event_Revive_Rise, nullptr, true);
	WaitRise->EventReceived.AddDynamic(this, &ThisClass::OnRiseEvent);
	WaitRise->ReadyForActivation();

	if (!ReviveMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Revival] %s has no Revive Montage: rising straight away"), *GetNameSafe(this));
		OnMontageFinished();
		return;
	}

	UAbilityTask_PlayMontageAndWait* PlayMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, ReviveMontage, ReviveMontageRate, MontageStartSection);

	PlayMontage->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
	PlayMontage->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
	PlayMontage->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
	PlayMontage->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
	PlayMontage->ReadyForActivation();
}

void UGodsentHeroGameplayAbility_Revive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Never leave the heir at 0 health without the Dead tag
	Rise();

	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		if (ASC->HasMatchingGameplayTag(GodsentGameplayTags::Player_Status_Reviving))
			ASC->RemoveLooseGameplayTag(GodsentGameplayTags::Player_Status_Reviving);

	const float Grace = GraceTime.GetValueAtLevel(GetAbilityLevel());

	// The base EndAbility clears every timer this ability owns, so the grace timer is started after it
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// A little longer to get back into it before enemies can land hits
	UWorld* World = GetWorld();

	if (Grace > 0.f && World)
		World->GetTimerManager().SetTimer(GraceTimerHandle, this, &ThisClass::RemoveInvincible, Grace, false);
	else
		RemoveInvincible();
}

void UGodsentHeroGameplayAbility_Revive::OnRiseEvent(FGameplayEventData Payload)
{
	Rise();
}

void UGodsentHeroGameplayAbility_Revive::OnMontageFinished()
{
	if (IsActive())
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGodsentHeroGameplayAbility_Revive::Rise()
{
	if (bHasRisen)
		return;

	bHasRisen = true;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	APawn* HeroPawn = Cast<APawn>(GetAvatarActorFromActorInfo());

	// Died for real while kneeling (the kill volume): stay dead
	if (!ASC || !HeroPawn || ASC->HasMatchingGameplayTag(GodsentGameplayTags::Shared_Status_Dead))
		return;

	ASC->SetNumericAttributeBase(UGodsentAttributeSet::GetCurrentHealthAttribute(),
		ASC->GetNumericAttribute(UGodsentAttributeSet::GetMaxHealthAttribute()));

	const float Level = GetAbilityLevel();

	UGodsentFunctionLibrary::KnockbackHostilePawnsInRadius(HeroPawn, HeroPawn->GetActorLocation(),
		KnockbackRadius.GetValueAtLevel(Level), KnockbackStrength.GetValueAtLevel(Level), KnockbackUpForce.GetValueAtLevel(Level));

	if (RiseCueTag.IsValid())
		ASC->ExecuteGameplayCue(RiseCueTag, ASC->MakeEffectContext());

	if (RiseVoiceLine.IsValid())
		UGodsentFunctionLibrary::PlayHeroVoiceLine(HeroPawn, RiseVoiceLine);
}

void UGodsentHeroGameplayAbility_Revive::RemoveInvincible()
{
	if (!bAddedInvincible)
		return;

	bAddedInvincible = false;

	if (UAbilitySystemComponent* ASC = InvincibleASC.Get())
		ASC->RemoveLooseGameplayTag(GodsentGameplayTags::Shared_Status_Invincible);

	InvincibleASC.Reset();
}
