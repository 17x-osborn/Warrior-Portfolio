// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameplayTagContainer.h"
#include "WarriorGameInstance.generated.h"


USTRUCT(BlueprintType)
struct FWarriorGameLevelSet
{
    // 必须包含的宏，用于生成反射代码
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, meta = (Categories = "GameData.Level"))
    FGameplayTag LevelTag;  // 使用 GameplayTag 系统来标识关卡类型

    UPROPERTY(EditDefaultsOnly)
    TSoftObjectPtr<UWorld> Level;  // 软引用，指向一个 UWorld（关卡）资源，便于异步加载和内存管理

    bool IsValid() const {
        return LevelTag.IsValid() && !Level.IsNull();
    }
};
/**
 * 
 */
UCLASS()
class WARRIOR_API UWarriorGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:

    virtual void OnPreLoadMap(const FString& MapName);
    virtual void OnDestinationWorldLoaded(UWorld* LoadedWorld);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TArray<FWarriorGameLevelSet> GameLevelSets;

public:
    virtual void Init() override;
    
    UFUNCTION(BlueprintPure, meta = (GameplayTagFilter= "GameData.Level"))
    TSoftObjectPtr<UWorld> GetGameLevelByTag(FGameplayTag InTag) const;
	
};
