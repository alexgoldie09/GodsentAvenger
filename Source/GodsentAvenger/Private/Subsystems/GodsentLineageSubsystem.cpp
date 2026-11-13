// Alex Goldberg rights reserved

#include "Subsystems/GodsentLineageSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/GodsentMainMenuGameMode.h"
#include "GodsentGameInstance.h"
#include "GodsentGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "SaveGame/GodsentLineageSaveGame.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/GodsentSavePopupWidget.h"
#include "Widgets/SViewport.h"

const TCHAR* UGodsentLineageSubsystem::SaveSlotName = TEXT("Lineage");

namespace GodsentSaveFile
{
	/// File layout: [Magic][CRC32 of payload][payload size][payload = the engine's own USaveGame bytes]
	static constexpr uint32 Magic = 0x56415347; // "GSAV"
	static constexpr int32 HeaderSize = sizeof(uint32) + sizeof(uint32) + sizeof(int32);

	/// Wraps the engine's save bytes with a magic number, size and checksum
	static void Wrap(const TArray<uint8>& Payload, TArray<uint8>& OutFileData)
	{
		uint32 FileMagic = Magic;
		uint32 Crc = FCrc::MemCrc32(Payload.GetData(), Payload.Num());
		int32 PayloadSize = Payload.Num();

		OutFileData.Reset(HeaderSize + PayloadSize);
		FMemoryWriter Writer(OutFileData);
		Writer << FileMagic << Crc << PayloadSize;
		Writer.Serialize(const_cast<uint8*>(Payload.GetData()), PayloadSize);
	}

	/// Checks the wrapper and pulls the payload out. False for anything damaged, cut short or not ours.
	static bool Unwrap(const TArray<uint8>& FileData, TArray<uint8>& OutPayload)
	{
		if (FileData.Num() <= HeaderSize)
			return false;

		uint32 FileMagic = 0;
		uint32 Crc = 0;
		int32 PayloadSize = 0;

		FMemoryReader Reader(FileData);
		Reader << FileMagic << Crc << PayloadSize;

		if (FileMagic != Magic || PayloadSize <= 0 || PayloadSize != FileData.Num() - HeaderSize)
			return false;

		const uint8* PayloadData = FileData.GetData() + HeaderSize;

		if (FCrc::MemCrc32(PayloadData, PayloadSize) != Crc)
			return false;

		OutPayload = TArray<uint8>(PayloadData, PayloadSize);
		return true;
	}
}

void UGodsentLineageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadFromSaveSlot();

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UGodsentLineageSubsystem::HandlePostLoadMap);
}

void UGodsentLineageSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	PendingAfterPopup.Reset();
	SavePopup = nullptr;

	Super::Deinitialize();
}

UGodsentLineageSubsystem* UGodsentLineageSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;

	return GameInstance ? GameInstance->GetSubsystem<UGodsentLineageSubsystem>() : nullptr;
}

/// The single save point: writes the lineage, or shows the save popup if that fails
void UGodsentLineageSubsystem::CommitProgress()
{
	LogLineage();

	if (WriteToSaveSlot())
		return;

	OnSaveFailed.Broadcast();

	// Already showing (e.g. a second save point while the popup is up): leave it as it is
	if (SavePopup)
		return;

	const UGameInstance* GameInstance = GetGameInstance();
	ShowSavePopup(GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr, EGodsentSavePopupKind::SaveFailed, false);
}

void UGodsentLineageSubsystem::LogLineage() const
{
	UE_LOG(LogTemp, Display, TEXT("[Lineage] CommitProgress: generation %d, heir '%s', %d skill point(s), %d skill(s) unlocked, level %s%s%s"),
		LineageData.Generation,
		*LineageData.HeirName,
		LineageData.SkillPoints,
		LineageData.UnlockedSkills.Num(),
		*LineageData.LastLevelTag.ToString(),
		LineageData.bAwaitingHeirName ? TEXT(", awaiting heir name") : TEXT(""),
		LineageData.bAwaitingCustomisation ? TEXT(", awaiting customisation") : TEXT(""));
}

/// Resets the lineage to a brand-new family and saves it, replacing the old family (or a damaged save)
void UGodsentLineageSubsystem::StartNewGame()
{
	LineageData = FGodsentLineageData();
	LoadResult = EGodsentSaveLoadResult::NoSave;
	BroadcastLineageChanged();

	UE_LOG(LogTemp, Display, TEXT("[Lineage] New game"));

	CommitProgress();
}

void UGodsentLineageSubsystem::StartNewGameAndOpenLevel(const UObject* WorldContextObject)
{
	StartNewGame();

	// If that save failed, the level opens once the player has answered the popup
	TWeakObjectPtr<const UObject> WeakContext = WorldContextObject;

	RunWhenSaveSettled([this, WeakContext]()
	{
		if (WeakContext.IsValid())
			OpenLastLevel(WeakContext.Get());
	});
}

/// Reloads the level the lineage was last in (the game map if none is recorded, or if it can't be opened)
void UGodsentLineageSubsystem::OpenLastLevel(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;

	if (!World)
		return;

	if (LineageData.LastLevelTag.IsValid() && OpenLevelByTag(World, LineageData.LastLevelTag))
		return;

	if (OpenLevelByTag(World, GodsentGameplayTags::GameData_Level_GameMap))
		return;

	// Game map not set up in the game instance's level list: reload whatever level we're in
	UE_LOG(LogTemp, Warning, TEXT("[Lineage] No level for GameData.Level.GameMap in the game instance's level sets, reloading the current level"));
	UGameplayStatics::OpenLevel(World, FName(*UGameplayStatics::GetCurrentLevelName(World)));
}

/// Opens a level from the level sets after the ownership check
bool UGodsentLineageSubsystem::OpenGameLevel(const UObject* WorldContextObject, FGameplayTag LevelTag, FText& OutReason)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	OutReason = FText::GetEmpty();

	if (!World)
		return false;

	if (OpenLevelByTag(World, LevelTag, &OutReason))
		return true;

	if (OutReason.IsEmpty())
		OutReason = FText::FromString(TEXT("That level isn't available."));

	return false;
}

/// Commits, then opens the main menu map. The lineage is kept for the menu's Continue button.
void UGodsentLineageSubsystem::OpenMainMenu(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;

	if (!World)
		return;

	if (HasLineageToContinue())
		CommitProgress();

	// If that save failed, go to the menu once the player has answered the popup
	TWeakObjectPtr<UWorld> WeakWorld = World;

	RunWhenSaveSettled([this, WeakWorld]()
	{
		if (WeakWorld.IsValid() && !OpenLevelByTag(WeakWorld.Get(), GodsentGameplayTags::GameData_Level_MainMenuMap))
			UE_LOG(LogTemp, Warning, TEXT("[Lineage] No level for GameData.Level.MainMenuMap in the game instance's level sets"));
	});
}

bool UGodsentLineageSubsystem::OpenLevelByTag(UWorld* World, const FGameplayTag& LevelTag, FText* OutReason) const
{
	const UGodsentGameInstance* GodsentGameInstance = Cast<UGodsentGameInstance>(GetGameInstance());

	if (!GodsentGameInstance)
		return false;

	// Add-on levels: ownership is checked every time, never remembered
	FText Reason;

	if (!GodsentGameInstance->CanOpenGameLevel(LevelTag, Reason))
	{
		if (OutReason)
			*OutReason = Reason;

		return false;
	}

	const TSoftObjectPtr<UWorld> Level = GodsentGameInstance->GetGameLevelByTag(LevelTag);

	if (Level.IsNull())
		return false;

	UGameplayStatics::OpenLevelBySoftObjectPtr(World, Level);
	return true;
}

/// Customiser's Apply & Continue: (new game: wipe the old family), save the look, then open the level
void UGodsentLineageSubsystem::ConfirmCustomisation(const UObject* WorldContextObject, const FGodsentHeirAppearance& Appearance, bool bStartNewGame)
{
	if (bStartNewGame)
	{
		LineageData = FGodsentLineageData();
		LoadResult = EGodsentSaveLoadResult::NoSave;
		UE_LOG(LogTemp, Display, TEXT("[Lineage] New game"));
	}

	LineageData.HeirAppearance = Appearance;
	LineageData.HeirAppearance.bIsSet = true;
	LineageData.bAwaitingCustomisation = false;

	BroadcastLineageChanged();
	CommitProgress();

	// If that save failed, the level opens once the player has answered the popup
	TWeakObjectPtr<const UObject> WeakContext = WorldContextObject;

	RunWhenSaveSettled([this, WeakContext]()
	{
		if (WeakContext.IsValid())
			OpenLastLevel(WeakContext.Get());
	});
}

/// Try Again after a death: the main menu map opens straight on the customiser
void UGodsentLineageSubsystem::OpenMainMenuToCustomiser(const UObject* WorldContextObject)
{
	bOpenCustomiserOnMenuLoad = true;
	OpenMainMenu(WorldContextObject);
}

bool UGodsentLineageSubsystem::ConsumeCustomiserRequest()
{
	const bool bRequested = bOpenCustomiserOnMenuLoad;
	bOpenCustomiserOnMenuLoad = false;
	return bRequested;
}

/// The heir died: next generation, +1 skill point, the next heir needs a name
void UGodsentLineageSubsystem::RecordHeirDeath()
{
	++LineageData.Generation;
	++LineageData.SkillPoints;
	LineageData.bAwaitingHeirName = true;
	LineageData.bAwaitingCustomisation = true;

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

// ---------------------------------------------------------------------------------------------------------------------
// Saving
// ---------------------------------------------------------------------------------------------------------------------

/// Reads the save slot. Anything wrong with the file is "Damaged": reported on the main menu, never a crash.
void UGodsentLineageSubsystem::LoadFromSaveSlot()
{
	LineageData = FGodsentLineageData();
	LoadResult = EGodsentSaveLoadResult::NoSave;

	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		UE_LOG(LogTemp, Display, TEXT("[Save] No save found: starting as a new family"));
		return;
	}

	TArray<uint8> FileData;
	TArray<uint8> Payload;

	if (!UGameplayStatics::LoadDataFromSlot(FileData, SaveSlotName, SaveUserIndex) || !GodsentSaveFile::Unwrap(FileData, Payload))
	{
		LoadResult = EGodsentSaveLoadResult::Damaged;
		UE_LOG(LogTemp, Warning, TEXT("[Save] Save can't be read (damaged, cut short, or not a Godsent save): reporting it on the main menu"));
		return;
	}

	const UGodsentLineageSaveGame* SaveObject = Cast<UGodsentLineageSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));

	if (!SaveObject || SaveObject->SaveVersion < 1 || SaveObject->SaveVersion > UGodsentLineageSaveGame::CurrentSaveVersion)
	{
		LoadResult = EGodsentSaveLoadResult::Damaged;
		UE_LOG(LogTemp, Warning, TEXT("[Save] Save has the wrong type or an unknown version (%d): reporting it on the main menu"),
			SaveObject ? SaveObject->SaveVersion : -1);
		return;
	}

	LineageData = SaveObject->Lineage;

	// Version 1 had no customiser: the fields load as their defaults (no look saved, customiser still to do)
	if (SaveObject->SaveVersion < UGodsentLineageSaveGame::FirstVersionWithCustomiser)
	{
		LineageData.HeirAppearance = FGodsentHeirAppearance();
		LineageData.bAwaitingCustomisation = true;
	}

	SanitiseLoadedData();
	LoadResult = EGodsentSaveLoadResult::Loaded;

	UE_LOG(LogTemp, Display, TEXT("[Save] Loaded save version %d"), SaveObject->SaveVersion);
	LogLineage();
}

/// Writes the lineage: engine save bytes, wrapped with a checksum, to Saved/SaveGames/Lineage.sav
bool UGodsentLineageSubsystem::WriteToSaveSlot() const
{
	UGodsentLineageSaveGame* SaveObject = Cast<UGodsentLineageSaveGame>(UGameplayStatics::CreateSaveGameObject(UGodsentLineageSaveGame::StaticClass()));

	if (!SaveObject)
		return false;

	SaveObject->SaveVersion = UGodsentLineageSaveGame::CurrentSaveVersion;
	SaveObject->Lineage = LineageData;

	TArray<uint8> Payload;

	if (!UGameplayStatics::SaveGameToMemory(SaveObject, Payload))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Save] Couldn't serialise the lineage"));
		return false;
	}

	TArray<uint8> FileData;
	GodsentSaveFile::Wrap(Payload, FileData);

	// Fails (returns false) if the file is read-only, the disk is full, etc. The old file is left as it was.
	const bool bSaved = UGameplayStatics::SaveDataToSlot(FileData, SaveSlotName, SaveUserIndex);

	UE_LOG(LogTemp, Display, TEXT("[Save] %s (%d bytes)"), bSaved ? TEXT("Saved") : TEXT("SAVE FAILED"), FileData.Num());
	return bSaved;
}

/// Loaded data comes from a file anyone could edit: keep every value inside the rules
void UGodsentLineageSubsystem::SanitiseLoadedData()
{
	LineageData.Generation = FMath::Max(LineageData.Generation, 1);
	LineageData.SkillPoints = FMath::Max(LineageData.SkillPoints, 0);

	LineageData.HeirName = LineageData.HeirName.TrimStartAndEnd().Left(MaxHeirNameLength);

	if (LineageData.HeirName.IsEmpty())
		LineageData.bAwaitingHeirName = true;

	// Only real skill tags (unknown or renamed tags are dropped)
	const FGameplayTag SkillRoot = FGameplayTag::RequestGameplayTag(FName(TEXT("Skill")), false);

	if (SkillRoot.IsValid())
		LineageData.UnlockedSkills = LineageData.UnlockedSkills.Filter(FGameplayTagContainer(SkillRoot));

	// Indexes past the end of the catalogue are handled where they're used (it can change between builds)
	LineageData.HeirAppearance.PatternIndex = FMath::Max(LineageData.HeirAppearance.PatternIndex, 0);
	LineageData.HeirAppearance.VoiceIndex = FMath::Max(LineageData.HeirAppearance.VoiceIndex, 0);

	// LastLevelTag needs nothing: an unknown tag loads as empty, and Open Last Level falls back to the game map
}

/// The main menu is up: if the save couldn't be read, tell the player now
void UGodsentLineageSubsystem::ShowLoadProblemIfAny(APlayerController* PlayerController, UUserWidget* ReturnFocusTo)
{
	if (LoadResult != EGodsentSaveLoadResult::Damaged || SavePopup)
		return;

	ShowSavePopup(PlayerController, EGodsentSavePopupKind::LoadFailed, false, ReturnFocusTo);
}

void UGodsentLineageSubsystem::RunWhenSaveSettled(TFunction<void()> Action)
{
	if (SavePopup)
		PendingAfterPopup.Add(MoveTemp(Action));
	else
		Action();
}

/// Shows (or refreshes) the popup on top of everything, paused, with focus on it
void UGodsentLineageSubsystem::ShowSavePopup(APlayerController* PlayerController, EGodsentSavePopupKind Kind, bool bRepeated, UUserWidget* ReturnFocusTo)
{
	if (SavePopup)
	{
		SavePopup->SetupPopup(Kind, bRepeated);
		return;
	}

	const UGodsentGameInstance* GodsentGameInstance = Cast<UGodsentGameInstance>(GetGameInstance());
	const TSubclassOf<UGodsentSavePopupWidget> PopupClass = GodsentGameInstance ? GodsentGameInstance->GetSavePopupWidgetClass() : nullptr;

	if (!PlayerController || !PopupClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Save] Can't show the save popup (no player controller, or BP_GodsentGameInstance has no Save Popup Widget Class)"));
		RunPendingActions(); // Never leave a level change stuck behind a popup that can't appear
		return;
	}

	// Remember where focus was (a menu, or the game viewport during play) and the pause state, to put them back afterwards
	FocusBeforePopup.Reset();
	ReturnFocusWidget = ReturnFocusTo;

	if (FSlateApplication::IsInitialized())
		FocusBeforePopup = FSlateApplication::Get().GetUserFocusedWidget(0);

	bWasPausedBeforePopup = UGameplayStatics::IsGamePaused(PlayerController);

	SavePopup = CreateWidget<UGodsentSavePopupWidget>(PlayerController, PopupClass);

	if (!SavePopup)
	{
		RunPendingActions();
		return;
	}

	SavePopup->AddToViewport(1000);
	PlayerController->SetPause(true);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(SavePopup->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(false);

	// After the input mode, so the Blueprint can move focus onto its primary button
	SavePopup->SetupPopup(Kind, bRepeated);
}

/// Removes the popup and puts focus, input mode and pause back the way they were
void UGodsentLineageSubsystem::CloseSavePopup()
{
	if (!SavePopup)
		return;

	APlayerController* PlayerController = SavePopup->GetOwningPlayer();

	SavePopup->RemoveFromParent();
	SavePopup = nullptr;

	if (!PlayerController)
		return;

	PlayerController->SetPause(bWasPausedBeforePopup);
	PlayerController->SetShowMouseCursor(false);

	const TSharedPtr<SWidget> PreviousFocus = FocusBeforePopup.Pin();
	FocusBeforePopup.Reset();

	UUserWidget* ReturnTo = ReturnFocusWidget.Get();
	ReturnFocusWidget.Reset();

	// A known menu underneath: back to it (its On Save Popup Closed handler then focuses the right button)
	if (ReturnTo && ReturnTo->IsInViewport())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ReturnTo->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);

		OnSavePopupClosed.Broadcast();
		return;
	}

	const UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
	TSharedPtr<SViewport> GameViewportWidget;

	if (ViewportClient)
		GameViewportWidget = ViewportClient->GetGameViewportWidget();

	const bool bWasInMenu = PreviousFocus.IsValid() && PreviousFocus.Get() != static_cast<SWidget*>(GameViewportWidget.Get());

	if (bWasInMenu)
	{
		// Back to the button that had focus (pause menu, skill tree, main menu...) so focus is never lost (UI-03)
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(PreviousFocus);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
	}
	else
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
	}

	OnSavePopupClosed.Broadcast();
}

void UGodsentLineageSubsystem::RunPendingActions()
{
	TArray<TFunction<void()>> Actions = MoveTemp(PendingAfterPopup);
	PendingAfterPopup.Reset();

	for (TFunction<void()>& Action : Actions)
		Action();
}

void UGodsentLineageSubsystem::HandleSavePopupChoice(UGodsentSavePopupWidget* Popup, bool bPrimary)
{
	if (!Popup)
		return;

	// A popup on screen that the subsystem lost track of must still work, never leave the player stuck
	if (Popup != SavePopup)
	{
		if (SavePopup)
			SavePopup->RemoveFromParent();

		SavePopup = Popup;
		UE_LOG(LogTemp, Warning, TEXT("[Save] Popup button pressed on an untracked popup: adopting it"));
	}

	switch (Popup->GetPopupKind())
	{
		case EGodsentSavePopupKind::SaveFailed:
		{
			if (bPrimary)
			{
				// Retry: write the same data again
				LogLineage();

				if (!WriteToSaveSlot())
				{
					OnSaveFailed.Broadcast();
					SavePopup->SetupPopup(EGodsentSavePopupKind::SaveFailed, true);
					return;
				}
			}

			// Saved on retry, or Continue without saving: carry on (and do any level change that was waiting)
			CloseSavePopup();
			RunPendingActions();
			break;
		}

		case EGodsentSavePopupKind::LoadFailed:
		{
			APlayerController* PlayerController = Popup->GetOwningPlayer();

			CloseSavePopup();

			// Dealt with: don't ask again this session. The damaged file stays until a new family replaces it.
			LoadResult = EGodsentSaveLoadResult::NoSave;

			if (bPrimary)
			{
				// New game goes through the customiser when we're on the main menu (the family is replaced on Apply)
				if (AGodsentMainMenuGameMode* MenuGameMode = AGodsentMainMenuGameMode::Get(PlayerController))
					MenuGameMode->OpenCustomiser(true);
				else
					StartNewGameAndOpenLevel(PlayerController);
			}

			break;
		}
	}
}

void UGodsentLineageSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// Safety net for a popup left behind in the OLD level (level changes wait for the popup, so this shouldn't happen).
	// This event fires after the new level's BeginPlay, so a popup the new level has just opened (the "loading in"
	// save on Continue) belongs to LoadedWorld and must be kept.
	if (SavePopup && SavePopup->GetWorld() != LoadedWorld)
	{
		SavePopup = nullptr;
		FocusBeforePopup.Reset();
		ReturnFocusWidget.Reset();
		RunPendingActions();
	}
}