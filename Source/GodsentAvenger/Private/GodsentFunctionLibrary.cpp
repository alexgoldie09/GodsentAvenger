// Alex Goldberg rights reserved

#include "GodsentFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "GodsentGameplayTags.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "GodsentTypes/GodsentCountDownAction.h"

/// Gets the Godsent Ability System Component from an actor (asserts if the actor has none)
UGodsentAbilitySystemComponent* UGodsentFunctionLibrary::NativeGetGodsentASCFromActor(AActor* InActor)
{
	check(InActor);
	return CastChecked<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InActor));
}

/// Adds a loose gameplay tag to an actor if it doesn't already have it
void UGodsentFunctionLibrary::AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd)
{
	UGodsentAbilitySystemComponent* ASC = NativeGetGodsentASCFromActor(InActor);

	if (!ASC->HasMatchingGameplayTag(TagToAdd))
		ASC->AddLooseGameplayTag(TagToAdd);
}

/// Removes a loose gameplay tag from an actor if it has it
void UGodsentFunctionLibrary::RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove)
{
	UGodsentAbilitySystemComponent* ASC = NativeGetGodsentASCFromActor(InActor);

	if (ASC->HasMatchingGameplayTag(TagToRemove))
		ASC->RemoveLooseGameplayTag(TagToRemove);
}

/// Checks whether an actor has a gameplay tag
bool UGodsentFunctionLibrary::NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck)
{
	UGodsentAbilitySystemComponent* ASC = NativeGetGodsentASCFromActor(InActor);
	return ASC->HasMatchingGameplayTag(TagToCheck);
}

/// Checks whether an actor has a gameplay tag, with Yes and No exec pins
void UGodsentFunctionLibrary::BP_DoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck,
	EGodsentConfirmType& OutConfirmType)
{
	OutConfirmType = NativeDoesActorHaveTag(InActor, TagToCheck) ? EGodsentConfirmType::Yes : EGodsentConfirmType::No;
}

/// Checks whether TargetPawn is on a different team to QueryPawn
bool UGodsentFunctionLibrary::IsTargetPawnHostile(APawn* QueryPawn, APawn* TargetPawn)
{
	check(QueryPawn && TargetPawn);

	IGenericTeamAgentInterface* QueryTeamAgent = Cast<IGenericTeamAgentInterface>(QueryPawn->GetController());
	IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(TargetPawn->GetController());

	if (QueryTeamAgent && TargetTeamAgent)
		return QueryTeamAgent->GetGenericTeamId() != TargetTeamAgent->GetGenericTeamId();

	return false;
}

/// Gets the value of a Scalable Float at a level
float UGodsentFunctionLibrary::GetScalableFloatValueAtLevel(const FScalableFloat& InScalableFloat, float InLevel)
{
	return InScalableFloat.GetValueAtLevel(InLevel);
}

/// Works out which direction a hit came from, relative to the victim
FGameplayTag UGodsentFunctionLibrary::ComputeHitReactDirectionTag(AActor* InAttacker, AActor* InVictim,
	float& OutAngleDifference)
{
	check(InAttacker && InVictim);

	const FVector VictimForward = InVictim->GetActorForwardVector();
	const FVector VictimToAttackerNorm = (InAttacker->GetActorLocation() - InVictim->GetActorLocation()).GetSafeNormal();

	const float DotResult = FVector::DotProduct(VictimForward, VictimToAttackerNorm);
	OutAngleDifference = UKismetMathLibrary::DegAcos(DotResult);

	const FVector CrossResult = FVector::CrossProduct(VictimForward, VictimToAttackerNorm);

	if (CrossResult.Z < 0.f)
		OutAngleDifference *= -1.f;

	if (OutAngleDifference >= -45.f && OutAngleDifference <= 45.f)
		return GodsentGameplayTags::Shared_Status_HitReact_Front;
	else if (OutAngleDifference < -45.f && OutAngleDifference >= -135.f)
		return GodsentGameplayTags::Shared_Status_HitReact_Left;
	else if (OutAngleDifference < -135.f || OutAngleDifference > 135.f)
		return GodsentGameplayTags::Shared_Status_HitReact_Back;
	else if (OutAngleDifference > 45.f && OutAngleDifference <= 135.f)
		return GodsentGameplayTags::Shared_Status_HitReact_Right;

	return GodsentGameplayTags::Shared_Status_HitReact_Front;
}

/// Checks whether the defender is facing the attacker closely enough for a block to count
bool UGodsentFunctionLibrary::IsValidBlock(AActor* InAttacker, AActor* InDefender, float Threshold)
{
	check(InAttacker && InDefender);

	const float DotResult = FVector::DotProduct(InAttacker->GetActorForwardVector(), InDefender->GetActorForwardVector());
	return DotResult < Threshold;
}

/// Applies a gameplay effect spec to a target actor using the instigator's ASC
bool UGodsentFunctionLibrary::ApplyGameplayEffectSpecHandleToTargetActor(AActor* InInstigator, AActor* InTargetActor,
	const FGameplayEffectSpecHandle& InGameplayEffectSpecHandle)
{
	UGodsentAbilitySystemComponent* SourceASC = NativeGetGodsentASCFromActor(InInstigator);
	UGodsentAbilitySystemComponent* TargetASC = NativeGetGodsentASCFromActor(InTargetActor);

	const FActiveGameplayEffectHandle ActiveGameplayEffectHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*InGameplayEffectSpecHandle.Data, TargetASC);
	return ActiveGameplayEffectHandle.WasSuccessfullyApplied();
}

/// Counts down over TotalTime, firing Updated every UpdateInterval (used for cooldown UI)
void UGodsentFunctionLibrary::CountDown(const UObject* WorldContextObject, float TotalTime, float UpdateInterval,
	float& OutRemainingTime, EGodsentCountDownActionInput CountDownInput,
	EGodsentCountDownActionOutput& CountDownOutput, FLatentActionInfo LatentInfo)
{
	UWorld* World = nullptr;

	if (GEngine)
		World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
		return;

	FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
	FGodsentCountDownAction* FoundAction = LatentActionManager.FindExistingAction<FGodsentCountDownAction>(LatentInfo.CallbackTarget, LatentInfo.UUID);

	if (CountDownInput == EGodsentCountDownActionInput::Start && !FoundAction)
	{
		LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID,
			new FGodsentCountDownAction(TotalTime, UpdateInterval, OutRemainingTime, CountDownOutput, LatentInfo));
	}

	if (CountDownInput == EGodsentCountDownActionInput::Cancel && FoundAction)
		FoundAction->CancelAction();
}
