#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WarriorPoolableInterface.h"
#include "WarriorObjectPoolSubsystem.generated.h"

struct FWarriorPoolStatistics
{
	int64 FreshSpawns = 0;
	int64 Reuses = 0;
	int64 FailedAcquires = 0;
	int64 Returns = 0;
	int64 Destroys = 0;
	int64 DuplicateReturns = 0;
	int64 InvalidIdleEntries = 0;
};

// UHT requires a reflected wrapper for the map's array values.
USTRUCT()
struct FWarriorPoolableActorList
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<AActor*> Actors;
};

UCLASS()
class WARRIOR_API UWarriorObjectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	template <typename T>
	T* SpawnOrGetFromPool(UClass* ClassToSpawn, const FTransform& SpawnTransform, FActorSpawnParameters SpawnParams = FActorSpawnParameters(), bool bAllowReuse = true)
	{
		if (!ClassToSpawn || !ClassToSpawn->IsChildOf(T::StaticClass()))
		{
			++Statistics.FailedAcquires;
			return nullptr;
		}
		return Cast<T>(AcquireActor(ClassToSpawn, SpawnTransform, SpawnParams, bAllowReuse));
	}

	// Both modes deactivate actors; the benchmark's fresh mode then destroys them.
	bool ReturnToPool(AActor* ActorToReturn, bool bRetainForReuse = true);
	const FWarriorPoolStatistics& GetStatistics() const { return Statistics; }
	int32 GetIdleActorCount() const;

private:
	AActor* AcquireActor(UClass* ClassToSpawn, const FTransform& SpawnTransform, const FActorSpawnParameters& SpawnParams, bool bAllowReuse);
	FWarriorPoolStatistics Statistics;

	UPROPERTY()
	TMap<UClass*, FWarriorPoolableActorList> AvailableObjectsMap;
};
