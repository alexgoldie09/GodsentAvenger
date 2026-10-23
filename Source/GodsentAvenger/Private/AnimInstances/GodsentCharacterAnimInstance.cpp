// Alex Goldberg rights reserved

#include "AnimInstances/GodsentCharacterAnimInstance.h"
#include "KismetAnimationLibrary.h"
#include "Characters/GodsentBaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UGodsentCharacterAnimInstance::NativeInitializeAnimation()
{
	OwningCharacter = Cast<AGodsentBaseCharacter>(TryGetPawnOwner());

	if (OwningCharacter)
		OwningMovementComponent = OwningCharacter->GetCharacterMovement();
}

void UGodsentCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	if (!OwningCharacter || !OwningMovementComponent)
		return;

	GroundSpeed = OwningCharacter->GetVelocity().Size2D();
	bHasAcceleration = OwningMovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.f;
	LocomotionDirection = UKismetAnimationLibrary::CalculateDirection(OwningCharacter->GetVelocity(), OwningCharacter->GetActorRotation());
	bIsFalling = OwningMovementComponent->IsFalling();
}
