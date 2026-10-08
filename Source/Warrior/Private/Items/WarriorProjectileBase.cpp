// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/WarriorProjectileBase.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "WFunctionLibrary.h"
#include "WarriorGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "WarriorDebugHelper.h"

AWarriorProjectileBase::AWarriorProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

    // 碰撞盒相关设置
	ProjectileCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ProjectileCollisionBox"));
	SetRootComponent(ProjectileCollisionBox);
    // 设置碰撞为仅查询模式 - 只检测碰撞，不产生物理效果
    ProjectileCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    // 设置对Pawn（玩家、AI等）的碰撞响应为阻挡
    ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    // 设置对动态物体（可移动物体）的碰撞响应为阻挡
    ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    // 设置对静态物体（地形、建筑等）的碰撞响应为阻挡
    ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

    // 绑定委托函数
    ProjectileCollisionBox->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnProjectileHit);
    ProjectileCollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnProjectileBeginOverlap);

    // 创建并设置Niagara粒子效果组件 - 用于投射物的视觉表现（轨迹、发光等）
    ProjectileNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileNiagaraComponent"));
    // 将粒子效果附加到根组件（碰撞盒体），跟随投射物一起移动
    ProjectileNiagaraComponent->SetupAttachment(GetRootComponent());

    // 创建投射物运动组件
    ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));
    // 设置初始发射速度
    ProjectileMovementComp->InitialSpeed = 700.f;
    // 设置最大飞行速度限制
    ProjectileMovementComp->MaxSpeed = 900.f;
    // 设置初始飞行方向为X轴正方向（前向）
    ProjectileMovementComp->Velocity = FVector(1.f, 0.f, 0.f);
    // 设置重力缩放为0 - 投射物不受重力影响，直线飞行
    ProjectileMovementComp->ProjectileGravityScale = 0.f;

    // 设置投射物生命周期为4秒 - 防止投射物无限飞行，自动销毁
    InitialLifeSpan = 4.f;

}

void AWarriorProjectileBase::BeginPlay()
{
	Super::BeginPlay();

    if (ProjectileDamagePolicy == EProjectileDamagePolicy::OnBeginOverlap) {

        ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);

    }
	
}

void AWarriorProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

    // 播放音效等
    BP_OnSpawnProjectileHitFx(Hit.ImpactPoint);

    APawn* HitPawn = Cast<APawn>(OtherActor);

    // 检查命中的是否为Pawn且是否为敌对目标
    if (!HitPawn || !UWFunctionLibrary::IsTargetPawnHostile(GetInstigator(), HitPawn))
    {
        Destroy();
        return;
    }

    bool bIsValidBlock = false;
    const bool bIsPlayerBlocking = UWFunctionLibrary::NativeDoesActorHaveTag(HitPawn, WarriorGameplayTags::Player_Status_Blocking);
    if (bIsPlayerBlocking)
    {
        bIsValidBlock = UWFunctionLibrary::IsValidBlock(this, HitPawn);
    }

    FGameplayEventData Data;
    Data.Instigator = this;      
    Data.Target = HitPawn;       

    if (bIsValidBlock)
    {
        // 如果是有效格挡，发送成功格挡事件
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
            HitPawn,
            WarriorGameplayTags::Player_Event_SuccessfulBlock,
            Data
        );
    }
    else
    {
        HandleApplyProjectileDamage(HitPawn,Data);
    }
    
    Destroy();
}

void AWarriorProjectileBase::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OverlappedActors.Contains(OtherActor))
    {
        return; 
    }
    OverlappedActors.AddUnique(OtherActor);

    if (APawn* HitPawn = Cast<APawn>(OtherActor))
    {
   
        FGameplayEventData Data;
        Data.Instigator = GetInstigator(); 
        Data.Target = HitPawn;           

        if (UWFunctionLibrary::IsTargetPawnHostile(GetInstigator(), HitPawn))
        {
            HandleApplyProjectileDamage(HitPawn, Data);
        }
    }
}

void AWarriorProjectileBase::HandleApplyProjectileDamage(APawn* InHitPawn, const FGameplayEventData& InPayload)
{
    checkf(ProjectileDamageEffectSpecHandle.IsValid(), TEXT("Forgot to assign a valid spec handle to the projectile: %s"), *GetActorNameOrLabel());
    const bool bWasApplied=UWFunctionLibrary::ApplyGameplayEffectSpecHandleToTargetActor(GetInstigator(), InHitPawn, ProjectileDamageEffectSpecHandle);
    if (bWasApplied)
    {
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
            InHitPawn,
            WarriorGameplayTags::Shared_Event_HitReact,
            InPayload
        );
    }
}



