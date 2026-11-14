// Alex Goldberg rights reserved

#include "AnimNotifies/AnimNotifyState_HeroTrail.h"
#include "Components/Combat/HeroCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

void UAnimNotifyState_HeroTrail::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
		return;

	// A quick restart can begin again before the last one ended: let the old trail fade
	if (const TWeakObjectPtr<UNiagaraComponent>* OldTrail = ActiveTrails.Find(MeshComp))
		if (UNiagaraComponent* OldTrailComponent = OldTrail->Get())
			OldTrailComponent->Deactivate();

	ActiveTrails.Remove(MeshComp);
	CleavingMeshes.Remove(MeshComp);

	UNiagaraComponent* TrailComponent = nullptr;
	const UHeroCombatComponent* CleavingCombat = GetCleavingCombatComponent(MeshComp);

	if (CleavingCombat && CleaveTrail)
	{
		// World space: the trail is placed from the blade every frame
		TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(MeshComp, CleaveTrail, MeshComp->GetComponentLocation(),
			FRotator::ZeroRotator, FVector::OneVector, true, false);

		if (TrailComponent)
		{
			UpdateCleaveParameters(TrailComponent, CleavingCombat);
			TrailComponent->Activate();
			CleavingMeshes.Add(MeshComp);
		}
	}
	else if (Trail)
	{
		TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(Trail, MeshComp, SocketName, LocationOffset, RotationOffset,
			EAttachLocation::KeepRelativeOffset, true);
	}

	if (TrailComponent)
		ActiveTrails.Add(MeshComp, TrailComponent);
}

void UAnimNotifyState_HeroTrail::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp || !CleavingMeshes.Contains(MeshComp))
		return;

	const TWeakObjectPtr<UNiagaraComponent>* ActiveTrail = ActiveTrails.Find(MeshComp);
	UNiagaraComponent* TrailComponent = ActiveTrail ? ActiveTrail->Get() : nullptr;

	if (!TrailComponent)
		return;

	// The cleave can end mid-trail (ability ended): the blade length drops back to normal
	if (const AActor* Owner = MeshComp->GetOwner())
		if (const UHeroCombatComponent* CombatComponent = Owner->FindComponentByClass<UHeroCombatComponent>())
			UpdateCleaveParameters(TrailComponent, CombatComponent);
}

void UAnimNotifyState_HeroTrail::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	// Stop spawning; what's already there fades out and the component destroys itself
	if (const TWeakObjectPtr<UNiagaraComponent>* ActiveTrail = ActiveTrails.Find(MeshComp))
		if (UNiagaraComponent* TrailComponent = ActiveTrail->Get())
			TrailComponent->Deactivate();

	ActiveTrails.Remove(MeshComp);
	CleavingMeshes.Remove(MeshComp);

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString UAnimNotifyState_HeroTrail::GetNotifyName_Implementation() const
{
	return TEXT("Hero Trail");
}

UHeroCombatComponent* UAnimNotifyState_HeroTrail::GetCleavingCombatComponent(const USkeletalMeshComponent* MeshComp) const
{
	if (!bCanShowCleave || !MeshComp)
		return nullptr;

	// The montage preview has no hero, so it always shows the normal trail
	const AActor* Owner = MeshComp->GetOwner();
	UHeroCombatComponent* CombatComponent = Owner ? Owner->FindComponentByClass<UHeroCombatComponent>() : nullptr;

	return CombatComponent && CombatComponent->IsCleaveSwing() ? CombatComponent : nullptr;
}

void UAnimNotifyState_HeroTrail::UpdateCleaveParameters(UNiagaraComponent* TrailComponent, const UHeroCombatComponent* CombatComponent) const
{
	FVector BladeStart;
	FVector BladeEnd;

	if (!TrailComponent || !CombatComponent || !CombatComponent->GetCleaveBladeSegment(BladeStart, BladeEnd))
		return;

	const FVector Blade = BladeEnd - BladeStart;

	TrailComponent->SetVariablePosition(BladeMidParameter, BladeStart + Blade * 0.5f);
	TrailComponent->SetVariableFloat(BladeLengthParameter, Blade.Size());
	TrailComponent->SetVariableVec3(BladeAxisParameter, Blade.GetSafeNormal());
}
