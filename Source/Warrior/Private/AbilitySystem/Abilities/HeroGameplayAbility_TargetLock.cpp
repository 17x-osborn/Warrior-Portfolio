// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/HeroGameplayAbility_TargetLock.h"
#include "Characters/WarriorHeroCharacter.h"
#include "Kismet/KismetSystemLibrary.h" 
#include "Kismet/GameplayStatics.h"
#include "Widgets/WarriorWidgetBase.h"
#include "Controllers/WarriorHeroController.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "WFunctionLibrary.h"
#include "WarriorGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "WarriorDebugHelper.h"

void UHeroGameplayAbility_TargetLock::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	TryLockOnTarget();  // 搜索并选择目标
    InitTargetLockMovement();  // 缓存原始速度 lock时速度更改
    InitTargetLockMappingContext();  // 添加锁定专用IMC，优先级3
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UHeroGameplayAbility_TargetLock::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    ResetTargetLockMovement();
    ResetTargetLockMappingContext();
    CleanUP();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// 每帧调用更新lock
void UHeroGameplayAbility_TargetLock::OnTargetLockTick(float DeltaTime)
{
    // 检查目标是否死亡、隐藏或无效
    if (!CurrentLockedActor || 
       UWFunctionLibrary::NativeDoesActorHaveTag(CurrentLockedActor, WarriorGameplayTags::Shared_Status_Dead) ||
       UWFunctionLibrary::NativeDoesActorHaveTag(GetHeroCharacterFromActorInfo(), WarriorGameplayTags::Shared_Status_Dead) ||
        CurrentLockedActor->IsHidden()){
        CancelTargetLockAbility();
        return;
    }

    // 更新锁定图标位置
    SetTargetLockWidgetPosition();

    // 翻滚或block就不更新英雄和摄像机朝向了
    const bool bShouldOverrideRotation =
        !UWFunctionLibrary::NativeDoesActorHaveTag(GetHeroCharacterFromActorInfo(), WarriorGameplayTags::Player_Status_Rolling)
        && !UWFunctionLibrary::NativeDoesActorHaveTag(GetHeroCharacterFromActorInfo(), WarriorGameplayTags::Player_Status_Blocking);

    if (bShouldOverrideRotation) {
        FRotator LookAtRot=UKismetMathLibrary::FindLookAtRotation(
            GetHeroCharacterFromActorInfo()->GetActorLocation(),
            CurrentLockedActor->GetActorLocation());
        LookAtRot -= FRotator(TargetLockCameraOffsetDistance,0.f, 0.f);
        const FRotator CurrentControlRot= GetHeroControllerFromActorInfo()->GetControlRotation();
        const FRotator TargetRot = FMath::RInterpTo(CurrentControlRot, LookAtRot, DeltaTime, TargetLockRotationInterpSpeed);
        // 相机跟随控制器动 换相机朝向 就换控制器就行
        GetHeroControllerFromActorInfo()->SetControlRotation(FRotator(TargetRot.Pitch, TargetRot.Yaw, 0.f));
        GetHeroCharacterFromActorInfo()->SetActorRotation(FRotator(0.f, TargetRot.Yaw, 0.f));
    }

}

// 切换目标
void UHeroGameplayAbility_TargetLock::SwitchTarget(const FGameplayTag& InSwitchDirectionTag)
{
    GetAvailableActorsToLock();

    TArray<AActor*> ActorsOnLeft;
    TArray<AActor*> ActorsOnRight;
    AActor* NewTargetToLock = nullptr;

    GetAvailableActorsAroundTarget(ActorsOnLeft, ActorsOnRight);

    if (InSwitchDirectionTag == WarriorGameplayTags::Player_Event_SwitchTarget_Left)
    {
        NewTargetToLock = GetNearestTargetFromAvailableActors(ActorsOnLeft);
    }
    else
    {
        NewTargetToLock = GetNearestTargetFromAvailableActors(ActorsOnRight);
    }
    if (NewTargetToLock)
    {
        CurrentLockedActor = NewTargetToLock;

        if (auto* Hero = Cast<AWarriorHeroCharacter>(GetHeroCharacterFromActorInfo()))
        {
            Hero->SetCurrentLockedTarget(CurrentLockedActor);
        }
    }

}

// 第一次尝试去lock
void UHeroGameplayAbility_TargetLock::TryLockOnTarget()
{
	GetAvailableActorsToLock();  // Boxtrace 获取所有候选
    if (AvailableActorsToLock.IsEmpty())
    {
        CancelTargetLockAbility();
        return;
    }
    CurrentLockedActor=GetNearestTargetFromAvailableActors(AvailableActorsToLock);  // 选最近的一个
    if (CurrentLockedActor) {

        DrawTargetLockWidget(); //创建锁定Widget
        SetTargetLockWidgetPosition(); // 将小图标绘制到lock对象的位置

        if (auto* Hero = Cast<AWarriorHeroCharacter>(GetHeroCharacterFromActorInfo()))
        {
            Hero->SetCurrentLockedTarget(CurrentLockedActor);
        }
    }
    else
    {
        CancelTargetLockAbility();
    }

}

void UHeroGameplayAbility_TargetLock::GetAvailableActorsToLock()
{
    AvailableActorsToLock.Empty();

    // 结果数组
    TArray<FHitResult> BoxTraceHits;

    // 使用盒子形状进行多重对象追踪
    UKismetSystemLibrary::BoxTraceMultiForObjects(
        GetHeroCharacterFromActorInfo(),        
        GetHeroCharacterFromActorInfo()->GetActorLocation(),                               // 追踪起点 下一个是终点
        GetHeroCharacterFromActorInfo()->GetActorLocation() + GetHeroCharacterFromActorInfo()->GetActorForwardVector() * BoxTraceDistance,
        TraceBoxSize/2.f,             // 追踪盒子的半尺寸
        GetHeroCharacterFromActorInfo()->GetActorForwardVector().ToOrientationRotator(),   //盒子的旋转 - 使用角色前方方向的旋转
        BoxTraceChannel,              // 要检测的碰撞通道 
        false,                        // 是否复杂碰撞 - false表示使用简单碰撞
        TArray<AActor*>(),            // 要忽略的Actor数组
        bShowPersistentDebugShape ? EDrawDebugTrace::Persistent : EDrawDebugTrace::None,   // 调试绘制模式
        BoxTraceHits,                 //结果
        true                          // 忽略自身
    );

    // 遍历所有盒子追踪的碰撞结果
    for (const FHitResult& TraceHit : BoxTraceHits)
    {
        // 获取碰撞到的Actor
        if (AActor* HitActor = TraceHit.GetActor())
        {
            // 检查这个Actor不是玩家角色自己
            if (HitActor != GetHeroCharacterFromActorInfo())
            {
                // 将可锁定的Actor添加到数组中（自动去重）
                AvailableActorsToLock.AddUnique(HitActor);
            }
        }
    }
}

AActor* UHeroGameplayAbility_TargetLock::GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors)
{
    float ClosestDistance = 0.f;
    // 第二个参数的数组中找到离第一个参数最近的actor
    return UGameplayStatics::FindNearestActor(GetHeroCharacterFromActorInfo()->GetActorLocation(), InAvailableActors, ClosestDistance);
}

// 找到左右两边的备选lock角色数组
void UHeroGameplayAbility_TargetLock::GetAvailableActorsAroundTarget(TArray<AActor*>& OutActorsOnLeft, TArray<AActor*>& OutActorsOnRight)
{
    if (!CurrentLockedActor || AvailableActorsToLock.IsEmpty())
    {
        CancelTargetLockAbility();
        return;
    }

    const FVector PlayerLocation = GetHeroCharacterFromActorInfo()->GetActorLocation();
    const FVector PlayerToCurrentNormalized = (CurrentLockedActor->GetActorLocation() - PlayerLocation).GetSafeNormal(); // 当前目标

    // 遍历所有可锁定的候选Actor
    for (AActor* AvailableActor : AvailableActorsToLock)
    {
        if (!AvailableActor || AvailableActor == CurrentLockedActor) continue;
        // 计算从玩家到候选Actor的归一化方向向量
        const FVector PlayerToAvailableNormalized = (AvailableActor->GetActorLocation() - PlayerLocation).GetSafeNormal();

        // 计算两个方向向量的叉积
        const FVector CrossResult = FVector::CrossProduct(PlayerToCurrentNormalized, PlayerToAvailableNormalized);
        if (CrossResult.Z > 0.f)
        {
            // Z>0 表示候选目标在当前目标的右侧（逆时针方向）
            OutActorsOnRight.AddUnique(AvailableActor);
        }
        else
        {
            // Z<=0 表示候选目标在当前目标的左侧（顺时针方向）
            OutActorsOnLeft.AddUnique(AvailableActor);
        }
    }
}

void UHeroGameplayAbility_TargetLock::DrawTargetLockWidget()
{
    if (!DrawnTargetLockWidget) {
        checkf(TargetLockWidgetClass, TEXT("Forgot assign a valid widget class in Blueprint"));
        DrawnTargetLockWidget = CreateWidget<UWarriorWidgetBase>(GetHeroControllerFromActorInfo(), TargetLockWidgetClass);
        check(DrawnTargetLockWidget);
        DrawnTargetLockWidget->AddToViewport();
    }
}

// 将小图标绘制到lock对象的位置
void UHeroGameplayAbility_TargetLock::SetTargetLockWidgetPosition()
{
    if (!DrawnTargetLockWidget || !CurrentLockedActor) {
        CancelTargetLockAbility();
        return;
    }
    FVector2D ScreenPosition;
    UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
        GetHeroControllerFromActorInfo(),
        CurrentLockedActor->GetActorLocation(),
        ScreenPosition,
        true
    );
    if (TargetLockWidgetSize == FVector2D::ZeroVector)
    {
        // 遍历控件树中的所有子控件
        DrawnTargetLockWidget->WidgetTree->ForEachWidget(
            [this](UWidget* FoundWidget)
            {
                // 尝试将找到的控件转换为尺寸框（SizeBox） 只有是尺寸框 才能成功转换
                if (USizeBox* FoundSizeBox = Cast<USizeBox>(FoundWidget))
                {
                    
                    TargetLockWidgetSize.X = FoundSizeBox->GetWidthOverride();
                    TargetLockWidgetSize.Y = FoundSizeBox->GetHeightOverride();
                }
            }
        );
    }
    // 将控件中心对齐到目标位置，而不是让控件左上角对齐
    ScreenPosition -= (TargetLockWidgetSize / 2.f);
    DrawnTargetLockWidget->SetPositionInViewport(ScreenPosition, false);
}

void UHeroGameplayAbility_TargetLock::InitTargetLockMovement()
{
    CachedDefaultMaxWalkSpeed=GetHeroCharacterFromActorInfo()->GetCharacterMovement()->MaxWalkSpeed;
    GetHeroCharacterFromActorInfo()->GetCharacterMovement()->MaxWalkSpeed = TargetLockMaxWalkSpeed;

}

void UHeroGameplayAbility_TargetLock::InitTargetLockMappingContext()
{
     // 从角色信息中获取玩家控制器，再获取本地玩家
     const ULocalPlayer* LocalPlayer = GetHeroControllerFromActorInfo()->GetLocalPlayer();

     // 获取增强输入本地玩家子系统 - 管理输入映射的核心系统
     UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
     check(Subsystem);

     // 优先级高的上下文会覆盖优先级低的上下文中的相同按键绑定
     Subsystem->AddMappingContext(TargetLockMappingContext, 3);
    
}

void UHeroGameplayAbility_TargetLock::CancelTargetLockAbility()
{
    CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
}

void UHeroGameplayAbility_TargetLock::CleanUP()
{
    AvailableActorsToLock.Empty();
    CurrentLockedActor = nullptr;
    if (DrawnTargetLockWidget) DrawnTargetLockWidget->RemoveFromParent();
    DrawnTargetLockWidget = nullptr;
    TargetLockWidgetSize = FVector2D::ZeroVector;
    CachedDefaultMaxWalkSpeed = 0.f;

    if (auto* Hero = Cast<AWarriorHeroCharacter>(GetHeroCharacterFromActorInfo()))
    {
        Hero->SetCurrentLockedTarget(nullptr);
    }


}

void UHeroGameplayAbility_TargetLock::ResetTargetLockMovement()
{
    if (CachedDefaultMaxWalkSpeed > 0.f) 
    {
        GetHeroCharacterFromActorInfo()->GetCharacterMovement()->MaxWalkSpeed = CachedDefaultMaxWalkSpeed;
    }
}

void UHeroGameplayAbility_TargetLock::ResetTargetLockMappingContext()
{
    if (!GetHeroControllerFromActorInfo()) return;
    const ULocalPlayer* LocalPlayer = GetHeroControllerFromActorInfo()->GetLocalPlayer();
    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
    check(Subsystem);
    Subsystem->RemoveMappingContext(TargetLockMappingContext);
}
