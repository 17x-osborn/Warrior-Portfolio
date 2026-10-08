// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/WarriorBaseCharacter.h"
#include "ObjectPool/WarriorPoolableInterface.h"
#include "WarriorEnemyCharacter.generated.h"

class UEnemyCombatComponent;
class UEnemyUIComponent;
class UWidgetComponent;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDiedPooled, AWarriorEnemyCharacter*, DeadEnemy);

/**
 * 
 */
UCLASS()
class WARRIOR_API AWarriorEnemyCharacter : public AWarriorBaseCharacter, public IWarriorPoolableInterface
{
    GENERATED_BODY()

public:
    AWarriorEnemyCharacter();

    // The benchmark waits for asynchronous startup grants, not just SpawnActor's return.
    bool IsStartUpDataReady() const { return bStartUpDataReady; }

    // 获取敌人战斗组件
    FORCEINLINE UEnemyCombatComponent* GetEnemyCombatComponent() const { return EnemyCombatComponent; }
    FORCEINLINE UBoxComponent* GetLeftHandCollisionBox() const { return LeftHandCollisionBox; }
    FORCEINLINE UBoxComponent* GetRightHandCollisionBox() const { return RightHandCollisionBox; }

    UPROPERTY(BlueprintAssignable)
    FOnEnemyDiedPooled OnEnemyDiedPooled;

    UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
    void EventHandleEnemyDeath();

    virtual void OnActivateFromPool_Implementation() override;
    virtual void OnDeactivateToPool_Implementation() override;

protected:
    //~ Begin APawn Interface.
    // 同步加载英雄启动数据
    virtual void PossessedBy(AController* NewController) override;
    //~ End APawn Interface

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    //~ Begin IPawnCombatInterface Interface.
    // 实现返回combat组件的接口
    virtual UPawnCombatComponent* GetPawnCombatComponent() const override;
    //~ End IPawnCombatInterface Interface

    //~ Begin IPawnUIInterface Interface.
    virtual UPawnUIComponent* GetPawnUIComponent() const override;
    virtual UEnemyUIComponent* GetEnemyUIComponent() const override;
    //~ End IPawnUIInterface Interface

#if WITH_EDITOR

    //~ Begin IObject Interface.
    // 当在编辑器中修改对象属性时会自动调用
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
    //~ End IObject Interface

#endif


    // 敌人战斗组件 - 在任意地方可见，蓝图只读，分类为"Combat"
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UEnemyCombatComponent> EnemyCombatComponent;

    // Boss手部碰撞盒
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr <UBoxComponent> LeftHandCollisionBox;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr <UBoxComponent> RightHandCollisionBox;

    // 碰撞盒附着socket的名字
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    FName LeftHandCollisionBoxAttachBoneName;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    FName RightHandCollisionBoxAttachBoneName;

    // UI组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UEnemyUIComponent> EnemyUIComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UWidgetComponent> EnemyHealthWidgetComponent;


    // 委托调用函数  碰撞开始
    UFUNCTION()
    virtual void OnCollisionBoxBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,  // 产生重叠的组件
        AActor* OtherActor,                        // 与之重叠的其他Actor（如敌人、物体）
        UPrimitiveComponent* OtherComp,            // 其他Actor上的具体组件
        int32 OtherBodyIndex,                      // 其他组件的body索引
        bool bFromSweep,                           // 是否来自扫描检测
        const FHitResult& SweepResult              // 扫描检测的详细结果
    );

private:
    void InitEnemyStartUpData();
    bool bStartUpDataReady = false;

    FVector CachedInitialScale;



};
