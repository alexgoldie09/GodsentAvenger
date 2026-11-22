// Alex Goldberg rights reserved

#include "AI/BTService_AldegonStateCheck.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Components/Aldegon/GodsentAldegonComponent.h"
#include "GameFramework/Pawn.h"
#include "GodsentTypes/GodsentEnumTypes.h"

namespace
{
	/// How often the rules are checked (s)
	constexpr float AldegonStateCheckInterval = 0.1f;
	constexpr float AldegonStateCheckDeviation = 0.02f;
}

UBTService_AldegonStateCheck::UBTService_AldegonStateCheck()
{
	NodeName = TEXT("Aldegon State Check");

	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	Interval = AldegonStateCheckInterval;
	RandomDeviation = AldegonStateCheckDeviation;
	bNotifyTick = true;
	bCallTickOnSearchStart = true;

	InTargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InTargetActorKey), AActor::StaticClass());
	OutAldegonStateKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutAldegonStateKey), StaticEnum<EGodsentAldegonState>());

	OutShouldGroundKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutShouldGroundKey));
	OutShouldTakeOffKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutShouldTakeOffKey));
	OutInClawRangeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutInClawRangeKey));
	OutIsFrenziedKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutIsFrenziedKey));
	OutIsKnockedDownKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutIsKnockedDownKey));
	OutLivingAddsKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutLivingAddsKey));
	OutLivingTotemsKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutLivingTotemsKey));
	OutConsecutiveClawsKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutConsecutiveClawsKey));

	OutInClawRangeKey.AllowNoneAsValue(true);
	OutIsFrenziedKey.AllowNoneAsValue(true);
	OutIsKnockedDownKey.AllowNoneAsValue(true);
	OutLivingAddsKey.AllowNoneAsValue(true);
	OutLivingTotemsKey.AllowNoneAsValue(true);
	OutConsecutiveClawsKey.AllowNoneAsValue(true);
}

void UBTService_AldegonStateCheck::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		InTargetActorKey.ResolveSelectedKey(*BBAsset);
		OutAldegonStateKey.ResolveSelectedKey(*BBAsset);
		OutShouldGroundKey.ResolveSelectedKey(*BBAsset);
		OutShouldTakeOffKey.ResolveSelectedKey(*BBAsset);
		OutInClawRangeKey.ResolveSelectedKey(*BBAsset);
		OutIsFrenziedKey.ResolveSelectedKey(*BBAsset);
		OutIsKnockedDownKey.ResolveSelectedKey(*BBAsset);
		OutLivingAddsKey.ResolveSelectedKey(*BBAsset);
		OutLivingTotemsKey.ResolveSelectedKey(*BBAsset);
		OutConsecutiveClawsKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTService_AldegonStateCheck::GetStaticDescription() const
{
	return FString::Printf(TEXT("State -> %s\nShould Ground -> %s, Should Take Off -> %s\n%s"),
		*OutAldegonStateKey.SelectedKeyName.ToString(), *OutShouldGroundKey.SelectedKeyName.ToString(),
		*OutShouldTakeOffKey.SelectedKeyName.ToString(), *GetStaticServiceDescription());
}

void UBTService_AldegonStateCheck::SetBoolIfSet(UBlackboardComponent& Blackboard, const FBlackboardKeySelector& Key, bool bValue)
{
	if (Key.SelectedKeyName != NAME_None)
		Blackboard.SetValueAsBool(Key.SelectedKeyName, bValue);
}

void UBTService_AldegonStateCheck::SetIntIfSet(UBlackboardComponent& Blackboard, const FBlackboardKeySelector& Key, int32 Value)
{
	if (Key.SelectedKeyName != NAME_None)
		Blackboard.SetValueAsInt(Key.SelectedKeyName, Value);
}

void UBTService_AldegonStateCheck::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	const UGodsentAldegonComponent* Aldegon = Pawn ? Pawn->FindComponentByClass<UGodsentAldegonComponent>() : nullptr;

	if (!Blackboard || !Aldegon)
		return;

	const EGodsentAldegonState State = Aldegon->GetAldegonState();
	const AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(InTargetActorKey.SelectedKeyName));
	const float DistanceToTarget = Target ? FVector::Dist2D(Pawn->GetActorLocation(), Target->GetActorLocation()) : TNumericLimits<float>::Max();

	const int32 LivingAdds = Aldegon->GetLivingSummonCount(EGodsentAldegonSummonType::Add);
	const int32 LivingTotems = Aldegon->GetLivingSummonCount(EGodsentAldegonSummonType::Totem);
	const int32 ConsecutiveClaws = Aldegon->GetConsecutiveClaws();

	// ---- Airborne: come down? ----
	bool bShouldGround = false;

	if (State == EGodsentAldegonState::Airborne)
	{
		// Player came close (once he's had his minimum time up there)
		const bool bPlayerClose = Target && Aldegon->GetTimeInState() >= Aldegon->GetMinAirborneTime()
			&& DistanceToTarget <= Aldegon->GetGroundTriggerDistance(Target);

		bShouldGround = bPlayerClose || Aldegon->IsForcedLandingPending();
	}

	// ---- Grounded: go back up? ----
	bool bShouldTakeOff = false;

	if (State == EGodsentAldegonState::Grounded)
	{
		const int32 MaxClaws = Aldegon->GetMaxConsecutiveClaws();
		const bool bTooManyClaws = MaxClaws > 0 && ConsecutiveClaws >= MaxClaws;

		// Everything he summoned is gone, the player is keeping their distance, and he can actually summon: go up and do it
		const bool bCanResummon = Aldegon->CanSummonNow(EGodsentAldegonSummonType::Add, FGameplayTag())
			|| Aldegon->CanSummonNow(EGodsentAldegonSummonType::Totem, FGameplayTag());

		const bool bResummon = LivingAdds + LivingTotems == 0 && Target && DistanceToTarget > Aldegon->GetResummonDistance() && bCanResummon;

		bShouldTakeOff = Aldegon->IsLiftOffPending() || bTooManyClaws || bResummon;
	}

	Blackboard->SetValueAsEnum(OutAldegonStateKey.SelectedKeyName, static_cast<uint8>(State));
	SetBoolIfSet(*Blackboard, OutShouldGroundKey, bShouldGround);
	SetBoolIfSet(*Blackboard, OutShouldTakeOffKey, bShouldTakeOff);
	SetBoolIfSet(*Blackboard, OutInClawRangeKey, Target && DistanceToTarget <= Aldegon->GetClawRange());
	SetBoolIfSet(*Blackboard, OutIsFrenziedKey, Aldegon->IsFrenzied());
	SetBoolIfSet(*Blackboard, OutIsKnockedDownKey, Aldegon->IsKnockedDown());
	SetIntIfSet(*Blackboard, OutLivingAddsKey, LivingAdds);
	SetIntIfSet(*Blackboard, OutLivingTotemsKey, LivingTotems);
	SetIntIfSet(*Blackboard, OutConsecutiveClawsKey, ConsecutiveClaws);
}
