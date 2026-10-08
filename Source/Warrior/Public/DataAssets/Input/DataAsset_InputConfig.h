// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DataAsset_InputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

// 结构体 放对应的tag和action
USTRUCT(BlueprintType)
struct FWarriorInputActionConfig {
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,meta=(Categories = "InputTag"))
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> InputAction;

	bool IsValid() const {
		return InputTag.IsValid() && InputAction;
	}

};
/**
 * UDataAsset_InputConfig 就像一个输入配置表，把所有的输入映射关系集中管理,但这里代码配置表没放东西，在蓝图里放
 */
UCLASS()
class WARRIOR_API UDataAsset_InputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// 上下文
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	//数组 里面放上面定义的结构体 行标题是InputActionB
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty="InputTag"))
	TArray<FWarriorInputActionConfig> NativeInputActions;
	TObjectPtr<UInputAction> FindNativeInputActionByTag(const FGameplayTag& InInputTag) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputTag"))
	TArray<FWarriorInputActionConfig> AbilityInputActions;
	
};
