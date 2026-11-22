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
	float InSpeed, float InMaxRadius, float InThickness, float InHopClearance)
{
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

	if (bFinishedGrowing)
		return;

	CurrentRadius = FMath::Min(CurrentRadius + Speed * DeltaTime, MaxRadius);

	// The effect and the hit check always use the same radius
	if (RingEffect)
	{
		RingEffect->SetVariableFloat(RadiusParameter, CurrentRadius);
		RingEffect->SetVariableFloat(ThicknessParameter, Thickness);
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
	}
}
