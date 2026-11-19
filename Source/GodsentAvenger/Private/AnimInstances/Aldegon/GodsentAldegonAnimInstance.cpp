// Alex Goldberg rights reserved

#include "AnimInstances/Aldegon/GodsentAldegonAnimInstance.h"
#include "Characters/GodsentBaseCharacter.h"
#include "Components/Aldegon/GodsentAldegonComponent.h"

void UGodsentAldegonAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (OwningCharacter)
		AldegonComponent = OwningCharacter->FindComponentByClass<UGodsentAldegonComponent>();
}

void UGodsentAldegonAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (!AldegonComponent)
		return;

	AldegonState = AldegonComponent->GetAldegonState();

	// Not while Waking: Rise blends out into standing, not back into the seat
	bUseThronePose = AldegonState == EGodsentAldegonState::Dormant || AldegonState == EGodsentAldegonState::SittingDown;
	bIsFlying = AldegonComponent->IsFlying();
}
