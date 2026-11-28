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
	bMirrorAttack = AldegonComponent->IsClawMirrored();
}

/// Spawned Aldegons build their Blueprint components after the mesh starts animating, so keep looking until it's there
void UGodsentAldegonAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!AldegonComponent && OwningCharacter)
		AldegonComponent = OwningCharacter->FindComponentByClass<UGodsentAldegonComponent>();
}
