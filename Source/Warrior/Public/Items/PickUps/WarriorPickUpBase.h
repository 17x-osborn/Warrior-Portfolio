// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarriorPickUpBase.generated.h"

class USphereComponent;
UCLASS()
class WARRIOR_API AWarriorPickUpBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AWarriorPickUpBase();

protected:
	// （治疗石头）碰撞盒
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pick Up Interaction")
	USphereComponent* PickUpCollisionSphere;

	UFUNCTION()
	virtual void OnPickUpCollisionSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,  // 自己
		AActor* OtherActor,                        // 与之重叠的其他Actor
		UPrimitiveComponent* OtherComp,            // 其他Actor上的具体组件
		int32 OtherBodyIndex,                      // 其他组件的body索引
		bool bFromSweep,                           // 是否来自扫描检测
		const FHitResult& SweepResult);           // 扫描检测的详细结果;

};
