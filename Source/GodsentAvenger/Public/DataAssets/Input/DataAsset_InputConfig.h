// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DataAsset_InputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/// Pairs an input tag with the Input Action that fires it
USTRUCT(BlueprintType)
struct FGodsentInputActionConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> InputAction;

	bool IsValid() const
	{
		return InputTag.IsValid() && InputAction != nullptr;
	}
};

/**
 * The hero's input setup: the default mapping context, native actions (move, look) and ability actions.
 */
UCLASS()
class GODSENTAVENGER_API UDataAsset_InputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/// Mapping context added when the hero is possessed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/// Actions handled directly by the character (InputTag.Move, InputTag.Look, InputTag.SwitchTarget)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputTag"))
	TArray<FGodsentInputActionConfig> NativeInputActions;

	/// Actions that activate abilities by input tag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputTag"))
	TArray<FGodsentInputActionConfig> AbilityInputActions;

	/// Finds the native Input Action bound to a tag
	UInputAction* FindNativeInputActionByTag(const FGameplayTag& InInputTag) const;
};
