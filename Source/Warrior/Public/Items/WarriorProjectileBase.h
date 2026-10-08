// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "WarriorProjectileBase.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UProjectileMovementComponent;
struct FGameplayEventData;

UENUM(BlueprintType)
enum class EProjectileDamagePolicy : uint8
{
	OnHit,             // 碰撞时造成伤害
	OnBeginOverlap     // 重叠开始时造成伤害
};

UCLASS()
class WARRIOR_API AWarriorProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AWarriorProjectileBase();

protected:

	virtual void BeginPlay() override;

    // 碰撞盒体组件 - 用于检测碰撞或重叠事件
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
    UBoxComponent* ProjectileCollisionBox;

    // Niagara粒子组件 - 投射物的视觉特效（轨迹、爆炸效果等）
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
    UNiagaraComponent* ProjectileNiagaraComponent;

    // 投射物运动组件 - 处理物理运动、速度、重力等
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
    UProjectileMovementComponent* ProjectileMovementComp;

    // 伤害策略配置 - 选择投射物造成伤害的触发时机
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
    EProjectileDamagePolicy ProjectileDamagePolicy = EProjectileDamagePolicy::OnHit;

    // 伤害规格  在spawn的结点中可见
    UPROPERTY(BlueprintReadOnly, Category = "Projectile", meta = (ExposeOnSpawn = "True"))
    FGameplayEffectSpecHandle ProjectileDamageEffectSpecHandle;


    // 碰撞后会自动调用的函数
    UFUNCTION()
    virtual void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
    UFUNCTION()
    virtual void OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    // 蓝图可实现事件：生成投射物命中特效
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Spawn Projectile Hit FX"))
    void BP_OnSpawnProjectileHitFx(const FVector& HitLocation);

private:
    // 处理应用伤害
    void HandleApplyProjectileDamage(APawn* InHitPawn,const FGameplayEventData& InPayload);

    TArray <AActor*>OverlappedActors;  // 大招技能伤害的敌人们
};
