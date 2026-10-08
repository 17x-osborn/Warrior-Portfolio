// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/WarriorCharacterAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Characters/WarriorBaseCharacter.h"
#include "KismetAnimationLibrary.h"

// 初始化 获取变量
void UWarriorCharacterAnimInstance::NativeInitializeAnimation()
{
	// 获取所有者
	OwningCharacter = Cast<AWarriorBaseCharacter>(TryGetPawnOwner());
	// 获取运动
	if (OwningCharacter) {
		OwningMovementComponent=OwningCharacter->GetCharacterMovement();
	}
}

// 动画更新
void UWarriorCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	if (!OwningCharacter || !OwningMovementComponent) return;
	Groundspeed = OwningCharacter->GetVelocity().Size2D();
	bHasAcceleration=OwningMovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.f;

	// 算出角色实际移动方向(将要)和角色面朝方向之间的角度差
	LocomotionDirection=UKismetAnimationLibrary::CalculateDirection(OwningCharacter->GetVelocity(), OwningCharacter->GetActorRotation());

}
