// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WarriorWidgetBase.generated.h"

class UHeroUIComponent;
class UEnemyUIComponent;
/**
 * 
 */
UCLASS()
class WARRIOR_API UWarriorWidgetBase : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	// 初始化 在窗口新建时候调用一次 初始化角色窗口
	virtual void NativeOnInitialized() override;

	// 纯蓝图函数 初始化heroUI组件
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On 0wning Hero UI Component Initialized")) //C++可以调用蓝图中实现的函数
	void BP_OnOwningHeroUIComponentInitialized(UHeroUIComponent* OwningHeroUIComponent);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On 0wning	Enemy UI Component Initialized")) 
	void BP_OnOwningEnemyUIComponentInitialized(UEnemyUIComponent* OwningEnemyUIComponent);


public:
	UFUNCTION(BlueprintCallable) // 蓝图可以调用C++函数
	void InitEnemyCreatedWidget(AActor* OwningEnemyActor);
};
