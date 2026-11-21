// Alex Goldberg rights reserved

#include "Arena/GodsentBossArena.h"
#include "Components/Aldegon/GodsentAldegonComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

namespace
{
	/// Starting size of the arena box (half extents), scale it in the level
	const FVector BossArenaDefaultExtent(2000.f, 2000.f, 600.f);
}

AGodsentBossArena::AGodsentBossArena()
{
	PrimaryActorTick.bCanEverTick = false;

	ArenaBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ArenaBounds"));
	SetRootComponent(ArenaBounds);

	ArenaBounds->SetBoxExtent(BossArenaDefaultExtent);
	ArenaBounds->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	ArenaBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ArenaBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ArenaBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ArenaBounds->SetGenerateOverlapEvents(true);
	ArenaBounds->SetHiddenInGame(true);
}

void AGodsentBossArena::BeginPlay()
{
	Super::BeginPlay();

	ArenaBounds->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnBoundsBeginOverlap);
	ArenaBounds->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnBoundsEndOverlap);

	if (!GetBossComponent())
		UE_LOG(LogTemp, Warning, TEXT("%s: Boss isn't set, or has no Godsent Aldegon component"), *GetNameSafe(this));

	// Next frame, once the player has spawned
	GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::CheckForPlayerAlreadyInside);
}

UGodsentAldegonComponent* AGodsentBossArena::GetBossComponent() const
{
	return Boss ? Boss->FindComponentByClass<UGodsentAldegonComponent>() : nullptr;
}

void AGodsentBossArena::CheckForPlayerAlreadyInside()
{
	TArray<AActor*> OverlappingPawns;
	ArenaBounds->GetOverlappingActors(OverlappingPawns, APawn::StaticClass());

	for (AActor* OverlappingActor : OverlappingPawns)
	{
		APawn* Pawn = Cast<APawn>(OverlappingActor);

		if (Pawn && Pawn->IsPlayerControlled())
		{
			HandlePlayerEntered(Pawn);
			return;
		}
	}
}

void AGodsentBossArena::OnBoundsBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);

	if (Pawn && Pawn->IsPlayerControlled())
		HandlePlayerEntered(Pawn);
}

void AGodsentBossArena::HandlePlayerEntered(APawn* Player)
{
	// The capsule and the mesh can both overlap: only the first one counts
	if (PlayerInside.Get() == Player)
		return;

	PlayerInside = Player;

	if (UGodsentAldegonComponent* BossComponent = GetBossComponent())
		BossComponent->StartFight(Player);

	BP_OnPlayerEntered(Player);
}

void AGodsentBossArena::OnBoundsEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);

	// Still inside with another of its components
	if (!Pawn || Pawn != PlayerInside.Get() || ArenaBounds->IsOverlappingActor(Pawn))
		return;

	PlayerInside.Reset();

	if (UGodsentAldegonComponent* BossComponent = GetBossComponent())
		BossComponent->EndFight();

	BP_OnPlayerLeft(Pawn);
}
