// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/WarriorHeroGameplayAbility.h"
#include "Characters/WarriorHeroCharacter.h"
#include "Controllers/WarriorHeroController.h"
#include "WarriorGameplayTags.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"


AWarriorHeroCharacter* UWarriorHeroGameplayAbility::GetHeroCharacterFromActorInfo()
{
    if (!CashedWarriorHeroCharacter.IsValid()) {
        //CurrentActorInfo结构体,包含了当前执行 GameplayAbility 的所有相关角色信息
        CashedWarriorHeroCharacter=Cast<AWarriorHeroCharacter>(CurrentActorInfo->AvatarActor);
    }
    // .Get() 用于从弱指针中安全地获取真实的对象指针
    return CashedWarriorHeroCharacter.IsValid() ? CashedWarriorHeroCharacter.Get() : nullptr;
}

AWarriorHeroController* UWarriorHeroGameplayAbility::GetHeroControllerFromActorInfo()
{
    if (!CashedWarriorHeroController.IsValid()) {
        CashedWarriorHeroController = Cast<AWarriorHeroController>(CurrentActorInfo->PlayerController);
    }
    // .Get() 用于从弱指针中安全地获取真实的对象指针
    return CashedWarriorHeroController.IsValid() ? CashedWarriorHeroController.Get() : nullptr;
}

UHeroCombatComponent* UWarriorHeroGameplayAbility::GetHeroCombatComponentFromActorInfo()
{
    return GetHeroCharacterFromActorInfo()->GetHeroCombatComponent();
}

UHeroUIComponent* UWarriorHeroGameplayAbility::GetHeroUIComponentFromActorInfo()
{
    return GetHeroCharacterFromActorInfo()->GetHeroUIComponentPublic();
}



//FGameplayEffectSpecHandle UWarriorHeroGameplayAbility::MakeHeroDamageEffectSpecHandle(TSubclassOf<UGameplayEffect> Effectclass, float InWeaponBaseDamage, FGameplayTag InCurrentAttackTypeTag, int32 InCurrentComboCount)
//{
//    check(Effectclass);
//    FGameplayEffectContextHandle ContextHandle =GetWarriorAbilitySystemComponentFromActorInfo()->MakeEffectContext();
//    ContextHandle.SetAbility(this);
//    ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());
//    ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());
//
//    FGameplayEffectSpecHandle EffectSpecHandle=GetWarriorAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
//        Effectclass,
//        GetAbilityLevel(),
//        ContextHandle);
//    EffectSpecHandle.Data->SetSetByCallerMagnitude(WarriorGameplayTags::Shared_SetByCaller_BaseDamage, InWeaponBaseDamage);
//    if (InCurrentAttackTypeTag.IsValid()) {
//        EffectSpecHandle.Data->SetSetByCallerMagnitude(InCurrentAttackTypeTag, InCurrentComboCount);
//    }
//    return EffectSpecHandle;
//}

// 函数作用：创建一个用于造成英雄攻击伤害的“游戏效果规格句柄”。 
// 参数：
//   - EffectClass: 你要创建的伤害效果的蓝图（UGameplayEffect类）。
//   - InWeaponBaseDamage: 武器的基础伤害值。
//   - InCurrentAttackTypeTag: 当前攻击类型的标签（比如：普通攻击、重击、火焰攻击）。
//   - InCurrentComboCount: 当前的连击数（比如：第1刀、第2刀、第3刀）。
// 返回值：一个创建好的“效果规格句柄”（FGameplayEffectSpecHandle），你可以把它想象成一个已经装好弹药的子弹。
FGameplayEffectSpecHandle UWarriorHeroGameplayAbility::MakeHeroDamageEffectSpecHandle(
    TSubclassOf<UGameplayEffect> Effectclass,
    float InWeaponBaseDamage,
    FGameplayTag InCurrentAttackTypeTag,
    int32 InUsedComboCount)
{
    // 1. 【安全检查】确保传入的“效果蓝图”是有效的，不能为空。
    check(Effectclass);

    // 2. 【创建“上下文”】“上下文”就像这个“子弹”的“快递单”，记录了关于这次伤害的“元信息”。
    //    ASC中的功能来创建
    FGameplayEffectContextHandle ContextHandle = GetWarriorAbilitySystemComponentFromActorInfo()->MakeEffectContext();

    // 3. 【填写“快递单”信息】
    //    a. 记录是哪个“技能”（Ability）产生了这个效果。
    ContextHandle.SetAbility(this);
    //    b. 记录“来源对象”。通常是释放技能的英雄（AvatarActor）。
    ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());
    //    c. 记录“发起者”。通常也是释放技能的英雄。在多人游戏中，这用于计算责任（谁杀了谁）。 两个参数一个是发起者 一个是实际物体武器
    ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

    // 4. 【根据蓝图制造“子弹”】
    //    我们告诉“能力系统组件”：“请根据这张蓝图（Effectclass），结合当前的技能等级和填好的快递单，给我造一个具体的‘子弹’出来。”
    FGameplayEffectSpecHandle EffectSpecHandle = GetWarriorAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
        Effectclass,        // 伤害效果的蓝图
        GetAbilityLevel(),  // 当前技能的等级（等级可能会影响伤害倍率等）
        ContextHandle       // 上面填好的“快递单”
    );

    // 5. 【为“子弹”装填“基础伤害”弹药】
    //    SetByCaller 是一种动态传递数据的方式。
    //    这里我们把“武器基础伤害”这个数值，以“标签”作为名字，存储到“子弹”里。
    //    之后，在GameplayEffect里，会根据这个名字来找到这个伤害值。
    EffectSpecHandle.Data->SetSetByCallerMagnitude(WarriorGameplayTags::Shared_SetByCaller_BaseDamage, InWeaponBaseDamage);

    // 6. 【为“子弹”装填“连击数”弹药】
    //    我们再把“连击数”这个数值，以 'InCurrentAttackTypeTag' 这个“标签”作为名字，也存储到“子弹”里。
    //    例如：Attack_Light 标签对应数值 2，表示这是连击的第二段。
    if (InCurrentAttackTypeTag.IsValid()) {
        EffectSpecHandle.Data->SetSetByCallerMagnitude(InCurrentAttackTypeTag, InUsedComboCount);
    }

    // 7. 【交付“子弹”】把这个已经装填好所有信息的“子弹”返回给调用者。
    return EffectSpecHandle;
}

bool UWarriorHeroGameplayAbility::GetAbilityRemainingCooldownByTag(FGameplayTag InCooldownTag, float& TotalCooldownTime, float& RemainingCooldownTime)
{
    check(InCooldownTag.IsValid());

    // 创建“游戏性效果查询条件”：筛选“拥有该冷却标签”的活跃GameplayEffect
    FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(InCooldownTag.GetSingleTagContainer());

    // 从技能对应的“技能系统组件（ASC）”中，查询符合条件的活跃效果的「剩余时间-总时长」对（Key=剩余时间，Value=总时长）
    TArray< TPair <float, float> > TimeRemainingAndDuration = GetAbilitySystemComponentFromActorInfo()->GetActiveEffectsTimeRemainingAndDuration(CooldownQuery);

    // 如果查询到了对应的活跃冷却效果
    if (!TimeRemainingAndDuration.IsEmpty())
    {
        // 取第一个匹配效果的“剩余时间”，赋值给函数内的「剩余冷却时间」变量
        RemainingCooldownTime = TimeRemainingAndDuration[0].Key;
        // 取第一个匹配效果的“总时长”，赋值给输出参数TotalCooldownTime（即该技能的总冷却时间）
        TotalCooldownTime = TimeRemainingAndDuration[0].Value;
    }

    // 返回：当前是否处于冷却中（剩余冷却时间>0则为true）
    return RemainingCooldownTime > 0.f;
}
