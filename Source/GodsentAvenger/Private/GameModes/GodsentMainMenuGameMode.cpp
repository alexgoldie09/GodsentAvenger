// Alex Goldberg rights reserved

#include "GameModes/GodsentMainMenuGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AGodsentMainMenuGameMode::AGodsentMainMenuGameMode()
{
	// No hero in the menu, and a plain controller (not BP_HeroController, which would start the rebirth loop)
	DefaultPawnClass = nullptr;
	PlayerControllerClass = APlayerController::StaticClass();
}

/// Shows the main menu with the mouse, looking through the level's camera if there is one
void AGodsentMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

	if (!PlayerController)
		return;

	if (bUseLevelCamera)
	{
		if (AActor* MenuCamera = UGameplayStatics::GetActorOfClass(this, ACameraActor::StaticClass()))
			PlayerController->SetViewTarget(MenuCamera);
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

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
}
