// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PlaySoundWhileActive.generated.h"

class UAudioComponent;
class USoundBase;

struct FPlaySoundWhileActiveMemory
{
	TWeakObjectPtr<UAudioComponent> ActiveSound;

	/// Played at least once since the branch started
	bool bHasPlayed = false;
};

/**
 * Plays a sound from the pawn while its branch (or task) runs, and fades it out when the branch ends
 * (finished, failed or aborted).
 * - A looping cue (SC_Zombie_RunLoop) just plays until the branch ends.
 * - A one-shot cue (SC_Zombie_Idle) plays again on each tick once the last one has finished, so Interval and
 *   Random Deviation set how often (e.g. 6 +/- 2 = a groan every 4-8 s).
 */
UCLASS()
class GODSENTAVENGER_API UBTService_PlaySoundWhileActive : public UBTService
{
	GENERATED_BODY()

	UBTService_PlaySoundWhileActive();

	//~ Begin UBTNode Interface
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTNode Interface

protected:
	//~ Begin UBTAuxiliaryNode Interface
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	//~ End UBTAuxiliaryNode Interface

	/// What to play (from the pawn's mesh)
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> Sound;

	/// Play as soon as the branch starts. Off: wait one Interval first (so an idle groan isn't instant).
	UPROPERTY(EditAnywhere, Category = "Sound")
	bool bPlayStraightAway = true;

	/// Play it again on each tick once it's finished (one-shot cues). Off: play once per branch.
	UPROPERTY(EditAnywhere, Category = "Sound")
	bool bRepeatWhileActive = true;

	/// Fade in when it starts (0 = no fade)
	UPROPERTY(EditAnywhere, Category = "Sound", meta = (ClampMin = "0.0", Units = "s"))
	float FadeInTime = 0.f;

	/// Fade out when the branch ends (0 = cut off straight away)
	UPROPERTY(EditAnywhere, Category = "Sound", meta = (ClampMin = "0.0", Units = "s"))
	float FadeOutTime = 0.25f;

private:
	void PlaySound(UBehaviorTreeComponent& OwnerComp, FPlaySoundWhileActiveMemory* Memory) const;
};
