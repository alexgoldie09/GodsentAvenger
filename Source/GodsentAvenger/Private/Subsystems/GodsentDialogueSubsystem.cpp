// Alex Goldberg rights reserved

#include "Subsystems/GodsentDialogueSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/Appearance/HeirAppearanceComponent.h"
#include "DataAssets/Customisation/GodsentVoiceSetDataAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	/// How often to check again while the hero's voice is still loading
	constexpr float DialogueVoiceWaitInterval = 0.1f;

	/// Give up on the voice after this long and skip the line
	constexpr float DialogueVoiceMaxWait = 3.f;

	/// Backup in case a line never reports that it ended: move on this long after its length
	constexpr float DialogueLineEndMargin = 0.5f;
}

UGodsentDialogueSubsystem* UGodsentDialogueSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UGodsentDialogueSubsystem>() : nullptr;
}

bool UGodsentDialogueSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGodsentDialogueSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(LineTimerHandle);

	ClearActiveLine();
	QueuedLines.Reset();
	LineIndex = INDEX_NONE;

	Super::Deinitialize();
}

void UGodsentDialogueSubsystem::PlayDialogue(const TArray<FGodsentDialogueLine>& NewLines)
{
	// One conversation at a time
	if (IsDialoguePlaying())
		EndDialogue(false);

	if (NewLines.IsEmpty())
		return;

	QueuedLines = NewLines;
	LineIndex = INDEX_NONE;
	AdvanceToNextLine();
}

void UGodsentDialogueSubsystem::StopDialogue()
{
	if (IsDialoguePlaying())
		EndDialogue(false);
}

void UGodsentDialogueSubsystem::StopDialogueFor(AActor* Speaker)
{
	if (!Speaker || !IsDialoguePlaying())
		return;

	for (int32 Index = LineIndex; Index < QueuedLines.Num(); ++Index)
	{
		if (QueuedLines[Index].Speaker == Speaker)
		{
			StopDialogue();
			return;
		}
	}
}

bool UGodsentDialogueSubsystem::IsSpeaking(const AActor* Speaker) const
{
	return Speaker && ActiveSpeaker.Get() == Speaker && IsValid(ActiveLine) && ActiveLine->IsPlaying();
}

void UGodsentDialogueSubsystem::AdvanceToNextLine()
{
	ClearActiveLine();
	GetWorld()->GetTimerManager().ClearTimer(LineTimerHandle);

	++LineIndex;

	if (!QueuedLines.IsValidIndex(LineIndex))
	{
		EndDialogue(true);
		return;
	}

	VoiceWaitTime = 0.f;
	const float Delay = QueuedLines[LineIndex].DelayBefore;

	if (Delay > 0.f)
		GetWorld()->GetTimerManager().SetTimer(LineTimerHandle, this, &ThisClass::StartCurrentLine, Delay, false);
	else
		StartCurrentLine();
}

void UGodsentDialogueSubsystem::StartCurrentLine()
{
	if (!QueuedLines.IsValidIndex(LineIndex))
		return;

	const FGodsentDialogueLine& Line = QueuedLines[LineIndex];
	AActor* Speaker = Line.Speaker;

	if (!IsValid(Speaker))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogue] Line %d has no speaker (or they're gone), skipped"), LineIndex);
		AdvanceToNextLine();
		return;
	}

	UHeirAppearanceComponent* Appearance = Speaker->FindComponentByClass<UHeirAppearanceComponent>();
	USoundBase* Sound = Line.Sound;

	// A hero line: find it in this heir's voice
	if (!Sound && Line.HeroVoiceLine.IsValid())
	{
		// The voice is still loading (just spawned): check again shortly
		if (Appearance && Appearance->IsVoiceLoading() && VoiceWaitTime < DialogueVoiceMaxWait)
		{
			VoiceWaitTime += DialogueVoiceWaitInterval;
			GetWorld()->GetTimerManager().SetTimer(LineTimerHandle, this, &ThisClass::StartCurrentLine, DialogueVoiceWaitInterval, false);
			return;
		}

		const UGodsentVoiceSetDataAsset* VoiceSet = Appearance ? Appearance->GetVoiceSet() : nullptr;
		Sound = VoiceSet ? VoiceSet->GetLine(Line.HeroVoiceLine) : nullptr;
	}

	if (!Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogue] %s has no sound for line %d (%s), skipped"),
			*GetNameSafe(Speaker), LineIndex, *Line.HeroVoiceLine.ToString());
		AdvanceToNextLine();
		return;
	}

	// His grunt makes way for the line
	if (Appearance)
		Appearance->StopVoiceLine();

	USceneComponent* AttachTo = nullptr;

	if (const ACharacter* SpeakerCharacter = Cast<ACharacter>(Speaker))
		AttachTo = SpeakerCharacter->GetMesh();

	if (!AttachTo)
		AttachTo = Speaker->GetRootComponent();

	// Stops if the speaker is destroyed
	ActiveLine = UGameplayStatics::SpawnSoundAttached(Sound, AttachTo, NAME_None, FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset, true);

	if (!ActiveLine)
	{
		AdvanceToNextLine();
		return;
	}

	ActiveSpeaker = Speaker;
	ActiveLine->OnAudioFinished.AddUniqueDynamic(this, &ThisClass::OnLineFinished);

	// Backup: move on anyway once the line should be over
	const float Duration = Sound->GetDuration();

	if (Duration > 0.f && !Sound->IsLooping())
		GetWorld()->GetTimerManager().SetTimer(LineTimerHandle, this, &ThisClass::OnLineFinished, Duration + DialogueLineEndMargin, false);
}

void UGodsentDialogueSubsystem::OnLineFinished()
{
	if (IsDialoguePlaying())
		AdvanceToNextLine();
}

void UGodsentDialogueSubsystem::ClearActiveLine()
{
	if (IsValid(ActiveLine))
	{
		ActiveLine->OnAudioFinished.RemoveDynamic(this, &ThisClass::OnLineFinished);
		ActiveLine->Stop();
	}

	ActiveLine = nullptr;
	ActiveSpeaker.Reset();
}

void UGodsentDialogueSubsystem::EndDialogue(bool bCompleted)
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(LineTimerHandle);

	ClearActiveLine();
	QueuedLines.Reset();
	LineIndex = INDEX_NONE;

	OnDialogueFinished.Broadcast(bCompleted);
}
