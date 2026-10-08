// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/StartUpData/DataAsset_StartUpDataBase.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/WarriorGameplayAbility.h"

void UDataAsset_StartUpDataBase::GiveToAbilitySystemComponent(UWarriorAbilitySystemComponent* InASCTogive, int32 ApplyLevel)
{
	check(InASCTogive);
	GrantAbilities(ActivateOnGivenAbilities, InASCTogive, ApplyLevel);
	GrantAbilities(ReactiveAbilities, InASCTogive, ApplyLevel);

	// 应用启动时的Gameplay Effects
	if (!StartUpGameplayEffects.IsEmpty()) {
		for (const auto& EffectClass : StartUpGameplayEffects) {
			if (!EffectClass) continue;
			// 获取Gameplay Effect类的默认对象(CDO - Class Default Object)
			// CDO是类的默认实例，包含类的初始属性值
			UGameplayEffect* EffectCDO = EffectClass->GetDefaultObject<UGameplayEffect>();
			// 将Gameplay Effect应用到自身
		    // MakeEffectContext(): 创建效果上下文，包含施法者、目标等信息
			InASCTogive->ApplyGameplayEffectToSelf(
				EffectCDO,
				ApplyLevel,
				InASCTogive->MakeEffectContext());
		}
	}
}

void UDataAsset_StartUpDataBase::GrantAbilities(const TArray<TSubclassOf<UWarriorGameplayAbility>>& InAbilitiesToGive, UWarriorAbilitySystemComponent* InASCTogive, int32 ApplyLevel)
{
	if (InAbilitiesToGive.IsEmpty()) return;
	for (const TSubclassOf<UWarriorGameplayAbility>& Ability : InAbilitiesToGive) {
		if (!Ability) continue;
		// 基于技能的类 创建一个技能规格（Spec） Spec是技能在游戏中的具体实例
		FGameplayAbilitySpec AbilitySpec(Ability);
		// 设置这个技能的来源对象。通常是技能拥有者（玩家角色）
		AbilitySpec.SourceObject = InASCTogive->GetAvatarActor();
		// 设置这个技能的初始等级
		AbilitySpec.Level = ApplyLevel;
		//调用能力系统组件的 `GiveAbility` 函数。 这个函数会将配置好的 `AbilitySpec` “注入”到角色的能力系统中。
		InASCTogive->GiveAbility(AbilitySpec);
	}

}
