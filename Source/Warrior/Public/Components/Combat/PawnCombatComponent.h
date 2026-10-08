#pragma once

#include "CoreMinimal.h"
#include "Components/PawnExtensionComponentBase.h"
#include "GameplayTagContainer.h"
// generated.h要放在最后
#include "PawnCombatComponent.generated.h"

class AWarriorWeaponBase;

UENUM(BlueprintType)
enum class EToggleDamageType :uint8 {
	CurrentEquippedWeapon,
	LeftHand,
	RightHand
};

/**
 * 在combat中会注册、获取当前武器，处理武器的碰撞
 */
UCLASS()
class WARRIOR_API UPawnCombatComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()

public:
	// bRegisterAsEquippedWeapon在hero是false,敌人是true,因为生成武器后会立刻装备
	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	void RegisterSpawnedWeapon(FGameplayTag InWeaponTagToRegister, AWarriorWeaponBase* InWeaponToRegister, bool bRegisterAsEquippedWeapon = false);

	// 通过武器tag返回武器 k-v
	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	AWarriorWeaponBase* GetCharacterCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const;

	// 当前使用的武器
	UPROPERTY(BlueprintReadWrite, Category = "Warrior|Combat")
	FGameplayTag CurrentEquippedWeaponTag;

	// 返回当前使用的武器
	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	AWarriorWeaponBase* GetCharacterCurrentEquippedWeapon() const;

	// 处理武器碰撞
	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	void ToggleWeaponCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType = EToggleDamageType::CurrentEquippedWeapon);

	// 回收时关闭攻击碰撞，并清除上一轮的命中记录。
	void ResetCombatState();

	// 具体的碰撞处理 子类中重写
	virtual void OnHitTargetActor(AActor* HitActor);
	virtual void OnWeaponPulledFromTargetActor(AActor* InteractedActor);

protected:
	// 存放被击中的对象
	TArray<AActor*> OverlappedActors;

	// 处理碰撞关闭和开启
	virtual void ToggleCurrentEquippedWeaponCollision(bool bShouldEnable);
	virtual void ToggleBodyCollsionBoxCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType);

private:

	// 存储武器的结构,key-value 后期可以添加其他武器
	TMap<FGameplayTag, AWarriorWeaponBase*> CharacterCarriedWeaponMap;

	
};
