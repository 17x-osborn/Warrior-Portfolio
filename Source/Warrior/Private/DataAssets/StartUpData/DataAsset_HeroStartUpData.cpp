// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/StartUpData/DataAsset_HeroStartUpData.h"
#include "AbilitySystem/Abilities/WarriorHeroGameplayAbility.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"


void UDataAsset_HeroStartUpData::GiveToAbilitySystemComponent(UWarriorAbilitySystemComponent* InASCTogive, int32 ApplyLevel)
{
    Super::GiveToAbilitySystemComponent(InASCTogive, ApplyLevel);
    // 把技能注册到能力系统中 统一管理调用
    for (const FWarriorHeroAbilitySet& AbilitySet : HeroStartUpAbilitySets) {
        if (!AbilitySet.IsValid()) continue;
        // 创建技能实例
        FGameplayAbilitySpec AbilitySpec(AbilitySet.AbilityToGrant);
        // 设置这个技能的来源对象。通常是技能拥有者（玩家角色）
        AbilitySpec.SourceObject = InASCTogive->GetAvatarActor();
        // 设置这个技能的初始等级
        AbilitySpec.Level = ApplyLevel;
        // 设置技能的Tag
        AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilitySet.InputTag);

        InASCTogive->GiveAbility(AbilitySpec);

    }
}
