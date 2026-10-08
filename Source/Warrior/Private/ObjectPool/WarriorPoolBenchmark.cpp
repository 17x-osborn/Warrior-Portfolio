#include "ObjectPool/WarriorPoolBenchmark.h"

#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "AbilitySystem/WarriorAttributeSet.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Characters/WarriorEnemyCharacter.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameplayEffect.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Items/Weapons/WarriorWeaponBase.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "ProfilingDebugging/MiscTrace.h"
#include "WarriorGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogWarriorPoolTest, Log, All);

namespace WarriorPoolTest
{
	constexpr double ActiveSeconds = 1.0;
	constexpr double IdleSeconds = 0.5;
	constexpr double StartupTimeoutSeconds = 15.0;
	const TCHAR* EnemyPath = TEXT("/Game/EnemyCharacter/Grunting/Guardian/BP_Grunting_Guardian.BP_Grunting_Guardian_C");
	const TCHAR* ArmorPath = TEXT("/Game/PlayerCharacter/GameplayEffect/GE_Hero_ArmorBreak.GE_Hero_ArmorBreak_C");

	FString Csv(const FString& Value)
	{
		return TEXT("\"") + Value.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\"");
	}

	double Percentile(TArray<double> Values, double Fraction)
	{
		if (Values.IsEmpty()) return 0.0;
		Values.Sort();
		return Values[FMath::Clamp(FMath::CeilToInt(Fraction * Values.Num()) - 1, 0, Values.Num() - 1)];
	}

	template<typename T>
	int32 CountActors(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<T> It(World); It; ++It) if (IsValid(*It)) ++Count;
		return Count;
	}

#if !UE_BUILD_SHIPPING
	FAutoConsoleCommandWithWorldAndArgs StartCommand(
		TEXT("warrior.PoolTest.Start"), TEXT("reuse|fresh|validate [batch=20] [measuredCycles=10] [warmupCycles=2]. Requires WarriorPoolBenchmarkGameMode and a fresh world."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World || !World->GetAuthGameMode<AWarriorPoolBenchmarkGameMode>())
			{
				UE_LOG(LogWarriorPoolTest, Error, TEXT("Open the dedicated benchmark GameMode first; see Pool_Benchmark_Guide_CN.md."));
				return;
			}
			if (Args.IsEmpty() || Args.Num() > 4 || WarriorPoolTest::CountActors<AWarriorPoolBenchmark>(World) > 0)
			{
				UE_LOG(LogWarriorPoolTest, Error, TEXT("Usage: warrior.PoolTest.Start reuse|fresh|validate [20] [10] [2]. Reload the map between runs."));
				return;
			}
			int32 Batch = 20, Cycles = 10, Warmup = 2;
			if ((Args.Num() > 1 && !LexTryParseString(Batch, *Args[1])) ||
				(Args.Num() > 2 && !LexTryParseString(Cycles, *Args[2])) ||
				(Args.Num() > 3 && !LexTryParseString(Warmup, *Args[3])))
			{
				UE_LOG(LogWarriorPoolTest, Error, TEXT("Batch/cycle arguments must be integers."));
				return;
			}
			AWarriorPoolBenchmark* Test = World->SpawnActor<AWarriorPoolBenchmark>();
			if (Test && !Test->StartTest(Args[0], Batch, Cycles, Warmup)) Test->Destroy();
		}));

	FAutoConsoleCommandWithWorldAndArgs StopCommand(
		TEXT("warrior.PoolTest.Stop"), TEXT("Abort the current test and write an INVALID report."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (World) for (TActorIterator<AWarriorPoolBenchmark> It(World); It; ++It) It->StopTest();
		}));
#endif
}

AWarriorPoolBenchmarkGameMode::AWarriorPoolBenchmarkGameMode()
{
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	CurrentGameDifficulty = EWarriorGameDifficulty::Easy;
}

void AWarriorPoolBenchmarkGameMode::BeginPlay()
{
	Super::BeginPlay();
#if !UE_BUILD_SHIPPING
	const FString Mode = UGameplayStatics::ParseOption(OptionsString, TEXT("PoolMode"));
	if (!Mode.IsEmpty())
	{
		AWarriorPoolBenchmark* Test = GetWorld()->SpawnActor<AWarriorPoolBenchmark>();
		const bool bQuit = UGameplayStatics::HasOption(OptionsString, TEXT("PoolQuit"));
		const bool bStarted = Test && Test->StartTest(Mode,
			UGameplayStatics::GetIntOption(OptionsString, TEXT("PoolBatch"), 20),
			UGameplayStatics::GetIntOption(OptionsString, TEXT("PoolCycles"), 10),
			UGameplayStatics::GetIntOption(OptionsString, TEXT("PoolWarmup"), 2), bQuit);
		if (!bStarted && bQuit) FPlatformMisc::RequestExitWithStatus(false, 1);
	}
	else UE_LOG(LogWarriorPoolTest, Display, TEXT("Ready. Use warrior.PoolTest.Start reuse|fresh|validate. Reload the map before each run."));
#endif
}

AWarriorPoolBenchmark::AWarriorPoolBenchmark()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

bool AWarriorPoolBenchmark::StartTest(const FString& InMode, int32 InBatchSize, int32 InMeasuredCycles, int32 InWarmupCycles, bool bInQuitOnFinish)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	Mode = InMode.ToLower();
	if (bRunning || !Cycles.IsEmpty() || GetNetMode() != NM_Standalone ||
		!GetWorld()->GetAuthGameMode<AWarriorPoolBenchmarkGameMode>() ||
		(Mode != TEXT("reuse") && Mode != TEXT("fresh") && Mode != TEXT("validate")) ||
		InBatchSize < 1 || InBatchSize > 200 || InMeasuredCycles < 1 || InMeasuredCycles > 100 ||
		InWarmupCycles < 0 || InWarmupCycles > 10)
	{
		UE_LOG(LogWarriorPoolTest, Error, TEXT("Invalid configuration. Dedicated standalone test world required; batch 1..200, measured cycles 1..100, warmup 0..10."));
		return false;
	}
	Pool = GetWorld()->GetSubsystem<UWarriorObjectPoolSubsystem>();
	if (!Pool || Pool->GetStatistics().FreshSpawns != 0 || Pool->GetIdleActorCount() != 0 ||
		WarriorPoolTest::CountActors<AWarriorEnemyCharacter>(GetWorld()) != 0 ||
		WarriorPoolTest::CountActors<AAIController>(GetWorld()) != 0 ||
		WarriorPoolTest::CountActors<AWarriorWeaponBase>(GetWorld()) != 0)
	{
		UE_LOG(LogWarriorPoolTest, Error, TEXT("The test needs a fresh map without existing enemies, weapons or AI controllers."));
		return false;
	}

	bValidateReset = Mode == TEXT("validate");
	bRetainForReuse = Mode != TEXT("fresh");
	BatchSize = bValidateReset ? 1 : InBatchSize;
	MeasuredCycles = bValidateReset ? 2 : InMeasuredCycles;
	WarmupCycles = bValidateReset ? 0 : InWarmupCycles;
	bQuitOnFinish = bInQuitOnFinish;
	const double LoadStart = FPlatformTime::Seconds();
	EnemyClass = LoadClass<AWarriorEnemyCharacter>(nullptr, WarriorPoolTest::EnemyPath);
	if (bValidateReset) ArmorBreakClass = LoadClass<UGameplayEffect>(nullptr, WarriorPoolTest::ArmorPath);
	ClassLoadMs = (FPlatformTime::Seconds() - LoadStart) * 1000.0;
	if (!EnemyClass || (bValidateReset && !ArmorBreakClass))
	{
		UE_LOG(LogWarriorPoolTest, Error, TEXT("Failed to load benchmark enemy/effect assets."));
		return false;
	}

	// A stable origin, no random navmesh queries. Tag a TargetPoint 'PoolBenchmarkOrigin' to override.
	TArray<ATargetPoint*> Origins;
	for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It) Origins.Add(*It);
	Origins.Sort([](const ATargetPoint& A, const ATargetPoint& B) { return A.GetName() < B.GetName(); });
	if (!Origins.IsEmpty())
	{
		SpawnOrigin = Origins[0]->GetActorLocation();
		for (const ATargetPoint* Origin : Origins) if (Origin->ActorHasTag(TEXT("PoolBenchmarkOrigin"))) { SpawnOrigin = Origin->GetActorLocation(); break; }
	}
	else for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It) { SpawnOrigin = It->GetActorLocation(); break; }
	SpawnOrigin.Z += 150.0;
	const FString RunId = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + TEXT("_") + Mode + TEXT("_") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8);
	RunDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Profiling/PoolBenchmark") / RunId);
	InitialPhysicalMiB = FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0);
	Cycles.Reserve(1 + WarmupCycles + MeasuredCycles);
	bRunning = true;
	Stage = EStage::Settle;
	DeadlineSeconds = FPlatformTime::Seconds() + 1.0;
	SetActorTickEnabled(true);
	TRACE_BOOKMARK(TEXT("PoolTest start mode=%s batch=%d cycles=%d"), *Mode, BatchSize, MeasuredCycles);
	if (bValidateReset)
	{
		UE_LOG(LogWarriorPoolTest, Display, TEXT("Started validate: 1 enemy, 3 reset-check cycles (not performance measurements). Output: %s"), *RunDirectory);
	}
	else
	{
		UE_LOG(LogWarriorPoolTest, Display, TEXT("Started %s: %d enemies/batch, 1 cold + %d warmup + %d measured cycles. Output: %s"), *Mode, BatchSize, WarmupCycles, MeasuredCycles, *RunDirectory);
	}
	return true;
#endif
}

const TCHAR* AWarriorPoolBenchmark::StageName(EStage InStage)
{
	switch (InStage)
	{
	case EStage::Acquire: return TEXT("acquire");
	case EStage::WaitReady: return TEXT("wait_startup");
	case EStage::HoldActive: return TEXT("active_idle_ai_disabled");
	case EStage::HoldIdle: return TEXT("released");
	default: return TEXT("settle");
	}
}

FString AWarriorPoolBenchmark::PhaseForCycle(int32 Index) const
{
	if (bValidateReset) return TEXT("validation");
	if (Index == 0) return TEXT("cold");
	return Index <= WarmupCycles ? TEXT("warmup") : TEXT("measured");
}

void AWarriorPoolBenchmark::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bRunning) return;
	const double Now = FPlatformTime::Seconds();
	if (Cycles.IsValidIndex(LastFrameCycle) && LastTickSeconds > 0.0)
	{
		const double Ms = (Now - LastTickSeconds) * 1000.0;
		Cycles[LastFrameCycle].FrameTimesMs.Add(Ms);
		Frames.Add({LastFrameCycle, StageName(LastFrameStage), Ms});
		if (Frames.Num() > 500000) { FinishRun(false, TEXT("Frame sample limit exceeded")); return; }
	}
	LastTickSeconds = Now;
	LastFrameCycle = Cycles.Num() - 1;
	LastFrameStage = Stage;
	switch (Stage)
	{
	case EStage::Settle:
		if (Now >= DeadlineSeconds) Stage = EStage::Acquire;
		break;
	case EStage::Acquire:
		AcquireBatch();
		LastFrameCycle = Cycles.Num() - 1;
		break;
	case EStage::WaitReady:
		if (AreEnemiesReady()) BeginActiveHold();
		else if (Now >= DeadlineSeconds) FinishRun(false, TEXT("Enemy startup did not become ready within 15 seconds"));
		break;
	case EStage::HoldActive:
		if (Now >= DeadlineSeconds) ReleaseBatch();
		break;
	case EStage::HoldIdle:
		if (Now >= DeadlineSeconds) FinishCycle();
		break;
	default: break;
	}
}

void AWarriorPoolBenchmark::AcquireBatch()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WarriorPoolTest_AcquireBatch);
	FWarriorPoolBenchmarkCycle& Cycle = Cycles.AddDefaulted_GetRef();
	Cycle.Phase = PhaseForCycle(Cycles.Num() - 1);
	Cycle.Before = Pool->GetStatistics();
	Enemies.Reset(BatchSize);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TRACE_BOOKMARK(TEXT("PoolTest cycle=%d phase=%s acquire"), Cycles.Num() - 1, *Cycle.Phase);
	AcquireStartSeconds = FPlatformTime::Seconds();
	for (int32 Index = 0; Index < BatchSize; ++Index)
	{
		const FVector Offset((Index % 10) * 250.0, (Index / 10) * 250.0, 0.0);
		Enemies.Add(Pool->SpawnOrGetFromPool<AWarriorEnemyCharacter>(EnemyClass, FTransform(FRotator::ZeroRotator, SpawnOrigin + Offset), Params, bRetainForReuse));
	}
	Cycle.AcquireMs = (FPlatformTime::Seconds() - AcquireStartSeconds) * 1000.0;
	for (AWarriorEnemyCharacter* Enemy : Enemies)
	{
		if (!IsValid(Enemy)) { FinishRun(false, TEXT("Acquire returned an invalid enemy")); return; }
		if (Cycles.Num() > 1 && bRetainForReuse && !PreviousBatch.Contains(TWeakObjectPtr<AWarriorEnemyCharacter>(Enemy)))
		{ FinishRun(false, TEXT("Expected the same actors to be reused")); return; }
		// Same idle workload for both modes. The lifecycle hooks themselves remain inside the timings.
		Enemy->GetCharacterMovement()->DisableMovement();
		if (AAIController* AI = Enemy->GetController<AAIController>())
		{
			AI->StopMovement();
			if (AI->BrainComponent) AI->BrainComponent->StopLogic(TEXT("PoolBenchmark"));
			if (UAIPerceptionComponent* Perception = AI->GetPerceptionComponent()) Perception->SetSenseEnabled(UAISense_Sight::StaticClass(), false);
		}
	}
	DeadlineSeconds = AcquireStartSeconds + WarriorPoolTest::StartupTimeoutSeconds;
	Stage = EStage::WaitReady;
	if (AreEnemiesReady()) BeginActiveHold();
}

bool AWarriorPoolBenchmark::AreEnemiesReady() const
{
	for (const AWarriorEnemyCharacter* Enemy : Enemies)
		if (!IsValid(Enemy) || !Enemy->IsStartUpDataReady()) return false;
	return Enemies.Num() == BatchSize;
}

FString AWarriorPoolBenchmark::ValidateActiveEnemies() const
{
	for (const AWarriorEnemyCharacter* Enemy : Enemies)
	{
		if (!IsValid(Enemy) || Enemy->IsHidden() || !Enemy->GetActorEnableCollision()) return TEXT("Acquired enemy is invalid/hidden/non-colliding");
		const UWarriorAbilitySystemComponent* ASC = Enemy->GetWarriorAbilitySystemComponent();
		const UWarriorAttributeSet* Attributes = ASC ? ASC->GetSet<UWarriorAttributeSet>() : nullptr;
		if (!Attributes || Attributes->GetMaxHealth() <= 0.0f || !FMath::IsNearlyEqual(Attributes->GetCurrentHealth(), Attributes->GetMaxHealth(), 0.01f))
			return TEXT("Acquired enemy does not have full initialized health");
		if (ASC->HasMatchingGameplayTag(WarriorGameplayTags::Shared_Status_Dead) || ASC->HasMatchingGameplayTag(WarriorGameplayTags::Enemy_Status_ArmorBroken))
			return TEXT("Dead/ArmorBroken tag leaked into an acquired enemy");
	}
	return FString();
}

void AWarriorPoolBenchmark::BeginActiveHold()
{
	Cycles.Last().ReadyMs = (FPlatformTime::Seconds() - AcquireStartSeconds) * 1000.0;
	const FString Failure = ValidateActiveEnemies();
	if (!Failure.IsEmpty()) { FinishRun(false, Failure); return; }
	Stage = EStage::HoldActive;
	DeadlineSeconds = FPlatformTime::Seconds() + WarriorPoolTest::ActiveSeconds;
}

void AWarriorPoolBenchmark::ReleaseBatch()
{
	const FString Failure = ValidateActiveEnemies();
	if (!Failure.IsEmpty()) { FinishRun(false, Failure); return; }
	PreviousBatch.Reset();
	for (AWarriorEnemyCharacter* Enemy : Enemies)
	{
		PreviousBatch.Add(TWeakObjectPtr<AWarriorEnemyCharacter>(Enemy));
		if (bValidateReset)
		{
			UWarriorAbilitySystemComponent* ASC = Enemy->GetWarriorAbilitySystemComponent();
			ASC->SetNumericAttributeBase(UWarriorAttributeSet::GetCurrentHealthAttribute(), 1.0f);
			ASC->AddLooseGameplayTag(WarriorGameplayTags::Shared_Status_Dead);
			ASC->ApplyGameplayEffectToSelf(ArmorBreakClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ASC->MakeEffectContext());
			if (!ASC->HasMatchingGameplayTag(WarriorGameplayTags::Enemy_Status_ArmorBroken))
			{ FinishRun(false, TEXT("Validation failed to apply the real ArmorBreak effect")); return; }
		}
	}
	bool bReleasedAll = true;
	const double Start = FPlatformTime::Seconds();
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(WarriorPoolTest_ReleaseBatch);
		for (AWarriorEnemyCharacter* Enemy : Enemies) bReleasedAll &= Pool->ReturnToPool(Enemy, bRetainForReuse);
	}
	Cycles.Last().ReleaseMs = (FPlatformTime::Seconds() - Start) * 1000.0;
	Cycles.Last().After = Pool->GetStatistics();
	if (!bReleasedAll) { FinishRun(false, TEXT("An enemy could not be released")); return; }
	if (bRetainForReuse)
	{
		for (const AWarriorEnemyCharacter* Enemy : Enemies)
		{
			if (!IsValid(Enemy) || !Enemy->IsHidden() || Enemy->GetActorEnableCollision() ||
				Enemy->GetWarriorAbilitySystemComponent()->HasMatchingGameplayTag(WarriorGameplayTags::Enemy_Status_ArmorBroken))
			{ FinishRun(false, TEXT("Returned actor is visible/colliding or retained ArmorBreak")); return; }
		}
	}
	Enemies.Reset(); // Do not keep strong references to destroyed actors in the control run.
	Stage = EStage::HoldIdle;
	DeadlineSeconds = FPlatformTime::Seconds() + WarriorPoolTest::IdleSeconds;
	TRACE_BOOKMARK(TEXT("PoolTest cycle=%d released"), Cycles.Num() - 1);
}

void AWarriorPoolBenchmark::FinishCycle()
{
	FWarriorPoolBenchmarkCycle& Cycle = Cycles.Last();
	Cycle.IdleActors = Pool->GetIdleActorCount();
	Cycle.LiveEnemies = WarriorPoolTest::CountActors<AWarriorEnemyCharacter>(GetWorld());
	Cycle.LiveAIControllers = WarriorPoolTest::CountActors<AAIController>(GetWorld());
	Cycle.LiveWeapons = WarriorPoolTest::CountActors<AWarriorWeaponBase>(GetWorld());
	Cycle.ProcessPhysicalMiB = FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0);
	const int32 Expected = bRetainForReuse ? BatchSize : 0;
	if (Cycle.IdleActors != Expected || Cycle.LiveEnemies != Expected || Cycle.LiveAIControllers != Expected || Cycle.LiveWeapons != Expected)
	{ FinishRun(false, TEXT("Enemy/controller/weapon counts differ from the Guardian test contract; inspect cycles.csv")); return; }
	const FWarriorPoolStatistics& Before = Cycle.Before;
	const FWarriorPoolStatistics& After = Cycle.After;
	const bool bExpectReused = bRetainForReuse && Cycles.Num() > 1;
	if (After.FreshSpawns - Before.FreshSpawns != (bExpectReused ? 0 : BatchSize) ||
		After.Reuses - Before.Reuses != (bExpectReused ? BatchSize : 0) ||
		After.Returns - Before.Returns != (bRetainForReuse ? BatchSize : 0) ||
		After.Destroys - Before.Destroys != (bRetainForReuse ? 0 : BatchSize) ||
		After.FailedAcquires != Before.FailedAcquires || After.DuplicateReturns != Before.DuplicateReturns || After.InvalidIdleEntries != Before.InvalidIdleEntries)
	{ FinishRun(false, TEXT("Lifecycle counts did not match the configured run")); return; }
	Cycle.bComplete = true;
	if (Cycles.Num() == 1 + WarmupCycles + MeasuredCycles) FinishRun(true, TEXT("All configured cycles and lifecycle checks passed"));
	else Stage = EStage::Acquire;
}

void AWarriorPoolBenchmark::FinishRun(bool bSuccess, const FString& Reason)
{
	if (!bRunning) return;
	bRunning = false;
	Stage = EStage::Finished;
	SetActorTickEnabled(false);
	const bool bReportSaved = WriteReport(bSuccess, Reason);
	UE_LOG(LogWarriorPoolTest, Display, TEXT("%s: %s. Report saved=%d: %s"), bSuccess ? TEXT("PASS") : TEXT("INVALID"), *Reason, bReportSaved, *RunDirectory);
	TRACE_BOOKMARK(TEXT("PoolTest finished success=%d"), bSuccess);
	// On abort, clean up only the test's still-active actors. Completed pooled runs retain their idle population.
	for (AWarriorEnemyCharacter* Enemy : Enemies)
		if (IsValid(Enemy) && !Enemy->IsHidden()) Pool->ReturnToPool(Enemy, false);
	Enemies.Reset();
	if (bQuitOnFinish) FPlatformMisc::RequestExitWithStatus(false, (bSuccess && bReportSaved) ? 0 : 1);
}

void AWarriorPoolBenchmark::StopTest()
{
	FinishRun(false, TEXT("Stopped manually before completion"));
}

void AWarriorPoolBenchmark::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bRunning)
	{
		bRunning = false;
		WriteReport(false, TEXT("World/test actor ended before completion"));
	}
	Super::EndPlay(EndPlayReason);
}

bool AWarriorPoolBenchmark::WriteReport(bool bSuccess, const FString& Reason)
{
	using namespace WarriorPoolTest;
	// An abort may occur before ReleaseBatch takes its snapshot.
	if (!Cycles.IsEmpty() && !Cycles.Last().bComplete && Pool)
		Cycles.Last().After = Pool->GetStatistics();
	if (!IFileManager::Get().MakeDirectory(*RunDirectory, true)) return false;
	FString CycleCsv = TEXT("cycle,phase,complete,fresh_spawns,reuses,returns,destroys,failed_acquires,duplicate_returns,invalid_idle_entries,acquire_batch_ms,ready_observed_ms,release_batch_ms,idle_actors,live_enemies,live_ai_controllers,live_weapons,process_physical_mib,frame_samples,wall_frame_p95_ms,wall_frame_max_ms\n");
	TArray<double> AcquireTimes, ReleaseTimes, ReadyTimes, FrameTimes;
	for (int32 Index = 0; Index < Cycles.Num(); ++Index)
	{
		const FWarriorPoolBenchmarkCycle& C = Cycles[Index];
		CycleCsv += FString::Printf(TEXT("%d,%s,%d,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%.6f,%.6f,%.6f,%d,%d,%d,%d,%.3f,%d,%.6f,%.6f\n"),
			Index, *C.Phase, C.bComplete, C.After.FreshSpawns - C.Before.FreshSpawns, C.After.Reuses - C.Before.Reuses,
			C.After.Returns - C.Before.Returns, C.After.Destroys - C.Before.Destroys, C.After.FailedAcquires - C.Before.FailedAcquires,
			C.After.DuplicateReturns - C.Before.DuplicateReturns, C.After.InvalidIdleEntries - C.Before.InvalidIdleEntries,
			C.AcquireMs, C.ReadyMs, C.ReleaseMs, C.IdleActors, C.LiveEnemies, C.LiveAIControllers, C.LiveWeapons, C.ProcessPhysicalMiB,
			C.FrameTimesMs.Num(), Percentile(C.FrameTimesMs, 0.95), Percentile(C.FrameTimesMs, 1.0));
		if (C.bComplete && C.Phase == TEXT("measured"))
		{
			AcquireTimes.Add(C.AcquireMs); ReleaseTimes.Add(C.ReleaseMs); ReadyTimes.Add(C.ReadyMs); FrameTimes.Append(C.FrameTimesMs);
		}
	}
	FString FrameCsv = TEXT("cycle,phase,preceding_stage,wall_frame_ms\n");
	for (const FWarriorPoolBenchmarkFrame& Frame : Frames)
		FrameCsv += FString::Printf(TEXT("%d,%s,%s,%.6f\n"), Frame.Cycle, *Cycles[Frame.Cycle].Phase, *Frame.Stage, Frame.WallMs);
	const bool bHeadless = FParse::Param(FCommandLine::Get(), TEXT("nullrhi"));
	FString Summary = TEXT("success,mode,validation_only,nullrhi,map,enemy_class,batch_size,warmup_cycles,measured_cycles_completed,acquire_p50_ms,acquire_p95_ms,release_p50_ms,release_p95_ms,ready_p95_ms,wall_frame_p95_ms,wall_frame_max_ms\n");
	Summary += FString::Printf(TEXT("%d,%s,%d,%d,%s,%s,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n"),
		bSuccess, *Mode, bValidateReset, bHeadless, *Csv(GetWorld()->GetMapName()), *Csv(EnemyClass->GetPathName()), BatchSize, WarmupCycles,
		AcquireTimes.Num(), Percentile(AcquireTimes, 0.5), Percentile(AcquireTimes, 0.95), Percentile(ReleaseTimes, 0.5), Percentile(ReleaseTimes, 0.95),
		Percentile(ReadyTimes, 0.95), Percentile(FrameTimes, 0.95), Percentile(FrameTimes, 1.0));
	const FString Metadata = FString::Printf(TEXT("success=%d\nreason=%s\nmode=%s\nengine=%s\nmap=%s\nenemy=%s\nbatch=%d\nwarmup=%d\nmeasured_requested=%d\nnullrhi=%d\nworld_type=%d\ndifficulty=Easy\norigin=%s\nactive_seconds=%.2f\nreleased_seconds=%.2f\nclass_load_ms=%.6f\ninitial_process_physical_mib=%.3f\n"),
		bSuccess, *Reason, *Mode, *FEngineVersion::Current().ToString(), *GetWorld()->GetMapName(), *EnemyClass->GetPathName(),
		BatchSize, WarmupCycles, MeasuredCycles, bHeadless, static_cast<int32>(GetWorld()->WorldType), *SpawnOrigin.ToString(), ActiveSeconds, IdleSeconds, ClassLoadMs, InitialPhysicalMiB)
		+ TEXT("\nScope: synchronous acquire/release batches and polled startup readiness; AI/perception/movement disabled equally after acquisition. Death animations and wave UI are excluded.\n")
		+ TEXT("First cycle is cold, warmup cycles are excluded from summary. Ready latency includes polling delay and only waits for startup grants, not every possible asset/shader task.\n")
		+ TEXT("Destroy time is the request/callback cost, NOT deferred GC cost. Process memory is the whole process, NOT exact pool memory. Wall-frame samples include test instrumentation and frame caps; use Insights for game-thread/GC attribution.\n")
		+ TEXT("No forced GC. Pool keeps idle actors, controllers and weapons after PASS. NullRHI runs are correctness smoke tests, not gameplay performance evidence.\n");
	const bool A = FFileHelper::SaveStringToFile(CycleCsv, *(RunDirectory / TEXT("cycles.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	const bool B = FFileHelper::SaveStringToFile(FrameCsv, *(RunDirectory / TEXT("frames.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	const bool C = FFileHelper::SaveStringToFile(Summary, *(RunDirectory / TEXT("summary.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	const bool D = FFileHelper::SaveStringToFile(Metadata, *(RunDirectory / TEXT("metadata.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	return A && B && C && D;
}
