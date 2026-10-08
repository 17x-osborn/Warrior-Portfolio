// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.
#include "GameModes/WarriorSurviveGameMode.h"
#include "Engine/AssetManager.h"
#include "Characters/WarriorEnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Engine/TargetPoint.h"
#include "ObjectPool/WarriorObjectPoolSubsystem.h"
#include "WFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
  
void AWarriorSurviveGameMode::BeginPlay()
{
	Super::BeginPlay();
	checkf(EnemyWaveSpawnerDataTable, TEXT("Forgot to assign a valid data table in survival game mode blueprint"));
	SetSurvialGameModeState(EWarriorSurvialGameModeState::WaitSpawnNewWave);
	TotalWavesToSpawn = EnemyWaveSpawnerDataTable->GetRowNames().Num();
	PreLoadNextWaveEnemies();
}

void AWarriorSurviveGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (CurrentSurvialGameModeState == EWarriorSurvialGameModeState::WaitSpawnNewWave)
	{
		TimePassedSinceStart += DeltaTime;

		if (TimePassedSinceStart >= SpawnNewWaveWaitTime && AreCurrentWaveEnemiesLoaded())
		{
			TimePassedSinceStart = 0.f;
			SetSurvialGameModeState(EWarriorSurvialGameModeState::SpawningNewWave);
		}
		
	}

	if (CurrentSurvialGameModeState == EWarriorSurvialGameModeState::SpawningNewWave)
	{
		TimePassedSinceStart += DeltaTime;

		if (TimePassedSinceStart >= SpawnEnemiesDelayTime)
		{
			CurrentSpawnedEnemiesCounter += TrySpawnWaveEnemies();
			TimePassedSinceStart = 0.f;
			SetSurvialGameModeState(EWarriorSurvialGameModeState::InProgress);
		}
	}

	if (CurrentSurvialGameModeState == EWarriorSurvialGameModeState::WaveCompleted)
	{
		TimePassedSinceStart += DeltaTime;

		if (TimePassedSinceStart >= WaveCompletedWaitTime)
		{
			TimePassedSinceStart = 0.f;
			if (CurrentWaveCount >= TotalWavesToSpawn)
			{
				CurrentWaveCount++; // 保持原有的“最后一波后超过总波数”语义
				SetSurvialGameModeState(EWarriorSurvialGameModeState::AllWavesDone);
			}
			else
			{
				PrepareUpgradeChoices();  // 随机生成卡
				if (CurrentUpgradeChoices.IsEmpty())
				{
					AdvanceToNextWave();
				}
				else
				{
					SetSurvialGameModeState(EWarriorSurvialGameModeState::ChoosingUpgrade);
				}
			}
		}

		
	}

}

void AWarriorSurviveGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	EWarriorGameDifficulty SavedGameDifficulty;
	if (bLoadSavedDifficulty && UWFunctionLibrary::TryLoadSavedGameDifficulty(SavedGameDifficulty)) {
		CurrentGameDifficulty = SavedGameDifficulty;
	}
}

void AWarriorSurviveGameMode::PreLoadNextWaveEnemies()
{
	// 如果已经完成了所有波次，直接返回
	if (HasFinishedAllWaves())
	{
		return;
	}
	PreLoadedEnemyClassMap.Empty();

	// 遍历当前波次配置表中的所有生成信息
	for (const FWarriorEnemyWaveSpawnerInfo& SpawnerInfo : GetCurrentWaveSpawnerTableRow()->EnemyWaveSpawnerDefinitions)
	{
		// 如果敌人类型的软引用为空，则跳过
		if (SpawnerInfo.SoftEnemyClassToSpawn.IsNull()) continue;

		const TSoftClassPtr<AWarriorEnemyCharacter> SoftEnemyClass = SpawnerInfo.SoftEnemyClassToSpawn;
		const TWeakObjectPtr<AWarriorSurviveGameMode> WeakThis(this);

		// 使用弱引用回调，避免切图后异步加载访问已经销毁的 GameMode。
		UAssetManager::GetStreamableManager().RequestAsyncLoad(
			SoftEnemyClass.ToSoftObjectPath(),
			FStreamableDelegate::CreateLambda(
				[WeakThis, SoftEnemyClass]()
				{
					if (!WeakThis.IsValid())
					{
						return;
					}

					// 加载完成后，获取加载好的类
					if (UClass* LoadedEnemyClass = SoftEnemyClass.Get())
					{
						// 将加载好的类存入映射表（Map）中备用
						WeakThis->PreLoadedEnemyClassMap.Emplace(SoftEnemyClass, LoadedEnemyClass);
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("Failed to preload enemy class %s"), *SoftEnemyClass.ToString());
					}
				}
			)
		);
	}

}

// 获取当前波次敌人的生成信息
FWarriorEnemyWaveSpawnerTableRow* AWarriorSurviveGameMode::GetCurrentWaveSpawnerTableRow() const
{
	const FName RowName = FName(TEXT("Wave") + FString::FromInt(CurrentWaveCount));
	FWarriorEnemyWaveSpawnerTableRow* FoundRow = EnemyWaveSpawnerDataTable->FindRow<FWarriorEnemyWaveSpawnerTableRow>(RowName, TEXT(""));
	checkf(FoundRow, TEXT("Could not find a valid row under the name %s in the data table"), *RowName.ToString());
	return FoundRow;
}

// 生成敌人在随机位置
int32 AWarriorSurviveGameMode::TrySpawnWaveEnemies()
{
	// 确保场景中有生成点（TargetPoint）
	if (TargetPointsArray.IsEmpty())
	{
		UGameplayStatics::GetAllActorsOfClass(this, ATargetPoint::StaticClass(), TargetPointsArray);
	}
	checkf(!TargetPointsArray.IsEmpty(), TEXT("No valid target point found in level..."));

	uint32 EnemiesSpawnedThisTime = 0;

	// 设置生成配置：即使碰撞也尝试调整位置生成，确保敌人能刷出来
	FActorSpawnParameters SpawnParam;
	SpawnParam.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 获取对象池子系统
	UWarriorObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UWarriorObjectPoolSubsystem>();

	// 遍历当前波次的不同敌人类信息 来生成 SpawnerInfo是数组中其中一类敌人
	for (const FWarriorEnemyWaveSpawnerInfo& SpawnerInfo : GetCurrentWaveSpawnerTableRow()->EnemyWaveSpawnerDefinitions)
	{
		if (SpawnerInfo.SoftEnemyClassToSpawn.IsNull()) continue;
		const int32 NumToSpawn = FMath::RandRange(SpawnerInfo.MinPerSpawnCount, SpawnerInfo.MaxPerSpawnCount);
		UClass* const* LoadedEnemyClassPtr = PreLoadedEnemyClassMap.Find(SpawnerInfo.SoftEnemyClassToSpawn);
		if (!ensureMsgf(LoadedEnemyClassPtr && *LoadedEnemyClassPtr, TEXT("Enemy class %s was not preloaded"), *SpawnerInfo.SoftEnemyClassToSpawn.ToString()))
		{
			continue;
		}
		UClass* LoadedEnemyClass = *LoadedEnemyClassPtr;

		for (int32 i = 0; i < NumToSpawn; i++)
		{
			// 计算随机生成位置
			const int32 RandomTargetPointIndex = FMath::RandRange(0, TargetPointsArray.Num() - 1);
			const FVector SpawnOrigin = TargetPointsArray[RandomTargetPointIndex]->GetActorLocation();
			const FRotator SpawnRotation = TargetPointsArray[RandomTargetPointIndex]->GetActorForwardVector().ToOrientationRotator();

			FVector RandomLocation;
			// 在导航网格半径内找一个随机点
			UNavigationSystemV1::K2_GetRandomLocationInNavigableRadius(this, SpawnOrigin, RandomLocation, 400.f);
			RandomLocation += FVector(0.f, 0.f, 150.f); // 稍微抬高一点防止陷地

			// 准备 Transform，因为 SpawnOrGetFromPool 需要它
			FTransform SpawnTransform(SpawnRotation, RandomLocation);

			AWarriorEnemyCharacter* SpawnedEnemy = nullptr;

			// 优先尝试从池里捞
			if (PoolSubsystem)
			{
				SpawnedEnemy = PoolSubsystem->SpawnOrGetFromPool<AWarriorEnemyCharacter>(LoadedEnemyClass, SpawnTransform, SpawnParam);
			}
			else
			{
				SpawnedEnemy = GetWorld()->SpawnActor<AWarriorEnemyCharacter>(LoadedEnemyClass, RandomLocation, SpawnRotation, SpawnParam);
			}

			if (SpawnedEnemy)
			{
				SpawnedEnemy->OnEnemyDiedPooled.AddUniqueDynamic(this, &ThisClass::OnEnemyDiedFromPool);

				EnemiesSpawnedThisTime++;
				TotalSpawnedEnemiesThisWaveCounter++;
			}

			// 检查是否达到波次上限，如果是则停止生成
			if (!ShouldKeepSpawnEnemies())
			{
				return EnemiesSpawnedThisTime;
			}
		}
	}

	return EnemiesSpawnedThisTime;
	
}

bool AWarriorSurviveGameMode::ShouldKeepSpawnEnemies() const
{
	return TotalSpawnedEnemiesThisWaveCounter<GetCurrentWaveSpawnerTableRow()->TotalEnemyToSpawnThisWave;
}

void AWarriorSurviveGameMode::RegisterSpawnedEnemies(const TArray<AWarriorEnemyCharacter*>& InEnemiesToRegister)
{
	for (AWarriorEnemyCharacter* SpawnedEnemy : InEnemiesToRegister)
	{
		if (SpawnedEnemy)
		{
			CurrentSpawnedEnemiesCounter++;
			SpawnedEnemy->OnEnemyDiedPooled.AddUniqueDynamic(this, &ThisClass::OnEnemyDiedFromPool);
		}
	}

}

void AWarriorSurviveGameMode::SetSurvialGameModeState(EWarriorSurvialGameModeState InState)
{
	// 修改当前current并调用委托函数
	CurrentSurvialGameModeState = InState;
	OnSurvialGameModeStateChanged.Broadcast(CurrentSurvialGameModeState);
}

bool AWarriorSurviveGameMode::HasFinishedAllWaves() const
{
	return CurrentWaveCount > TotalWavesToSpawn;;
}


void AWarriorSurviveGameMode::OnEnemyDiedFromPool(AWarriorEnemyCharacter* DeadEnemy)
{
	if (CurrentSurvialGameModeState == EWarriorSurvialGameModeState::WaveCompleted ||
		CurrentSurvialGameModeState == EWarriorSurvialGameModeState::WaitSpawnNewWave ||
		CurrentSurvialGameModeState == EWarriorSurvialGameModeState::ChoosingUpgrade)
	{
		return;
	}
	CurrentSpawnedEnemiesCounter--;

	// 打印剩余敌人数量
	UE_LOG(LogTemp, Warning, TEXT("Enemy Died. Remaining: %d. Should Spawn More: %s"),
		CurrentSpawnedEnemiesCounter,
		ShouldKeepSpawnEnemies() ? TEXT("YES") : TEXT("NO"));

	if (ShouldKeepSpawnEnemies())
	{
		int32 Spawned = TrySpawnWaveEnemies();
		UE_LOG(LogTemp, Warning, TEXT("Spawned %d new enemies."), Spawned);
		CurrentSpawnedEnemiesCounter += Spawned;
	}
	else if (CurrentSpawnedEnemiesCounter == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Wave %d Completed! Entering wait state."), CurrentWaveCount);
		TotalSpawnedEnemiesThisWaveCounter = 0;
		CurrentSpawnedEnemiesCounter = 0;
		SetSurvialGameModeState(EWarriorSurvialGameModeState::WaveCompleted);
	}
}

// 波次结束 随机提供强化卡选择
TArray<FWarriorSurvivalUpgradeCard> AWarriorSurviveGameMode::GenerateUpgradeChoices(int32 MaxChoices) const
{
	TArray<FWarriorSurvivalUpgradeCard> Candidates;
	if (MaxChoices <= 0)
	{
		return Candidates;
	}

	ACharacter* Hero = UGameplayStatics::GetPlayerCharacter(this, 0);
	const UAbilitySystemComponent* HeroASC = Hero ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Hero) : nullptr;
	if (!HeroASC)
	{
		return Candidates;
	}

	TSet<FGameplayTag> SeenTags;
	for (const FWarriorSurvivalUpgradeCard& Card : UpgradeCards)
	{
		if (!Card.UpgradeTag.IsValid() || SeenTags.Contains(Card.UpgradeTag) ||
			HeroASC->HasMatchingGameplayTag(Card.UpgradeTag) ||
			(Card.RequiredUpgradeTag.IsValid() && !HeroASC->HasMatchingGameplayTag(Card.RequiredUpgradeTag)))
		{
			continue;
		}

		SeenTags.Add(Card.UpgradeTag);
		Candidates.Add(Card);
	}

	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		Candidates.Swap(Index, FMath::RandRange(Index, Candidates.Num() - 1));
	}
	Candidates.SetNum(FMath::Min(MaxChoices, Candidates.Num()));
	return Candidates;
}

void AWarriorSurviveGameMode::PrepareUpgradeChoices()
{
	if (bUpgradeChoicesPrepared)
	{
		return;
	}

	CurrentUpgradeChoices = GenerateUpgradeChoices(3);
	bUpgradeChoicesPrepared = true;
}

// 真正选择对应的卡 给hero加上对应tag
bool AWarriorSurviveGameMode::TryChooseUpgrade(FGameplayTag SelectedUpgradeTag)
{
	if (CurrentSurvialGameModeState != EWarriorSurvialGameModeState::ChoosingUpgrade || !SelectedUpgradeTag.IsValid())
	{
		return false;
	}

	const FWarriorSurvivalUpgradeCard* SelectedCard = CurrentUpgradeChoices.FindByPredicate(
		[SelectedUpgradeTag](const FWarriorSurvivalUpgradeCard& Card)
		{
			return Card.UpgradeTag == SelectedUpgradeTag;
		});
	if (!SelectedCard)
	{
		return false;
	}

	ACharacter* Hero = UGameplayStatics::GetPlayerCharacter(this, 0);
	UAbilitySystemComponent* HeroASC = Hero ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Hero) : nullptr;
	if (!HeroASC || HeroASC->HasMatchingGameplayTag(SelectedUpgradeTag) ||
		(SelectedCard->RequiredUpgradeTag.IsValid() && !HeroASC->HasMatchingGameplayTag(SelectedCard->RequiredUpgradeTag)))
	{
		return false;
	}

	UWFunctionLibrary::AddGameplayTagToActorIfNone(Hero, SelectedUpgradeTag);
	AdvanceToNextWave();
	return true;
}

void AWarriorSurviveGameMode::AdvanceToNextWave()
{
	CurrentUpgradeChoices.Reset();
	bUpgradeChoicesPrepared = false;
	CurrentWaveCount++;
	TimePassedSinceStart = 0.f;
	SetSurvialGameModeState(EWarriorSurvialGameModeState::WaitSpawnNewWave);
	PreLoadNextWaveEnemies();
}

bool AWarriorSurviveGameMode::AreCurrentWaveEnemiesLoaded() const
{
	for (const FWarriorEnemyWaveSpawnerInfo& SpawnerInfo : GetCurrentWaveSpawnerTableRow()->EnemyWaveSpawnerDefinitions)
	{
		if (!SpawnerInfo.SoftEnemyClassToSpawn.IsNull() && !PreLoadedEnemyClassMap.Contains(SpawnerInfo.SoftEnemyClassToSpawn))
		{
			return false;
		}
	}

	return true;
}
