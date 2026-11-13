// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Subsystems/GodsentLineageSubsystem.h"
#include "GodsentMainMenuGameMode.generated.h"

class AActor;
class UHeirAppearanceComponent;
class UUserWidget;

/**
 * Game mode for the main menu map. Spawns no hero (Default Pawn Class is None) and uses a plain PlayerController,
 * so none of the rebirth loop (name prompt, death handling) runs here.
 *
 * On BeginPlay it shows the main menu widget (no mouse cursor, gamepad/keyboard focus), reports a damaged save if there is one, and looks through the level's first Camera Actor if there is one.
 *
 * Also runs the customiser: the camera pans from the menu camera to the customiser camera (a Camera Actor tagged
 * "Customiser") looking at BP_MenuHeroDisplay, then the customiser widget opens. After a death (Try Again) the map
 * opens straight on the customiser.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGodsentMainMenuGameMode();

	/// The main menu game mode for this world (nullptr anywhere else)
	static AGodsentMainMenuGameMode* Get(const UObject* WorldContextObject);

	/// New Game (Start New Game ticked) or Continue: hides the menu, pans to the display knight, opens the customiser
	UFUNCTION(BlueprintCallable, Category = "Godsent|Customiser")
	void OpenCustomiser(bool bStartNewGame);

	/// The menu's Continue: the customiser if the next heir hasn't picked their look yet, otherwise straight into the level
	UFUNCTION(BlueprintCallable, Category = "Godsent|Customiser")
	void ContinueGame();

	/// The customiser's Back: closes it, pans back and shows the menu again. Nothing is saved.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Customiser")
	void CloseCustomiser();

	/// The customiser's Apply & Continue: saves the look (wiping the old family first on New Game) and starts the game
	UFUNCTION(BlueprintCallable, Category = "Godsent|Customiser")
	void ConfirmCustomisation(const FGodsentHeirAppearance& Appearance);

	/// True when the customiser was opened from New Game (start from the default look)
	UFUNCTION(BlueprintPure, Category = "Godsent|Customiser")
	bool IsCustomisingNewGame() const { return bCustomisingNewGame; }

	/// The display knight's appearance component, for the customiser's live preview
	UFUNCTION(BlueprintPure, Category = "Godsent|Customiser")
	UHeirAppearanceComponent* GetDisplayAppearance() const;

	/// The display knight itself (the customiser rotates it and changes its pose)
	UFUNCTION(BlueprintPure, Category = "Godsent|Customiser")
	AActor* GetDisplayActor() const { return DisplayActor; }

protected:
	virtual void BeginPlay() override;

	/// The main menu (Start Game, Continue, Controls...)
	UPROPERTY(EditDefaultsOnly, Category = "Main Menu")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	/// Look through the first Camera Actor placed in the level (for a 3D backdrop). Off, or no camera: the widget's own background shows.
	UPROPERTY(EditDefaultsOnly, Category = "Main Menu")
	bool bUseLevelCamera = true;

	/// WBP_Customiser
	UPROPERTY(EditDefaultsOnly, Category = "Customiser")
	TSubclassOf<UUserWidget> CustomiserWidgetClass;

	/// The Camera Actor with this tag frames the display knight. Any other Camera Actor is the menu camera.
	UPROPERTY(EditDefaultsOnly, Category = "Customiser")
	FName CustomiserCameraTag = TEXT("Customiser");

	/// How long the camera takes to pan between the menu and the customiser
	UPROPERTY(EditDefaultsOnly, Category = "Customiser", meta = (ClampMin = "0.0", Units = "s"))
	float CameraBlendTime = 1.f;

private:
	/// Shows a widget and gives it keyboard/gamepad focus (no cursor)
	void FocusWidget(UUserWidget* Widget) const;

	void ShowCustomiserWidget();
	void ShowMainMenuWidget();

	/// Moves the view to a camera, blending over Camera Blend Time (0 = cut)
	void BlendToCamera(AActor* Camera, float BlendTime) const;

	UPROPERTY()
	TObjectPtr<UUserWidget> MainMenuWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> CustomiserWidget;

	UPROPERTY()
	TObjectPtr<AActor> MenuCamera;

	UPROPERTY()
	TObjectPtr<AActor> CustomiserCamera;

	/// The first actor in the map with a Heir Appearance component (BP_MenuHeroDisplay)
	UPROPERTY()
	TObjectPtr<AActor> DisplayActor;

	bool bCustomisingNewGame = false;

	FTimerHandle CameraBlendTimerHandle;
};
