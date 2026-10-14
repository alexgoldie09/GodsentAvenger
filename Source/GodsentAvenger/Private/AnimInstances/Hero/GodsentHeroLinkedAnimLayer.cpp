// Alex Goldberg rights reserved

#include "AnimInstances/Hero/GodsentHeroLinkedAnimLayer.h"
#include "AnimInstances/Hero/GodsentHeroAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

/// The hero anim instance this layer is linked into
UGodsentHeroAnimInstance* UGodsentHeroLinkedAnimLayer::GetHeroAnimInstance() const
{
	return Cast<UGodsentHeroAnimInstance>(GetOwningComponent()->GetAnimInstance());
}
