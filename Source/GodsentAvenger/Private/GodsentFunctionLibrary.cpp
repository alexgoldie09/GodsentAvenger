// Alex Goldberg rights reserved

#include "GodsentFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "GodsentGameplayTags.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "Interfaces/PawnCombatInterface.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "GodsentTypes/GodsentCountDownAction.h"
#include "GodsentGameInstance.h"

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

/// Gets an actor's combat component through IPawnCombatInterface, or nullptr
UPawnCombatComponent* UGodsentFunctionLibrary::NativeGetPawnCombatComponentFromActor(AActor* InActor)
{
	check(InActor);

	if (IPawnCombatInterface* PawnCombatInterface = Cast<IPawnCombatInterface>(InActor))
		return PawnCombatInterface->GetPawnCombatComponent();

	return nullptr;
}

/// Gets an actor's combat component, with Valid and Invalid exec pins
UPawnCombatComponent* UGodsentFunctionLibrary::BP_GetPawnCombatComponentFromActor(AActor* InActor,
	EGodsentValidType& OutValidType)
{
	UPawnCombatComponent* PawnCombatComponent = NativeGetPawnCombatComponentFromActor(InActor);
	OutValidType = PawnCombatComponent ? EGodsentValidType::Valid : EGodsentValidType::Invalid;
	return PawnCombatComponent;
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

/// Checks whether the attacker is in front of the defender closely enough for a block to count
bool UGodsentFunctionLibrary::IsValidBlock(AActor* InAttacker, AActor* InDefender, float Threshold)
{
	check(InAttacker && InDefender);

	const FVector DefenderForward = InDefender->GetActorForwardVector().GetSafeNormal2D();
	const FVector DefenderToAttacker = (InAttacker->GetActorLocation() - InDefender->GetActorLocation()).GetSafeNormal2D();

	// Threshold is negative (-0.1), so the attacker must be within ~84° either side of where we're facing
	return FVector::DotProduct(DefenderForward, DefenderToAttacker) > -Threshold;
}

/// Works out how a hit on InDefender is blocked
EGodsentBlockResult UGodsentFunctionLibrary::NativeResolveBlock(AActor* InAttacker, AActor* InDefender,
	bool bAttackIsUnblockable, float FacingThreshold)
{
	check(InAttacker && InDefender);

	if (bAttackIsUnblockable || !NativeDoesActorHaveTag(InDefender, GodsentGameplayTags::Player_Status_Blocking))
		return EGodsentBlockResult::None;

	// Blocks only work while facing the attacker
	if (!IsValidBlock(InAttacker, InDefender, FacingThreshold))
		return EGodsentBlockResult::None;

	if (NativeDoesActorHaveTag(InDefender, GodsentGameplayTags::Player_Status_Parrying))
		return EGodsentBlockResult::Parried;

	if (NativeDoesActorHaveTag(InDefender, GodsentGameplayTags::Player_Status_FullBlock))
		return EGodsentBlockResult::FullBlocked;

	return EGodsentBlockResult::Blocked;
}

/// Sends the block or parry events for a resolved hit. Returns true when the hit should deal no damage.
bool UGodsentFunctionLibrary::NativeHandleBlockResult(AActor* InAttacker, AActor* InDefender, EGodsentBlockResult BlockResult)
{
	if (BlockResult == EGodsentBlockResult::None)
		return false;

	FGameplayEventData EventData;
	EventData.Instigator = InAttacker;
	EventData.Target = InDefender;

	if (BlockResult == EGodsentBlockResult::Parried)
	{
		// The block ability plays the parry response and decides whether the attacker staggers (Riposte tier)
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(InDefender, GodsentGameplayTags::Player_Event_Parry, EventData);

		return true;
	}

	// Blocked and FullBlocked both tell the block ability a hit was blocked (FX and stamina cost)
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(InDefender, GodsentGameplayTags::Player_Event_SuccessfulBlock, EventData);

	return BlockResult == EGodsentBlockResult::FullBlocked;
}

/// Finds every living, non-invincible hostile pawn (with an ability system) within Radius of Origin
void UGodsentFunctionLibrary::GetHostilePawnsInRadius(APawn* QueryPawn, FVector Origin, float Radius, TArray<APawn*>& OutHostilePawns)
{
	OutHostilePawns.Reset();

	if (!QueryPawn || Radius <= 0.f)
		return;

	UWorld* World = QueryPawn->GetWorld();

	if (!World)
		return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GodsentHostilePawnsInRadius), false, QueryPawn);

	World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		APawn* OverlappedPawn = Cast<APawn>(Overlap.GetActor());

		// A pawn can overlap with several components, so only count it once
		if (!OverlappedPawn || OutHostilePawns.Contains(OverlappedPawn))
			continue;

		// Skip anything without a Godsent ability system (the tag checks below would assert)
		if (!Cast<UGodsentAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OverlappedPawn)))
			continue;

		if (!IsTargetPawnHostile(QueryPawn, OverlappedPawn))
			continue;

		if (NativeDoesActorHaveTag(OverlappedPawn, GodsentGameplayTags::Shared_Status_Dead) ||
			NativeDoesActorHaveTag(OverlappedPawn, GodsentGameplayTags::Shared_Status_Invincible))
			continue;

		OutHostilePawns.Add(OverlappedPawn);
	}
}

/// Snaps ActorToRotate (yaw only) to face TargetActor
void UGodsentFunctionLibrary::RotateActorToFaceTarget(AActor* ActorToRotate, AActor* TargetActor)
{
	if (!ActorToRotate || !TargetActor)
		return;

	const FVector ToTarget = (TargetActor->GetActorLocation() - ActorToRotate->GetActorLocation()).GetSafeNormal2D();

	if (ToTarget.IsNearlyZero())
		return;

	ActorToRotate->SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
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

/// Gets the Godsent Game Instance from a world context object, or nullptr if not found
UGodsentGameInstance* UGodsentFunctionLibrary::GetGodsentGameInstance(const UObject* WorldContextObject)
{
	if (GEngine)
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
			return World->GetGameInstance<UGodsentGameInstance>();
	
	return nullptr;
}

/// Toggles the input mode between GameOnly and UIOnly, showing or hiding the mouse cursor accordingly
void UGodsentFunctionLibrary::ToggleInputMode(const UObject* WorldContextObject, EGodsentInputMode InInputMode)
{
	APlayerController* PlayerController = nullptr;
	
	if (GEngine)
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
			PlayerController = World->GetFirstPlayerController();
	
	if (!PlayerController)
		return;
	
	FInputModeGameOnly GameOnlyInputMode;
	FInputModeUIOnly UIOnlyInputMode;
	
	switch (InInputMode) 
	{
		case EGodsentInputMode::GameOnly:
			PlayerController->SetInputMode(GameOnlyInputMode);
			PlayerController->bShowMouseCursor = false;
		break;
		case EGodsentInputMode::UIOnly:
			PlayerController->SetInputMode(UIOnlyInputMode);
			PlayerController->bShowMouseCursor = true;
		break;
	}



}
