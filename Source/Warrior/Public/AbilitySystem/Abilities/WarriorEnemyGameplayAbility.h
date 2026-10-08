// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/WarriorGameplayAbility.h"
#include "WarriorEnemyGameplayAbility.generated.h"

class AWarriorEnemyCharacter;
class UEnemyCombatComponent;
/**
 *  敌人技能 在蓝图实现 下面为一些辅助函数
 */
UCLASS()
class WARRIOR_API UWarriorEnemyGameplayAbility : public UWarriorGameplayAbility
{
	GENERATED_BODY()

public:
    // 从ActorInfo获取敌人角色
    UFUNCTION(BlueprintPure, Category = "Warrior|Ability")
    AWarriorEnemyCharacter* GetEnemyCharacterFromActorInfo();

    // 从ActorInfo获取敌人战斗组件
    UFUNCTION(BlueprintPure, Category = "Warrior|Ability")
    UEnemyCombatComponent* GetEnemyCombatComponentFromActorInfo();

    // 攻击后的回调函数 传此次攻击的相关信息 UGameplayEffect就是写具体数值的那个 蓝图中 不同等级数值不同
    UFUNCTION(BlueprintPure, Category = "Warrior|Ability")
    FGameplayEffectSpecHandle MakeEnemyDamageEffectSpecHandle(
        TSubclassOf<UGameplayEffect> Effectclass,
        const FScalableFloat& InDamageScalableFloat);

private:
    // 缓存的敌人角色引用
    TWeakObjectPtr<AWarriorEnemyCharacter> CachedWarriorEnemyCharacter;
	
};
