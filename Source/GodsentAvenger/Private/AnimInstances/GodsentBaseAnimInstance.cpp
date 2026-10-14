// Alex Goldberg rights reserved

#include "AnimInstances/GodsentBaseAnimInstance.h"
#include "GodsentFunctionLibrary.h"
#include "GameFramework/Pawn.h"

/// Checks whether the owning pawn has a gameplay tag
bool UGodsentBaseAnimInstance::DoesOwnerHaveTag(FGameplayTag TagToCheck) const
{
	if (APawn* OwningPawn = TryGetPawnOwner())
		return UGodsentFunctionLibrary::NativeDoesActorHaveTag(OwningPawn, TagToCheck);

	return false;
}
