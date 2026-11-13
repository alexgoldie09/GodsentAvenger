// Alex Goldberg rights reserved

#include "GameModes/GodsentMainMenuGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Components/Appearance/HeirAppearanceComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

AGodsentMainMenuGameMode::AGodsentMainMenuGameMode()
{
	// No hero in the menu, and a plain controller (not BP_HeroController, which would start the rebirth loop)
	DefaultPawnClass = nullptr;
	PlayerControllerClass = APlayerController::StaticClass();
}

AGodsentMainMenuGameMode* AGodsentMainMenuGameMode::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetAuthGameMode<AGodsentMainMenuGameMode>() : nullptr;
}

/// Shows the main menu (or the customiser after a death), reports a damaged save, and finds the cameras and display knight
void AGodsentMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

	if (!PlayerController)
		return;

	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(CustomiserCameraTag))
		{
			if (!CustomiserCamera)
				CustomiserCamera = *It;
		}
		else if (!MenuCamera)
		{
			MenuCamera = *It;
		}
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->FindComponentByClass<UHeirAppearanceComponent>())
		{
			DisplayActor = *It;
			break;
		}
	}

	if (!MainMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] BP_MainMenuGameMode has no Main Menu Widget Class"));
		return;
	}

	MainMenuWidget = CreateWidget<UUserWidget>(PlayerController, MainMenuWidgetClass);

	if (!MainMenuWidget)
		return;

	MainMenuWidget->AddToViewport();

	UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this);

	// Try Again after a death: start on the customiser for the next heir, no pan
	if (Lineage && Lineage->ConsumeCustomiserRequest() && CustomiserWidgetClass)
	{
		bCustomisingNewGame = false;
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
		BlendToCamera(CustomiserCamera, 0.f);

		// Next frame, once the display knight has run its own BeginPlay (it reads its default look there)
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::ShowCustomiserWidget);
	}
	else
	{
		if (bUseLevelCamera)
			BlendToCamera(MenuCamera, 0.f);

		FocusWidget(MainMenuWidget);
	}

	// A save that couldn't be read is reported here, on top of the menu (SAV-03)
	if (Lineage)
		Lineage->ShowLoadProblemIfAny(PlayerController, MainMenuWidget);
}

void AGodsentMainMenuGameMode::OpenCustomiser(bool bStartNewGame)
{
	if (!CustomiserWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] BP_MainMenuGameMode has no Customiser Widget Class"));
		return;
	}

	bCustomisingNewGame = bStartNewGame;

	if (MainMenuWidget)
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);

	// Nothing focused while the camera moves, so no hidden button can be pressed
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		PlayerController->SetInputMode(FInputModeUIOnly());

	BlendToCamera(CustomiserCamera, CameraBlendTime);

	if (CameraBlendTime > 0.f && CustomiserCamera)
		GetWorldTimerManager().SetTimer(CameraBlendTimerHandle, this, &ThisClass::ShowCustomiserWidget, CameraBlendTime, false);
	else
		ShowCustomiserWidget();
}

void AGodsentMainMenuGameMode::ContinueGame()
{
	UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this);

	if (!Lineage)
		return;

	if (Lineage->NeedsCustomisation() && CustomiserWidgetClass)
		OpenCustomiser(false);
	else
		Lineage->OpenLastLevel(this);
}

void AGodsentMainMenuGameMode::CloseCustomiser()
{
	GetWorldTimerManager().ClearTimer(CameraBlendTimerHandle);

	if (CustomiserWidget)
	{
		CustomiserWidget->RemoveFromParent();
		CustomiserWidget = nullptr;
	}

	// Put the family's saved look back on the display knight
	if (UHeirAppearanceComponent* DisplayAppearance = GetDisplayAppearance())
	{
		DisplayAppearance->StopVoiceLine();
		DisplayAppearance->ApplyLineageAppearance();
	}

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		PlayerController->SetInputMode(FInputModeUIOnly());

	BlendToCamera(MenuCamera, CameraBlendTime);

	if (CameraBlendTime > 0.f && MenuCamera)
		GetWorldTimerManager().SetTimer(CameraBlendTimerHandle, this, &ThisClass::ShowMainMenuWidget, CameraBlendTime, false);
	else
		ShowMainMenuWidget();
}

void AGodsentMainMenuGameMode::ConfirmCustomisation(const FGodsentHeirAppearance& Appearance)
{
	// One press only: the level is about to load
	if (CustomiserWidget)
		CustomiserWidget->SetIsEnabled(false);

	if (UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this))
		Lineage->ConfirmCustomisation(this, Appearance, bCustomisingNewGame);
}

UHeirAppearanceComponent* AGodsentMainMenuGameMode::GetDisplayAppearance() const
{
	return DisplayActor ? DisplayActor->FindComponentByClass<UHeirAppearanceComponent>() : nullptr;
}

void AGodsentMainMenuGameMode::ShowCustomiserWidget()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

	if (!PlayerController)
		return;

	// A fresh widget each time, so its Construct reads the current look again
	if (CustomiserWidget)
		CustomiserWidget->RemoveFromParent();

	CustomiserWidget = CreateWidget<UUserWidget>(PlayerController, CustomiserWidgetClass);

	if (!CustomiserWidget)
		return;

	CustomiserWidget->AddToViewport();
	FocusWidget(CustomiserWidget);
}

void AGodsentMainMenuGameMode::ShowMainMenuWidget()
{
	if (!MainMenuWidget)
		return;

	MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
	FocusWidget(MainMenuWidget);
}

void AGodsentMainMenuGameMode::FocusWidget(UUserWidget* Widget) const
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

	if (!PlayerController || !Widget)
		return;

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Widget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(false); // Console rule: the cursor is never shown (UI-03)
}

void AGodsentMainMenuGameMode::BlendToCamera(AActor* Camera, float BlendTime) const
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

	if (!PlayerController || !Camera)
		return;

	if (BlendTime > 0.f)
		PlayerController->SetViewTargetWithBlend(Camera, BlendTime, VTBlend_Cubic);
	else
		PlayerController->SetViewTarget(Camera);
}
