// Alex Goldberg rights reserved

#include "Components/Shake/GodsentShakeComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Actor.h"

namespace
{
	/// Each axis reads a different stretch of the noise, so they don't all move together
	constexpr float ShakeNoiseOffsetX = 0.f;
	constexpr float ShakeNoiseOffsetY = 31.7f;
	constexpr float ShakeNoiseOffsetZ = 63.1f;
	constexpr float ShakeNoiseOffsetPitch = 97.3f;
	constexpr float ShakeNoiseOffsetYaw = 131.9f;
	constexpr float ShakeNoiseOffsetRoll = 163.7f;

	/// Range the random noise seed is picked from
	constexpr float ShakeNoiseSeedRange = 1000.f;
}

UGodsentShakeComponent::UGodsentShakeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGodsentShakeComponent::BeginPlay()
{
	Super::BeginPlay();

	NoiseSeed = FMath::FRandRange(0.f, ShakeNoiseSeedRange);

	if (!ShakeTarget.IsValid())
		FindShakeTarget();
}

void UGodsentShakeComponent::FindShakeTarget()
{
	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
		return;

	// Tagged first
	if (ShakeTargetTag != NAME_None)
	{
		TArray<UActorComponent*> Tagged = OwnerActor->GetComponentsByTag(USceneComponent::StaticClass(), ShakeTargetTag);

		if (Tagged.Num() > 0)
		{
			ShakeTarget = Cast<USceneComponent>(Tagged[0]);
			return;
		}
	}

	ShakeTarget = OwnerActor->FindComponentByClass<UStaticMeshComponent>();

	if (!ShakeTarget.IsValid())
		UE_LOG(LogTemp, Warning, TEXT("%s: Godsent Shake found nothing to shake (tag a mesh \"%s\")"), *GetNameSafe(OwnerActor), *ShakeTargetTag.ToString());
}

void UGodsentShakeComponent::SetShakeTarget(USceneComponent* NewTarget)
{
	if (bShaking)
		StopShake();

	ShakeTarget = NewTarget;
}

void UGodsentShakeComponent::Shake(float Strength)
{
	USceneComponent* Target = ShakeTarget.Get();

	if (!Target || Strength <= 0.f)
		return;

	// Only remember the rest pose when starting fresh (a restart mid-shake would save a shaken pose)
	if (!bShaking)
		RestTransform = Target->GetRelativeTransform();

	ElapsedTime = 0.f;
	ShakeStrength = Strength;
	bShaking = true;
	SetComponentTickEnabled(true);
}

void UGodsentShakeComponent::StopShake()
{
	if (!bShaking)
		return;

	RestoreRestTransform();
	bShaking = false;
	SetComponentTickEnabled(false);
}

void UGodsentShakeComponent::RestoreRestTransform()
{
	if (USceneComponent* Target = ShakeTarget.Get())
		Target->SetRelativeTransform(RestTransform);
}

void UGodsentShakeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	USceneComponent* Target = ShakeTarget.Get();

	if (!bShaking || !Target)
	{
		bShaking = false;
		SetComponentTickEnabled(false);
		return;
	}

	ElapsedTime += DeltaTime;
	const float Progress = FMath::Clamp(ElapsedTime / Duration, 0.f, 1.f);

	if (Progress >= 1.f)
	{
		StopShake();
		return;
	}

	// Strong at the start, nothing at the end
	const float Fade = (FadeCurve ? FadeCurve->GetFloatValue(Progress) : 1.f - Progress) * ShakeStrength;
	const float NoiseTime = NoiseSeed + ElapsedTime * Frequency;

	const FVector Jitter(
		FMath::PerlinNoise1D(NoiseTime + ShakeNoiseOffsetX) * PositionAmplitude.X,
		FMath::PerlinNoise1D(NoiseTime + ShakeNoiseOffsetY) * PositionAmplitude.Y,
		FMath::PerlinNoise1D(NoiseTime + ShakeNoiseOffsetZ) * PositionAmplitude.Z);

	const FRotator Wobble(
		FMath::PerlinNoise1D(NoiseTime + ShakeNoiseOffsetPitch) * RotationAmplitude.Pitch,
		FMath::PerlinNoise1D(NoiseTime + ShakeNoiseOffsetYaw) * RotationAmplitude.Yaw,
		FMath::PerlinNoise1D(NoiseTime + ShakeNoiseOffsetRoll) * RotationAmplitude.Roll);

	// Swells up and back down once over the shake
	const float Punch = 1.f + ScalePunch * ShakeStrength * FMath::Sin(Progress * PI);

	FTransform Shaken = RestTransform;
	Shaken.AddToTranslation(Jitter * Fade);
	Shaken.SetRotation(RestTransform.GetRotation() * (Wobble * Fade).Quaternion());
	Shaken.SetScale3D(RestTransform.GetScale3D() * Punch);

	Target->SetRelativeTransform(Shaken);
}
