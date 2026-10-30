// Alex Goldberg rights reserved

#include "AI/BTService_TrackTargetActor.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Controllers/GodsentAIController.h"
#include "GameFramework/Pawn.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"

UBTService_TrackTargetActor::UBTService_TrackTargetActor()
{
	NodeName = TEXT("Native Track Target Actor");

	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	Interval = 0.2f;
	RandomDeviation = 0.05f;
	bNotifyTick = true;

	// Measure once as the branch starts, so the attack-range check never reads the distance left over from the last fight
	bCallTickOnSearchStart = true;

	InTargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InTargetActorKey), AActor::StaticClass());
	OutDistToTargetKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutDistToTargetKey));
	OutDistToTargetKey.AllowNoneAsValue(true);
}

void UBTService_TrackTargetActor::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		InTargetActorKey.ResolveSelectedKey(*BBAsset);
		OutDistToTargetKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTService_TrackTargetActor::GetStaticDescription() const
{
	FString Description = FString::Printf(TEXT("Tracks %s"), *InTargetActorKey.SelectedKeyName.ToString());

	if (OutDistToTargetKey.SelectedKeyName != NAME_None)
		Description += FString::Printf(TEXT(", distance -> %s"), *OutDistToTargetKey.SelectedKeyName.ToString());

	if (MaxChaseDistanceFromHome > 0.f)
		Description += FString::Printf(TEXT("\nLeash: %s cm from home"), *FString::SanitizeFloat(MaxChaseDistanceFromHome));

	return Description + TEXT("\n") + GetStaticServiceDescription();
}

void UBTService_TrackTargetActor::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* OwningPawn = AIController ? AIController->GetPawn() : nullptr;

	if (!BlackboardComponent || !OwningPawn)
		return;

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(InTargetActorKey.SelectedKeyName));

	if (!TargetActor)
		return;

	AGodsentAIController* GodsentAIController = Cast<AGodsentAIController>(AIController);

	const bool bTargetIsGone = !IsValid(TargetActor) || UGodsentFunctionLibrary::NativeDoesActorHaveTag(TargetActor, GodsentGameplayTags::Shared_Status_Dead);

	const bool bLeashBroken = MaxChaseDistanceFromHome > 0.f &&
		GodsentAIController && GodsentAIController->HasHomeLocation() &&
		FVector::Dist2D(OwningPawn->GetActorLocation(), GodsentAIController->GetHomeLocation()) > MaxChaseDistanceFromHome;

	if (bTargetIsGone || bLeashBroken)
	{
		if (GodsentAIController)
			GodsentAIController->ClearTargetActor();
		else
			BlackboardComponent->ClearValue(InTargetActorKey.SelectedKeyName);

		return;
	}

	if (OutDistToTargetKey.SelectedKeyName != NAME_None)
	{
		const float Distance = bUse2DDistance
			? FVector::Dist2D(OwningPawn->GetActorLocation(), TargetActor->GetActorLocation())
			: FVector::Dist(OwningPawn->GetActorLocation(), TargetActor->GetActorLocation());

		BlackboardComponent->SetValueAsFloat(OutDistToTargetKey.SelectedKeyName, Distance);
	}
}
