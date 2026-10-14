// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Interfaces/PawnUIInterface.h"
#include "GodsentBaseCharacter.generated.h"

class UGodsentAbilitySystemComponent;
class UGodsentAttributeSet;
class UDataAsset_StartUpDataBase;
class UMotionWarpingComponent;
struct FOnAttributeChangeData;

/**
 * Shared base for the hero and enemies: ASC, attribute set, motion warping and startup data.
 * IPawnCombatInterface is added back in stage 2 with the combat components.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentBaseCharacter : public ACharacter, public IAbilitySystemInterface, public IPawnUIInterface
{
	GENERATED_BODY()

public:
	AGodsentBaseCharacter();

	//~ Begin IAbilitySystemInterface Interface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface Interface

	//~ Begin IPawnUIInterface Interface
	virtual UPawnUIComponent* GetPawnUIComponent() const override;
	//~ End IPawnUIInterface Interface

	FORCEINLINE UGodsentAbilitySystemComponent* GetGodsentAbilitySystemComponent() const { return GodsentAbilitySystemComponent; }
	FORCEINLINE UGodsentAttributeSet* GetGodsentAttributeSet() const { return GodsentAttributeSet; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UGodsentAbilitySystemComponent> GodsentAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UGodsentAttributeSet> GodsentAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionWarping")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	/// Abilities and effects given to this character when it's possessed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterData")
	TSoftObjectPtr<UDataAsset_StartUpDataBase> CharacterStartUpData;

	//~ Begin APawn Interface.
	virtual void PossessedBy(AController* NewController) override;
	//~ End APawn Interface.

private:
	/// Pushes the MoveSpeed attribute into the movement component's Max Walk Speed
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);

	/// Handle for the MoveSpeed change binding, so it's only bound once
	FDelegateHandle MoveSpeedChangedHandle;
};
