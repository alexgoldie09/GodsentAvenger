// Alex Goldberg rights reserved

#include "GodsentTypes/GodsentStructTypes.h"
#include "GameplayEffect.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"
#include "Characters/GodsentEnemyCharacter.h"

/// True when both the input tag and the ability are set
bool FGodsentHeroAbilitySet::IsValid() const
{
	return InputTag.IsValid() && AbilityToGrant;
}

/// False for skills that aren't built yet (no ability and no passive)
bool FGodsentSkillNode::IsImplemented() const
{
	return Ability.AbilityToGrant || PassiveEffect;
}

// ---- Enemy pools ----

TSubclassOf<AGodsentEnemyCharacter> FGodsentEnemySpawnEntry::ResolveClass() const
{
	if (EnemyClass.IsNull())
		return nullptr;

	if (UClass* LoadedClass = EnemyClass.Get())
		return LoadedClass;

	// Should have been loaded already: works, but may hitch
	UE_LOG(LogTemp, Warning, TEXT("Enemy pool: %s wasn't loaded yet, loading it now"), *EnemyClass.ToString());
	return EnemyClass.LoadSynchronous();
}

const FGodsentEnemySpawnEntry* FGodsentEnemySpawnEntry::PickWeighted(const TArray<FGodsentEnemySpawnEntry>& Pool)
{
	float TotalWeight = 0.f;

	for (const FGodsentEnemySpawnEntry& Entry : Pool)
	{
		if (!Entry.EnemyClass.IsNull() && Entry.Weight > 0.f)
			TotalWeight += Entry.Weight;
	}

	if (TotalWeight <= 0.f)
		return nullptr;

	float Roll = FMath::FRandRange(0.f, TotalWeight);

	for (const FGodsentEnemySpawnEntry& Entry : Pool)
	{
		if (Entry.EnemyClass.IsNull() || Entry.Weight <= 0.f)
			continue;

		if (Roll <= Entry.Weight)
			return &Entry;

		Roll -= Entry.Weight;
	}

	return nullptr;
}

TArray<FSoftObjectPath> FGodsentEnemySpawnEntry::GetClassPaths(const TArray<FGodsentEnemySpawnEntry>& Pool)
{
	TArray<FSoftObjectPath> Paths;

	for (const FGodsentEnemySpawnEntry& Entry : Pool)
	{
		if (!Entry.EnemyClass.IsNull())
			Paths.AddUnique(Entry.EnemyClass.ToSoftObjectPath());
	}

	return Paths;
}

bool FGodsentEnemySpawnEntry::IsPoolLoaded(const TArray<FGodsentEnemySpawnEntry>& Pool)
{
	for (const FGodsentEnemySpawnEntry& Entry : Pool)
	{
		if (!Entry.EnemyClass.IsNull() && !Entry.EnemyClass.Get())
			return false;
	}

	return true;
}
