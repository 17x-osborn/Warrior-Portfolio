// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/StartUpData/DataAsset_EnemyStartUpData.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/WarriorEnemyGameplayAbility.h"

void UDataAsset_EnemyStartUpData::GiveToAbilitySystemComponent(UWarriorAbilitySystemComponent* InASCTogive, int32 ApplyLevel)
{
    Super::GiveToAbilitySystemComponent(InASCTogive, ApplyLevel);

    // 检查敌人战斗能力数组是否为空
    if (!EnemyCombatAbilities.IsEmpty())
    {
        // 遍历所有敌人战斗能力类
        for (const auto& AbilityClass : EnemyCombatAbilities)
        {
            // 跳过无效的能力类
            if (!AbilityClass) continue;

            // 创建能力规格
            FGameplayAbilitySpec AbilitySpec(AbilityClass);
            AbilitySpec.SourceObject = InASCTogive->GetAvatarActor();
            AbilitySpec.Level = ApplyLevel;

            // 赋予能力给能力系统组件
            InASCTogive->GiveAbility(AbilitySpec);
        }
    }
}
