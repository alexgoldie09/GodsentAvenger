// Alex Goldberg rights reserved

#include "Subsystems/GodsentLineageSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GodsentGameInstance.h"
#include "GodsentGameplayTags.h"
#include "Kismet/GameplayStatics.h"

void UGodsentLineageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// No save file yet: every session starts as a new family
	LineageData = FGodsentLineageData();
}

UGodsentLineageSubsystem* UGodsentLineageSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;

	return GameInstance ? GameInstance->GetSubsystem<UGodsentLineageSubsystem>() : nullptr;
}

/// The single save point. Logs only for now; the real save goes here later.
void UGodsentLineageSubsystem::CommitProgress()
{
	UE_LOG(LogTemp, Display, TEXT("[Lineage] CommitProgress: generation %d, heir '%s', %d skill point(s), %d skill(s) unlocked, level %s%s"),
		LineageData.Generation,
		*LineageData.HeirName,
		LineageData.SkillPoints,
		LineageData.UnlockedSkills.Num(),
		*LineageData.LastLevelTag.ToString(),
		LineageData.bAwaitingHeirName ? TEXT(", awaiting heir name") : TEXT(""));
}

/// Resets the lineage to a brand-new family
void UGodsentLineageSubsystem::StartNewGame()
{
	LineageData = FGodsentLineageData();
	BroadcastLineageChanged();

	UE_LOG(LogTemp, Display, TEXT("[Lineage] New game"));
}

void UGodsentLineageSubsystem::StartNewGameAndOpenLevel(const UObject* WorldContextObject)
{
	StartNewGame();
	OpenLastLevel(WorldContextObject);
}

/// Reloads the level the lineage was last in (the game map if none is recorded)
void UGodsentLineageSubsystem::OpenLastLevel(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;

	if (!World)
		return;

	const FGameplayTag LevelTag = LineageData.LastLevelTag.IsValid() ? LineageData.LastLevelTag : GodsentGameplayTags::GameData_Level_GameMap;

	if (OpenLevelByTag(World, LevelTag))
		return;

	// Tag not set up in the game instance's level list: reload whatever level we're in
	UE_LOG(LogTemp, Warning, TEXT("[Lineage] No level for %s in the game instance's level sets, reloading the current level"), *LevelTag.ToString());
	UGameplayStatics::OpenLevel(World, FName(*UGameplayStatics::GetCurrentLevelName(World)));
}

/// Commits, then opens the main menu map. The lineage is kept for the menu's Continue button.
void UGodsentLineageSubsystem::OpenMainMenu(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;

	if (!World)
		return;

	if (HasLineageToContinue())
		CommitProgress();

	if (!OpenLevelByTag(World, GodsentGameplayTags::GameData_Level_MainMenuMap))
		UE_LOG(LogTemp, Warning, TEXT("[Lineage] No level for GameData.Level.MainMenuMap in the game instance's level sets"));
}

bool UGodsentLineageSubsystem::OpenLevelByTag(UWorld* World, const FGameplayTag& LevelTag) const
{
	const UGodsentGameInstance* GodsentGameInstance = Cast<UGodsentGameInstance>(GetGameInstance());

	if (!GodsentGameInstance)
		return false;

	const TSoftObjectPtr<UWorld> Level = GodsentGameInstance->GetGameLevelByTag(LevelTag);

	if (Level.IsNull())
		return false;

	UGameplayStatics::OpenLevelBySoftObjectPtr(World, Level);
	return true;
}

/// The heir died: next generation, +1 skill point, the next heir needs a name
void UGodsentLineageSubsystem::RecordHeirDeath()
{
	++LineageData.Generation;
	++LineageData.SkillPoints;
	LineageData.bAwaitingHeirName = true;

	BroadcastLineageChanged();
	CommitProgress();
}

bool UGodsentLineageSubsystem::ValidateHeirName(const FString& ProposedName, FText& OutReason) const
{
	const FString Name = ProposedName.TrimStartAndEnd();
	OutReason = FText::GetEmpty();

	if (Name.IsEmpty())
	{
		OutReason = FText::FromString(TEXT("Enter a name."));
		return false;
	}

	if (Name.Len() > MaxHeirNameLength)
	{
		OutReason = FText::FromString(FString::Printf(TEXT("Names can be at most %d characters."), MaxHeirNameLength));
		return false;
	}

	for (const TCHAR Character : Name)
	{
		if (!FChar::IsAlpha(Character) && Character != TEXT(' ') && Character != TEXT('-') && Character != TEXT('\''))
		{
			OutReason = FText::FromString(TEXT("Use letters, spaces, hyphens and apostrophes only."));
			return false;
		}
	}

	// The founder names freely; each later heir inherits the name with exactly one change
	if (!LineageData.HeirName.IsEmpty())
	{
		const int32 Distance = GetNameEditDistance(LineageData.HeirName, Name);

		if (Distance == 0)
		{
			OutReason = FText::FromString(TEXT("Change one letter: swap, add or remove one."));
			return false;
		}

		if (Distance > 1)
		{
			OutReason = FText::FromString(TEXT("Only one letter can change."));
			return false;
		}
	}

	return true;
}

bool UGodsentLineageSubsystem::SetHeirName(const FString& ProposedName, FText& OutReason)
{
	if (!ValidateHeirName(ProposedName, OutReason))
		return false;

	LineageData.HeirName = ProposedName.TrimStartAndEnd();
	LineageData.bAwaitingHeirName = false;

	BroadcastLineageChanged();
	return true;
}

void UGodsentLineageSubsystem::SetLastLevelTag(const FGameplayTag& LevelTag)
{
	if (!LevelTag.IsValid() || LevelTag == LineageData.LastLevelTag)
		return;

	LineageData.LastLevelTag = LevelTag;
	BroadcastLineageChanged();
}

void UGodsentLineageSubsystem::SetSkillProgress(int32 InSkillPoints, const FGameplayTagContainer& InUnlockedSkills)
{
	LineageData.SkillPoints = FMath::Max(InSkillPoints, 0);
	LineageData.UnlockedSkills = InUnlockedSkills;

	BroadcastLineageChanged();
}

void UGodsentLineageSubsystem::DebugSetGeneration(int32 NewGeneration)
{
	LineageData.Generation = FMath::Max(NewGeneration, 1);
	BroadcastLineageChanged();
}

/// 1 for generations 1-5, 2 for 6-10, and so on
int32 UGodsentLineageSubsystem::GetEnemyTier() const
{
	return 1 + FMath::Max(LineageData.Generation - 1, 0) / GenerationsPerEnemyTier;
}

/// Levenshtein distance: the fewest single-letter swaps, additions or removals to turn A into B
int32 UGodsentLineageSubsystem::GetNameEditDistance(const FString& A, const FString& B)
{
	const int32 LengthA = A.Len();
	const int32 LengthB = B.Len();

	TArray<int32> PreviousRow;
	TArray<int32> CurrentRow;
	PreviousRow.SetNumUninitialized(LengthB + 1);
	CurrentRow.SetNumUninitialized(LengthB + 1);

	for (int32 Column = 0; Column <= LengthB; ++Column)
		PreviousRow[Column] = Column;

	for (int32 Row = 1; Row <= LengthA; ++Row)
	{
		CurrentRow[0] = Row;

		for (int32 Column = 1; Column <= LengthB; ++Column)
		{
			const int32 SwapCost = A[Row - 1] == B[Column - 1] ? 0 : 1;

			CurrentRow[Column] = FMath::Min3(
				PreviousRow[Column] + 1,			// remove a letter
				CurrentRow[Column - 1] + 1,			// add a letter
				PreviousRow[Column - 1] + SwapCost);	// swap a letter (or keep it)
		}

		Swap(PreviousRow, CurrentRow);
	}

	return PreviousRow[LengthB];
}

void UGodsentLineageSubsystem::BroadcastLineageChanged()
{
	OnLineageChanged.Broadcast(LineageData);
}
