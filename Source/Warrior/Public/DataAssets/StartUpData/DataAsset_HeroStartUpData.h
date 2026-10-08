// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/StartUpData/DataAsset_StartUpDataBase.h"
#include "WarriorTypes/WarriorStructTypes.h"
#include "DataAsset_HeroStartUpData.generated.h"


/**
 *  蓝图 DA_Hero
 */
UCLASS()
class WARRIOR_API UDataAsset_HeroStartUpData : public UDataAsset_StartUpDataBase
{
	GENERATED_BODY()

public:
	// 将技能/技能tag注册到目标（hero）的能力系统组件中
	virtual void GiveToAbilitySystemComponent(UWarriorAbilitySystemComponent* InASCTogive, int32 ApplyLevel = 1) override;

private:
	UPROPERTY(EditAnywhere, Category = "StartUpData", meta = (TitleProperty="InputTag"))
	TArray<FWarriorHeroAbilitySet> HeroStartUpAbilitySets;
};
