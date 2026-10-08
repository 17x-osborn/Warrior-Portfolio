 // Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/WarriorAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"



AWarriorAIController::AWarriorAIController(const FObjectInitializer& ObjectInitializer) 
	// 把默认的寻路跟随组件换成支持 Detour Crowd Avoidance 的版本
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>("PathFollowingComponent"))
{

	// ---识别英雄相关配置---
	AISenseConfig_Sight=CreateDefaultSubobject<UAISenseConfig_Sight>("EnemySenseConfig_Sight");
	// 在初始化函数中设置视觉感知参数
	AISenseConfig_Sight->SightRadius = 5000.f;                    // 设置视觉检测范围为5000单位（约50米）
	AISenseConfig_Sight->LoseSightRadius = 5500.f;                // 给视野边界留出迟滞，避免目标在边缘反复获得/丢失
	AISenseConfig_Sight->PeripheralVisionAngleDegrees = 360.f;    // 周边视觉角度为360度，表示全方向无死角检测

	// 配置阵营检测规则
	AISenseConfig_Sight->DetectionByAffiliation.bDetectEnemies = true;    // 启用检测敌对阵营的角色
	AISenseConfig_Sight->DetectionByAffiliation.bDetectFriendlies = false; // 禁用检测友方阵营的角色  
	AISenseConfig_Sight->DetectionByAffiliation.bDetectNeutrals = false;   // 禁用检测中立阵营的角色

	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("EnemyPerceptionComponent");
	// 将视觉感知配置应用到感知组件中
	EnemyPerceptionComponent->ConfigureSense(*AISenseConfig_Sight);
	// 设置视觉感知为主导感知类型（优先级最高）
	EnemyPerceptionComponent->SetDominantSense(UAISenseConfig_Sight::StaticClass());
	// 绑定感知更新事件委托：当AI感知到目标更新时调用OnEnemyPerceptionUpdated函数
	EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ThisClass::OnEnemyPerceptionUpdated);

	SetGenericTeamId(FGenericTeamId(1)); // 设置AI的ID为1
}

// 判断传入对象和我方(AIcontroller)关系
ETeamAttitude::Type AWarriorAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const APawn* PawnToCheck = Cast<const APawn>(&Other);
	if (!PawnToCheck)
	{
		return ETeamAttitude::Neutral;
	}

	// 接口指针指向实现了接口的对象
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(PawnToCheck->GetController());
	if (OtherTeamAgent && OtherTeamAgent->GetGenericTeamId() < GetGenericTeamId())
	{
		return ETeamAttitude::Hostile;
	}

	return ETeamAttitude::Friendly;
}

// 设置避障
void AWarriorAIController::BeginPlay()
{
	Super::BeginPlay();

	if (UCrowdFollowingComponent* CrowdComp = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent())) {
		// 是否开启人群避障
		CrowdComp->SetCrowdSimulationState(bEnableDetourCrowdAvoidance ?ECrowdSimulationState::Enabled :ECrowdSimulationState::Disabled);

		// 根据质量等级设置避障质量
		switch (DetourCrowdAvoidanceQuality)
		{
		case 1:
			CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Low);
			break;
		case 2:
			CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Medium);
			break;
		case 3:
			CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good);
			break;
		case 4:
			CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High);
			break;
		default:
			break; 
		}

		// 设置避障组相关参数
		CrowdComp->SetAvoidanceGroup(1);    // 设置当前代理所属的避障组
		CrowdComp->SetGroupsToAvoid(1);     // 设置需要避免的其他组  避开其他敌人

		// 设置碰撞查询范围
		CrowdComp->SetCrowdCollisionQueryRange(CollisionQueryRange);
	}
}

// 感知到actor写入黑板 丢失感知清空黑板内容
 //Stimulus：这次感知结果的数据，包括是否成功感知、感知类型、位置、强度等
void AWarriorAIController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	static const FName TargetActorKey(TEXT("TargetActor"));
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return;
	}

	AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetActorKey));
	if (Stimulus.WasSuccessfullySensed() && IsValid(Actor))
	{
		if (!IsValid(CurrentTarget))
		{
			BlackboardComponent->SetValueAsObject(TargetActorKey, Actor);
		}
	}
	else if (CurrentTarget == Actor)
	{
		BlackboardComponent->ClearValue(TargetActorKey);
	}
}
