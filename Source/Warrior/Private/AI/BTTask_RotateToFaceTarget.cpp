// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_RotateToFaceTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Kismet/KismetMathLibrary.h"

UBTTask_RotateToFaceTarget::UBTTask_RotateToFaceTarget()
{
    NodeName = TEXT("Native Rotate to Face Target Actor"); // 节点在行为树编辑器中显示的名称

    // 设置默认参数
    AnglePrecision = 10.f;    // 默认角度精度：10.5度
    RotationInterpSpeed = 5.f; // 默认旋转插值速度

    // 行为树节点配置
    bNotifyTick = true;           // 启用Tick通知，任务需要每帧更新
    bNotifyTaskFinished = true;   // 启用任务完成通知
    bCreateNodeInstance = false;  // 不为每个AI实例创建单独的节点实例

    // 初始化任务节点通知标志
    INIT_TASK_NODE_NOTIFY_FLAGS();

    // 配置黑板键过滤器：只允许选择Actor类型的Key
    InTargetToFaceKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_RotateToFaceTarget, InTargetToFaceKey), AActor::StaticClass());
}

/**
 * 获取任务实例内存大小
 * 行为树系统使用此函数为每个任务实例分配内存
 */
uint16 UBTTask_RotateToFaceTarget::GetInstanceMemorySize() const
{
    return sizeof(FRotateToFaceTargetTaskMemory);
}

/**
 * 获取任务的静态描述
 * 在行为树编辑器中显示任务的配置信息
 */
FString UBTTask_RotateToFaceTarget::GetStaticDescription() const
{
    const FString KeyDescription = InTargetToFaceKey.SelectedKeyName.ToString();
    return FString::Printf(
        TEXT("Smoothly rotates to face target from %s key until angle precision: %s degrees is reached"),
        *KeyDescription,
        *FString::SanitizeFloat(AnglePrecision)
    );
}


EBTNodeResult::Type UBTTask_RotateToFaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 从黑板获取目标对象
    UObject* ActorObject = OwnerComp.GetBlackboardComponent()->GetValueAsObject(InTargetToFaceKey.SelectedKeyName);
    AActor* TargetActor = Cast<AActor>(ActorObject);
    // 获取被AI控制器控制的角色实体
    APawn* OwningPawn = OwnerComp.GetAIOwner()->GetPawn();
    // 获取任务内存并初始化  放内容的结构体
    FRotateToFaceTargetTaskMemory* Memory = CastInstanceNodeMemory<FRotateToFaceTargetTaskMemory>(NodeMemory);
    check(Memory);
    // 获取的内容放入结构体对应成员
    Memory->OwningPawn = OwningPawn;
    Memory->TargetActor = TargetActor;

    if (!Memory->IsValid())
    {
        return EBTNodeResult::Failed;
    }
    if (HasReachedAnglePercision(OwningPawn, TargetActor))
    {
        Memory->Reset();
        return EBTNodeResult::Succeeded;
    }
    return EBTNodeResult::InProgress;
}

void UBTTask_RotateToFaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // 获取任务内存并转换为自定义的内存结构
    FRotateToFaceTargetTaskMemory* Memory = CastInstanceNodeMemory<FRotateToFaceTargetTaskMemory>(NodeMemory);

    if (!Memory->IsValid())
    {
        // 数据无效，终止任务并返回失败
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    if (HasReachedAnglePercision(Memory->OwningPawn.Get(), Memory->TargetActor.Get()))
    {
        Memory->Reset();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }else{
        // 计算从角色位置指向目标位置的旋转
        const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(
            Memory->OwningPawn->GetActorLocation(),
            Memory->TargetActor->GetActorLocation()
        );

        // 使用插值平滑旋转到目标朝向
        const FRotator TargetRot = FMath::RInterpTo(
            Memory->OwningPawn->GetActorRotation(), // 当前旋转
            LookAtRot,                              // 目标旋转
            DeltaSeconds,                           // 帧时间间隔
            RotationInterpSpeed                     // 插值速度
        );

        // 应用新的旋转到角色
        Memory->OwningPawn->SetActorRotation(TargetRot);
    }

}

bool UBTTask_RotateToFaceTarget::HasReachedAnglePercision(APawn* QueryPawn, AActor* TargetActor) const
{
    // 获取角色朝向
    const FVector OwnerForward = QueryPawn->GetActorForwardVector();

    // 计算指向目标的单位方向向量
    const FVector OwnerToTargetNormalized = (TargetActor->GetActorLocation() - QueryPawn->GetActorLocation()).GetSafeNormal();

    // 计算角度差
    const float DotResult = FVector::DotProduct(OwnerForward, OwnerToTargetNormalized);
    const float AngleDiff = UKismetMathLibrary::DegAcos(DotResult);

    // 检查是否达到精度
    return AngleDiff <= AnglePrecision;
}

/**
 * 从资源初始化任务
 * 在行为树加载时调用，用于解析黑板键引用
 */
void UBTTask_RotateToFaceTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    // 获取行为树使用的黑板资源并解析选择的键
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        InTargetToFaceKey.ResolveSelectedKey(*BBAsset);
    }
}
