// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "NiagaraComponentPoolMethodEnum.h"
#include "AnimNotifyState_HeroTrail.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UHeroCombatComponent;

/**
 * The hero's sword trail. Replaces Timed Niagara Effect on the attack montages.
 *
 * Normal swings: spawns Trail on Socket Name, the same as before.
 * Chaste Cleave swings: spawns Cleave Trail instead and feeds it the extended blade every frame
 * (the same line the cleave hit sweep uses), so the glowing arc is exactly the area that gets hit:
 *  - User.BladeMid    (Position) middle of the extended blade
 *  - User.BladeLength (Float)  hilt to extended tip, use it as the ribbon width
 *  - User.BladeAxis   (Vector) hilt to tip direction, use it as the ribbon facing (Custom Side Vector)
 */
UCLASS(meta = (DisplayName = "Hero Trail"))
class GODSENTAVENGER_API UAnimNotifyState_HeroTrail : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	/// The everyday trail (NS_Hero_Trail)
	UPROPERTY(EditAnywhere, Category = "Trail")
	TObjectPtr<UNiagaraSystem> Trail;

	/// Socket on the character mesh the normal trail follows
	UPROPERTY(EditAnywhere, Category = "Trail")
	FName SocketName = TEXT("WeaponTrailSocket");

	UPROPERTY(EditAnywhere, Category = "Trail")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Trail")
	FRotator RotationOffset = FRotator::ZeroRotator;

	/// Used instead of Trail on a Chaste Cleave swing (NS_Hero_CleaveTrail). Empty = cleave swings use the normal trail.
	UPROPERTY(EditAnywhere, Category = "Cleave")
	TObjectPtr<UNiagaraSystem> CleaveTrail;

	/// Untick on montages that never cleave (the riposte)
	UPROPERTY(EditAnywhere, Category = "Cleave")
	bool bCanShowCleave = true;

	/// Reuse trail components instead of making a new one every swing (Auto Release hands it back once it's faded)
	UPROPERTY(EditAnywhere, Category = "Trail")
	ENCPoolMethod TrailPoolMethod = ENCPoolMethod::AutoRelease;

	UPROPERTY(EditAnywhere, Category = "Cleave|Parameters")
	FName BladeMidParameter = TEXT("BladeMid");

	UPROPERTY(EditAnywhere, Category = "Cleave|Parameters")
	FName BladeLengthParameter = TEXT("BladeLength");

	UPROPERTY(EditAnywhere, Category = "Cleave|Parameters")
	FName BladeAxisParameter = TEXT("BladeAxis");

private:
	/// The hero's combat component when this swing is a cleave, otherwise nullptr
	UHeroCombatComponent* GetCleavingCombatComponent(const USkeletalMeshComponent* MeshComp) const;

	/// Sends the extended blade to the cleave trail
	void UpdateCleaveParameters(UNiagaraComponent* TrailComponent, const UHeroCombatComponent* CombatComponent) const;

	/// One notify object is shared by every mesh playing the montage, so trails are kept per mesh
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<UNiagaraComponent>> ActiveTrails;

	/// Meshes whose current trail is the cleave one
	TSet<TWeakObjectPtr<USkeletalMeshComponent>> CleavingMeshes;
};
