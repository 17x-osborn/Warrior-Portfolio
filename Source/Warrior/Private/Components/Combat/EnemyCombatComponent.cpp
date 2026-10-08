// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/EnemyCombatComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "WarriorGameplayTags.h"
#include "WFunctionLibrary.h"
#include "Characters/WarriorEnemyCharacter.h"
#include "Components/BoxComponent.h"

void UEnemyCombatComponent::OnHitTargetActor(AActor* HitActor)
{
	if (OverlappedActors.Contains(HitActor)) {
		return; // 如果已经击中过这个目标，直接返回，不再处理 避免在一次攻击中对同一个敌人造成多次伤害
	}
	// 把攻击的对象加到overlap数组中
	OverlappedActors.AddUnique(HitActor);

    // 实现格挡检测
    bool bisValidBlock = false;                 // 是有效的格挡 面对面
    const bool bisPlayerBlocking = UWFunctionLibrary::NativeDoesActorHaveTag(HitActor, WarriorGameplayTags::Player_Status_Blocking);   // 检测玩家是否正在格挡 这里应该从玩家状态获取实际值
    const bool bisMyAttackUnblockable = UWFunctionLibrary::NativeDoesActorHaveTag(GetOwningPawn(), WarriorGameplayTags::Enemy_Status_UnBlockable);  // 检测攻击是否不可被格挡

    // 如果玩家正在格挡且我的攻击可以被格挡
    if (bisPlayerBlocking && !bisMyAttackUnblockable)
    {
        bisValidBlock = UWFunctionLibrary::IsValidBlock(GetOwningPawn(), HitActor);
    }

    // 创建一个事件数据包，包含"谁攻击了谁"的信息
    FGameplayEventData Data;
    Data.Instigator = GetOwningPawn(); // 设置事件发起者（攻击者）
    Data.Target = HitActor; // 设置事件目标（被击中的actor）

    // 如果格挡有效
    if (bisValidBlock)
    {
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
            HitActor,
            WarriorGameplayTags::Player_Event_SuccessfulBlock,
            Data
        );
    }
    else
    {
        // 发送游戏事件给攻击的ability  1.发送检测到攻击了  2.发送hit pause
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
            GetOwningPawn(),
            WarriorGameplayTags::Shared_Event_MeleeHit,
            Data
        );
    }

}

void UEnemyCombatComponent::ToggleBodyCollsionBoxCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType)
{
    AWarriorEnemyCharacter* OwningEnemyCharacter = GetOwningPawn<AWarriorEnemyCharacter>();
    check(OwningEnemyCharacter);

    // 获取敌方角色的左右手碰撞盒组件
    UBoxComponent* LeftHandCollisionBox = OwningEnemyCharacter->GetLeftHandCollisionBox();
    UBoxComponent* RightHandCollisionBox = OwningEnemyCharacter->GetRightHandCollisionBox();

    // 确保两个碰撞盒都已成功获取（安全检查）
    check(LeftHandCollisionBox && RightHandCollisionBox);

    // 根据切换伤害类型开关执行相应操作
    switch (ToggleDamageType)
    {
    case EToggleDamageType::LeftHand:  // 左手伤害类型
            // 根据bShouldEnable决定启用或禁用左手碰撞盒的碰撞
            LeftHandCollisionBox->SetCollisionEnabled(bShouldEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
            break;
        
    case EToggleDamageType::RightHand: // 右手伤害类型
            // 根据bShouldEnable决定启用或禁用右手碰撞盒的碰撞
            RightHandCollisionBox->SetCollisionEnabled(bShouldEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
            break;
     default:  
            break;
    }

    // 如果碰撞被禁用，清空已重叠的演员列表
    if (!bShouldEnable)
    {
        OverlappedActors.Empty();
    }

}
