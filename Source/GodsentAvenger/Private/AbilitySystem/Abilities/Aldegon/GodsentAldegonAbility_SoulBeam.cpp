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

namespace
{
	const FName SoulBeamTraceTag = TEXT("GodsentSoulBeam");

	/// How long the debug shapes stay up (one frame)
	constexpr float SoulBeamDebugLifetime = 0.f;
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

	// The first tick lands as soon as it touches someone
	TimeSinceDamageTick = TickInterval.GetValueAtLevel(GetAbilityLevel());

	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	if (BeamEffect && Character)
	{
		BeamComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(BeamEffect, Character->GetMesh(), NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
	}
}

void UGodsentAldegonAbility_SoulBeam::StopBeam()
{
	bFiring = false;

	if (BeamComponent)
	{
		BeamComponent->DestroyComponent();
		BeamComponent = nullptr;
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

	FVector BeamEnd = BeamStart + BeamDirection * Range.GetValueAtLevel(Level);

	// Walls and pillars stop it
	FCollisionQueryParams Params(SoulBeamTraceTag, false, Character);
	FHitResult WallHit;

	if (GetWorld()->LineTraceSingleByObjectType(WallHit, BeamStart, BeamEnd, FCollisionObjectQueryParams(WallObjectType.GetValue()), Params))
		BeamEnd = WallHit.ImpactPoint;

	// Everyone hostile inside the beam
	TArray<FHitResult> PawnHits;
	const float BeamRadius = Radius.GetValueAtLevel(Level);

	GetWorld()->SweepMultiByObjectType(PawnHits, BeamStart, BeamEnd, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(BeamRadius), Params);

	TArray<APawn*> PawnsInBeam;

	for (const FHitResult& Hit : PawnHits)
	{
		APawn* HitPawn = Cast<APawn>(Hit.GetActor());

		if (!HitPawn || PawnsInBeam.Contains(HitPawn) || !UGodsentFunctionLibrary::IsTargetPawnHostile(Character, HitPawn))
			continue;

		// Dodging through it
		if (UGodsentFunctionLibrary::NativeDoesActorHaveTag(HitPawn, GodsentGameplayTags::Shared_Status_Invincible))
			continue;

		PawnsInBeam.Add(HitPawn);
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
			for (APawn* HitPawn : PawnsInBeam)
			{
				if (UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitPawn))
					NativeApplyEffectSpecHandleToTarget(HitPawn, DamageSpec);
			}
		}
	}

	if (BeamComponent)
	{
		BeamComponent->SetVariablePosition(BeamStartParameter, BeamStart);
		BeamComponent->SetVariablePosition(BeamEndParameter, BeamEnd);
	}

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), BeamStart, BeamEnd, FColor::Purple, false, SoulBeamDebugLifetime);
		DrawDebugSphere(GetWorld(), BeamEnd, BeamRadius, 12, FColor::Purple, false, SoulBeamDebugLifetime);
	}
}
