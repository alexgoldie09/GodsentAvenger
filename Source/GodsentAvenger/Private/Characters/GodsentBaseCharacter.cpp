// Alex Goldberg rights reserved

#include "Characters/GodsentBaseCharacter.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "AbilitySystem/GodsentAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"

AGodsentBaseCharacter::AGodsentBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GetMesh()->bReceivesDecals = false;

	GodsentAbilitySystemComponent = CreateDefaultSubobject<UGodsentAbilitySystemComponent>(TEXT("GodsentAbilitySystemComponent"));
	GodsentAttributeSet = CreateDefaultSubobject<UGodsentAttributeSet>(TEXT("GodsentAttributeSet"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
}

UAbilitySystemComponent* AGodsentBaseCharacter::GetAbilitySystemComponent() const
{
	return GetGodsentAbilitySystemComponent();
}

/// The base class has no UI component; the hero and enemy classes override this
UPawnUIComponent* AGodsentBaseCharacter::GetPawnUIComponent() const
{
	return nullptr;
}

/// Initialises the ASC and links the MoveSpeed attribute to the movement component.
/// Child classes give their startup data after calling Super.
void AGodsentBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!GodsentAbilitySystemComponent)
		return;

	GodsentAbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (!MoveSpeedChangedHandle.IsValid())
	{
		// The Blueprint's Max Walk Speed becomes the MoveSpeed default; the stats effect can still override it
		GodsentAttributeSet->InitMoveSpeed(GetCharacterMovement()->MaxWalkSpeed);

		MoveSpeedChangedHandle = GodsentAbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(UGodsentAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &ThisClass::OnMoveSpeedChanged);
	}

	ensureMsgf(!CharacterStartUpData.IsNull(), TEXT("CharacterStartUpData is null for %s. Assign a startup data asset in the character's Class Defaults."), *GetName());
}

/// Pushes the MoveSpeed attribute into the movement component's Max Walk Speed
void AGodsentBaseCharacter::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}
