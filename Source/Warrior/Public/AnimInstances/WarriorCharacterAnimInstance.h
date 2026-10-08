// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimInstances/WarriorBaseAnimInstance.h"
#include "WarriorCharacterAnimInstance.generated.h"

class AWarriorBaseCharacter;
class UCharacterMovementComponent;
/**
 * 
 */
UCLASS()
class WARRIOR_API UWarriorCharacterAnimInstance : public UWarriorBaseAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	// 初始化
	UPROPERTY()
	TObjectPtr<AWarriorBaseCharacter> OwningCharacter;
	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> OwningMovementComponent;

	// 更新
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadonly, Category = "AnimData|LocomotionData")
	float Groundspeed;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadonly, Category = "AnimData|LocomotionData")
	bool bHasAcceleration; // 加速度

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadonly, Category = "AnimData|LocomotionData")
	float LocomotionDirection;  //运动方向

};
