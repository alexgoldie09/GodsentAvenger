// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "GodsentDialogueSubsystem.generated.h"

class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGodsentDialogueFinished, bool, bCompleted);

/**
 * Plays voice lines that aren't in a montage, one after another (the game's "audio manager" for dialogue).
 * - Play Dialogue: a list of lines. Each waits its Delay Before, plays from its speaker, and the next starts once it ends.
 * - A hero line can be a voice tag (Voice.Entry.Family...), looked up in that heir's voice, so it works for every voice.
 * - Only one conversation at a time: playing a new one replaces the old one.
 * - The hero doesn't grunt (attack, hit...) over his own dialogue: Heir Appearance skips those while he's speaking.
 * Used for the heir's entry lines (game mode) and the boss start / kill player lines (Aldegon). Callable from Blueprint too.
 */
UCLASS(BlueprintType)
class GODSENTAVENGER_API UGodsentDialogueSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/// The dialogue subsystem for this world (nullptr outside a game world)
	static UGodsentDialogueSubsystem* Get(const UObject* WorldContextObject);

	/// Plays these lines in order (replaces anything already playing)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Dialogue")
	void PlayDialogue(const TArray<FGodsentDialogueLine>& NewLines);

	/// Stops the current line and drops the rest
	UFUNCTION(BlueprintCallable, Category = "Godsent|Dialogue")
	void StopDialogue();

	/// Stops the dialogue if this actor is speaking now or has a line still to come (e.g. they died)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Dialogue")
	void StopDialogueFor(AActor* Speaker);

	/// True from Play Dialogue until the last line ends (including the waits between lines)
	UFUNCTION(BlueprintPure, Category = "Godsent|Dialogue")
	bool IsDialoguePlaying() const { return LineIndex != INDEX_NONE; }

	/// True while this actor's line is playing
	UFUNCTION(BlueprintPure, Category = "Godsent|Dialogue")
	bool IsSpeaking(const AActor* Speaker) const;

	/// The dialogue ended. bCompleted is false if it was stopped or replaced.
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Dialogue")
	FOnGodsentDialogueFinished OnDialogueFinished;

	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/// Waits the next line's Delay Before, then starts it (or finishes if there are none left)
	void AdvanceToNextLine();

	/// Plays the current line (waits a little if the hero's voice is still loading)
	void StartCurrentLine();

	/// The current line ended (or its fallback timer ran out)
	UFUNCTION()
	void OnLineFinished();

	/// Stops and forgets the current line's sound
	void ClearActiveLine();

	/// Clears everything and fires On Dialogue Finished
	void EndDialogue(bool bCompleted);

	/// The conversation being played
	UPROPERTY()
	TArray<FGodsentDialogueLine> QueuedLines;

	int32 LineIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UAudioComponent> ActiveLine;

	TWeakObjectPtr<AActor> ActiveSpeaker;

	/// The wait before a line, the voice loading retries, and the backup "line's over" timer
	FTimerHandle LineTimerHandle;

	/// How long the current line has waited for the hero's voice to load
	float VoiceWaitTime = 0.f;
};
