// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GEExecCalc/GEExecCalc_DamageTaken.h"
#include "AbilitySystem/WarriorAttributeSet.h"
#include "WarriorGameplayTags.h"
#include "WarriorDebugHelper.h"

// 结构体用于获取伤害计算所需的角色属性 宏
struct FWarriorDamageCapture
{
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower)  
    DECLARE_ATTRIBUTE_CAPTUREDEF(DefensePower) 
    DECLARE_ATTRIBUTE_CAPTUREDEF(DamageTaken)

    FWarriorDamageCapture()
    {
        // 捕获attribute属性 第三个参数是从哪个asc读 只是说明 没有读取
        DEFINE_ATTRIBUTE_CAPTUREDEF(UWarriorAttributeSet, AttackPower, Source, false)
        DEFINE_ATTRIBUTE_CAPTUREDEF(UWarriorAttributeSet, DefensePower, Target, false)
        DEFINE_ATTRIBUTE_CAPTUREDEF(UWarriorAttributeSet, DamageTaken, Target, false)

    }
};
static const FWarriorDamageCapture& GetWarriorDamageCapture()
{
        static FWarriorDamageCapture WarriorDamageCapture; // 静态变量，只初始化一次
        return WarriorDamageCapture;
}
UGEExecCalc_DamageTaken::UGEExecCalc_DamageTaken()
{
    RelevantAttributesToCapture.Add(GetWarriorDamageCapture().AttackPowerDef);
    RelevantAttributesToCapture.Add(GetWarriorDamageCapture().DefensePowerDef);
    RelevantAttributesToCapture.Add(GetWarriorDamageCapture().DamageTakenDef);

}



// 计算最终伤害量
void UGEExecCalc_DamageTaken::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    // 拿到攻击 GA 创建的本次伤害 Spec
    const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();

    FAggregatorEvaluateParameters EvaluateParameters;

    EvaluateParameters.SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();
    EvaluateParameters.TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags();

    float SourceAttackPower = 0.f;
    float TargetDefensePower = 0.f;

    // 捕获属性
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetWarriorDamageCapture().AttackPowerDef, EvaluateParameters, SourceAttackPower);
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetWarriorDamageCapture().DefensePowerDef, EvaluateParameters, TargetDefensePower);


    // 声明其他伤害计算需要的变量
    float BaseDamage = 0.f;           
    int32 UsedLightAttackComboCount = 0; 
    int32 UsedHeavyAttackComboCount = 0; 

    // 遍历所有通过SetByCaller传递的数值
    // SetByCaller是一种动态传递数据的方式，在使用技能时传递动态数值
    for (const TPair<FGameplayTag, float>& TagMagnitude : EffectSpec.SetByCallerTagMagnitudes)
    {
        // 检查是否是基础伤害标签
        if (TagMagnitude.Key.MatchesTagExact(WarriorGameplayTags::Shared_SetByCaller_BaseDamage))
        {
            BaseDamage = TagMagnitude.Value; // 获取基础伤害值

        }
        // 检查是否是轻重攻击类型标签  
        if (TagMagnitude.Key.MatchesTagExact(WarriorGameplayTags::Player_SetByCaller_AttackType_Light))
        {
            UsedLightAttackComboCount = TagMagnitude.Value; // 获取连击数

        }
        if (TagMagnitude.Key.MatchesTagExact(WarriorGameplayTags::Player_SetByCaller_AttackType_Heavy))
        {
            UsedHeavyAttackComboCount = TagMagnitude.Value; 

        }
    }

    // 最终伤害计算
    if (UsedLightAttackComboCount != 0)
    {
        const float DamageIncreasePercentLight = (UsedLightAttackComboCount - 1) * 0.05 + 1.f;
        BaseDamage *= DamageIncreasePercentLight;

        if (EvaluateParameters.SourceTags &&
            EvaluateParameters.SourceTags->HasTagExact(WarriorGameplayTags::Player_Upgrade_ExploitArmorBreak) &&
            EvaluateParameters.TargetTags &&
            EvaluateParameters.TargetTags->HasTagExact(WarriorGameplayTags::Enemy_Status_ArmorBroken))
        {
            BaseDamage *= ArmorBrokenLightDamageMultiplier;
        }

    }
    if (UsedHeavyAttackComboCount != 0)
    {
        const float DamageIncreasePercentHeavy = UsedHeavyAttackComboCount * 0.15f + 1.f;
        BaseDamage *= DamageIncreasePercentHeavy;


    }
    const float SafeTargetDefensePower = FMath::Max(TargetDefensePower, 1.f);
    const float FinalDamageDone = BaseDamage * SourceAttackPower / SafeTargetDefensePower;


    
    if(FinalDamageDone>0.f)
    {
        // 把计算出的最终伤害 修改DamageTaken这个属性值
        OutExecutionOutput.AddOutputModifier(
            FGameplayModifierEvaluatedData(
                GetWarriorDamageCapture().DamageTakenProperty,
                EGameplayModOp::Override,
                FinalDamageDone
            )
        );
    }
}
