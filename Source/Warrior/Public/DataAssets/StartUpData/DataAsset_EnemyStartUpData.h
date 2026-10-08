// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/StartUpData/DataAsset_StartUpDataBase.h"
#include "DataAsset_EnemyStartUpData.generated.h"

class UWarriorEnemyGameplayAbility;
/**
 * 
 */
UCLASS()
class WARRIOR_API UDataAsset_EnemyStartUpData : public UDataAsset_StartUpDataBase
{
	GENERATED_BODY()

public:
	// 将技能/技能tag注册到目标的能力系统组件中
	virtual void GiveToAbilitySystemComponent(UWarriorAbilitySystemComponent* InASCTogive, int32 ApplyLevel = 1) override;

private:
    /** 敌人启动时赋予的能力集合 */
    UPROPERTY(EditDefaultsOnly, Category = "StartUpData")
    TArray<TSubclassOf< UWarriorEnemyGameplayAbility>> EnemyCombatAbilities;
	
};
