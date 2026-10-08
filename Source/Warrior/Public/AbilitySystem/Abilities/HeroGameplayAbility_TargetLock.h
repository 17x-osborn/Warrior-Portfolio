// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/WarriorHeroGameplayAbility.h"
#include "HeroGameplayAbility_TargetLock.generated.h"

class UWarriorWidgetBase;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class WARRIOR_API UHeroGameplayAbility_TargetLock : public UWarriorHeroGameplayAbility
{
	GENERATED_BODY()

protected:
	//~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~	End	 UGameplayAbility Interface

	// 每帧调用 更新位置
	UFUNCTION(BlueprintCallable)
	void OnTargetLockTick(float DeltaTime);
	// 切换目标
	UFUNCTION(BlueprintCallable)
	void SwitchTarget(const FGameplayTag& InSwitchDirectionTag);

private:
	void TryLockOnTarget();
	void GetAvailableActorsToLock();
	AActor* GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors);
	void GetAvailableActorsAroundTarget(TArray<AActor*> & OutActorsOnLeft, TArray<AActor*> & OutActorsOnRight);
	void DrawTargetLockWidget();
	void SetTargetLockWidgetPosition();
	void InitTargetLockMovement();
	void InitTargetLockMappingContext();

	// 结束能力后的一些处理工作
	void CancelTargetLockAbility();
	void CleanUP();
	void ResetTargetLockMovement();
	void ResetTargetLockMappingContext();

	// 盒子追踪的距离
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float BoxTraceDistance = 5000.f;

	// 盒子追踪的尺寸
	// X=长度，Y=宽度，Z=高度
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	FVector TraceBoxSize = FVector(5000.f, 5000.f, 300.f);

	// 盒子追踪的碰撞通道
	// 指定要检测哪些类型的对象（如Pawn、WorldDynamic、WorldStatic等）
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TArray< TEnumAsByte < EObjectTypeQuery > > BoxTraceChannel;

	// 是否显示持久的调试形状
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	bool bShowPersistentDebugShape = false;

	// 角色转向目标的速度
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float TargetLockRotationInterpSpeed = 5.f;
	// 角色在lock时候的最大速度
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float TargetLockMaxWalkSpeed = 150.f;
	// 上下文
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	UInputMappingContext* TargetLockMappingContext;
	// 设置摄像机
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float TargetLockCameraOffsetDistance = 20.f;


	// 小图标
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TSubclassOf <UWarriorWidgetBase> TargetLockWidgetClass;
	UPROPERTY()
	UWarriorWidgetBase* DrawnTargetLockWidget;
	UPROPERTY()
	FVector2D TargetLockWidgetSize = FVector2D::ZeroVector;

	// 可以被lock的actor们
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TArray<AActor*> AvailableActorsToLock;
	// 其中最近的一个actor
	UPROPERTY()
	AActor* CurrentLockedActor;

	// 缓存的角色移动最大速度
	UPROPERTY()
	float CachedDefaultMaxWalkSpeed = 0.f;








};
