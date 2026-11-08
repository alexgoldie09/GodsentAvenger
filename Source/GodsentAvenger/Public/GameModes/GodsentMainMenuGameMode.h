// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GodsentMainMenuGameMode.generated.h"

class UUserWidget;

/**
 * Game mode for the main menu map. Spawns no hero (Default Pawn Class is None) and uses a plain PlayerController,
 * so none of the rebirth loop (name prompt, death handling) runs here.
 *
 * On BeginPlay it shows the main menu widget (no mouse cursor, gamepad/keyboard focus), reports a damaged save if there is one, and looks through the level's first Camera Actor if there is one.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGodsentMainMenuGameMode();

protected:
	virtual void BeginPlay() override;

	/// The main menu (Start Game, Continue, Controls...)
	UPROPERTY(EditDefaultsOnly, Category = "Main Menu")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	/// Look through the first Camera Actor placed in the level (for a 3D backdrop). Off, or no camera: the widget's own background shows.
	UPROPERTY(EditDefaultsOnly, Category = "Main Menu")
	bool bUseLevelCamera = true;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> MainMenuWidget;
};
