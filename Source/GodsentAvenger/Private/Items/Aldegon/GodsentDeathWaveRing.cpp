// Alex Goldberg rights reserved

#include "Items/Aldegon/GodsentDeathWaveRing.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "NiagaraComponent.h"

namespace
{
	/// Segments in the debug circles
	constexpr int32 DeathWaveDebugSegments = 64;

	/// Debug shapes last one frame
	constexpr float DeathWaveDebugLifetime = 0.f;

	/// Extra reach for the "who's near" check, so big capsules at the band's edge aren't missed
	constexpr float DeathWaveQueryPadding = 200.f;

	/// Progress and Fade run 0 to 1
	constexpr float DeathWaveFullProgress = 1.f;
}

AGodsentDeathWaveRing::AGodsentDeathWaveRing()
{
	PrimaryActorTick.bCanEverTick = true;

	RingRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RingRoot"));
	SetRootComponent(RingRoot);

	RingEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RingEffect"));
	RingEffect->SetupAttachment(RingRoot);
}

void AGodsentDeathWaveRing::InitRing(APawn* InOwnerPawn, UAbilitySystemComponent* InSourceASC, const FGameplayEffectSpecHandle& InDamageSpec,
	float InSpeed, float InMaxRadius, float InThickness, float InHopClearance, const FGameplayTag& InHitCueTag)
{
	HitCueTag = InHitCueTag;
	OwnerPawn = InOwnerPawn;
	SourceASC = InSourceASC;
	DamageSpec = InDamageSpec;
	Speed = FMath::Max(InSpeed, 0.f);
	MaxRadius = FMath::Max(InMaxRadius, 0.f);
	Thickness = FMath::Max(InThickness, 0.f);
	HopClearance = FMath::Max(InHopClearance, 0.f);
	CurrentRadius = 0.f;

	if (Speed <= 0.f)
		UE_LOG(LogTemp, Warning, TEXT("%s: Death Wave speed is 0 (check Aldegon.DeathWaveSpeed), it won't grow"), *GetNameSafe(this));
}

void AGodsentDeathWaveRing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Full size: no more hits, just fade out over Linger Time
	if (bFinishedGrowing)
	{
		LingerElapsed += DeltaTime;

		if (RingEffect)
		{
			const float Fade = LingerTime > 0.f ? FMath::Clamp(LingerElapsed / LingerTime, 0.f, DeathWaveFullProgress) : DeathWaveFullProgress;
			RingEffect->SetVariableFloat(FadeParameter, Fade);
		}

		return;
	}

	CurrentRadius = FMath::Min(CurrentRadius + Speed * DeltaTime, MaxRadius);

	// The effect and the hit check always use the same radius
	if (RingEffect)
	{
		const float Progress = MaxRadius > 0.f ? FMath::Clamp(CurrentRadius / MaxRadius, 0.f, DeathWaveFullProgress) : DeathWaveFullProgress;

		RingEffect->SetVariableFloat(RadiusParameter, CurrentRadius);
		RingEffect->SetVariableFloat(ThicknessParameter, Thickness);
		RingEffect->SetVariableFloat(ProgressParameter, Progress);
		RingEffect->SetVariableFloat(FadeParameter, 0.f);
	}

	BP_OnRingGrown(CurrentRadius, Thickness);
	CheckHits();

	if (bDrawDebug)
	{
		const FVector Centre = GetActorLocation();
		DrawDebugCircle(GetWorld(), Centre, CurrentRadius, DeathWaveDebugSegments, FColor::Red, false, DeathWaveDebugLifetime, 0, 0.f, FVector::ForwardVector, FVector::RightVector, false);
		DrawDebugCircle(GetWorld(), Centre, FMath::Max(CurrentRadius - Thickness, 0.f), DeathWaveDebugSegments, FColor::Orange, false, DeathWaveDebugLifetime, 0, 0.f, FVector::ForwardVector, FVector::RightVector, false);
	}

	// Full size (or can't grow): stop hurting, let the effect fade, then go
	if (CurrentRadius >= MaxRadius || Speed <= 0.f)
	{
		bFinishedGrowing = true;
		SetLifeSpan(FMath::Max(LingerTime, KINDA_SMALL_NUMBER));
	}
}

void AGodsentDeathWaveRing::CheckHits()
{
	APawn* RingOwner = OwnerPawn.Get();
	UAbilitySystemComponent* Source = SourceASC.Get();

	// Aldegon gone (or no damage set up): just a visual
	if (!RingOwner || !Source || !DamageSpec.IsValid())
		return;

	const FVector Centre = GetActorLocation();

	TArray<APawn*> NearbyHostiles;
	UGodsentFunctionLibrary::GetHostilePawnsInRadius(RingOwner, Centre, CurrentRadius + DeathWaveQueryPadding, NearbyHostiles);

	for (APawn* Hostile : NearbyHostiles)
	{
		if (!Hostile || HitActors.Contains(Hostile))
			continue;

		float BodyRadius = 0.f;
		float FeetZ = Hostile->GetActorLocation().Z;

		if (const ACharacter* HostileCharacter = Cast<ACharacter>(Hostile))
		{
			BodyRadius = HostileCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
			FeetZ -= HostileCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}

		// Is the band over any part of them?
		const float FlatDistance = FVector::Dist2D(Centre, Hostile->GetActorLocation());
		const bool bInBand = FlatDistance - BodyRadius <= CurrentRadius && FlatDistance + BodyRadius >= CurrentRadius - Thickness;

		if (!bInBand)
			continue;

		// Jumped it (not marked as hit: landing while it's still over them still counts)
		if (FeetZ - Centre.Z > HopClearance)
			continue;

		// Rolling through it
		if (UGodsentFunctionLibrary::NativeDoesActorHaveTag(Hostile, GodsentGameplayTags::Shared_Status_Invincible))
			continue;

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Hostile);

		if (!TargetASC)
			continue;

		HitActors.Add(Hostile);
		Source->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
		PlayHitCue(Hostile, FeetZ);
	}
}

void AGodsentDeathWaveRing::PlayHitCue(APawn* HitPawn, float FeetZ) const
{
	if (!HitCueTag.IsValid())
		return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitPawn);

	if (!TargetASC)
		return;

	// The point of the band under them, facing outwards (the way the ring is travelling)
	const FVector Centre = GetActorLocation();
	FVector Outwards = (HitPawn->GetActorLocation() - Centre).GetSafeNormal2D();

	if (Outwards.IsNearlyZero())
		Outwards = HitPawn->GetActorForwardVector();

	FGameplayCueParameters CueParameters;
	CueParameters.Location = FVector(Centre.X, Centre.Y, FeetZ) + Outwards * CurrentRadius;
	CueParameters.Normal = Outwards;
	CueParameters.Instigator = OwnerPawn.Get();
	CueParameters.EffectCauser = const_cast<AGodsentDeathWaveRing*>(this);
	TargetASC->ExecuteGameplayCue(HitCueTag, CueParameters);
}
