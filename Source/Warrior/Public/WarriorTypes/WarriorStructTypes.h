// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "WarriorStructTypes.generated.h"

class UWarriorHeroLinkedAnimLayer;
class UWarriorHeroGameplayAbility;
class UInputMappingContext;

// 英雄技能 对应的技能名称和技能结构体
USTRUCT(BlueprintType)
struct FWarriorHeroAbilitySet {
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UWarriorHeroGameplayAbility> AbilityToGrant;

	bool IsValid() const;

};
// 特殊武器结构
USTRUCT(BlueprintType)
struct FWarrorHeroSpecialAbilitySet : public FWarriorHeroAbilitySet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UMaterialInterface> SoftAbilityIconMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Player.Cooldown"))
	FGameplayTag AbilityCooldownTag;
};


// 武器数据
USTRUCT(BLueprintType)
struct FWarriorHeroWeaponData {
	GENERATED_BODY()

	// 动画层
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UWarriorHeroLinkedAnimLayer> WeaponAnimLayerToLink;
	// 技能 （卸下武器，轻击，重击）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputTag"))
	TArray< FWarriorHeroAbilitySet> DefaultWeaponAbilities;
	// 特殊武器技能
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputTag"))
	TArray< FWarrorHeroSpecialAbilitySet> SpecialWeaponAbilities;

	// 新的上下文映射
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> WeaponInputMappingContext;

	// 武器的基础伤害
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	//ScalableFloat 是游戏项目自定义的一个浮点数类，专门用于处理根据等级或上下文进行缩放的数值
	FScalableFloat WeaponBaseDamage;

	// 武器图标
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> SoftWeaponIconTexture;

};

