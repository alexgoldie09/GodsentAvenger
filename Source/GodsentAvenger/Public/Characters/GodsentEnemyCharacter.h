// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Characters/GodsentBaseCharacter.h"
#include "GodsentEnemyCharacter.generated.h"

class UBoxComponent;
class UEnemyCombatComponent;
class UEnemyUIComponent;
class UWidgetComponent;

/**
 * Base for every enemy (and the training dummy): combat and UI components, hand hit boxes and a health bar widget.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentEnemyCharacter : public AGodsentBaseCharacter
{
	GENERATED_BODY()

public:
	AGodsentEnemyCharacter();

	//~ Begin IPawnCombatInterface Interface
	virtual UPawnCombatComponent* GetPawnCombatComponent() const override;
	//~ End IPawnCombatInterface Interface

	//~ Begin IPawnUIInterface Interface
	virtual UPawnUIComponent* GetPawnUIComponent() const override;
	virtual UEnemyUIComponent* GetEnemyUIComponent() const override;
	//~ End IPawnUIInterface Interface

	FORCEINLINE UEnemyCombatComponent* GetEnemyCombatComponent() const { return EnemyCombatComponent; }
	FORCEINLINE UBoxComponent* GetLeftHandCollisionBox() const { return LeftHandCollisionBox; }
	FORCEINLINE UBoxComponent* GetRightHandCollisionBox() const { return RightHandCollisionBox; }

	UFUNCTION(BlueprintPure, Category = "Godsent|AI")
	float GetPatrolRadius() const { return PatrolRadius; }

protected:
	/// Hooks the health bar widget up to this enemy's UI component
	virtual void BeginPlay() override;

	//~ Begin APawn Interface.
	virtual void PossessedBy(AController* NewController) override;
	//~ End APawn Interface.

#if WITH_EDITOR
	//~ Begin UObject Interface.
	/// Re-attaches the hand boxes when their bone names are changed in the editor
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface.
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UEnemyCombatComponent> EnemyCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UBoxComponent> LeftHandCollisionBox;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FName LeftHandCollisionBoxBoneName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UBoxComponent> RightHandCollisionBox;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FName RightHandCollisionBoxBoneName;

	/// How far from its home (where it was placed) this enemy wanders while patrolling.
	/// Set a default in the Blueprint and override it per placed enemy in the level.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0.0", Units = "cm"))
	float PatrolRadius = 800.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UEnemyUIComponent> EnemyUIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> EnemyHealthWidgetComponent;

	/// Reports hand hits on hostile pawns to the enemy combat component
	UFUNCTION()
	virtual void OnBodyCollisionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	/// Loads the startup data asynchronously and gives it to the ASC
	void InitEnemyStartupData();
};
