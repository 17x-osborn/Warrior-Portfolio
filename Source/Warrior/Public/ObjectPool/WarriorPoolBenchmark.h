#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameModes/WarriorBaseGameMode.h"
#include "ObjectPool/WarriorObjectPoolSubsystem.h"
#include "WarriorPoolBenchmark.generated.h"

class AWarriorEnemyCharacter;
class UGameplayEffect;

// Kept in memory until the run ends: file IO never sits inside measured batches.
struct FWarriorPoolBenchmarkCycle
{
	FString Phase;
	double AcquireMs = 0.0;
	double ReadyMs = 0.0;
	double ReleaseMs = 0.0;
	double ProcessPhysicalMiB = 0.0;
	int32 IdleActors = 0;
	int32 LiveEnemies = 0;
	int32 LiveAIControllers = 0;
	int32 LiveWeapons = 0;
	bool bComplete = false;
	FWarriorPoolStatistics Before;
	FWarriorPoolStatistics After;
	TArray<double> FrameTimesMs;
};

struct FWarriorPoolBenchmarkFrame
{
	int32 Cycle = 0;
	FString Stage;
	double WallMs = 0.0;
};

/** Opt-in lifecycle benchmark. No death animation, combat, or automatic wave spawning. */
UCLASS(NotBlueprintable)
class WARRIOR_API AWarriorPoolBenchmark : public AActor
{
	GENERATED_BODY()
public:
	AWarriorPoolBenchmark();
	bool StartTest(const FString& InMode, int32 InBatchSize, int32 InMeasuredCycles, int32 InWarmupCycles, bool bInQuitOnFinish = false);
	void StopTest();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	enum class EStage : uint8 { Settle, Acquire, WaitReady, HoldActive, HoldIdle, Finished };
	void AcquireBatch();
	bool AreEnemiesReady() const;
	void BeginActiveHold();
	void ReleaseBatch();
	void FinishCycle();
	void FinishRun(bool bSuccess, const FString& Reason);
	bool WriteReport(bool bSuccess, const FString& Reason);
	FString ValidateActiveEnemies() const;
	FString PhaseForCycle(int32 Index) const;
	static const TCHAR* StageName(EStage InStage);

	UPROPERTY()
	TSubclassOf<AWarriorEnemyCharacter> EnemyClass;
	UPROPERTY()
	TSubclassOf<UGameplayEffect> ArmorBreakClass;
	UPROPERTY()
	TArray<TObjectPtr<AWarriorEnemyCharacter>> Enemies;
	UPROPERTY()
	TObjectPtr<UWarriorObjectPoolSubsystem> Pool;

	TSet<TWeakObjectPtr<AWarriorEnemyCharacter>> PreviousBatch;
	TArray<FWarriorPoolBenchmarkCycle> Cycles;
	TArray<FWarriorPoolBenchmarkFrame> Frames;
	FString Mode;
	FString RunDirectory;
	FVector SpawnOrigin = FVector::ZeroVector;
	int32 BatchSize = 20;
	int32 MeasuredCycles = 10;
	int32 WarmupCycles = 2;
	int32 LastFrameCycle = INDEX_NONE;
	EStage Stage = EStage::Finished;
	EStage LastFrameStage = EStage::Finished;
	double DeadlineSeconds = 0.0;
	double AcquireStartSeconds = 0.0;
	double LastTickSeconds = 0.0;
	double InitialPhysicalMiB = 0.0;
	double ClassLoadMs = 0.0;
	bool bRunning = false;
	bool bRetainForReuse = true;
	bool bValidateReset = false;
	bool bQuitOnFinish = false;
};

/** Select this via a temporary map URL; the production survival GameMode is unchanged. */
UCLASS()
class WARRIOR_API AWarriorPoolBenchmarkGameMode : public AWarriorBaseGameMode
{
	GENERATED_BODY()
public:
	AWarriorPoolBenchmarkGameMode();
protected:
	virtual void BeginPlay() override;
};
