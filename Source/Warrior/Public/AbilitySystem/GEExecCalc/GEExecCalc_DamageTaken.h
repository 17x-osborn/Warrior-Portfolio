// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GEExecCalc_DamageTaken.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class WARRIOR_API UGEExecCalc_DamageTaken : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
public:

	UGEExecCalc_DamageTaken();

	// 轻击命中破甲目标时的额外伤害倍率
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage|Armor Break", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float ArmorBrokenLightDamageMultiplier = 1.2f;

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
