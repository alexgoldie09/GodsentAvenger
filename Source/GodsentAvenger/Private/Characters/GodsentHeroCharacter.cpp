// Alex Goldberg rights reserved

#include "Characters/GodsentHeroCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GodsentFunctionLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "GodsentGameplayTags.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "Components/Combat/HeroCombatComponent.h"
#include "Components/Input/GodsentInputComponent.h"
#include "Components/UI/HeroUIComponent.h"
#include "DataAssets/Input/DataAsset_InputConfig.h"
#include "DataAssets/StartUpData/DataAsset_StartUpDataBase.h"

AGodsentHeroCharacter::AGodsentHeroCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// The controller only rotates the camera; the character turns towards its movement
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 200.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 400.f; // Becomes the MoveSpeed attribute's default
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	HeroCombatComponent = CreateDefaultSubobject<UHeroCombatComponent>(TEXT("HeroCombatComponent"));
	HeroUIComponent = CreateDefaultSubobject<UHeroUIComponent>(TEXT("HeroUIComponent"));

	// Hidden until the Light skill is unlocked; the light ability sets its radius per tier
	LightAuraComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightAuraComponent"));
	LightAuraComponent->SetupAttachment(GetRootComponent());
	LightAuraComponent->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	LightAuraComponent->SetCastShadows(false);
	LightAuraComponent->SetVisibility(false);
}

UPawnCombatComponent* AGodsentHeroCharacter::GetPawnCombatComponent() const
{
	return HeroCombatComponent;
}

UPawnUIComponent* AGodsentHeroCharacter::GetPawnUIComponent() const
{
	return HeroUIComponent;
}

UHeroUIComponent* AGodsentHeroCharacter::GetHeroUIComponent() const
{
	return HeroUIComponent;
}

/// Loads the hero's startup data synchronously and gives it to the ASC
void AGodsentHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!CharacterStartUpData.IsNull())
	{
		if (UDataAsset_StartUpDataBase* LoadedData = CharacterStartUpData.LoadSynchronous())
			LoadedData->GiveToAbilitySystemComponent(GodsentAbilitySystemComponent);
	}
}

/// Adds the default mapping context and binds native and ability inputs
void AGodsentHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	checkf(InputConfigDataAsset, TEXT("Input config data asset is null, can not proceed with binding!"));

	ULocalPlayer* LocalPlayer = GetController<APlayerController>()->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	check(Subsystem);

	Subsystem->AddMappingContext(InputConfigDataAsset->DefaultMappingContext, 0);

	// Crashes here mean DefaultInputComponentClass in DefaultInput.ini isn't set to GodsentInputComponent
	UGodsentInputComponent* GodsentInputComponent = CastChecked<UGodsentInputComponent>(PlayerInputComponent);

	GodsentInputComponent->BindNativeInputAction(InputConfigDataAsset, GodsentGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	GodsentInputComponent->BindNativeInputAction(InputConfigDataAsset, GodsentGameplayTags::InputTag_Look, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	GodsentInputComponent->BindNativeInputAction(InputConfigDataAsset, GodsentGameplayTags::InputTag_SwitchTarget, ETriggerEvent::Triggered, this, &ThisClass::Input_SwitchTargetTriggered);
	GodsentInputComponent->BindNativeInputAction(InputConfigDataAsset, GodsentGameplayTags::InputTag_SwitchTarget, ETriggerEvent::Completed, this, &ThisClass::Input_SwitchTargetCompleted);

	GodsentInputComponent->BindAbilityInputAction(InputConfigDataAsset, this, &ThisClass::Input_AbilityInputPressed, &ThisClass::Input_AbilityInputReleased);
}

/// Moves relative to the camera's yaw (ignored while an ability has locked movement)
void AGodsentHeroCharacter::Input_Move(const FInputActionValue& InputActionValue)
{
	if (UGodsentFunctionLibrary::NativeDoesActorHaveTag(this, GodsentGameplayTags::Player_Status_MovementLocked))
		return;

	const FVector2D MovementVector = InputActionValue.Get<FVector2D>();
	const FRotator MovementRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

	if (MovementVector.Y != 0.f)
		AddMovementInput(MovementRotation.RotateVector(FVector::ForwardVector), MovementVector.Y);

	if (MovementVector.X != 0.f)
		AddMovementInput(MovementRotation.RotateVector(FVector::RightVector), MovementVector.X);
}

/// Rotates the camera
void AGodsentHeroCharacter::Input_Look(const FInputActionValue& InputActionValue)
{
	const FVector2D LookDirection = InputActionValue.Get<FVector2D>();

	if (LookDirection.X != 0.f)
		AddControllerYawInput(LookDirection.X);

	if (LookDirection.Y != 0.f)
		AddControllerPitchInput(LookDirection.Y);
}

/// Stores the stick/mouse direction while switching lock-on targets
void AGodsentHeroCharacter::Input_SwitchTargetTriggered(const FInputActionValue& InputActionValue)
{
	SwitchDirection = InputActionValue.Get<FVector2D>();
}

/// Sends a SwitchTarget.Left or .Right event to the target lock ability (small flicks are ignored)
void AGodsentHeroCharacter::Input_SwitchTargetCompleted(const FInputActionValue& InputActionValue)
{
	const float SwitchAmount = SwitchDirection.X;
	SwitchDirection = FVector2D::ZeroVector;

	if (FMath::Abs(SwitchAmount) < SwitchTargetInputThreshold)
		return;

	FGameplayEventData Data;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		this,
		SwitchAmount > 0.f ? GodsentGameplayTags::Player_Event_SwitchTarget_Right : GodsentGameplayTags::Player_Event_SwitchTarget_Left,
		Data);
}

void AGodsentHeroCharacter::Input_AbilityInputPressed(FGameplayTag InInputTag)
{
	GodsentAbilitySystemComponent->OnAbilityInputPressed(InInputTag);
}

void AGodsentHeroCharacter::Input_AbilityInputReleased(FGameplayTag InInputTag)
{
	GodsentAbilitySystemComponent->OnAbilityInputReleased(InInputTag);
}
