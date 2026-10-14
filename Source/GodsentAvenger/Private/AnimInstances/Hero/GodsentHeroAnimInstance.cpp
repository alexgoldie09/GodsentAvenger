// Alex Goldberg rights reserved

#include "AnimInstances/Hero/GodsentHeroAnimInstance.h"
#include "Characters/GodsentHeroCharacter.h"

void UGodsentHeroAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (OwningCharacter)
		OwningHeroCharacter = Cast<AGodsentHeroCharacter>(OwningCharacter);
}

void UGodsentHeroAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (bHasAcceleration)
	{
		IdleElapsedTime = 0.f;
		bShouldEnterRelaxState = false;
	}
	else
	{
		IdleElapsedTime += DeltaSeconds;
		bShouldEnterRelaxState = IdleElapsedTime >= EnterRelaxStateThreshold;
	}
}
