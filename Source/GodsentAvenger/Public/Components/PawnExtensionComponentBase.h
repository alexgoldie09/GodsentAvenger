// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "PawnExtensionComponentBase.generated.h"

/**
 * Base for components that live on a pawn (combat and UI components).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GODSENTAVENGER_API UPawnExtensionComponentBase : public UActorComponent
{
	GENERATED_BODY()

protected:
	/// Gets the owning pawn cast to T
	template<class T>
	T* GetOwningPawn() const
	{
		static_assert(TPointerIsConvertibleFromTo<T, APawn>::Value, "T template Parameter to GetPawn must be derived from APawn");
		return CastChecked<T>(GetOwner());
	}

	/// Gets the owning pawn
	APawn* GetOwningPawn() const
	{
		return GetOwningPawn<APawn>();
	}

	/// Gets the owning pawn's controller cast to T
	template<class T>
	T* GetOwningController() const
	{
		static_assert(TPointerIsConvertibleFromTo<T, AController>::Value, "T template Parameter to GetController must be derived from AController");
		return GetOwningPawn<APawn>()->GetController<T>();
	}
};
