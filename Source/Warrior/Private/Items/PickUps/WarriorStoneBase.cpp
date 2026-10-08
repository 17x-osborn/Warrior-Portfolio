// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/PickUps/WarriorStoneBase.h"
#include "Characters/WarriorHeroCharacter.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "WarriorGameplayTags.h"
void AWarriorStoneBase::Comsume(UWarriorAbilitySystemComponent* AbilitySystemComponent, int32 ApplyLevel)
{
    check(StoneGameplayEffectClass);

    // 获取游戏效果类的默认对象
    UGameplayEffect* EffectCDO = StoneGameplayEffectClass->GetDefaultObject<UGameplayEffect>();

    // 将游戏效果应用到自身（拥有该能力系统组件的实体）
    AbilitySystemComponent->ApplyGameplayEffectToSelf(
        EffectCDO,                        // 要应用的游戏效果
        ApplyLevel,                       // 效果的应用等级
        AbilitySystemComponent->MakeEffectContext()  // 创建效果上下文
    );

    BP_OnStoneConsumed();
}
void AWarriorStoneBase::OnPickUpCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (AWarriorHeroCharacter* OverlappedHeroCharacter = Cast<AWarriorHeroCharacter>(OtherActor))
    {
        OverlappedHeroCharacter->GetWarriorAbilitySystemComponent()->TryActivateAbilityByTag(
            WarriorGameplayTags::Player_Ability_PickUp_Stones);
     
    }
}
