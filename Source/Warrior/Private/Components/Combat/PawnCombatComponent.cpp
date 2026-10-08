// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/PawnCombatComponent.h"
#include "Items/Weapons/WarriorWeaponBase.h"
#include "Components/BoxComponent.h"


// 主要就是根据武器名返回武器 把武器实例和tag注册到Map中
void UPawnCombatComponent::RegisterSpawnedWeapon(FGameplayTag InWeaponTagToRegister, AWarriorWeaponBase* InWeaponToRegister, bool bRegisterAsEquippedWeapon)
{
	checkf(!CharacterCarriedWeaponMap.Contains(InWeaponTagToRegister), TEXT("A tag named %s has already been added as carried weapon"), *InWeaponTagToRegister.ToString());
	check(InWeaponToRegister);

	// 添加kv
	CharacterCarriedWeaponMap.Emplace(InWeaponTagToRegister, InWeaponToRegister);

	// 武器绑定碰撞自定义委托
	InWeaponToRegister->OnWeaponHitTarget.BindUObject(this, &ThisClass::OnHitTargetActor);
	InWeaponToRegister->OnWeaponPulledFormTarget.BindUObject(this, &ThisClass::OnWeaponPulledFromTargetActor);


	if (bRegisterAsEquippedWeapon) {
		CurrentEquippedWeaponTag = InWeaponTagToRegister;
	}
}

AWarriorWeaponBase* UPawnCombatComponent::GetCharacterCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const
{
	if (CharacterCarriedWeaponMap.Contains(InWeaponTagToGet)) {
		if (AWarriorWeaponBase* const* FoundWeapon = CharacterCarriedWeaponMap.Find(InWeaponTagToGet)) {
			return *FoundWeapon;
		}
	}
	return nullptr;
}

AWarriorWeaponBase* UPawnCombatComponent::GetCharacterCurrentEquippedWeapon() const
{
	if (!CurrentEquippedWeaponTag.IsValid()) return nullptr;

	return GetCharacterCarriedWeaponByTag(CurrentEquippedWeaponTag);
}

// 处理武器碰撞
void UPawnCombatComponent::ToggleWeaponCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType)
{
	if (ToggleDamageType == EToggleDamageType::CurrentEquippedWeapon)  // 是手持武器
	{
		ToggleCurrentEquippedWeaponCollision(bShouldEnable);
	}
	else   // 左右手就直接攻击
	{
		ToggleBodyCollsionBoxCollision(bShouldEnable, ToggleDamageType);
	}

}

void UPawnCombatComponent::ResetCombatState()
{
	if (AWarriorWeaponBase* EquippedWeapon = GetCharacterCurrentEquippedWeapon())
	{
		EquippedWeapon->GetWeaponCollisonBox()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	ToggleBodyCollsionBoxCollision(false, EToggleDamageType::LeftHand);
	ToggleBodyCollsionBoxCollision(false, EToggleDamageType::RightHand);
	OverlappedActors.Reset();
}

//ECollisionEnabled::NoCollision    // 完全无碰撞
//ECollisionEnabled::QueryOnly      // 只检测，无物理（武器常用）
//ECollisionEnabled::PhysicsOnly    // 只物理，不检测
//ECollisionEnabled::QueryAndPhysics // 既检测又物理   

void UPawnCombatComponent::OnHitTargetActor(AActor* HitActor)
{
}

void UPawnCombatComponent::OnWeaponPulledFromTargetActor(AActor* InteractedActor)
{
}

void UPawnCombatComponent::ToggleCurrentEquippedWeaponCollision(bool bShouldEnable)
{
	// 获取当前的武器
	AWarriorWeaponBase* WeaponToToggle = GetCharacterCurrentEquippedWeapon();
	check(WeaponToToggle);
	// 开启/关闭碰撞
	if (bShouldEnable) {
		WeaponToToggle->GetWeaponCollisonBox()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else {
		WeaponToToggle->GetWeaponCollisonBox()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OverlappedActors.Empty();
	}
}

void UPawnCombatComponent::ToggleBodyCollsionBoxCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType)
{

}

