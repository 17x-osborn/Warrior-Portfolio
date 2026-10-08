// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarriorWeaponBase.generated.h"

class UBoxComponent;

// 声明一个带一个参数的事件委托
DECLARE_DELEGATE_OneParam(FOnTargetInteractedDelegate, AActor*)

UCLASS()
class WARRIOR_API AWarriorWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWarriorWeaponBase();

	// 武器击中和离开两个事件
	FOnTargetInteractedDelegate OnWeaponHitTarget;
	FOnTargetInteractedDelegate OnWeaponPulledFormTarget;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	TObjectPtr<UBoxComponent> WeaponCollisonBox;

	UFUNCTION()
	// 碰撞开始重叠事件 - 当武器碰撞体与其他物体开始重叠时调用
	virtual void OnCollisionBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,  // 产生重叠的组件（武器碰撞盒）
		AActor* OtherActor,                        // 与之重叠的其他Actor（如敌人、物体）
		UPrimitiveComponent* OtherComp,            // 其他Actor上的具体组件
		int32 OtherBodyIndex,                      // 其他组件的body索引
		bool bFromSweep,                           // 是否来自扫描检测
		const FHitResult& SweepResult              // 扫描检测的详细结果
	);

	UFUNCTION()
	// 碰撞结束重叠事件 - 当武器碰撞体与其他物体结束重叠时调用
	virtual void OnCollisionBoxEndOverlap(
		UPrimitiveComponent* OverlappedComponent,  // 产生重叠的组件
		AActor* OtherActor,                        // 结束重叠的其他Actor
		UPrimitiveComponent* OtherComp,            // 其他Actor上的具体组件
		int32 OtherBodyIndex                       // 其他组件的body索引
	);
public:
	FORCEINLINE UBoxComponent* GetWeaponCollisonBox() const { return WeaponCollisonBox; }

};
