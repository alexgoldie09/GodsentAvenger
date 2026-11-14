// Alex Goldberg rights reserved

#include "Components/Combat/HeroCombatComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"
#include "Items/Weapons/GodsentHeroWeapon.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"

namespace
{
	/// How long the debug shapes stay on screen
	constexpr float CleaveDebugDrawTime = 1.f;

	/// Glow values at either end
	constexpr float GlowOff = 0.f;
	constexpr float GlowOn = 1.f;
}

UHeroCombatComponent::UHeroCombatComponent()
{
	// Off until a cleave swing needs it. Runs after animation so the blade is where it's drawn this frame.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

/// Gets a carried hero weapon by tag
AGodsentHeroWeapon* UHeroCombatComponent::GetHeroCarriedWeaponByTag(FGameplayTag InWeaponTag) const
{
	return Cast<AGodsentHeroWeapon>(GetCharacterCarriedWeaponByTag(InWeaponTag));
}

/// Gets the equipped hero weapon, or nullptr
AGodsentHeroWeapon* UHeroCombatComponent::GetHeroCurrentEquippedWeapon() const
{
	return Cast<AGodsentHeroWeapon>(GetCharacterCurrentEquippedWeapon());
}

/// Gets the equipped weapon's base damage at a level (0 with no weapon equipped)
float UHeroCombatComponent::GetHeroCurrentEquippedWeaponDamageAtLevel(float InLevel) const
{
	const AGodsentHeroWeapon* EquippedWeapon = GetHeroCurrentEquippedWeapon();
	return EquippedWeapon ? EquippedWeapon->HeroWeaponData.WeaponBaseDamage.GetValueAtLevel(InLevel) : 0.f;
}

/// Sends MeleeHit (for the attack ability to apply damage) and HitPause
void UHeroCombatComponent::OnHitTargetActor(AActor* HitActor)
{
	if (OverlappedActors.Contains(HitActor))
		return;

	OverlappedActors.AddUnique(HitActor);

	// Invincible targets (e.g. mid-dodge) ignore the hit entirely
	if (UGodsentFunctionLibrary::NativeDoesActorHaveTag(HitActor, GodsentGameplayTags::Shared_Status_Invincible))
		return;

	FGameplayEventData EventData;
	EventData.Instigator = GetOwningPawn();
	EventData.Target = HitActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetOwningPawn(),
		GodsentGameplayTags::Shared_Event_MeleeHit,
		EventData);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetOwningPawn(),
		GodsentGameplayTags::Player_Event_HitPause,
		FGameplayEventData());
}

/// Sends HitPause as the blade leaves the target
void UHeroCombatComponent::OnWeaponPulledFromTargetActor(AActor* InteractedActor)
{
	
}

bool UHeroCombatComponent::TryStartCleaveSwing(float Chance, float ExtraLength, float Width)
{
	// A new swing always starts clean
	bCleaveSwing = false;
	bHasPreviousBlade = false;

	if (!UGodsentFunctionLibrary::NativeDoesActorHaveTag(GetOwningPawn(), GodsentGameplayTags::Player_Passive_Cleave))
	{
		RefreshTickEnabled();
		return false;
	}

	bCleaveSwing = FMath::FRand() < Chance;

	if (bCleaveSwing)
	{
		CleaveExtraLength = FMath::Max(ExtraLength, 0.f);
		CleaveWidth = FMath::Max(Width, 0.f);

		// Echo stretch: grip to tip, then grip to extended tip
		FVector BladeStart;
		FVector BladeEnd;
		const AGodsentWeaponBase* Weapon = GetCharacterCurrentEquippedWeapon();

		if (Weapon && GetCleaveBladeSegment(BladeStart, BladeEnd))
		{
			const float ExtendedReach = FVector::Dist(Weapon->GetActorLocation(), BladeEnd);
			const float BladeReach = ExtendedReach - CleaveExtraLength;
			CleaveEchoStretch = BladeReach > KINDA_SMALL_NUMBER ? ExtendedReach / BladeReach : 1.f;
		}

		ShowCleaveEcho();
	}

	RefreshTickEnabled();
	return bCleaveSwing;
}

void UHeroCombatComponent::EndCleaveSwing()
{
	bCleaveSwing = false;
	bHasPreviousBlade = false;

	// Ended before it had a chance to grow (it fades out on tick otherwise)
	if (CleaveGlowAlpha <= GlowOff)
		HideCleaveEcho();

	RefreshTickEnabled(); // Keeps ticking until the echo has faded
}

bool UHeroCombatComponent::GetCleaveBladeSegment(FVector& OutStart, FVector& OutEnd) const
{
	const AGodsentWeaponBase* Weapon = GetCharacterCurrentEquippedWeapon();
	const UBoxComponent* HitBox = Weapon ? Weapon->GetWeaponCollisionBox() : nullptr;

	if (!HitBox)
		return false;

	// The hit box's longest side runs along the blade
	const FVector Extent = HitBox->GetScaledBoxExtent();
	const FTransform& BoxTransform = HitBox->GetComponentTransform();

	EAxis::Type BladeAxis = EAxis::X;
	float HalfLength = Extent.X;

	if (Extent.Y > HalfLength) { BladeAxis = EAxis::Y; HalfLength = Extent.Y; }
	if (Extent.Z > HalfLength) { BladeAxis = EAxis::Z; HalfLength = Extent.Z; }

	const FVector Centre = BoxTransform.GetLocation();
	const FVector Along = BoxTransform.GetUnitAxis(BladeAxis) * HalfLength;

	FVector Hilt = Centre - Along;
	FVector Tip = Centre + Along;

	// The tip is the end further from the grip (the weapon's root sits in the hand)
	const FVector Grip = Weapon->GetActorLocation();

	if (FVector::DistSquared(Hilt, Grip) > FVector::DistSquared(Tip, Grip))
		Swap(Hilt, Tip);

	const float ExtraLength = bCleaveSwing ? CleaveExtraLength : 0.f;

	OutStart = Hilt;
	OutEnd = Tip + (Tip - Hilt).GetSafeNormal() * ExtraLength;
	return true;
}

void UHeroCombatComponent::ToggleCurrentEquippedWeaponCollision(bool bShouldEnable)
{
	Super::ToggleCurrentEquippedWeaponCollision(bShouldEnable);

	bWeaponCollisionOn = bShouldEnable;
	bHasPreviousBlade = false; // Each hit window starts its own sweep
	RefreshTickEnabled();
}

void UHeroCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bCleaveSwing && bWeaponCollisionOn)
		SweepCleaveBlade();

	UpdateCleaveGlow(DeltaTime);
	RefreshTickEnabled();
}

void UHeroCombatComponent::SweepCleaveBlade()
{
	FVector BladeStart;
	FVector BladeEnd;

	if (!GetCleaveBladeSegment(BladeStart, BladeEnd))
		return;

	if (!bHasPreviousBlade)
	{
		PreviousBladeStart = BladeStart;
		PreviousBladeEnd = BladeEnd;
		bHasPreviousBlade = true;
		OverlapCleaveCapsule(BladeStart, BladeEnd);
		return;
	}

	// Enough steps that the tip never jumps further than the blade is thick
	const float TipTravel = FVector::Dist(PreviousBladeEnd, BladeEnd);
	const float StepSpacing = FMath::Max(CleaveWidth, KINDA_SMALL_NUMBER);
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(TipTravel / StepSpacing), 1, MaxCleaveSweepSteps);

	for (int32 Step = 1; Step <= Steps; ++Step)
	{
		const float Alpha = static_cast<float>(Step) / Steps;
		OverlapCleaveCapsule(FMath::Lerp(PreviousBladeStart, BladeStart, Alpha), FMath::Lerp(PreviousBladeEnd, BladeEnd, Alpha));
	}

	PreviousBladeStart = BladeStart;
	PreviousBladeEnd = BladeEnd;
}

void UHeroCombatComponent::OverlapCleaveCapsule(const FVector& BladeStart, const FVector& BladeEnd)
{
	APawn* OwningPawn = GetOwningPawn();
	UWorld* World = GetWorld();

	if (!World)
		return;

	const FVector Blade = BladeEnd - BladeStart;
	const float Radius = CleaveWidth * 0.5f;
	const float HalfHeight = Blade.Size() * 0.5f + Radius;
	const FVector Centre = BladeStart + Blade * 0.5f;
	const FQuat Rotation = FRotationMatrix::MakeFromZ(Blade).ToQuat();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GodsentCleave), false, OwningPawn);

	World->OverlapMultiByObjectType(Overlaps, Centre, Rotation, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeCapsule(Radius, HalfHeight), QueryParams);

	if (bDrawCleaveDebug)
		DrawDebugCapsule(World, Centre, HalfHeight, Radius, Rotation, FColor::Green, false, CleaveDebugDrawTime);

	const FVector OwnerForward = OwningPawn->GetActorForwardVector().GetSafeNormal2D();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		APawn* HitPawn = Cast<APawn>(Overlap.GetActor());

		if (!HitPawn || OverlappedActors.Contains(HitPawn) || !UGodsentFunctionLibrary::IsTargetPawnHostile(OwningPawn, HitPawn) ||
			UGodsentFunctionLibrary::NativeDoesActorHaveTag(HitPawn, GodsentGameplayTags::Shared_Status_Dead))
			continue;

		// Nothing behind the hero, whatever the animation does
		const FVector ToTarget = (HitPawn->GetActorLocation() - OwningPawn->GetActorLocation()).GetSafeNormal2D();

		if (FVector::DotProduct(OwnerForward, ToTarget) < CleaveMinFrontDot)
			continue;

		if (bDrawCleaveDebug)
			DrawDebugSphere(World, HitPawn->GetActorLocation(), Radius, 12, FColor::Red, false, CleaveDebugDrawTime);

		// Same path as a normal sword hit (one hit per target per swing)
		OnHitTargetActor(HitPawn);
	}
}

void UHeroCombatComponent::UpdateCleaveGlow(float DeltaTime)
{
	const float Target = bCleaveSwing ? GlowOn : GlowOff;

	if (CleaveGlowAlpha == Target)
		return;

	const float FadeTime = bCleaveSwing ? CleaveGlowFadeInTime : CleaveGlowFadeOutTime;

	CleaveGlowAlpha = FadeTime > 0.f
		? FMath::FInterpConstantTo(CleaveGlowAlpha, Target, DeltaTime, (GlowOn - GlowOff) / FadeTime)
		: Target;

	if (CleaveEchoInstance)
	{
		CleaveEchoInstance->SetScalarParameterValue(CleaveGlowParameterName, CleaveGlowAlpha);
		CleaveEchoInstance->SetScalarParameterValue(CleaveEchoStretchParameterName, CleaveEchoStretch);
	}

	// Fully faded: stop drawing it
	if (CleaveGlowAlpha <= GlowOff)
		HideCleaveEcho();
}

void UHeroCombatComponent::ShowCleaveEcho()
{
	const AGodsentWeaponBase* Weapon = GetCharacterCurrentEquippedWeapon();
	UStaticMeshComponent* WeaponMesh = Weapon ? Weapon->GetWeaponMesh() : nullptr;

	if (!CleaveEchoMaterial || !WeaponMesh)
		return;

	// One instance, made the first time it's needed
	if (!CleaveEchoInstance)
		CleaveEchoInstance = UMaterialInstanceDynamic::Create(CleaveEchoMaterial, this);

	// Swapped weapon: take it off the old one
	if (CleaveEchoMesh.IsValid() && CleaveEchoMesh.Get() != WeaponMesh)
		HideCleaveEcho();

	CleaveEchoInstance->SetScalarParameterValue(CleaveGlowParameterName, CleaveGlowAlpha);
	CleaveEchoInstance->SetScalarParameterValue(CleaveEchoStretchParameterName, CleaveEchoStretch);

	if (bHideSwordDuringCleave)
	{
		// Remember the sword's own look once (a new cleave during the fade keeps the first copy)
		if (!bSwordMaterialsSwapped)
		{
			SwappedOutSwordMaterials.Reset();

			for (int32 MaterialIndex = 0; MaterialIndex < WeaponMesh->GetNumMaterials(); ++MaterialIndex)
				SwappedOutSwordMaterials.Add(WeaponMesh->GetMaterial(MaterialIndex));

			bSwordMaterialsSwapped = true;
		}

		for (int32 MaterialIndex = 0; MaterialIndex < WeaponMesh->GetNumMaterials(); ++MaterialIndex)
			WeaponMesh->SetMaterial(MaterialIndex, CleaveEchoInstance);
	}
	else
	{
		WeaponMesh->SetOverlayMaterial(CleaveEchoInstance);
	}

	CleaveEchoMesh = WeaponMesh;
}

void UHeroCombatComponent::HideCleaveEcho()
{
	UMeshComponent* EchoMesh = CleaveEchoMesh.Get();

	// Only clear it if it's still ours
	if (EchoMesh && EchoMesh->GetOverlayMaterial() == CleaveEchoInstance.Get())
		EchoMesh->SetOverlayMaterial(nullptr);

	// Give the sword its own materials back
	if (bSwordMaterialsSwapped)
	{
		if (EchoMesh)
		{
			for (int32 MaterialIndex = 0; MaterialIndex < SwappedOutSwordMaterials.Num(); ++MaterialIndex)
				EchoMesh->SetMaterial(MaterialIndex, SwappedOutSwordMaterials[MaterialIndex]);
		}

		SwappedOutSwordMaterials.Reset();
		bSwordMaterialsSwapped = false;
	}

	CleaveEchoMesh.Reset();
}

void UHeroCombatComponent::RefreshTickEnabled()
{
	const bool bNeedsTick = bCleaveSwing || CleaveGlowAlpha > GlowOff;

	if (IsComponentTickEnabled() != bNeedsTick)
		SetComponentTickEnabled(bNeedsTick);
}
