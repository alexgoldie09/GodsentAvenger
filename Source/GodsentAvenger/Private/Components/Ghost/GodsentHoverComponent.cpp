// Alex Goldberg rights reserved

#include "Components/Ghost/GodsentHoverComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	/// Hover Alpha at either end
	constexpr float HoverAlphaSettled = 0.f;
	constexpr float HoverAlphaFull = 1.f;
}

UGodsentHoverComponent::UGodsentHoverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// After movement, so the sway reads this frame's velocity
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UGodsentHoverComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!VisualRoot.IsValid() && GetOwner())
	{
		TArray<UActorComponent*> TaggedComponents = GetOwner()->GetComponentsByTag(USceneComponent::StaticClass(), VisualRootTag);

		if (TaggedComponents.Num() > 0)
			SetVisualRoot(Cast<USceneComponent>(TaggedComponents[0]));
	}

	if (!VisualRoot.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: GodsentHoverComponent found no scene component tagged '%s'"),
			*GetNameSafe(GetOwner()), *VisualRootTag.ToString());
		SetComponentTickEnabled(false);
	}

	CurrentHoverHeight = HoverHeight;

	// Something (Aldegon) may have already switched the float on or off before this started
	if (!bHoverActiveSetByCode)
	{
		bHoverActive = bStartActive;
		HoverAlpha = bStartActive ? HoverAlphaFull : HoverAlphaSettled;
	}

	// Start each enemy at a different point in its bob so a group doesn't move in sync
	BobPhase = FMath::FRandRange(0.f, 2.f * UE_PI);
	SwayPhase = FMath::FRandRange(0.f, 2.f * UE_PI);
}

void UGodsentHoverComponent::SetVisualRoot(USceneComponent* NewVisualRoot)
{
	// Put the previous root back where it started
	if (USceneComponent* OldRoot = VisualRoot.Get())
		OldRoot->SetRelativeLocationAndRotation(BaseRelativeLocation, BaseRelativeRotation);

	VisualRoot = NewVisualRoot;

	if (NewVisualRoot)
	{
		BaseRelativeLocation = NewVisualRoot->GetRelativeLocation();
		BaseRelativeRotation = NewVisualRoot->GetRelativeRotation();
		SetComponentTickEnabled(true);
	}
}

void UGodsentHoverComponent::SetHoverActive(bool bActive, bool bInstant)
{
	bHoverActive = bActive;
	bHoverActiveSetByCode = true;

	if (bInstant)
		HoverAlpha = bActive ? HoverAlphaFull : HoverAlphaSettled;
}

void UGodsentHoverComponent::SetHoverHeight(float NewHoverHeight)
{
	HoverHeight = NewHoverHeight;
}

void UGodsentHoverComponent::SetAgitation(float NewAgitation)
{
	TargetAgitation = FMath::Clamp(NewAgitation, 0.f, 1.f);
}

void UGodsentHoverComponent::PlayRecoil(float Strength)
{
	RecoilAlpha = FMath::Max(RecoilAlpha, FMath::Max(Strength, 0.f));
}

void UGodsentHoverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	USceneComponent* Root = VisualRoot.Get();
	const AActor* Owner = GetOwner();

	if (!Root || !Owner || DeltaTime <= 0.f)
		return;

	// Movement relative to the owner's facing: X = forward, Y = right
	const FVector LocalVelocity = Owner->GetActorTransform().InverseTransformVectorNoScale(Owner->GetVelocity());
	const float MoveAlpha = FMath::Clamp(LocalVelocity.Size2D() / FullSwaySpeed, 0.f, 1.f);

	HoverAlpha = FMath::FInterpTo(HoverAlpha, bHoverActive ? HoverAlphaFull : HoverAlphaSettled, DeltaTime, HoverBlendSpeed);
	CurrentHoverHeight = FMath::FInterpTo(CurrentHoverHeight, HoverHeight, DeltaTime, HoverBlendSpeed);

	CurrentAgitation = FMath::FInterpTo(CurrentAgitation, TargetAgitation, DeltaTime, AgitationInterpSpeed);
	RecoilAlpha = FMath::FInterpTo(RecoilAlpha, 0.f, DeltaTime, RecoilRecoverySpeed);

	// Bob (faster when agitated, smaller while moving)
	BobPhase = FMath::Fmod(BobPhase + DeltaTime * 2.f * UE_PI * BobFrequency * (1.f + CurrentAgitation * AgitationBobSpeedUp), 2.f * UE_PI);
	const float BobScale = FMath::Lerp(1.f, MovingBobScale, MoveAlpha);
	const float BobZ = FMath::Sin(BobPhase) * BobAmplitude * BobScale;

	// Side-to-side sway while moving
	SwayPhase = FMath::Fmod(SwayPhase + DeltaTime * 2.f * UE_PI * SwayFrequency, 2.f * UE_PI);
	const float SwayWave = FMath::Sin(SwayPhase);
	const float SwayY = SwayWave * SwayDistance * MoveAlpha;
	const float SwayRollNow = SwayWave * SwayRoll * MoveAlpha;

	// Lean into forward movement, bank into sideways movement
	const float ForwardAlpha = FMath::Clamp(LocalVelocity.X / FullSwaySpeed, -1.f, 1.f);
	const float SideAlpha = FMath::Clamp(LocalVelocity.Y / FullSwaySpeed, -1.f, 1.f);
	CurrentLeanPitch = FMath::FInterpTo(CurrentLeanPitch, -ForwardAlpha * MaxLeanPitch, DeltaTime, LeanInterpSpeed);
	CurrentBankRoll = FMath::FInterpTo(CurrentBankRoll, SideAlpha * MaxBankRoll, DeltaTime, LeanInterpSpeed);

	// Agitation shake: a few unrelated fast sine waves read as a jitter
	ShakeTime += DeltaTime;
	const FVector Shake = FVector(
		FMath::Sin(ShakeTime * 37.f),
		FMath::Sin(ShakeTime * 43.f + 1.3f),
		FMath::Sin(ShakeTime * 31.f + 2.1f)) * AgitationShakeDistance * CurrentAgitation;
	const float ShakeRoll = FMath::Sin(ShakeTime * 29.f) * AgitationShakeRoll * CurrentAgitation;

	// Recoil: back and up, nose tipped up
	const FVector RecoilOffset = FVector(-RecoilDistance, 0.f, RecoilDistance * RecoilLift) * RecoilAlpha;
	const float RecoilPitchNow = RecoilPitch * RecoilAlpha;

	// Hover Alpha eases the whole float in and out (settled = exactly where it was placed)
	const FVector Offset = (FVector(0.f, SwayY, CurrentHoverHeight + BobZ) + Shake + RecoilOffset) * HoverAlpha;
	const FRotator RotationOffset = FRotator(CurrentLeanPitch + RecoilPitchNow, 0.f, SwayRollNow + CurrentBankRoll + ShakeRoll) * HoverAlpha;

	// The visual root is assumed to be attached to the capsule, so its relative space is the owner's local space.
	// The tilt is applied in that space too, so a mesh turned -90 in the Blueprint still leans forwards, not sideways.
	const FQuat TiltedRotation = RotationOffset.Quaternion() * BaseRelativeRotation.Quaternion();
	Root->SetRelativeLocationAndRotation(BaseRelativeLocation + Offset, TiltedRotation);
}
