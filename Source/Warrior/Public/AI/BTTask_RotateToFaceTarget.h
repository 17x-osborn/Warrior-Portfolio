// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RotateToFaceTarget.generated.h"

struct FRotateToFaceTargetTaskMemory
{
    // 拥有该任务的角色
    TWeakObjectPtr<APawn> OwningPawn;
    // 要面对的目标角色
    TWeakObjectPtr<AActor> TargetActor;

    bool IsValid() const
    {
        return OwningPawn.IsValid() && TargetActor.IsValid();
    }
    void Reset()
    {
        OwningPawn.Reset();
        TargetActor.Reset();
    }
};
/**
 * 行为树任务：让角色旋转面向指定的目标
 */
UCLASS()
class WARRIOR_API UBTTask_RotateToFaceTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
    UBTTask_RotateToFaceTarget();

    //~ Begin UBTNode Interface
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
    virtual uint16 GetInstanceMemorySize() const override;
    virtual FString GetStaticDescription() const override;
    //~ End UBTNode Interface

    // 执行任务 - 当行为树进入这个节点时调用
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    // 每帧更新任务 - 当任务需要持续运行时调用  
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    // 判断转向在精度之内了吗
    bool HasReachedAnglePercision(APawn* QueryPawn, AActor* TargetActor) const;

protected:
    /** 角度精度：当角色面向目标的角度小于此值时认为已经面向目标 */
    UPROPERTY(EditAnywhere, Category = "Face Target")
    float AnglePrecision;

    /** 旋转插值速度：控制角色旋转的平滑度 */
    UPROPERTY(EditAnywhere, Category = "Face Target")
    float RotationInterpSpeed;

    /** 黑板键选择器：指定要面对的目标对象在黑板中的Key */
    UPROPERTY(EditAnywhere, Category = "Face Target")
    FBlackboardKeySelector InTargetToFaceKey;
};
