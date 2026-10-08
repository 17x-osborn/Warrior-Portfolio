// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/WarriorAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "WFunctionLibrary.h"
#include "WarriorGameplayTags.h"
#include "Interfaces/PawnUIInterface.h"
#include "Components/UI/PawnUIComponent.h"
#include "Components/UI/HeroUIComponent.h"

#include "WarriorDebugHelper.h"



UWarriorAttributeSet::UWarriorAttributeSet()
{
    InitCurrentHealth(1.f);
    InitMaxHealth(1.f);
    InitCurrentRage(1.f);
    InitMaxRage(1.f);
    InitAttackPower(1.f);
    InitDefensePower(1.f);
}

// 属性变化时的自动回调
void UWarriorAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    // 通知UI 
    if (!CashedPawnUIInterface.IsValid())
    {
        // 检查这个Actor是否实现了IPawnUIInterface接口  如果实现了，返回接口指针；如果没有，返回nullptr
        CashedPawnUIInterface = TWeakInterfacePtr<IPawnUIInterface>(Data.Target.GetAvatarActor());
        //CashedPawnUIInterface = Cast<IPawnUIInterface>(Data.Target.GetAvatarActor()); 和上一句一样的功能
    }
    checkf(CashedPawnUIInterface.IsValid(), TEXT("didn't implement IPawnUIInterface"));

    // 返回ui组件 函数已重写 各自返回各自的
    UPawnUIComponent* PawnUIComponent = CashedPawnUIInterface->GetPawnUIComponent();
    check(PawnUIComponent);

    // 下面两个是为了保护生命值和怒气值在合法的范围内
    // Data.EvaluatedData.Attribute：当前正在被修改的那个属性
    if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
    {
        const float NewCurrentHealth = FMath::Clamp(GetCurrentHealth(), 0.f, GetMaxHealth());
        SetCurrentHealth(NewCurrentHealth);


        // 广播
        PawnUIComponent->OnCurrentHealthChanged.Broadcast(GetCurrentHealth() / GetMaxHealth());

    }
    if (Data.EvaluatedData.Attribute == GetCurrentRageAttribute())
    {
        const float NewCurrentRage = FMath::Clamp(GetCurrentRage(), 0.f, GetMaxRage());
        SetCurrentRage(NewCurrentRage);

        if (GetCurrentRage() == GetMaxRage())
        {
            UWFunctionLibrary::AddGameplayTagToActorIfNone(Data.Target.GetAvatarActor(), WarriorGameplayTags::Player_Status_Rage_Full);
        }
        else if (GetCurrentRage() == 0.f)
        {
            UWFunctionLibrary::AddGameplayTagToActorIfNone(Data.Target.GetAvatarActor(), WarriorGameplayTags::Player_Status_Rage_None);
        }
        else
        {
            UWFunctionLibrary::RemoveGameplayTagFromActorIfFound(Data.Target.GetAvatarActor(), WarriorGameplayTags::Player_Status_Rage_Full);
            UWFunctionLibrary::RemoveGameplayTagFromActorIfFound(Data.Target.GetAvatarActor(), WarriorGameplayTags::Player_Status_Rage_None);

        }

        // 广播
        if (UHeroUIComponent* HeroUIComponent = CashedPawnUIInterface->GetHeroUIComponent()) {
            HeroUIComponent->OnCurrentRageChanged.Broadcast(GetCurrentRage()/ GetMaxRage());
        }
    }

    // 当受到攻击时候 扣血
    if (Data.EvaluatedData.Attribute == GetDamageTakenAttribute())
    {
        const float OldHealth = GetCurrentHealth();
        const float DamageDone = GetDamageTaken();

        // 计算新生命值：旧生命值减去伤害，并限制在0到最大生命值之间
        const float NewCurrentHealth = FMath::Clamp(OldHealth - DamageDone, 0.f, GetMaxHealth());

        // 更新当前生命值
        SetCurrentHealth(NewCurrentHealth);

        // 广播生命值的改变 使用在组件中声明的委托来广播
        PawnUIComponent->OnCurrentHealthChanged.Broadcast(GetCurrentHealth() / GetMaxHealth());

        if (NewCurrentHealth == 0.f)
        {
            //  获取目标角色 加上死亡标签 gas激活死亡技能
            UWFunctionLibrary::AddGameplayTagToActorIfNone(Data.Target.GetAvatarActor(), WarriorGameplayTags::Shared_Status_Dead);
            
        }
    }
}
