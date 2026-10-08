// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WarriorTypes/WarriorStructTypes.h"
#include "WarriorAbilitySystemComponent.generated.h"

/**
 * 
 */
UCLASS()
class WARRIOR_API UWarriorAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	// 处理技能输入
	void OnAbilityInputPressed(const FGameplayTag& InInputTag);
	void OnAbilityInputReleased(const FGameplayTag& InInputTag);

	// 赋予武器技能
	UFUNCTION(BlueprintCallable, Category = "Warrior|Ability", meta = (ApplyLevel="1"))
	void GrantHeroWeaponAbilities(const TArray<FWarriorHeroAbilitySet>& InDefaultWeaponAbilities, const TArray< FWarrorHeroSpecialAbilitySet>& InSpecialWeaponAbilities,int32 ApplyLevel,TArray<FGameplayAbilitySpecHandle>& OutGrantedAbilitySpecHandles);
	
	// 剥夺武器技能
	UFUNCTION(BlueprintCallable, Category = "Warrior|Ability")
	void RemovedGrantedHeroWeaponAbilities(UPARAM(ref) TArray<FGameplayAbilitySpecHandle>& InSpecHandlesToRemove);

	// 根据tag激活技能 随机激活一个对应的技能
	UFUNCTION(BlueprintCallable, Category = "Warrior|Ability")
	bool TryActivateAbilityByTag(FGameplayTag AbilityTagToActivate);

protected:
	virtual void BeginPlay() override;

private:
	// A second light-attack press near the end of the current attack can start the next combo hit.
	UPROPERTY(EditDefaultsOnly, Category = "Warrior|Input Buffer", meta = (ClampMin = "0.0"))
	float LightAttackBufferWindowSeconds = 0.45f;

	FGameplayAbilitySpecHandle BufferedLightAttackHandle;
	double BufferedLightAttackExpiresAtSeconds = 0.0;
	FTimerHandle BufferedLightAttackReplayTimer;

	void HandleAbilityEnded(const FAbilityEndedData& EndedData);
	void ReplayBufferedLightAttack(FGameplayAbilitySpecHandle AbilityHandle);
	void ClearBufferedLightAttack();
};
