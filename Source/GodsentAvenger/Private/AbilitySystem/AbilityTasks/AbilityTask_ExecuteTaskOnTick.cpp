// Alex Goldberg rights reserved

#include "AbilitySystem/AbilityTasks/AbilityTask_ExecuteTaskOnTick.h"

UAbilityTask_ExecuteTaskOnTick::UAbilityTask_ExecuteTaskOnTick()
{
	bTickingTask = true;
}

/// Ticks every frame until the ability ends
UAbilityTask_ExecuteTaskOnTick* UAbilityTask_ExecuteTaskOnTick::ExecuteTaskOnTick(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UAbilityTask_ExecuteTaskOnTick>(OwningAbility);
}

/// Broadcasts the tick, or ends the task once the ability can no longer receive it
void UAbilityTask_ExecuteTaskOnTick::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (ShouldBroadcastAbilityTaskDelegates())
		OnAbilityTaskTick.Broadcast(DeltaTime);
	else
		EndTask();
}
