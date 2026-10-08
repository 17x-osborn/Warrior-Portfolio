// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/PickUps/WarriorPickUpBase.h"
#include "WarriorStoneBase.generated.h"

class UWarriorAbilitySystemComponent;
class UGameplayEffect;
/**
 * 
 */
UCLASS()
class WARRIOR_API AWarriorStoneBase : public AWarriorPickUpBase
{
	GENERATED_BODY()

public:
	void Comsume(UWarriorAbilitySystemComponent* AbilitySystemComponent, int32 ApplyLevel);

protected:

	// 在蓝图实现 播放音效特效等
	UFUNCTION(BLueprintImplementableEvent, meta = (DisplayName = "On Stone Consumed"))
	void BP_OnStoneConsumed(); 

	// 当角色和治疗石头重叠时
	virtual void OnPickUpCollisionSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,  // 自己
		AActor* OtherActor,                        // 与之重叠的其他Actor
		UPrimitiveComponent* OtherComp,            // 其他Actor上的具体组件
		int32 OtherBodyIndex,                      // 其他组件的body索引
		bool bFromSweep,                           // 是否来自扫描检测
		const FHitResult& SweepResult) override;           // 扫描检测的详细结果;

	// 捡到石头的增益 在此
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> StoneGameplayEffectClass;
};
