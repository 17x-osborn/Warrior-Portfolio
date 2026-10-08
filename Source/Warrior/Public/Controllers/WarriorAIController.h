// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WarriorAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
/**
 * 
 */
UCLASS()
class WARRIOR_API AWarriorAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AWarriorAIController(const FObjectInitializer& ObjectInitializer);

	//~ Begin IGenericTeamAgentInterface Interface.
	// 重写这个 获取敌对方是谁
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~ End IGenericTeamAgentInterface Interface

protected:
	// 在这里设置闪避组件的一些参数
	virtual void BeginPlay() override;

	// AI感知的组件
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TObjectPtr<UAIPerceptionComponent> EnemyPerceptionComponent;
	// 视觉感知的配置组件
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TObjectPtr<UAISenseConfig_Sight> AISenseConfig_Sight;

	// 委托 回调函数
	UFUNCTION()
	virtual void OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Detour Crowd Avoidance Config")
	bool bEnableDetourCrowdAvoidance = true; // 是否启用Detour人群避障功能

	UPROPERTY(EditDefaultsOnly, Category = "Detour Crowd Avoidance Config",
		meta = (EditCondition = "bEnableDetourCrowdAvoidance", UIMin = "1", UIMax = "4"))
	int32 DetourCrowdAvoidanceQuality = 4; // 避障质量等级 (1-4)，仅在启用避障时可编辑

	UPROPERTY(EditDefaultsOnly, Category = "Detour Crowd Avoidance Config",
		meta = (EditCondition = "bEnableDetourCrowdAvoidance"))
	float CollisionQueryRange = 600.f;


};
