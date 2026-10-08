// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WarriorTypes/WarriorEnumTypes.h" 

// 倒计时延迟操作类 - 继承UE延迟系统
class FWarriorCountDownAction : public FPendingLatentAction
{
public:
    FWarriorCountDownAction(float InTotalCountDownTime, float InUpdateInterval,
        float& InOutRemainingTime, EWarriorCountDownActionOutput& InCountDownOutput,const FLatentActionInfo& LatentInfo)
        :bNeedToCancel(false), TotalCountDownTime(InTotalCountDownTime), UpdateInterval(InUpdateInterval),OutRemainingTime(InOutRemainingTime),
        CountDownOutput(InCountDownOutput), ExecutionFunction(LatentInfo.ExecutionFunction), OutputLink(LatentInfo.Linkage),
        CallbackTarget(LatentInfo.CallbackTarget), ElapsedInterval(0.f), ElapsedTimeSinceStart(0.f)
    {

    }
    // 每帧调用 做实际操作
    virtual void UpdateOperation(FLatentResponse& Response) override;
    void CancelAction();

private:
    bool bNeedToCancel;              // 取消标志位
    float TotalCountDownTime;        // 总倒计时时间
    float UpdateInterval;            // 更新间隔
    float& OutRemainingTime;         // 输出：剩余时间
    EWarriorCountDownActionOutput& CountDownOutput;  // 输出：执行流程控制

    // UE延迟系统所需参数
    FName ExecutionFunction;         // 完成时调用的函数名
    int32 OutputLink;                // 输出引脚索引
    FWeakObjectPtr CallbackTarget;   // 回调目标对象（弱引用）

    // 计时状态
    float ElapsedInterval;           // 小间隔计时器，用来控制多久更新一次倒计时的显示/通知。
    float ElapsedTimeSinceStart;     // 倒计时开始后的总时间
};