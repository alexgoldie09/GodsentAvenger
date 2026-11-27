// Alex Goldberg rights reserved

#include "AI/BTService_PlaySoundWhileActive.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	/// Default gap between one-shot plays (Andre changes Interval / Random Deviation on the node)
	constexpr float PlaySoundServiceDefaultInterval = 6.f;
	constexpr float PlaySoundServiceDefaultDeviation = 2.f;
}

UBTService_PlaySoundWhileActive::UBTService_PlaySoundWhileActive()
{
	NodeName = TEXT("Native Play Sound While Active");

	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	bNotifyTick = true;

	// The first play is handled in On Become Relevant
	bCallTickOnSearchStart = false;

	Interval = PlaySoundServiceDefaultInterval;
	RandomDeviation = PlaySoundServiceDefaultDeviation;
}

uint16 UBTService_PlaySoundWhileActive::GetInstanceMemorySize() const
{
	return sizeof(FPlaySoundWhileActiveMemory);
}

void UBTService_PlaySoundWhileActive::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	if (InitType == EBTMemoryInit::Initialize)
	{
		FPlaySoundWhileActiveMemory* Memory = CastInstanceNodeMemory<FPlaySoundWhileActiveMemory>(NodeMemory);
		Memory->ActiveSound = nullptr;
		Memory->bHasPlayed = false;
	}
}

FString UBTService_PlaySoundWhileActive::GetStaticDescription() const
{
	const FString SoundName = Sound ? Sound->GetName() : TEXT("(no sound)");

	if (bRepeatWhileActive)
		return FString::Printf(TEXT("Plays %s while this branch runs (again every %.1f +/- %.1f s once it's finished)"),
			*SoundName, Interval, RandomDeviation);

	return FString::Printf(TEXT("Plays %s once while this branch runs"), *SoundName);
}

void UBTService_PlaySoundWhileActive::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	FPlaySoundWhileActiveMemory* Memory = CastInstanceNodeMemory<FPlaySoundWhileActiveMemory>(NodeMemory);
	Memory->bHasPlayed = false;

	if (bPlayStraightAway)
		PlaySound(OwnerComp, Memory);
}

void UBTService_PlaySoundWhileActive::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FPlaySoundWhileActiveMemory* Memory = CastInstanceNodeMemory<FPlaySoundWhileActiveMemory>(NodeMemory);
	const UAudioComponent* ActiveSound = Memory->ActiveSound.Get();
	const bool bStillPlaying = ActiveSound && ActiveSound->IsPlaying();

	// Waiting for the first play (Play Straight Away off), or the last one finished and it repeats
	if (!bStillPlaying && (bRepeatWhileActive || !Memory->bHasPlayed))
		PlaySound(OwnerComp, Memory);
}

void UBTService_PlaySoundWhileActive::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FPlaySoundWhileActiveMemory* Memory = CastInstanceNodeMemory<FPlaySoundWhileActiveMemory>(NodeMemory);

	if (UAudioComponent* ActiveSound = Memory->ActiveSound.Get())
	{
		if (FadeOutTime > 0.f)
			ActiveSound->FadeOut(FadeOutTime, 0.f);
		else
			ActiveSound->Stop();
	}

	Memory->ActiveSound = nullptr;
	Memory->bHasPlayed = false;

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UBTService_PlaySoundWhileActive::PlaySound(UBehaviorTreeComponent& OwnerComp, FPlaySoundWhileActiveMemory* Memory) const
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;

	if (!Sound || !Pawn)
		return;

	USceneComponent* AttachTo = nullptr;

	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
		AttachTo = Character->GetMesh();

	if (!AttachTo)
		AttachTo = Pawn->GetRootComponent();

	// Stops if the pawn is destroyed (dissolved away)
	UAudioComponent* NewSound = UGameplayStatics::SpawnSoundAttached(Sound, AttachTo, NAME_None, FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset, true);

	if (NewSound && FadeInTime > 0.f)
		NewSound->FadeIn(FadeInTime);

	Memory->ActiveSound = NewSound;
	Memory->bHasPlayed = true;
}
