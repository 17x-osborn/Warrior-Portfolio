// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTasks/AbilityTask_WaitSpawnEnemies.h"
#include "AbilitySystemComponent.h"
#include "Engine/AssetManager.h"
#include "NavigationSystem.h"
#include "Characters/WarriorEnemyCharacter.h"

#include "WarriorDebugHelper.h"

void UAbilityTask_WaitSpawnEnemies::Activate()
{
	// 事件回调表：GenericGameplayEventCallbacks
	// 事件标签 (FGameplayTag) → 函数列表 (Delegate列表)
	FGameplayEventMulticastDelegate& Delegate =AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(CachedEventTag);
	DelegateHandle = Delegate.AddUObject(this, &ThisClass::OnGameplayEventReceived);
}

void UAbilityTask_WaitSpawnEnemies::OnDestroy(bool bInOwnerFinished)
{
	FGameplayEventMulticastDelegate& Delegate = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(CachedEventTag);
	Delegate.Remove(DelegateHandle);
	Super::OnDestroy(bInOwnerFinished);
}

UAbilityTask_WaitSpawnEnemies* UAbilityTask_WaitSpawnEnemies::WaitSpawnEnemies(UGameplayAbility* OwningAbility, FGameplayTag EventTag, TSoftClassPtr<AWarriorEnemyCharacter> SoftEnemyClassToSpawn, int32 NumToSpawn, const FVector& SpawnOrigin, float RandomSpawnRadius)
{
	// 使用NewAbilityTask模板函数创建Task实例
    // NewAbilityTask会分配内存并设置Task的所有者为传入的OwningAbility
	UAbilityTask_WaitSpawnEnemies* Node = NewAbilityTask<UAbilityTask_WaitSpawnEnemies>(OwningAbility);

	// 将传入的参数缓存到Task实例的成员变量中
	Node->CachedEventTag = EventTag;
	Node->CachedSoftEnemyClassToSpawn = SoftEnemyClassToSpawn;
	Node->CachedNumToSpawn = NumToSpawn;
	Node->CachedSpawnOrigin = SpawnOrigin;
	Node->CachedRandomSpawnRadius = RandomSpawnRadius;

	return Node;
}
// 收到event后调用
void UAbilityTask_WaitSpawnEnemies::OnGameplayEventReceived(const FGameplayEventData* InPayload)
{
	/*Debug::Print(TEXT("Gameplay Event Received"));
	EndTask();*/

	// ensure宏会在开发时在编辑器中弹出警告，但不会中断游戏执行
	if (ensure(!CachedSoftEnemyClassToSpawn.IsNull()))
	{
		// 异步加载敌人类资源（仅仅是加载到内存，还没生成到场景中）
		// UAssetManager是虚幻引擎的资源管理器
		UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
			// 将软引用转换为加载路径
			CachedSoftEnemyClassToSpawn.ToSoftObjectPath(),
			// 创建回调委托，当异步加载完成后，会触发OnEnemyClassLoaded函数
			FStreamableDelegate::CreateUObject(this, &ThisClass::OnEnemyClassLoaded)
		);
	}
	else
	{
		// 如果软引用为空，判断是否需要广播任务委托
		// ShouldBroadcastAbilityTaskDelegates()通常检查任务是否仍有效
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			// 广播"未生成"委托，传递一个空的敌人数组
			// 这是GameplayAbilitySystem中通知其他系统"任务失败/未生成敌人"的方式
			DidNotSpawn.Broadcast(TArray<AWarriorEnemyCharacter*>());
		}
		EndTask();
	}
}

// 当异步加载完成后调用的回调函数  真正生成敌人到场景
void UAbilityTask_WaitSpawnEnemies::OnEnemyClassLoaded()
{
    // 获取已加载的敌人蓝图类
    UClass* LoadedClass = CachedSoftEnemyClassToSpawn.Get();
    // 获取当前世界（关卡实例）
    UWorld* World = GetWorld();

    if (!LoadedClass || !World)
    {
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            DidNotSpawn.Broadcast(TArray<AWarriorEnemyCharacter*>());
        }
        EndTask();
        return;
    }
    // 创建一个数组来存储所有生成的敌人
    TArray<AWarriorEnemyCharacter*> SpawnedEnemies;
    // 配置Actor生成参数
    FActorSpawnParameters SpawnParam;
    // 设置碰撞处理方式：尽可能调整位置，但无论如何都要生成 这确保即使位置有轻微碰撞，敌人也能被生成出来
    SpawnParam.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (int32 i = 0; i < CachedNumToSpawn; i++)
    {
        // 计算随机生成位置 下面函数返回值存在这里面
        FVector RandomLocation;
        // 获取导航系统：用于在可到达的区域生成敌人
        UNavigationSystemV1::K2_GetRandomReachablePointInRadius(
            this,                      // 世界上下文对象
            CachedSpawnOrigin,         // 生成中心点
            RandomLocation,            // 输出：计算出的随机位置
            CachedRandomSpawnRadius    // 随机生成半径
        );

        // 在Z轴上增加150单位，避免敌人卡在地面里
        RandomLocation += FVector(0.f, 0.f, 150.f);

        const FRotator SpawnFacingRotation = AbilitySystemComponent->GetAvatarActor()->GetActorForwardVector().ToOrientationRotator();

        // 真正生成敌人Actor到场景中
        AWarriorEnemyCharacter* SpawnedEnemy = World->SpawnActor<AWarriorEnemyCharacter>(
            LoadedClass,        // 从异步加载获取的敌人类
            RandomLocation,     // 计算出的随机位置
            SpawnFacingRotation, 
            SpawnParam          // 碰撞处理等参数
        );
        // 将生成的敌人添加到数组中
        if (SpawnedEnemy) SpawnedEnemies.Add(SpawnedEnemy);
    }

    // 检查是否有敌人被成功生成
    if (SpawnedEnemies.Num() > 0 && ShouldBroadcastAbilityTaskDelegates())
    {
        // 广播"生成完成"委托，传递所有生成的敌人
        OnSpawnFinished.Broadcast(SpawnedEnemies);
    }
    else if (ShouldBroadcastAbilityTaskDelegates())
    {
        // 如果没有生成任何敌人，广播失败
        DidNotSpawn.Broadcast(TArray<AWarriorEnemyCharacter*>());
    }
    EndTask();
}