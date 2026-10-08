// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DataAsset_StartUpDataBase.generated.h"

class UWarriorGameplayAbility;
class UWarriorAbilitySystemComponent;
class UGameplayEffect;
/**
 *  英雄和敌人的父类 一些公用的操作和公用技能的放入
 */
UCLASS()
class WARRIOR_API UDataAsset_StartUpDataBase : public UDataAsset
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, Category = "StartUpData")
	// 被动技能 若一个能力赋予同时就激活，放入这个数组 
	TArray<TSubclassOf< UWarriorGameplayAbility>> ActivateOnGivenAbilities;

	UPROPERTY(EditAnywhere, Category = "StartUpData")
	// 主动技能 放入这个数组
	TArray<TSubclassOf< UWarriorGameplayAbility>> ReactiveAbilities;

	UPROPERTY(EditAnywhere, Category = "StartUpData")
	// 初始的属性效果
	TArray <TSubclassOf<UGameplayEffect>> StartUpGameplayEffects;

	// 负责将一组技能类具体地赋予到能力系统组件中
	void GrantAbilities(const TArray<TSubclassOf< UWarriorGameplayAbility>>& InAbilitiesToGive, UWarriorAbilitySystemComponent* InASCTogive, int32 ApplyLevel = 1);

public:
	// 将本数据资产（DataAsset）中定义的技能赋予给一个目标能力系统组件
	virtual void GiveToAbilitySystemComponent(UWarriorAbilitySystemComponent* InASCTogive, int32 ApplyLevel = 1);
};
