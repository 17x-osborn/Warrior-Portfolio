// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_ExecuteTaskOnTick.generated.h"

// 声明委托类型
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityTaskTickDelegate, float, DeltaTime);
/**
 * 
 */
UCLASS()
class WARRIOR_API UAbilityTask_ExecuteTaskOnTick : public UAbilityTask
{
	GENERATED_BODY()

public:
    UAbilityTask_ExecuteTaskOnTick();

    /**
     * 静态工厂方法：创建一个会在每帧Tick的AbilityTask
     *
     * @param OwningAbility 拥有这个Task的GameplayAbility
     * @return 返回新创建的AbilityTask实例
     *
     * HidePin: 在蓝图节点中隐藏OwningAbility参数，简化界面
     * DefaultToSelf: 自动将调用此函数的当前Ability作为OwningAbility参数
     * BlueprintInternalUseOnly: 主要供Ability系统内部使用
     */
    UFUNCTION(BlueprintCallable, Category = "Warrior|AbilityTasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
    static UAbilityTask_ExecuteTaskOnTick* ExecuteTaskOnTick(UGameplayAbility* OwningAbility);

    // 重写UGameplayTask的Tick函数
    virtual void TickTask(float DeltaTime) override;

    // 委托实例
    UPROPERTY(BlueprintAssignable)
    FOnAbilityTaskTickDelegate OnAbilityTaskTick;
	
};
