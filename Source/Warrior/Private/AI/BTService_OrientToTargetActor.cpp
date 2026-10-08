// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_OrientToTargetActor.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Kismet/KismetMathLibrary.h"

// 构造函数 初始化属性
UBTService_OrientToTargetActor::UBTService_OrientToTargetActor() 
{
	NodeName = TEXT("Native Orient Rotation To Target Actor");

	// 在这些特定时刻调用对应成员函数
	// TickNode - 每帧执行 OnBecomeRelevant - 开始生效时 OnCeaseRelevant - 停止生效时 OnSearchStart - 搜索开始时
	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	// 设置默认的旋转插值速度为 5.0（中等速度）
	RotationInterpSpeed = 5.f;
	// 10 Hz 足以保持平滑转向，也避免每帧运行 Service。
	Interval = 0.1f;
	// 固定间隔，便于稳定调试。
	RandomDeviation = 0.f;
	// 配置黑板键选择器的过滤器，限制只能选择AActor类型的对象
	InTargetActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(ThisClass, InTargetActorKey),
		AActor::StaticClass()
	);
}

// 从资源初始化服务
// 当行为树资源被加载时调用，用于初始化服务所需的依赖项
void UBTService_OrientToTargetActor::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// 获取行为树使用的黑板数据资源
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		// 解析选中的键
		// 确保 InTargetActorKey 指向的黑板键在实际的黑板数据中存在且有效
		InTargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}
//  获取服务的静态描述
FString UBTService_OrientToTargetActor::GetStaticDescription() const
{
	// 获取actor键的名称
	const FString KeyDescription = InTargetActorKey.SelectedKeyName.ToString();
	return FString::Printf(TEXT("Orient rotation to %s Key %s"),*KeyDescription,*GetStaticServiceDescription());
}


void UBTService_OrientToTargetActor::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	// AI敌人转向目标
	UObject* ActorObject=OwnerComp.GetBlackboardComponent()->GetValueAsObject(InTargetActorKey.SelectedKeyName);  // 获取value
	AActor* TargetActor = Cast<AActor>(ActorObject);
	APawn* OwningPawn = OwnerComp.GetAIOwner()->GetPawn();

	if (OwningPawn && TargetActor)
	{
		// 计算"看向"旋转：从OwningPawn位置指向TargetActor位置的方向
		const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(
			OwningPawn->GetActorLocation(),
			TargetActor->GetActorLocation()  
		);

		// 使用插值平滑过渡到目标旋转
		const FRotator TargetRot = FMath::RInterpTo(
			OwningPawn->GetActorRotation(),  // 当前旋转
			LookAtRot,                       // 目标旋转  
			DeltaSeconds,                    // 帧时间间隔
			RotationInterpSpeed              // 插值速度
		);

		// 应用计算出的旋转到角色
		OwningPawn->SetActorRotation(TargetRot);
	}


}
