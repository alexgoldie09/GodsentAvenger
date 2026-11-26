// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/Aldegon/GodsentAldegonAbility_SoulBeam.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AbilityTasks/AbilityTask_ExecuteTaskOnTick.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

namespace
{
	const FName SoulBeamTraceTag = TEXT("GodsentSoulBeam");

	/// How long the debug shapes stay up (one frame)
	constexpr float SoulBeamDebugLifetime = 0.f;
	constexpr int32 SoulBeamDebugSegments = 12;

	/// Fully shot out
	constexpr float SoulBeamFullExtend = 1.f;
}

UGodsentAldegonAbility_SoulBeam::UGodsentAldegonAbility_SoulBeam()
{
	// The beam is a fixed animation; speeding it up would just shorten the firing window
	bUseAttackSpeed = false;

	BeamStartEventTag = GodsentGameplayTags::Enemy_Event_Aldegon_BeamStart;
	BeamEndEventTag = GodsentGameplayTags::Enemy_Event_Aldegon_BeamEnd;
}

void UGodsentAldegonAbility_SoulBeam::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bFiring = false;
	TimeSinceDamageTick = 0.f;

	if (!BeamMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no Beam Montage"), *GetNameSafe(this));
		FinishMove(false);
		return;
	}

	UAbilityTask_WaitGameplayEvent* WaitStart = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, BeamStartEventTag, nullptr, true);
	WaitStart->EventReceived.AddDynamic(this, &ThisClass::OnBeamStartEvent);
	WaitStart->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitEnd = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, BeamEndEventTag, nullptr, true);
	WaitEnd->EventReceived.AddDynamic(this, &ThisClass::OnBeamEndEvent);
	WaitEnd->ReadyForActivation();

	UAbilityTask_ExecuteTaskOnTick* TickTask = UAbilityTask_ExecuteTaskOnTick::ExecuteTaskOnTick(this);
	TickTask->OnAbilityTaskTick.AddDynamic(this, &ThisClass::OnTick);
	TickTask->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* PlayMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, BeamMontage, GetMoveMontageRate());

	PlayMontage->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
	PlayMontage->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
	PlayMontage->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
	PlayMontage->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	PlayMontage->ReadyForActivation();
}

void UGodsentAldegonAbility_SoulBeam::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// However it ends, the beam goes
	StopBeam();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGodsentAldegonAbility_SoulBeam::OnBeamStartEvent(FGameplayEventData Payload)
{
	StartBeam();
}

void UGodsentAldegonAbility_SoulBeam::OnBeamEndEvent(FGameplayEventData Payload)
{
	StopBeam();
}

void UGodsentAldegonAbility_SoulBeam::OnMontageFinished()
{
	FinishMove(true);
}

void UGodsentAldegonAbility_SoulBeam::OnMontageCancelled()
{
	FinishMove(false);
}

void UGodsentAldegonAbility_SoulBeam::StartBeam()
{
	if (bFiring)
		return;

	bFiring = true;
	FiringTime = 0.f;

	// The first tick lands as soon as it touches someone
	TimeSinceDamageTick = TickInterval.GetValueAtLevel(GetAbilityLevel());

	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	if (BeamEffect && Character)
	{
		// Starts at his hand facing his way; every tick after moves it with the beam
		BeamComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(BeamEffect, Character->GetMesh(), NAME_None,
			GetBeamStart(), Character->GetActorRotation(), EAttachLocation::KeepWorldPosition, false);
	}
}

void UGodsentAldegonAbility_SoulBeam::StopBeam()
{
	bFiring = false;

	if (!BeamComponent)
		return;

	// Stop spawning and let what's there finish (no pop), then make sure it's gone
	UNiagaraComponent* FinishingBeam = BeamComponent;
	BeamComponent = nullptr;

	FinishingBeam->SetAutoDestroy(true);
	FinishingBeam->Deactivate();

	if (UWorld* World = GetWorld())
	{
		FTimerHandle WindDownHandle;
		World->GetTimerManager().SetTimer(WindDownHandle, FTimerDelegate::CreateWeakLambda(FinishingBeam, [FinishingBeam]()
		{
			FinishingBeam->DestroyComponent();
		}), FMath::Max(WindDownTime, KINDA_SMALL_NUMBER), false);
	}
}

FVector UGodsentAldegonAbility_SoulBeam::GetBeamStart() const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;

	if (!Mesh)
		return Character ? Character->GetActorLocation() : FVector::ZeroVector;

	if (Mesh->DoesSocketExist(BeamSocketName))
		return Mesh->GetSocketLocation(BeamSocketName);

	return Mesh->GetSocketLocation(FallbackBoneName);
}

void UGodsentAldegonAbility_SoulBeam::OnTick(float DeltaTime)
{
	if (!bFiring)
		return;

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	if (!Character)
		return;

	if (!bWarnedMissingSocket && Character->GetMesh() && !Character->GetMesh()->DoesSocketExist(BeamSocketName))
	{
		bWarnedMissingSocket = true;
		UE_LOG(LogTemp, Warning, TEXT("%s: no '%s' socket on the skeleton, firing from '%s'"),
			*GetNameSafe(this), *BeamSocketName.ToString(), *FallbackBoneName.ToString());
	}

	const float Level = GetAbilityLevel();
	const AActor* Target = GetCombatTarget();

	// Slowly turn to follow the player (yaw only)
	if (Target)
	{
		const FVector ToTarget = Target->GetActorLocation() - Character->GetActorLocation();
		const FRotator FacingTarget(0.f, ToTarget.Rotation().Yaw, 0.f);
		Character->SetActorRotation(FMath::RInterpConstantTo(Character->GetActorRotation(), FacingTarget, DeltaTime, TurnRate.GetValueAtLevel(Level)));
	}

	// Aim along where he's facing, tipped up or down to the player's height
	const FVector BeamStart = GetBeamStart();
	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	FVector BeamDirection = Forward;

	if (Target)
	{
		const float FlatDistance = FVector::Dist2D(BeamStart, Target->GetActorLocation());
		BeamDirection = (Forward * FlatDistance + FVector(0.f, 0.f, Target->GetActorLocation().Z - BeamStart.Z)).GetSafeNormal();
	}

	// Shooting out: it only reaches (and hurts) as far as it's got
	FiringTime += DeltaTime;
	const float FullExtendTime = ExtendTime.GetValueAtLevel(Level);
	const float ExtendAlpha = FullExtendTime > 0.f ? FMath::Clamp(FiringTime / FullExtendTime, 0.f, SoulBeamFullExtend) : SoulBeamFullExtend;

	FVector BeamEnd = BeamStart + BeamDirection * Range.GetValueAtLevel(Level) * ExtendAlpha;

	// Walls and pillars stop it
	FCollisionQueryParams Params(SoulBeamTraceTag, false, Character);
	FHitResult WallHit;
	const bool bBlocked = GetWorld()->LineTraceSingleByObjectType(WallHit, BeamStart, BeamEnd, FCollisionObjectQueryParams(WallObjectType.GetValue()), Params);

	if (bBlocked)
		BeamEnd = WallHit.ImpactPoint;

	// Everyone hostile inside the beam
	TArray<FHitResult> PawnHits;
	const float BeamRadius = Radius.GetValueAtLevel(Level);

	GetWorld()->SweepMultiByObjectType(PawnHits, BeamStart, BeamEnd, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(BeamRadius), Params);

	TArray<APawn*> PawnsInBeam;
	TArray<FVector> HitLocations;

	for (const FHitResult& Hit : PawnHits)
	{
		APawn* HitPawn = Cast<APawn>(Hit.GetActor());

		if (!HitPawn || PawnsInBeam.Contains(HitPawn) || !UGodsentFunctionLibrary::IsTargetPawnHostile(Character, HitPawn))
			continue;

		// Dodging through it
		if (UGodsentFunctionLibrary::NativeDoesActorHaveTag(HitPawn, GodsentGameplayTags::Shared_Status_Invincible))
			continue;

		// Where the beam's line passes them (a sweep's impact point isn't reliable when it starts inside them)
		PawnsInBeam.Add(HitPawn);
		HitLocations.Add(FMath::ClosestPointOnSegment(HitPawn->GetActorLocation(), BeamStart, BeamEnd));
	}

	// Damage on the tick
	TimeSinceDamageTick += DeltaTime;
	const float Interval = TickInterval.GetValueAtLevel(Level);

	if (TimeSinceDamageTick >= Interval && !PawnsInBeam.IsEmpty())
	{
		TimeSinceDamageTick = 0.f;
		const FGameplayEffectSpecHandle DamageSpec = MakeMoveDamageSpec(DamagePerTick, bMagicDamage, bCausesHitStun);

		if (DamageSpec.IsValid())
		{
			for (int32 Index = 0; Index < PawnsInBeam.Num(); ++Index)
			{
				APawn* HitPawn = PawnsInBeam[Index];

				if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitPawn))
					continue;

				NativeApplyEffectSpecHandleToTarget(HitPawn, DamageSpec);
				PlayHitCue(HitPawn, HitLocations[Index], BeamDirection);
			}
		}
	}

	UpdateBeamEffect(BeamStart, BeamEnd, BeamDirection, BeamRadius, ExtendAlpha, bBlocked);

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), BeamStart, BeamEnd, FColor::Purple, false, SoulBeamDebugLifetime);
		DrawDebugSphere(GetWorld(), BeamEnd, BeamRadius, SoulBeamDebugSegments, bBlocked ? FColor::Orange : FColor::Purple, false, SoulBeamDebugLifetime);
	}
}

void UGodsentAldegonAbility_SoulBeam::UpdateBeamEffect(const FVector& BeamStart, const FVector& BeamEnd, const FVector& BeamDirection,
	float BeamRadius, float ExtendAlpha, bool bBlocked)
{
	if (!BeamComponent)
		return;

	// At his hand, X pointing down the beam (a mesh along local X just needs scaling by Beam Length)
	BeamComponent->SetWorldLocationAndRotation(BeamStart, BeamDirection.Rotation());

	BeamComponent->SetVariablePosition(BeamStartParameter, BeamStart);
	BeamComponent->SetVariablePosition(BeamEndParameter, BeamEnd);
	BeamComponent->SetVariableFloat(BeamLengthParameter, FVector::Dist(BeamStart, BeamEnd));
	BeamComponent->SetVariableFloat(BeamRadiusParameter, BeamRadius);
	BeamComponent->SetVariableFloat(BeamExtendParameter, ExtendAlpha);
	BeamComponent->SetVariableBool(BeamBlockedParameter, bBlocked);
}

void UGodsentAldegonAbility_SoulBeam::PlayHitCue(APawn* HitPawn, const FVector& HitLocation, const FVector& BeamDirection) const
{
	if (!HitCueTag.IsValid())
		return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitPawn);

	if (!TargetASC)
		return;

	// On the player, so the cue can attach to them
	FGameplayCueParameters CueParameters;
	CueParameters.Location = HitLocation;
	CueParameters.Normal = -BeamDirection;
	CueParameters.Instigator = GetAvatarActorFromActorInfo();
	CueParameters.EffectCauser = GetAvatarActorFromActorInfo();
	TargetASC->ExecuteGameplayCue(HitCueTag, CueParameters);
}
