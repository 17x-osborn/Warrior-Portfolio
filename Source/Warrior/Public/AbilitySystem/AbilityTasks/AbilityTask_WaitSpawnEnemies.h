// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitSpawnEnemies.generated.h"

// 声明多播委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FWaitSpawnEnemiesDelegate,                  
    const TArray<AWarriorEnemyCharacter*>&,     
    SpawnedEnemies                              
);
class AWarriorEnemyCharacter;
UCLASS()
class WARRIOR_API UAbilityTask_WaitSpawnEnemies : public UAbilityTask
{
	GENERATED_BODY()

public:
    //~ Begin UGameplayTask Interface
    virtual void Activate() override;
    virtual void OnDestroy(bool bInOwnerFinished) override;
    //~ End UGameplayTask Interface

	UFUNCTION(BlueprintCallable, Category = "Warrior|AbilityTasks",
		meta = (DisplayName = "Wait Gameplay Event And Spawn Enemies", HidePin = "OwningAbility",DefaultToSelf = "OwningAbility",
		BlueprintInternalUseOnly = "true",NumToSpawn = "1",RandomSpawnRadius = "200"))
    static UAbilityTask_WaitSpawnEnemies* WaitSpawnEnemies(
        UGameplayAbility* OwningAbility,
        FGameplayTag EventTag,                          
        TSoftClassPtr<AWarriorEnemyCharacter> SoftEnemyClassToSpawn,
        int32 NumToSpawn,                                
        const FVector& SpawnOrigin,                     
        float RandomSpawnRadius                          
    );

	UPROPERTY(BlueprintAssignable)
    FWaitSpawnEnemiesDelegate OnSpawnFinished;  // 成功

    UPROPERTY(BlueprintAssignable)
    FWaitSpawnEnemiesDelegate DidNotSpawn;      // 失败

private:
    FGameplayTag CachedEventTag;
    TSoftClassPtr<AWarriorEnemyCharacter> CachedSoftEnemyClassToSpawn;
    int32 CachedNumToSpawn;
    FVector CachedSpawnOrigin;
    float CachedRandomSpawnRadius;
    FDelegateHandle DelegateHandle;

    void OnGameplayEventReceived(const FGameplayEventData* InPayload);
    void OnEnemyClassLoaded();

};
