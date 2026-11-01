// Alex Goldberg rights reserved

#include "Components/Ghost/GodsentRevealComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraComponent.h"
#include "Components/MeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "GodsentGameplayTags.h"

UGodsentRevealComponent::UGodsentRevealComponent()
{
	// Ticks only while fading
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	RevealTag = GodsentGameplayTags::Enemy_Status_Revealed;
}

void UGodsentRevealComponent::BeginPlay()
{
	Super::BeginPlay();

	// Start hidden
	CurrentRevealAlpha = 0.f;
	TargetRevealAlpha = 0.f;
	ApplyRevealAlpha();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());

	if (!ASC)
		return;

	BoundASC = ASC;

	RevealTagHandle = ASC->RegisterGameplayTagEvent(RevealTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::OnRevealTagChanged);

	DeadTagHandle = ASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::OnDeadTagChanged);

	// In case the tag was added before this component started listening
	if (ASC->HasMatchingGameplayTag(RevealTag))
		SetTargetRevealAlpha(1.f);
}

void UGodsentRevealComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->RegisterGameplayTagEvent(RevealTag, EGameplayTagEventType::NewOrRemoved).Remove(RevealTagHandle);
		ASC->RegisterGameplayTagEvent(GodsentGameplayTags::Shared_Status_Dead, EGameplayTagEventType::NewOrRemoved).Remove(DeadTagHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UGodsentRevealComponent::OnRevealTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// Dying removes the combat branch (and its tag); stay visible for the death dissolve
	if (bIsDead)
		return;

	SetTargetRevealAlpha(NewCount > 0 ? 1.f : 0.f);
}

void UGodsentRevealComponent::OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
		return;

	bIsDead = true;
	SetTargetRevealAlpha(1.f);
}

void UGodsentRevealComponent::SetTargetRevealAlpha(float NewTarget)
{
	const bool bWasRevealing = TargetRevealAlpha > 0.5f;
	TargetRevealAlpha = NewTarget;

	if (bWasRevealing != (TargetRevealAlpha > 0.5f))
		OnRevealChanged.Broadcast(TargetRevealAlpha > 0.5f);

	SetComponentTickEnabled(!FMath::IsNearlyEqual(CurrentRevealAlpha, TargetRevealAlpha));
}

void UGodsentRevealComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Speed = TargetRevealAlpha > CurrentRevealAlpha ? RevealSpeed : HideSpeed;
	CurrentRevealAlpha = FMath::FInterpConstantTo(CurrentRevealAlpha, TargetRevealAlpha, DeltaTime, Speed);

	ApplyRevealAlpha();

	if (FMath::IsNearlyEqual(CurrentRevealAlpha, TargetRevealAlpha))
		SetComponentTickEnabled(false);
}

void UGodsentRevealComponent::ApplyRevealAlpha()
{
	const AActor* Owner = GetOwner();

	if (!Owner)
		return;

	// Every mesh (skull, weapon...): creates dynamic material instances on first use, shared with the dissolve
	TArray<UMeshComponent*> MeshComponents;
	Owner->GetComponents<UMeshComponent>(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
		MeshComponent->SetScalarParameterValueOnMaterials(MaterialParameterName, CurrentRevealAlpha);

	TArray<UNiagaraComponent*> NiagaraComponents;
	Owner->GetComponents<UNiagaraComponent>(NiagaraComponents);

	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
		NiagaraComponent->SetVariableFloat(NiagaraParameterName, CurrentRevealAlpha);

	if (bHideWidgetsWhileHidden)
	{
		TArray<UWidgetComponent*> WidgetComponents;
		Owner->GetComponents<UWidgetComponent>(WidgetComponents);

		for (UWidgetComponent* WidgetComponent : WidgetComponents)
			WidgetComponent->SetVisibility(CurrentRevealAlpha > 0.5f);
	}
}
