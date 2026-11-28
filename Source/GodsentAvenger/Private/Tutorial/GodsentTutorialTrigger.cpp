// Alex Goldberg rights reserved

#include "Tutorial/GodsentTutorialTrigger.h"
#include "Components/BoxComponent.h"
#include "Controllers/GodsentHeroController.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameModes/GodsentBaseGameMode.h"
#include "Subsystems/GodsentLineageSubsystem.h"
#include "TimerManager.h"
#include "Widgets/GodsentTutorialPopupWidget.h"

namespace
{
	/// Starting size of the trigger box (half extents), scale it in the level
	const FVector TutorialTriggerDefaultExtent(200.f, 200.f, 150.f);
}

AGodsentTutorialTrigger::AGodsentTutorialTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);

	TriggerBox->SetBoxExtent(TutorialTriggerDefaultExtent);
	TriggerBox->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->SetHiddenInGame(true);
}

void AGodsentTutorialTrigger::BeginPlay()
{
	Super::BeginPlay();

	// Not the founder: this heir never sees it
	if (bFounderOnly)
	{
		const UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this);

		if (Lineage && Lineage->GetLineageData().Generation > 1)
		{
			SwitchOff();
			return;
		}
	}

	if (!PopupWidgetClass)
		UE_LOG(LogTemp, Warning, TEXT("%s: no Popup Widget Class set"), *GetName());

	TriggerBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnTriggerBeginOverlap);

	if (AGodsentBaseGameMode* GameMode = AGodsentBaseGameMode::Get(this))
		GameMode->OnRunStateChanged.AddUniqueDynamic(this, &ThisClass::OnRunStateChanged);
}

void AGodsentTutorialTrigger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AGodsentBaseGameMode* GameMode = AGodsentBaseGameMode::Get(this))
		GameMode->OnRunStateChanged.RemoveDynamic(this, &ThisClass::OnRunStateChanged);

	GetWorldTimerManager().ClearAllTimersForObject(this);

	Super::EndPlay(EndPlayReason);
}

void AGodsentTutorialTrigger::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);

	if (Pawn && Pawn->IsPlayerControlled())
		RequestPopup();
}

/// The heir just got control: show it if they started inside the box (no overlap event fires for that)
void AGodsentTutorialTrigger::OnRunStateChanged(EGodsentRunState NewState)
{
	if (NewState != EGodsentRunState::Playing || bShown)
		return;

	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;

	if (PlayerPawn && TriggerBox->IsOverlappingActor(PlayerPawn))
		RequestPopup();
}

void AGodsentTutorialTrigger::RequestPopup()
{
	if (bShown)
		return;

	// Next frame: the game mode unpauses and gives input back after it says Playing, which would undo our pause
	GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::ShowPopup);
}

void AGodsentTutorialTrigger::ShowPopup()
{
	// Not in control yet (name prompt, skill tree): On Run State Changed tries again when they are
	if (!CanShowNow())
		return;

	AGodsentHeroController* HeroController = Cast<AGodsentHeroController>(GetWorld()->GetFirstPlayerController());

	if (!HeroController)
		return;

	// Another tutorial is open: try again once it's closed (timers wait while it has the game paused)
	if (HeroController->IsTutorialPopupOpen())
	{
		RequestPopup();
		return;
	}

	if (HeroController->ShowTutorialPopup(PopupWidgetClass))
		SwitchOff();
}

bool AGodsentTutorialTrigger::CanShowNow() const
{
	if (bShown || !PopupWidgetClass)
		return false;

	// No Godsent game mode (a test map): just show it
	const AGodsentBaseGameMode* GameMode = AGodsentBaseGameMode::Get(this);
	return !GameMode || GameMode->GetRunState() == EGodsentRunState::Playing;
}

void AGodsentTutorialTrigger::SwitchOff()
{
	bShown = true;
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
