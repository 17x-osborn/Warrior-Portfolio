// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/WarriorHeroCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "DataAssets/Input/DataAsset_InputConfig.h"
#include "Components/Input/WarriorInputComponent.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "WarriorGameplayTags.h"
#include "DataAssets/StartUpData/DataAsset_HeroStartUpData.h"
#include "Components/Combat/HeroCombatComponent.h"
#include "Components/UI/HeroUIComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameModes/WarriorBaseGameMode.h"
#include "MotionWarpingComponent.h"
#include "WarriorDebugHelper.h"

AWarriorHeroCharacter::AWarriorHeroCharacter()
{
	// 初始化当前角色胶囊体组件的尺寸，半径/半高
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f); 

	// 角色实体不会自动跟随玩家鼠标或手柄右摇杆的输入而旋转。它的旋转将由其他逻辑（如移动方向或动画）来控制
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 200.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);
	// 让弹簧臂的旋转完全由玩家的输入来决定
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom,USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	GetCharacterMovement()->bOrientRotationToMovement = true; //角色会自动朝向其加速度/移动的方向旋转
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);  //旋转到目标方向的速度
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f; // 控制角色停止移动时（即没有输入时）的减速快慢

	// 初始化战斗组件
	HeroCombatComponent=CreateDefaultSubobject<UHeroCombatComponent>(TEXT("HeroCombatComponent"));
	HeroUIComponent= CreateDefaultSubobject<UHeroUIComponent>(TEXT("HeroUIComponent"));


}

// 当这个角色被一个控制器所“占据”时调用
void AWarriorHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// 指定的数据资产（Data Asset）中，加载并赋予这个角色所有初始的技能（Abilities）和属性（Attributes）
	if (!CharcterStartUpData.IsNull()){
		// 同步加载数据资产 加载成功后，返回一个指向该数据资产的指针
		if (UDataAsset_StartUpDataBase* LoadedData = CharcterStartUpData.LoadSynchronous()) {

			int32 AbilityApplyLevel = 1;
			if (AWarriorBaseGameMode* BaseGameMode = GetWorld()->GetAuthGameMode<AWarriorBaseGameMode>())
			{
				switch (BaseGameMode->GetCurrentGameDifficulty())
				{
				case EWarriorGameDifficulty::Easy:
					AbilityApplyLevel = 4;
					Debug::Print(TEXT("Current Difficulty: Easy"));
					break;

				case EWarriorGameDifficulty::Normal:
					AbilityApplyLevel = 3;
					break;

				case EWarriorGameDifficulty::Hard:
					AbilityApplyLevel = 2;
					break;

				case EWarriorGameDifficulty::VeryHard:
					AbilityApplyLevel = 1;
					break;

				default:
					break;
				}
			}

			// 授予技能和属性
			LoadedData->GiveToAbilitySystemComponent(WarriorAbilitySystemComponent, AbilityApplyLevel);
		}
	}

	/*if (WarriorAbilitySystemComponent && WarriorAttributeSet) {
		const FString ASCText = FString::Printf(TEXT("Owner Actor: %s, AvatarActor: &s"), *WarriorAbilitySystemComponent->GetOwnerActor()->GetActorLabel(), *WarriorAbilitySystemComponent->GetAvatarActor()->GetActorLabel());
		Debug::Print(TEXT("Ability system component valid. ") + ASCText,FColor::Green);
		Debug::Print(TEXT("AttributeSet valid. ") + ASCText, FColor::Green);
	}*/

}

UPawnCombatComponent* AWarriorHeroCharacter::GetPawnCombatComponent() const
{
	return HeroCombatComponent;
}

UHeroUIComponent* AWarriorHeroCharacter::GetHeroUIComponent() const
{
	return HeroUIComponent;
}

UPawnUIComponent* AWarriorHeroCharacter::GetPawnUIComponent() const
{
	return HeroUIComponent;
}

// 启用默认IMC+IA绑定回调
void AWarriorHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	checkf(InputConfigDataAsset, TEXT("Forgot to assign a valid data asset as input config"));

	// 从角色的控制器获取本地角色
	ULocalPlayer* LocalPlayer= GetController<APlayerController>()->GetLocalPlayer();
	// 管理输入映射上下文（Input Mapping Contexts） 的核心系统
	UEnhancedInputLocalPlayerSubsystem* Subsystem=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	check(Subsystem);
	// 角色的控制设置为蓝图在assets设置的输入映射上下文，优先级0是默认基础输入
	Subsystem->AddMappingContext(InputConfigDataAsset->DefaultMappingContext,0);

	// 把输入组件转换为之前写的输入组件 定义信号槽的那个cpp
	UWarriorInputComponent* WarriorInputComponent= CastChecked<UWarriorInputComponent>(PlayerInputComponent);

	// 原生输入
	WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_Move,
		ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_Look,
		ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_SwitchTarget,
		ETriggerEvent::Triggered, this, &ThisClass::Input_SwitchTargetTriggered); 
	WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_SwitchTarget,
		ETriggerEvent::Completed, this, &ThisClass::Input_SwitchTargetCompleted);
	WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_PickUp_Stones,
		ETriggerEvent::Started, this, &ThisClass::Input_PickUpStonesStarted);

	// 技能输入 最终都交给ASC
	WarriorInputComponent->BindAbilityInputAction(
		InputConfigDataAsset,                    // 去哪里读取 IA 与 Tag
		this,                                    // 回调调用哪个角色实例
		&ThisClass::Input_AbilityInputPressed,   // 按下时调用哪个函数
		&ThisClass::Input_AbilityInputReleased   // 松开时调用哪个函数
	);
}

void AWarriorHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// InputActionValue: 输入参数，包含了来自输入设备的原始输入数据
void AWarriorHeroCharacter::Input_Move(const FInputActionValue& InputActionValue)
{
	// 提取二维输入向量 (X,Y)移动距离
	const FVector2D MovementVector = InputActionValue.Get<FVector2D>();
	// 获取移动基准方向
	const FRotator MovementRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

	// 处理前后移动
	if (MovementVector.Y != 0.f) {
		// 将世界坐标系的方向转换为基于摄像机视角的局部坐标系方向
		const FVector ForwardDirection = MovementRotation.RotateVector(FVector::ForwardVector);
		// 实际移动
		AddMovementInput(ForwardDirection,MovementVector.Y);
	}
	if (MovementVector.X != 0.f) {
		const FVector RightDirection = MovementRotation.RotateVector(FVector::RightVector);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

// 视角旋转输入
void AWarriorHeroCharacter::Input_Look(const FInputActionValue& InputActionValue)
{
	const FVector2D LookAxisVector = InputActionValue.Get<FVector2D>();
	if (LookAxisVector.X != 0.f) {
		// Yaw - 偏航
		AddControllerYawInput(LookAxisVector.X);
	}
	if (LookAxisVector.Y != 0.f) {
		// Pitch - 俯仰
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
// 当玩家按下切换目标按钮时调用
void AWarriorHeroCharacter::Input_SwitchTargetTriggered(const FInputActionValue& InputActionValue)
{
	SwitchDirection = InputActionValue.Get<FVector2D>();
}

void AWarriorHeroCharacter::Input_SwitchTargetCompleted(const FInputActionValue& InputActionValue)
{
	FGameplayEventData Data;

	// 根据切换方向发送相应的游戏事件
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		this, // 目标Actor
		SwitchDirection.X > 0.f ?
		WarriorGameplayTags::Player_Event_SwitchTarget_Right : 
		WarriorGameplayTags::Player_Event_SwitchTarget_Left,  
		Data  
	);
}




void AWarriorHeroCharacter::Input_PickUpStonesStarted(const FInputActionValue& InputActionValue)
{
	FGameplayEventData Data;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		this,
		WarriorGameplayTags::Player_Event_ConsumeStones,
		Data);
}


void AWarriorHeroCharacter::Input_AbilityInputPressed(FGameplayTag InInputTag)
{
	WarriorAbilitySystemComponent->OnAbilityInputPressed(InInputTag);
}

void AWarriorHeroCharacter::Input_AbilityInputReleased(FGameplayTag InInputTag)
{
	WarriorAbilitySystemComponent->OnAbilityInputReleased(InInputTag);
}

void AWarriorHeroCharacter::UpdateMotionWarpingTarget(AActor* TargetActor, FName WarpTargetName)
{
	AActor* TargetToUse = TargetActor;

	// 如果没传参，尝试使用锁定目标
	if (TargetToUse == nullptr)
	{
		TargetToUse = CurrentLockedTarget;
	}

	// 检查合法性
	if (!MotionWarpingComponent) return;
	if (TargetToUse == nullptr || !IsValid(TargetToUse) || TargetToUse->IsHidden())
	{
		MotionWarpingComponent->RemoveWarpTarget(WarpTargetName);
		return;
	}
	// 目标距离太远 (超过吸附极限)
	const float DistToTarget = GetDistanceTo(TargetToUse);
	const float WarpMaxDistance = 500.0f; // 4米

	if (DistToTarget > WarpMaxDistance)
	{
		// 同样要清除，否则上一帧在范围内，这一帧出去了，还会强行吸过去
		MotionWarpingComponent->RemoveWarpTarget(WarpTargetName);
		return;
	}

	FVector TargetLoc = TargetToUse->GetActorLocation();
	FVector MyLoc = GetActorLocation();

	FVector Direction = (MyLoc - TargetLoc).GetSafeNormal();
	const float AttackRange = 70.0f; // 贴脸距离

	FVector FinalWarpLocation = TargetLoc + (Direction * AttackRange); // 最终位置 = 敌人位置 + 敌人指向英雄的方向 × 70

	FRotator FinalWarpRotation = (TargetLoc - MyLoc).Rotation();
	FinalWarpRotation.Pitch = 0;

	// 提交数据
	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
		WarpTargetName,
		FinalWarpLocation,
		FinalWarpRotation
	);
}
