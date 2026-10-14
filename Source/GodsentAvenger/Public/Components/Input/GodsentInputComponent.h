// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "DataAssets/Input/DataAsset_InputConfig.h"
#include "GodsentInputComponent.generated.h"

/**
 * Binds Input Actions by gameplay tag. Set as DefaultInputComponentClass in DefaultInput.ini.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	/// Binds the native Input Action for InInputTag to a callback
	template<class UserObject, typename CallbackFunc>
	void BindNativeInputAction(const UDataAsset_InputConfig* InInputConfig,
		const FGameplayTag& InInputTag,
		ETriggerEvent TriggerEvent,
		UserObject* ContextObject,
		CallbackFunc Func);

	/// Binds every ability Input Action to pressed and released callbacks, passing the input tag
	template<class UserObject, typename CallbackFunc>
	void BindAbilityInputAction(const UDataAsset_InputConfig* InInputConfig,
		UserObject* ContextObject,
		CallbackFunc InputPressedFunc,
		CallbackFunc InputReleasedFunc);
};

template<class UserObject, typename CallbackFunc>
inline void UGodsentInputComponent::BindNativeInputAction(const UDataAsset_InputConfig* InInputConfig,
	const FGameplayTag& InInputTag,
	ETriggerEvent TriggerEvent,
	UserObject* ContextObject,
	CallbackFunc Func)
{
	checkf(InInputConfig, TEXT("Input config data asset is null, can not proceed with binding!"));

	if (UInputAction* FoundAction = InInputConfig->FindNativeInputActionByTag(InInputTag))
		BindAction(FoundAction, TriggerEvent, ContextObject, Func);
}

template<class UserObject, typename CallbackFunc>
inline void UGodsentInputComponent::BindAbilityInputAction(const UDataAsset_InputConfig* InInputConfig,
	UserObject* ContextObject,
	CallbackFunc InputPressedFunc,
	CallbackFunc InputReleasedFunc)
{
	checkf(InInputConfig, TEXT("Input config data asset is null, can not proceed with binding!"));

	for (const FGodsentInputActionConfig& AbilityActionConfig : InInputConfig->AbilityInputActions)
	{
		if (!AbilityActionConfig.IsValid())
			continue;

		BindAction(AbilityActionConfig.InputAction, ETriggerEvent::Started, ContextObject, InputPressedFunc, AbilityActionConfig.InputTag);
		BindAction(AbilityActionConfig.InputAction, ETriggerEvent::Completed, ContextObject, InputReleasedFunc, AbilityActionConfig.InputTag);
	}
}
