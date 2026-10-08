// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTasks/AbilityTask_ExecuteTaskOnTick.h"

UAbilityTask_ExecuteTaskOnTick::UAbilityTask_ExecuteTaskOnTick()
{
	// 这个标志告诉UE的GameplayTask 需要每帧调用TickTask函数
	bTickingTask = true;
}

UAbilityTask_ExecuteTaskOnTick* UAbilityTask_ExecuteTaskOnTick::ExecuteTaskOnTick(UGameplayAbility* OwningAbility)
{
    // NewAbilityTask是UE提供的模板函数，专门用于创建AbilityTask
    UAbilityTask_ExecuteTaskOnTick* Node = NewAbilityTask<UAbilityTask_ExecuteTaskOnTick>(OwningAbility);

    return Node;
}

void UAbilityTask_ExecuteTaskOnTick::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    // ShouldBroadcastAbilityTaskDelegates() 检查：
    // - Ability是否仍然有效
    // - Ability是否没有被取消
    // - 其他必要的有效性检查
    if (ShouldBroadcastAbilityTaskDelegates())
    {
        // 调用委托
        OnAbilityTaskTick.Broadcast(DeltaTime);
    }
    else
    {
        EndTask();
    }
}
