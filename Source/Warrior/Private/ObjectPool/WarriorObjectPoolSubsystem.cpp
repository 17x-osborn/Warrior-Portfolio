#include "ObjectPool/WarriorObjectPoolSubsystem.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

AActor* UWarriorObjectPoolSubsystem::AcquireActor(UClass* ClassToSpawn, const FTransform& SpawnTransform,
	const FActorSpawnParameters& SpawnParams, bool bAllowReuse)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WarriorPool_Acquire);
	if (bAllowReuse)
	{
		FWarriorPoolableActorList* Pool = AvailableObjectsMap.Find(ClassToSpawn);
		while (Pool && !Pool->Actors.IsEmpty())
		{
			AActor* Actor = Pool->Actors.Pop();
			if (!IsValid(Actor))
			{
				++Statistics.InvalidIdleEntries;
				continue;
			}
			TRACE_CPUPROFILER_EVENT_SCOPE(WarriorPool_Reuse);
			Actor->SetActorTransform(SpawnTransform);
			IWarriorPoolableInterface::Execute_OnActivateFromPool(Actor);
			++Statistics.Reuses;
			return Actor;
		}
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(WarriorPool_FreshSpawn);
	AActor* Actor = GetWorld()->SpawnActor<AActor>(ClassToSpawn, SpawnTransform, SpawnParams);
	if (!IsValid(Actor))
	{
		++Statistics.FailedAcquires;
		return nullptr;
	}
	++Statistics.FreshSpawns;
	if (Actor->Implements<UWarriorPoolableInterface>())
	{
		IWarriorPoolableInterface::Execute_OnActivateFromPool(Actor);
	}
	return Actor;
}

bool UWarriorObjectPoolSubsystem::ReturnToPool(AActor* ActorToReturn, bool bRetainForReuse)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WarriorPool_Release);
	if (!IsValid(ActorToReturn)) return false;

	UClass* ActorClass = ActorToReturn->GetClass();
	const FWarriorPoolableActorList* ExistingPool = AvailableObjectsMap.Find(ActorClass);
	if (ExistingPool && ExistingPool->Actors.Contains(ActorToReturn))
	{
		++Statistics.DuplicateReturns;
		ensureMsgf(false, TEXT("Actor %s was returned to the pool more than once"), *ActorToReturn->GetName());
		return false;
	}

	if (ActorToReturn->Implements<UWarriorPoolableInterface>())
	{
		IWarriorPoolableInterface::Execute_OnDeactivateToPool(ActorToReturn);
	}
	else
	{
		bRetainForReuse = false;
	}

	if (!bRetainForReuse)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(WarriorPool_Destroy);
		const bool bDestroyed = ActorToReturn->Destroy();
		Statistics.Destroys += bDestroyed ? 1 : 0;
		return bDestroyed;
	}
	if (!IsValid(ActorToReturn)) return false;
	AvailableObjectsMap.FindOrAdd(ActorClass).Actors.Add(ActorToReturn);
	++Statistics.Returns;
	return true;
}

int32 UWarriorObjectPoolSubsystem::GetIdleActorCount() const
{
	int32 Count = 0;
	for (const auto& Entry : AvailableObjectsMap)
	{
		for (const AActor* Actor : Entry.Value.Actors)
		{
			Count += IsValid(Actor) ? 1 : 0;
		}
	}
	return Count;
}
