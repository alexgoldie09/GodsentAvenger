// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/GodsentEnemyGameplayAbility.h"
#include "GodsentGameplayTags.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "AbilitySystem/GodsentAttributeSet.h"
#include "Characters/GodsentEnemyCharacter.h"
#include "Engine/World.h"
#include "Items/Projectile/GodsentProjectileBase.h"

/// Gets (and caches) the enemy character that owns this ability
AGodsentEnemyCharacter* UGodsentEnemyGameplayAbility::GetEnemyCharacterFromActorInfo()
{
	if (!CachedGodsentEnemyCharacter.IsValid())
		CachedGodsentEnemyCharacter = Cast<AGodsentEnemyCharacter>(GetAvatarActorFromActorInfo());

	return CachedGodsentEnemyCharacter.IsValid() ? CachedGodsentEnemyCharacter.Get() : nullptr;
}

/// Gets the enemy's combat component
UEnemyCombatComponent* UGodsentEnemyGameplayAbility::GetEnemyCombatComponentFromActorInfo()
{
	AGodsentEnemyCharacter* EnemyCharacter = GetEnemyCharacterFromActorInfo();
	return EnemyCharacter ? EnemyCharacter->GetEnemyCombatComponent() : nullptr;
}

/// Builds the damage spec for an enemy attack from a per-level damage value
FGameplayEffectSpecHandle UGodsentEnemyGameplayAbility::CreateEnemyDamageEffectSpecHandle(
	TSubclassOf<UGameplayEffect> EffectClass, const FScalableFloat& InDamageScalableFloat)
{
	check(EffectClass);

	FGameplayEffectContextHandle ContextHandle = GetGodsentAbilitySystemComponentFromActorInfo()->MakeEffectContext();
	ContextHandle.SetAbility(this);
	ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());
	ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

	FGameplayEffectSpecHandle EffectSpecHandle = GetGodsentAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
		EffectClass,
		GetAbilityLevel(),
		ContextHandle);

	EffectSpecHandle.Data->SetSetByCallerMagnitude(
		GodsentGameplayTags::Shared_SetByCaller_BaseDamage,
		InDamageScalableFloat.GetValueAtLevel(GetAbilityLevel()));

	return EffectSpecHandle;
}

/// The enemy's AttackSpeed attribute, for the Rate pin of Play Montage And Wait
float UGodsentEnemyGameplayAbility::GetAttackSpeedMontageRate() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!ASC)
		return 1.f;

	bool bFound = false;
	const float AttackSpeed = ASC->GetGameplayAttributeValue(UGodsentAttributeSet::GetAttackSpeedAttribute(), bFound);

	return bFound ? FMath::Max(AttackSpeed, 0.1f) : 1.f;
}

/// Spawns a fan of homing projectiles sharing one damage spec
int32 UGodsentEnemyGameplayAbility::SpawnProjectileVolley(TSubclassOf<AGodsentProjectileBase> ProjectileClass,
	const FGameplayEffectSpecHandle& DamageSpecHandle, AActor* HomingTarget, FVector SpawnLocation,
	int32 ProjectileCount, float SpreadAngle, float LaunchPitch)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = AvatarActor ? AvatarActor->GetWorld() : nullptr;

	if (!ProjectileClass || !World || ProjectileCount <= 0)
		return 0;

	// Centre the fan on the target (on the ground plane), or the caster's facing without one
	FVector AimDirection = HomingTarget
		? (HomingTarget->GetActorLocation() - SpawnLocation).GetSafeNormal2D()
		: AvatarActor->GetActorForwardVector().GetSafeNormal2D();

	if (AimDirection.IsNearlyZero())
		AimDirection = AvatarActor->GetActorForwardVector().GetSafeNormal2D();

	const float BaseYaw = AimDirection.Rotation().Yaw;
	int32 SpawnedCount = 0;

	for (int32 Index = 0; Index < ProjectileCount; ++Index)
	{
		// Evenly across the spread, with a little jitter so every volley looks different
		const float SpreadAlpha = ProjectileCount > 1 ? static_cast<float>(Index) / (ProjectileCount - 1) : 0.5f;
		const float Yaw = BaseYaw + FMath::Lerp(-SpreadAngle * 0.5f, SpreadAngle * 0.5f, SpreadAlpha) + FMath::FRandRange(-6.f, 6.f);
		const float Pitch = LaunchPitch + FMath::FRandRange(-8.f, 8.f);

		const FTransform SpawnTransform(FRotator(Pitch, Yaw, 0.f), SpawnLocation);

		AGodsentProjectileBase* Projectile = World->SpawnActorDeferred<AGodsentProjectileBase>(
			ProjectileClass, SpawnTransform, AvatarActor, Cast<APawn>(AvatarActor),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!Projectile)
			continue;

		Projectile->InitProjectile(DamageSpecHandle, HomingTarget);
		Projectile->FinishSpawning(SpawnTransform);
		++SpawnedCount;
	}

	return SpawnedCount;
}
