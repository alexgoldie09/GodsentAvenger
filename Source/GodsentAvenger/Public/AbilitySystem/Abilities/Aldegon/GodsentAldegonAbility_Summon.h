// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Abilities/Aldegon/GodsentAldegonGameplayAbility.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "GodsentAldegonAbility_Summon.generated.h"

class AGodsentEnemyCharacter;
class UAnimMontage;
struct FStreamableHandle;

/**
 * Summon and Life Siphon (GA_Aldegon_Summon, GA_Aldegon_LifeSiphon): plays the Summon montage and,
 * on its Summon Event notify, spawns enemies from a weighted pool around the tree's SummonLocation.
 * - Summon Type picks which cap they count towards (Add = zombies and ghosts, Totem = bone totems).
 * - How many: a random number between Min and Max Per Cast, never more than his cap has room for.
 * - The pool is soft references, loaded in the background as soon as he's given the ability.
 * - Each summon gets his level, goes straight for the player and is registered with him
 *   (so it counts towards his cap and dies when he dies or the player leaves).
 * The cooldown starts after the montage, unless he was cut off before the summon notify.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentAldegonAbility_Summon : public UGodsentAldegonGameplayAbility
{
	GENERATED_BODY()

public:
	UGodsentAldegonAbility_Summon();

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/// AM_Aldegon_Anim_Summon (Summon and Life Siphon share it)
	UPROPERTY(EditDefaultsOnly, Category = "Summon")
	TObjectPtr<UAnimMontage> SummonMontage;

	/// Notify event in the montage that spawns everything
	UPROPERTY(EditDefaultsOnly, Category = "Summon", meta = (Categories = "Enemy.Event"))
	FGameplayTag SummonEventTag;

	/// Add (zombies and ghosts) or Totem: which of his caps these count towards
	UPROPERTY(EditDefaultsOnly, Category = "Summon")
	EGodsentAldegonSummonType SummonType = EGodsentAldegonSummonType::Add;

	/// What to summon (weighted, e.g. Zombie 2 + Ghost 1 = two thirds zombies)
	UPROPERTY(EditDefaultsOnly, Category = "Summon")
	TArray<FGodsentEnemySpawnEntry> SummonPool;

	/// Blackboard vector the tree's EQS writes (where he's summoning). Not set = around him.
	UPROPERTY(EditDefaultsOnly, Category = "Summon")
	FName SummonLocationKeyName = TEXT("SummonLocation");

	// ---- Stats (CT_AldegonStats rows) ----

	/// Fewest summoned per cast (before the cap)
	UPROPERTY(EditDefaultsOnly, Category = "Summon|Stats")
	FScalableFloat MinPerCast = 1.f;

	/// Most summoned per cast (before the cap)
	UPROPERTY(EditDefaultsOnly, Category = "Summon|Stats")
	FScalableFloat MaxPerCast = 1.f;

	/// They appear within this distance of the summon spot (cm). 0 = right on it.
	UPROPERTY(EditDefaultsOnly, Category = "Summon|Stats")
	FScalableFloat SpreadRadius = 1.f;

	/// No two summons closer than this, his living ones included (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Summon|Stats")
	FScalableFloat MinSpacing = 1.f;

	// ---- Rules ----

	/// Summons spawn facing the player (otherwise a random way)
	UPROPERTY(EditDefaultsOnly, Category = "Summon|Rules")
	bool bFaceTarget = true;

	/// Tries per summon to find a spot that respects Min Spacing (after that it takes any reachable spot)
	UPROPERTY(EditDefaultsOnly, Category = "Summon|Rules", meta = (ClampMin = "1"))
	int32 MaxPlacementAttempts = 12;

	/// How far from the summon spot to look for the navmesh (he's in the air, so mostly down)
	UPROPERTY(EditDefaultsOnly, Category = "Summon|Rules")
	FVector NavSearchExtent = FVector(500.f, 500.f, 2000.f);

	// ---- Visuals ----

	/// GameplayCue played where each one appears (Location = the floor there, Effect Causer = the summon). Empty = none.
	UPROPERTY(EditDefaultsOnly, Category = "Summon|Visuals", meta = (Categories = "GameplayCue"))
	FGameplayTag SpawnCueTag;

	/// Draws each spawn spot and the spread circle for a few seconds
	UPROPERTY(EditDefaultsOnly, Category = "Summon|Debug")
	bool bDrawDebug = false;

private:
	UFUNCTION()
	void OnSummonEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageDone();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	/// Spawns this cast's summons (once per cast)
	void SpawnSummons();

	/// The summon spot on the navmesh: the blackboard's SummonLocation, or under him
	bool GetSummonCentre(FVector& OutCentre) const;

	/// A reachable spot near Centre with real floor and room for the capsule, Min Spacing from Taken where possible.
	/// OutSpot is the floor there.
	bool FindSummonSpot(const FVector& Centre, float Spread, float Spacing, float CapsuleRadius, float CapsuleHalfHeight,
		TArray<FVector>& Taken, FVector& OutSpot) const;

	/// Finds the real floor at a navmesh point and checks the capsule fits standing on it (not inside the floor or under it)
	bool FitsOnFloor(const FVector& NavPoint, float CapsuleRadius, float CapsuleHalfHeight, FVector& OutFloorSpot) const;

	AGodsentEnemyCharacter* SpawnOneSummon(TSubclassOf<AGodsentEnemyCharacter> SummonClass, const FVector& FloorSpot, AActor* Target);

	/// Keeps the pool loaded while he has the ability
	TSharedPtr<FStreamableHandle> PoolLoadHandle;

	bool bSummonedThisCast = false;
};
