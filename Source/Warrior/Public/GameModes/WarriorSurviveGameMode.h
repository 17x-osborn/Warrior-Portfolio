// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/WarriorBaseGameMode.h"
#include "GameplayTagContainer.h"
#include "WarriorSurviveGameMode.generated.h"

class AWarriorEnemyCharacter;
class UTexture2D;
UENUM(BlueprintType)
enum class EWarriorSurvialGameModeState :uint8
{
	WaitSpawnNewWave,  // 每一轮的起点
	SpawningNewWave, 
	InProgress,
	WaveCompleted,
	AllWavesDone,
	PlayerDied,
	ChoosingUpgrade // 追加在末尾，保留已有枚举值的序号
};
// 同一类敌人的生成信息
USTRUCT(BlueprintType)
struct FWarriorEnemyWaveSpawnerInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<AWarriorEnemyCharacter> SoftEnemyClassToSpawn;

	UPROPERTY(EditAnywhere)
	int32 MinPerSpawnCount = 1;

	UPROPERTY(EditAnywhere)
	int32 MaxPerSpawnCount = 3;
};
// 规定数据表样式
USTRUCT(BlueprintType)
struct FWarriorEnemyWaveSpawnerTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// 单个波次中，不同敌人生成配置的数组
	UPROPERTY(EditAnywhere)
	TArray<FWarriorEnemyWaveSpawnerInfo> EnemyWaveSpawnerDefinitions;

	// 当前波次总共需要生成的敌人数量
	UPROPERTY(EditAnywhere)
	int32 TotalEnemyToSpawnThisWave = 1;
};

// 一张局内强化卡的静态配置
USTRUCT(BlueprintType)
struct FWarriorSurvivalUpgradeCard
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag UpgradeTag;

	// 卡片展示用的小图标
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 留空表示无前置条件
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag RequiredUpgradeTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = "true"))
	FText Description;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSurvialGameModeStateChanged, EWarriorSurvialGameModeState, CurrentState);
/**
 * 
 */
UCLASS()
class WARRIOR_API AWarriorSurviveGameMode : public AWarriorBaseGameMode
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay()  override;
	virtual void Tick(float  DeltaTime) override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	// 测试地图可关闭存档难度覆盖，直接使用 GameMode 蓝图的默认难度。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings")
	bool bLoadSavedDifficulty = true;
	

private:
	void SetSurvialGameModeState(EWarriorSurvialGameModeState InState);
	bool HasFinishedAllWaves() const;
	void PreLoadNextWaveEnemies();
	bool AreCurrentWaveEnemiesLoaded() const;
	FWarriorEnemyWaveSpawnerTableRow* GetCurrentWaveSpawnerTableRow() const;
	int32 TrySpawnWaveEnemies();
	bool ShouldKeepSpawnEnemies() const;
	TArray<FWarriorSurvivalUpgradeCard> GenerateUpgradeChoices(int32 MaxChoices) const;
	void PrepareUpgradeChoices();
	void AdvanceToNextWave();

	// 敌人死亡后自动调用
	//UFUNCTION()
	//void OnEnemyDestroyed(AActor*  DestroyedActor);

	UFUNCTION()
	void OnEnemyDiedFromPool(AWarriorEnemyCharacter* DeadEnemy);

	UPROPERTY()
	EWarriorSurvialGameModeState CurrentSurvialGameModeState;

	// 委托变量 放传入state参数的函数
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnSurvialGameModeStateChanged OnSurvialGameModeStateChanged;

	// 敌人波次生成用的数据表
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true")) // 即使是 private 成员也允许蓝图访问
	UDataTable * EnemyWaveSpawnerDataTable;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true"))
	int32 TotalWavesToSpawn;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true"))
	int32 CurrentWaveCount = 1;

	UPROPERTY()
	int32 CurrentSpawnedEnemiesCounter = 0;

	// 这波生成的总数
	UPROPERTY()
	int32 TotalSpawnedEnemiesThisWaveCounter = 0;
	
	UPROPERTY()
	TArray<AActor*> TargetPointsArray;

	UPROPERTY()
	float TimePassedSinceStart=0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true"))
	float SpawnNewWaveWaitTime = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true"))
	float SpawnEnemiesDelayTime = 2.f;

	UPROPERTY(EditDefaultsOnly,  BlueprintReadOnly, Category = "WaveDefinition", meta = (AllowPrivateAccess = "true"))
	float WaveCompletedWaitTime = 5.f;

	// 本局可能出现的强化卡配置；已获得的卡以英雄 ASC 上的 Tag 为准。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades", meta = (AllowPrivateAccess = "true"))
	TArray<FWarriorSurvivalUpgradeCard> UpgradeCards;

	// 仅保存这一轮实际展示的卡；UI 读取它，而不是每次打开时重新随机。
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Upgrades", meta = (AllowPrivateAccess = "true"))
	TArray<FWarriorSurvivalUpgradeCard> CurrentUpgradeChoices;

	bool bUpgradeChoicesPrepared = false;

	UPROPERTY()
	TMap< TSoftClassPtr<AWarriorEnemyCharacter>, UClass*>PreLoadedEnemyClassMap;



public:
	// 只接受当前展示的卡；成功后授予英雄 Tag 并开始下一波。
	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	bool TryChooseUpgrade(FGameplayTag SelectedUpgradeTag);

	// 注册boss召唤的敌人
	UFUNCTION(BlueprintCallable)
	void RegisterSpawnedEnemies(const TArray<AWarriorEnemyCharacter*>& InEnemiesToRegister);
		
};
