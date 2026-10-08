// Fill out your copyright notice in the Description page of Project Settings.


#include "WarriorTypes/WarriorCountDownAction.h"

void FWarriorCountDownAction::UpdateOperation(FLatentResponse& Response)
{
    if (bNeedToCancel)
    {
        // 设置输出为"取消"状态
        CountDownOutput = EWarriorCountDownActionOutput::Canceled;
        // 完成当前任务并触发后续节点
        Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
        return; 
    }
    if (ElapsedTimeSinceStart >= TotalCountDownTime)  
    {
        // 设置输出为"完成"状态
        CountDownOutput = EWarriorCountDownActionOutput::Completed;
        // 完成当前任务并触发后续节点
        Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
        return; 
    }
    // 检查是否达到更新时间间隔
    if (ElapsedInterval < UpdateInterval)
    {
        // 还没到更新时间，累计经过的时间
        ElapsedInterval += Response.ElapsedTime();  // 增加经过的时间
    }
    else
    {
        // 已经达到更新时间间隔，执行更新操作
        
        // 增加总经过时间  给予灵活性 若updateinterval是负数 表示使用实时时间精确计时
        ElapsedTimeSinceStart += UpdateInterval > 0.f ? UpdateInterval : Response.ElapsedTime();

        // 计算剩余时间
        OutRemainingTime = TotalCountDownTime - ElapsedTimeSinceStart;

        // 设置输出为"已更新"状态，触发连接的下一个节点（但不结束当前任务）
        CountDownOutput = EWarriorCountDownActionOutput::Updated;
        Response.TriggerLink(ExecutionFunction, OutputLink, CallbackTarget);

        // 重置间隔计时器
        ElapsedInterval = 0.f;
    }

}

void FWarriorCountDownAction::CancelAction()
{
	bNeedToCancel = true;
}
