// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/HeroGameplayAbility_PickUpStones.h"
#include "Kismet/KismetSystemLibrary.h" 
#include "Characters/WarriorHeroCharacter.h"
#include "Items/PickUps/WarriorStoneBase.h"
#include "Components/UI/HeroUIComponent.h"

void UHeroGameplayAbility_PickUpStones::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    // 显示 pickup 的键位提示 
    GetHeroUIComponentFromActorInfo()->OnStoneInteracted.Broadcast(true);  // 调用 传入参数
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
}

void UHeroGameplayAbility_PickUpStones::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    GetHeroUIComponentFromActorInfo()->OnStoneInteracted.Broadcast(false);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// 检测地上的治疗石头 每帧调用
void UHeroGameplayAbility_PickUpStones::CollectStones()
{
    CollectedStones.Empty();
    // BoxTrace = 把一个“盒子”从 Start 的位置，平移到 End 的位置 路上碰到了什么物体
    // 用来存放 BoxTrace 检测到的所有命中结果
	TArray<FHitResult> TraceHits;
    UKismetSystemLibrary::BoxTraceMultiForObjects(
        GetHeroCharacterFromActorInfo(),  //world
        GetHeroCharacterFromActorInfo()->GetActorLocation(),  //start
        GetHeroCharacterFromActorInfo()->GetActorLocation()+ (-GetHeroCharacterFromActorInfo()->GetActorUpVector()) * BoxTraceDistance, //end
        TraceBoxSize / 2.f,
        (-GetHeroCharacterFromActorInfo()->GetActorUpVector()).ToOrientationRotator(), // 盒子自身旋转 这里让盒子的“前方向”指向竖直向下
        StoneTraceChannel,  // 检测类型
        false,  // bTraceComplex：是否使用复杂碰撞（false = 简单碰撞）
        TArray<AActor*>(), //  ActorsToIgnore：忽略的 Actor，这里为空
        bDrawPersistenDebugShape? EDrawDebugTrace::ForOneFrame: EDrawDebugTrace::None,
        TraceHits,  //  OutHits：所有被盒子检测到的命中结果
        true  // bIgnoreSelf：忽略自己（避免检测到角色本身）
    );

    for (const FHitResult& TraceHit : TraceHits)
    {
        if (AWarriorStoneBase* FoundStone = Cast<AWarriorStoneBase>(TraceHit.GetActor()))
        {
            CollectedStones.AddUnique(FoundStone);
        }
    }

    if (CollectedStones.IsEmpty())
    {
        // 如果周围没有石头 取消这个ab
        CancelAbility(GetCurrentAbilitySpecHandle(),GetCurrentActorInfo(),GetCurrentActivationInfo(),true);
    }
}

void UHeroGameplayAbility_PickUpStones::ConsumeStones()
{
    if (CollectedStones.IsEmpty())
    {
        CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
        return;
    }
    for (AWarriorStoneBase* CollectedStone : CollectedStones) {
        CollectedStone->Comsume(GetWarriorAbilitySystemComponentFromActorInfo(), GetAbilityLevel());
    }
}
