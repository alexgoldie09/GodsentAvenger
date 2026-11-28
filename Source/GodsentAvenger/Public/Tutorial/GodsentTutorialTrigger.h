// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentTutorialTrigger.generated.h"

class UBoxComponent;
class UGodsentTutorialPopupWidget;

/**
 * Box that opens a tutorial popup (WBP_TutorialPopup) the first time the player walks into it.
 * - Founder Only: just the first heir of a new family sees it (never heir 2 onwards).
 * - Once per level load, then it switches itself off.
 * - Never on top of the name prompt or skill tree: if the player starts inside it, it waits until they have control.
 * Place one at the player start to show it as soon as a new game begins.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentTutorialTrigger : public AActor
{
	GENERATED_BODY()

public:
	AGodsentTutorialTrigger();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/// Walk in here to see the popup. Scale it in the level.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TObjectPtr<UBoxComponent> TriggerBox;

	/// WBP_TutorialPopup (or any child of Godsent Tutorial Popup Widget)
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	TSubclassOf<UGodsentTutorialPopupWidget> PopupWidgetClass;

	/// Only the first heir of a new family sees it
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	bool bFounderOnly = true;

private:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/// Waits for the heir to get control when the player started inside the box
	UFUNCTION()
	void OnRunStateChanged(EGodsentRunState NewState);

	/// Shows it next frame (once the game mode has finished handing control over)
	void RequestPopup();
	void ShowPopup();

	/// The player can see it now: founder (if Founder Only), not shown yet, and the heir is in control
	bool CanShowNow() const;

	/// Done for good this level: no more overlaps
	void SwitchOff();

	bool bShown = false;
};
