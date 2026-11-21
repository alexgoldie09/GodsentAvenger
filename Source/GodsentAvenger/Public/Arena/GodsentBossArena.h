// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GodsentBossArena.generated.h"

class APawn;
class UBoxComponent;
class UGodsentAldegonComponent;

/**
 * The boss fight boundary (BP_BossArena, or place this class directly). Size the box to cover the arena.
 * Player walks in: the boss gets them as a target and wakes up (or turns back if he was going home).
 * Player walks out: the boss drops them, clears his summons, flies home, sits and heals.
 * The boss bar is handled by the boss's own On Fight Started / On Fight Ended events.
 */
UCLASS(Blueprintable)
class GODSENTAVENGER_API AGodsentBossArena : public AActor
{
	GENERATED_BODY()

public:
	AGodsentBossArena();

	/// The player currently inside (null if nobody is)
	UFUNCTION(BlueprintPure, Category = "Godsent|Arena")
	APawn* GetPlayerInside() const { return PlayerInside.Get(); }

protected:
	virtual void BeginPlay() override;

	/// The arena's bounds. Scale it in the level to fit the room.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena")
	TObjectPtr<UBoxComponent> ArenaBounds;

	/// The boss placed in this arena (BP_Aldegon)
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Arena")
	TObjectPtr<AActor> Boss;

	/// The player came in (after the boss has been told)
	UFUNCTION(BlueprintImplementableEvent, Category = "Arena", meta = (DisplayName = "On Player Entered"))
	void BP_OnPlayerEntered(APawn* Player);

	/// The player left (after the boss has been told)
	UFUNCTION(BlueprintImplementableEvent, Category = "Arena", meta = (DisplayName = "On Player Left"))
	void BP_OnPlayerLeft(APawn* Player);

private:
	UFUNCTION()
	void OnBoundsBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBoundsEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/// The player already standing inside when play starts
	void CheckForPlayerAlreadyInside();

	void HandlePlayerEntered(APawn* Player);

	UGodsentAldegonComponent* GetBossComponent() const;

	TWeakObjectPtr<APawn> PlayerInside;
};
