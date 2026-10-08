// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/WarriorEnemyCharacter.h"
#include "Components/Combat/EnemyCombatComponent.h"
#include "Engine/AssetManager.h"
#include "DataAssets/StartUpData/DataAsset_EnemyStartUpData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/UI/EnemyUIComponent.h"
#include "Components/WidgetComponent.h"
#include "Widgets/WarriorWidgetBase.h"
#include "Components/BoxComponent.h"
#include "GameModes/WarriorBaseGameMode.h"
#include "ObjectPool/WarriorObjectPoolSubsystem.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "AbilitySystem/WarriorAttributeSet.h"
#include "Perception/AIPerceptionComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "WarriorGameplayTags.h"
#include "Items/Weapons/WarriorWeaponBase.h"
#include "Components/CapsuleComponent.h"
#include "Characters/WarriorHeroCharacter.h" 
#include "Kismet/GameplayStatics.h"
#include "WFunctionLibrary.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"


AWarriorEnemyCharacter::AWarriorEnemyCharacter()
{
    // 设置AI自动控制方式：当角色被放置在世界中或生成时，自动由AI控制器接管
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // 配置角色移动组件
    UCharacterMovementComponent* MovementComponent = GetCharacterMovement();

    // 禁用控制器旋转Yaw，让角色只跟随移动方向旋转
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    // 配置移动组件属性
    MovementComponent->bUseControllerDesiredRotation = false;
    MovementComponent->bOrientRotationToMovement = true;    // 启用角色朝向移动方向
    MovementComponent->RotationRate = FRotator(0.f, 180.f, 0.f);  // 设置角色旋转速率（Yaw轴180度/秒）
    MovementComponent->MaxAcceleration = 300.f;             // 设置最大加速度
    MovementComponent->BrakingDecelerationWalking = 1000.f; // 设置行走时的刹车减速度

    // 创建敌人战斗组件
    EnemyCombatComponent = CreateDefaultSubobject<UEnemyCombatComponent>(TEXT("EnemyCombatComponent"));

    EnemyUIComponent = CreateDefaultSubobject<UEnemyUIComponent>(TEXT("EnemyUIComponent"));

    EnemyHealthWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("EnemyHealthWidgetComponent"));
    EnemyHealthWidgetComponent->SetupAttachment(GetMesh());

    // Boss手部碰撞盒
    LeftHandCollisionBox =CreateDefaultSubobject<UBoxComponent>("LeftHandCollisionBox");
    LeftHandCollisionBox->SetupAttachment(GetMesh());
    LeftHandCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LeftHandCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnCollisionBoxBeginOverlap);

    RightHandCollisionBox = CreateDefaultSubobject<UBoxComponent>("RightHandCollisionBox");
    RightHandCollisionBox->SetupAttachment(GetMesh());
    RightHandCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RightHandCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnCollisionBoxBeginOverlap);

}

// 当角色被控制器占据时调用（通常是AI控制器）
void AWarriorEnemyCharacter::PossessedBy(AController* NewController)
{
    // 首先调用父类的实现，确保基本的控制器绑定功能正常工作
    Super::PossessedBy(NewController);

    // 初始化敌人的启动数据，在AI控制器接管后立即设置角色的初始状态
    InitEnemyStartUpData();
}

void AWarriorEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (UWarriorWidgetBase* HealthWidget = Cast<UWarriorWidgetBase>(EnemyHealthWidgetComponent->GetUserWidgetObject()))
    {
        HealthWidget->InitEnemyCreatedWidget(this);
    }

    CachedInitialScale = GetActorScale3D();
}

UPawnCombatComponent* AWarriorEnemyCharacter::GetPawnCombatComponent() const
{
    return EnemyCombatComponent;
}

void AWarriorEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // A fresh Spawn/Destroy run must not leave separately spawned weapons behind.
    if (EndPlayReason == EEndPlayReason::Destroyed && EnemyCombatComponent)
    {
        if (AWarriorWeaponBase* Weapon = EnemyCombatComponent->GetCharacterCurrentEquippedWeapon())
        {
            if (IsValid(Weapon)) Weapon->Destroy();
        }
    }
    Super::EndPlay(EndPlayReason);
}

UPawnUIComponent* AWarriorEnemyCharacter::GetPawnUIComponent() const
{
    return EnemyUIComponent;
}

UEnemyUIComponent* AWarriorEnemyCharacter::GetEnemyUIComponent() const
{
    return EnemyUIComponent;
}

// 编辑器有改动的时候调用 为了把碰撞盒附着到对应位置
#if WITH_EDITOR
void AWarriorEnemyCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, LeftHandCollisionBoxAttachBoneName))
    {
        LeftHandCollisionBox->AttachToComponent(
            GetMesh(),  // 目标组件：角色的骨架网格体
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,  // 附着规则
            LeftHandCollisionBoxAttachBoneName  // 目标Socket名称（骨骼插槽）
        );
    }

    if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, RightHandCollisionBoxAttachBoneName))
    {
        RightHandCollisionBox->AttachToComponent(
            GetMesh(), 
            FAttachmentTransformRules::SnapToTargetNotIncludingScale, 
            RightHandCollisionBoxAttachBoneName  
        );
    }
}
#endif

void AWarriorEnemyCharacter::OnCollisionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (APawn* HitPawn = Cast<APawn>(OtherActor))
    {
        if (UWFunctionLibrary::IsTargetPawnHostile(this, HitPawn))
        {
            EnemyCombatComponent->OnHitTargetActor(HitPawn);
        }
    }

}

// 初始化敌人的启动数据 CharcterStartUpData是CharcterBase里面定义的
void AWarriorEnemyCharacter::InitEnemyStartUpData()
{
    bStartUpDataReady = false;
    // 检查启动数据资产是否有效，如果为空则直接返回
    if (CharcterStartUpData.IsNull())
    {
        bStartUpDataReady = true;
        return;
    }

    int32 AbilityApplyLevel = 1;
    if (AWarriorBaseGameMode* BaseGameMode = GetWorld()->GetAuthGameMode<AWarriorBaseGameMode>())
    {
        switch (BaseGameMode->GetCurrentGameDifficulty())
        {
        case EWarriorGameDifficulty::Easy:
            AbilityApplyLevel = 1;
            break;

        case EWarriorGameDifficulty::Normal:
            AbilityApplyLevel = 2;
            break;

        case EWarriorGameDifficulty::Hard:
            AbilityApplyLevel = 3;
            break;

        case EWarriorGameDifficulty::VeryHard:
            AbilityApplyLevel = 4;
            break;

        default:
            break;
        }
    }

    // 使用资源管理器异步加载启动数据资产
    UAssetManager::GetStreamableManager().RequestAsyncLoad(
        // 指定要加载的软对象路径
        CharcterStartUpData.ToSoftObjectPath(),
        // 创建加载完成后的回调委托（使用Lambda表达式）
        FStreamableDelegate::CreateLambda(
            // 弱引用避免加载结束时访问已经销毁的敌人。
            [WeakThis = TWeakObjectPtr<AWarriorEnemyCharacter>(this), AbilityApplyLevel]() {
                if (!WeakThis.IsValid()) return;
                // 尝试获取加载完成的数据资产
                if (UDataAsset_StartUpDataBase* LoadedData = WeakThis->CharcterStartUpData.Get()) {
                    TRACE_CPUPROFILER_EVENT_SCOPE(WarriorEnemy_ApplyStartUpData);
                    // 将加载的数据应用到角色的能力系统组件
                    // 这通常包括初始属性、游戏能力、效果等配置
                    LoadedData->GiveToAbilitySystemComponent(WeakThis->WarriorAbilitySystemComponent, AbilityApplyLevel);
                    WeakThis->bStartUpDataReady = true;
                }
            }
        )
    );
}


void AWarriorEnemyCharacter::EventHandleEnemyDeath()
{
    OnEnemyDiedPooled.Broadcast(this);

    if (UWorld* World = GetWorld())
    {
        if (UWarriorObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<UWarriorObjectPoolSubsystem>())
        {
            PoolSubsystem->ReturnToPool(this);
            return;
        }
    }

    Destroy();
}


void AWarriorEnemyCharacter::OnActivateFromPool_Implementation()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(WarriorEnemy_ActivateFromPool);
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorTickEnabled(true);

    SetActorScale3D(CachedInitialScale);

    if (GetMesh())
    {
        GetMesh()->bPauseAnims = false;
        GetMesh()->GlobalAnimRateScale = 1.0f;
        GetMesh()->SetScalarParameterValueOnMaterials(FName("DissolveAmount"), 0.f);

        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetCollisionProfileName(FName("CharacterMesh"));

        GetMesh()->SetRelativeScale3D(FVector::OneVector);

        if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
        {
            AnimInst->StopAllMontages(0.f);
        }
    }

    if (UCapsuleComponent* Cap = GetCapsuleComponent())
    {
        Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Cap->SetCollisionProfileName(FName("Pawn"));
        Cap->UpdateOverlaps();
    }

    if (EnemyCombatComponent)
    {
        if (AWarriorWeaponBase* EquippedWeapon = EnemyCombatComponent->GetCharacterCurrentEquippedWeapon())
        {
            EquippedWeapon->SetActorHiddenInGame(false);
            EquippedWeapon->SetActorEnableCollision(true);
            EquippedWeapon->SetActorTickEnabled(true);

            EquippedWeapon->SetActorScale3D(FVector::OneVector);

            if (UMeshComponent* WeaponMesh = EquippedWeapon->FindComponentByClass<UMeshComponent>())
            {
                WeaponMesh->SetScalarParameterValueOnMaterials(FName("DissolveAmount"), 0.f);
            }
        }
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->SetMovementMode(MOVE_Walking);
        MoveComp->StopMovementImmediately();
        MoveComp->UpdatedComponent->SetComponentTickEnabled(true);
    }

    if (WarriorAbilitySystemComponent)
    {
        // 致命命中的后续流程可能在回池后施加破甲，复用前再清一次。
        WarriorAbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(
            FGameplayTagContainer(WarriorGameplayTags::Enemy_Status_ArmorBroken));
        WarriorAbilitySystemComponent->RemoveLooseGameplayTag(WarriorGameplayTags::Shared_Status_Dead);

        if (const UWarriorAttributeSet* WarriorAS = Cast<UWarriorAttributeSet>(WarriorAbilitySystemComponent->GetAttributeSet(UWarriorAttributeSet::StaticClass())))
        {
            WarriorAbilitySystemComponent->SetNumericAttributeBase(
                UWarriorAttributeSet::GetCurrentHealthAttribute(),
                WarriorAS->GetMaxHealth()
            );

            if (UWarriorWidgetBase* HealthWidget = Cast<UWarriorWidgetBase>(EnemyHealthWidgetComponent->GetUserWidgetObject()))
            {
                HealthWidget->InitEnemyCreatedWidget(this);
            }
        }
    }

    if (AAIController* AICon = GetController<AAIController>())
    {
		if (UBlackboardComponent* Blackboard = AICon->GetBlackboardComponent())
		{
			Blackboard->ClearValue(TEXT("TargetActor"));
		}

        if (UAIPerceptionComponent* Perception = AICon->GetPerceptionComponent())
        {
            Perception->ForgetAll();
        }

        if (AICon->BrainComponent)
        {
            AICon->BrainComponent->RestartLogic();
        }
    }
}

void AWarriorEnemyCharacter::OnDeactivateToPool_Implementation()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(WarriorEnemy_DeactivateToPool);
    if (WarriorAbilitySystemComponent)
    {
        WarriorAbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(
            FGameplayTagContainer(WarriorGameplayTags::Enemy_Status_ArmorBroken));
    }

    if (AAIController* AICon = GetController<AAIController>())
    {
        if (AICon->BrainComponent)
        {
            AICon->BrainComponent->StopLogic("Dead");
        }
        AICon->StopMovement();
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        MoveComp->SetMovementMode(MOVE_None);
    }

    if (EnemyCombatComponent)
    {
		EnemyCombatComponent->ResetCombatState();

        if (AWarriorWeaponBase* EquippedWeapon = EnemyCombatComponent->GetCharacterCurrentEquippedWeapon())
        {
            EquippedWeapon->SetActorHiddenInGame(true);
            EquippedWeapon->SetActorEnableCollision(false);
            EquippedWeapon->SetActorTickEnabled(false);
        }
    }

    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);

    OnEnemyDiedPooled.Clear();
    StopAnimMontage();
}
