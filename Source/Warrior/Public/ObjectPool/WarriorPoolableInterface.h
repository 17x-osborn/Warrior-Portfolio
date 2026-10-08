#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WarriorPoolableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UWarriorPoolableInterface : public UInterface
{
	GENERATED_BODY()
};

class WARRIOR_API IWarriorPoolableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Object Pool")
	void OnActivateFromPool();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Object Pool")
	void OnDeactivateToPool();
};
