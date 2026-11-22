// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/Aldegon/GodsentAldegonAbility_DeathWave.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Components/Aldegon/GodsentAldegonComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GodsentGameplayTags.h"
#include "Items/Aldegon/GodsentDeathWaveRing.h"

namespace
{
	const FName DeathWaveGroundTraceTag = TEXT("GodsentDeathWaveGround");
}

UGodsentAldegonAbility_DeathWave::UGodsentAldegonAbility_DeathWave()
{
	WaveEventTag = GodsentGameplayTags::Enemy_Event_Aldegon_DeathWave;
	RingClass = AGodsentDeathWaveRing::StaticClass();
}

void UGodsentAldegonAbility_DeathWave::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!FirstWaveMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no First Wave Montage"), *GetNameSafe(this));
		FinishMove(false);
		return;
	}

	// More waves the lower his health
	const UGodsentAldegonComponent* Aldegon = GetAldegonComponent();
	TotalWaves = Aldegon ? Aldegon->GetDeathWaveCount() : 1;
	WavesStarted = 0;
	WavesHandled = 0;
	CurrentMontageTask = nullptr;

	// One listener for every wave's notify
	UAbilityTask_WaitGameplayEvent* WaitWave = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WaveEventTag, nullptr, false);
	WaitWave->EventReceived.AddDynamic(this, &ThisClass::OnWaveEvent);
	WaitWave->ReadyForActivation();

	PlayNextWave();
}

void UGodsentAldegonAbility_DeathWave::PlayNextWave()
{
	// Stop listening to the last montage, so its late callbacks can't count for this one
	if (CurrentMontageTask)
		CurrentMontageTask->EndTask();

	UAnimMontage* Montage = (WavesStarted == 0 || !NextWaveMontage) ? FirstWaveMontage.Get() : NextWaveMontage.Get();
	++WavesStarted;

	CurrentMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, GetMoveMontageRate());

	CurrentMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnWaveMontageDone);
	CurrentMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnWaveMontageDone);
	CurrentMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnWaveMontageDone);
	CurrentMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnWaveMontageCancelled);
	CurrentMontageTask->ReadyForActivation();
}

void UGodsentAldegonAbility_DeathWave::OnWaveMontageDone()
{
	// Blend Out and Completed both fire for one montage
	if (WavesHandled == WavesStarted)
		return;

	WavesHandled = WavesStarted;

	if (WavesStarted >= TotalWaves)
	{
		FinishMove(true);
		return;
	}

	// The gap between waves (the same in frenzy: only his animations speed up)
	const float Spacing = WaveSpacing.GetValueAtLevel(GetAbilityLevel());

	if (Spacing <= 0.f)
	{
		PlayNextWave();
		return;
	}

	UAbilityTask_WaitDelay* WaitSpacing = UAbilityTask_WaitDelay::WaitDelay(this, Spacing);
	WaitSpacing->OnFinish.AddDynamic(this, &ThisClass::OnSpacingFinished);
	WaitSpacing->ReadyForActivation();
}

void UGodsentAldegonAbility_DeathWave::OnSpacingFinished()
{
	if (IsActive())
		PlayNextWave();
}

void UGodsentAldegonAbility_DeathWave::OnWaveMontageCancelled()
{
	FinishMove(false);
}

void UGodsentAldegonAbility_DeathWave::OnWaveEvent(FGameplayEventData Payload)
{
	SpawnRing();
}

void UGodsentAldegonAbility_DeathWave::SpawnRing()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!Character || !ASC || !RingClass)
		return;

	// Dropped on the floor under him (he floats a little above it)
	const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector RingLocation = Character->GetActorLocation() - FVector(0.f, 0.f, CapsuleHalfHeight);

	FHitResult GroundHit;
	FCollisionQueryParams Params(DeathWaveGroundTraceTag, false, Character);
	const FVector TraceEnd = Character->GetActorLocation() - FVector(0.f, 0.f, CapsuleHalfHeight + GroundSearchDistance);

	if (GetWorld()->LineTraceSingleByObjectType(GroundHit, Character->GetActorLocation(), TraceEnd, FCollisionObjectQueryParams(GroundObjectType.GetValue()), Params))
		RingLocation = GroundHit.ImpactPoint;

	const float Level = GetAbilityLevel();
	const FGameplayEffectSpecHandle DamageSpec = MakeMoveDamageSpec(Damage, bMagicDamage, bCausesHitStun);
	const FTransform SpawnTransform(FRotator::ZeroRotator, RingLocation);

	AGodsentDeathWaveRing* Ring = GetWorld()->SpawnActorDeferred<AGodsentDeathWaveRing>(RingClass, SpawnTransform, Character, Character,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Ring)
		return;

	Ring->InitRing(Character, ASC, DamageSpec, RingSpeed.GetValueAtLevel(Level), MaxRadius.GetValueAtLevel(Level),
		RingThickness.GetValueAtLevel(Level), HopClearance.GetValueAtLevel(Level));

	Ring->FinishSpawning(SpawnTransform);
}
