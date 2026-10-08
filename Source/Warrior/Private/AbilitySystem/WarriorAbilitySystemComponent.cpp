// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/WarriorHeroGameplayAbility.h"
#include "WarriorGameplayTags.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "TimerManager.h"

void UWarriorAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
}

void UWarriorAbilitySystemComponent::OnAbilityInputPressed(const FGameplayTag& InInputTag)
{
	if (!InInputTag.IsValid()) return;
	const bool bIsLightAttackInput = InInputTag.MatchesTagExact(WarriorGameplayTags::InputTag_LightAttack_Axe);
	if (bIsLightAttackInput)
	{
		ClearBufferedLightAttack();
	}
	
	// 可能一个tag 对应多个spec
	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate; // 本次需要激活的技能 Handle
	TArray<FGameplayAbilitySpecHandle> AbilitiesToCancel;   // 本次需要取消的技能 Handle
	FGameplayAbilitySpecHandle ActiveLightAttackHandle;

	{
		ABILITYLIST_SCOPE_LOCK();  // 锁住技能列表

		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InInputTag))
			{
				if (bIsLightAttackInput && AbilitySpec.IsActive())  // 如果是激活状态的轻击 记录handle 后面用于再激活
				{
					ActiveLightAttackHandle = AbilitySpec.Handle;
				}
				else if (InInputTag.MatchesTag(WarriorGameplayTags::InputTag_Toggleable) && AbilitySpec.IsActive()) // 判断是不是toggle技能
				{
					AbilitiesToCancel.Add(AbilitySpec.Handle);
				}
				else
				{
					AbilitiesToActivate.Add(AbilitySpec.Handle);
				}
			}
		}
	}

	if (ActiveLightAttackHandle.IsValid() && LightAttackBufferWindowSeconds > 0.0f)
	{
		BufferedLightAttackHandle = ActiveLightAttackHandle;
		BufferedLightAttackExpiresAtSeconds = FPlatformTime::Seconds() + LightAttackBufferWindowSeconds;  // 缓存期
	}

	// 先处理取消
	for (const FGameplayAbilitySpecHandle& Handle : AbilitiesToCancel)
	{
		CancelAbilityHandle(Handle);
	}

	// 再处理激活
	for (const FGameplayAbilitySpecHandle& Handle : AbilitiesToActivate)
	{
		TryActivateAbility(Handle);
	}
}

void UWarriorAbilitySystemComponent::HandleAbilityEnded(const FAbilityEndedData& EndedData)  // 这个回调会收到所有技能的结束通知
{
	if (EndedData.AbilitySpecHandle != BufferedLightAttackHandle)  // 结束的不是缓存的轻击技能
	{
		return;
	}

	if (EndedData.bWasCancelled || FPlatformTime::Seconds() > BufferedLightAttackExpiresAtSeconds) // 取消或超时
	{
		ClearBufferedLightAttack();
		return;
	}

	if (const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(EndedData.AbilitySpecHandle))
	{
		if (Spec->IsActive())
		{
			return;
		}
	}

	// 复制一次 把缓存清除 防止下一次缓存技能再消费
	const FGameplayAbilitySpecHandle HandleToReplay = BufferedLightAttackHandle;
	ClearBufferedLightAttack();

	if (UWorld* World = GetWorld())
	{
		BufferedLightAttackReplayTimer = World->GetTimerManager().SetTimerForNextTick(  // 下一帧执行
			FTimerDelegate::CreateUObject(this, &ThisClass::ReplayBufferedLightAttack, HandleToReplay));
	}
}

// 激活输入缓冲技能
void UWarriorAbilitySystemComponent::ReplayBufferedLightAttack(FGameplayAbilitySpecHandle AbilityHandle)
{
	BufferedLightAttackReplayTimer.Invalidate();  // 预约开始执行了 把当前作废
	if (!GetAvatarActor() || HasMatchingGameplayTag(WarriorGameplayTags::Shared_Status_Dead))
	{
		return;
	}

	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	if (!Spec || Spec->IsActive() ||
		!Spec->GetDynamicSpecSourceTags().HasTagExact(WarriorGameplayTags::InputTag_LightAttack_Axe))
	{
		return;
	}

	TryActivateAbility(AbilityHandle);
}

void UWarriorAbilitySystemComponent::ClearBufferedLightAttack()
{
	BufferedLightAttackHandle = FGameplayAbilitySpecHandle();
	BufferedLightAttackExpiresAtSeconds = 0.0;
	if (BufferedLightAttackReplayTimer.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(BufferedLightAttackReplayTimer);
		}
		BufferedLightAttackReplayTimer.Invalidate();
	}
}

void UWarriorAbilitySystemComponent::OnAbilityInputReleased(const FGameplayTag& InInputTag)
{
	// 检查输入标签是否有效，并且是否匹配按下激活松开取消
	if (!InInputTag.IsValid() || !InInputTag.MatchesTag(WarriorGameplayTags::InputTag_MustBeHeld))
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> AbilitiesToCancel;
	{
		ABILITYLIST_SCOPE_LOCK();

		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InInputTag) && AbilitySpec.IsActive())
			{
				AbilitiesToCancel.Add(AbilitySpec.Handle);
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitiesToCancel)
	{
		CancelAbilityHandle(Handle);
	}
}

// 把武器技能注册到ASC中 OutGrantedAbilitySpecHandles: 用于返回授予成功的技能句柄
void UWarriorAbilitySystemComponent::GrantHeroWeaponAbilities(const TArray<FWarriorHeroAbilitySet>& InDefaultWeaponAbilities, const TArray< FWarrorHeroSpecialAbilitySet>& InSpecialWeaponAbilities,int32 ApplyLevel, TArray<FGameplayAbilitySpecHandle>& OutGrantedAbilitySpecHandles)
{
	if (InDefaultWeaponAbilities.IsEmpty()) return;
	for (const FWarriorHeroAbilitySet& AbilitySet : InDefaultWeaponAbilities) {
		if (!AbilitySet.IsValid()) continue;
		FGameplayAbilitySpec AbilitySpec(AbilitySet.AbilityToGrant);
		AbilitySpec.SourceObject = GetAvatarActor();
		AbilitySpec.Level = ApplyLevel;
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilitySet.InputTag);
		// AddUnique: 将句柄添加到输出数组，避免重复
		// GiveAbility() 将技能添加到 AbilitySystemComponent 的激活技能列表中
		// 返回的 FGameplayAbilitySpecHandle 用于后续引用这个技能
		OutGrantedAbilitySpecHandles.AddUnique(GiveAbility(AbilitySpec));
	}
	for (const FWarrorHeroSpecialAbilitySet& AbilitySet : InSpecialWeaponAbilities) {
		if (!AbilitySet.IsValid()) continue;
		FGameplayAbilitySpec AbilitySpec(AbilitySet.AbilityToGrant);
		AbilitySpec.SourceObject = GetAvatarActor();
		AbilitySpec.Level = ApplyLevel;
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilitySet.InputTag);
		// AddUnique: 将句柄添加到输出数组，避免重复
		// GiveAbility() 将技能添加到 AbilitySystemComponent 的激活技能列表中
		// 返回的 FGameplayAbilitySpecHandle 用于后续引用这个技能
		OutGrantedAbilitySpecHandles.AddUnique(GiveAbility(AbilitySpec));
	}

}
// 把武器技能从ASC中删除 
void UWarriorAbilitySystemComponent::RemovedGrantedHeroWeaponAbilities(UPARAM(ref)TArray<FGameplayAbilitySpecHandle>& InSpecHandlesToRemove)
{
	if (InSpecHandlesToRemove.IsEmpty()) return;
	ClearBufferedLightAttack();
	for (const FGameplayAbilitySpecHandle& SpecHandle : InSpecHandlesToRemove) {
		if (SpecHandle.IsValid()) {
			ClearAbility(SpecHandle);
		}
	}
	InSpecHandlesToRemove.Empty();
}
// 根据tag激活技能 随机激活一个对应的技能
bool UWarriorAbilitySystemComponent::TryActivateAbilityByTag(FGameplayTag AbilityTagToActivate)
{
	check(AbilityTagToActivate.IsValid());
	// 存储找到的技能规格的数组
	TArray<FGameplayAbilitySpec*> FoundAbilitySpecs;
	// 根据标签查找所有匹配的技能规格
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTagToActivate.GetSingleTagContainer(), FoundAbilitySpecs);

	// 如果找到了匹配的技能
	if (!FoundAbilitySpecs.IsEmpty())
	{
		// 随机选择一个技能索引（从0到技能数量-1）
		const int32 RandomAbilityIndex = FMath::RandRange(0, FoundAbilitySpecs.Num() - 1);
		// 获取要激活的技能规格指针
		FGameplayAbilitySpec* SpecToActivate = FoundAbilitySpecs[RandomAbilityIndex];
		// 确保技能规格有效
		check(SpecToActivate);
		// 如果该技能当前未激活
		if (!SpecToActivate->IsActive())
		{
			// 尝试激活该技能并返回结果
			return TryActivateAbility(SpecToActivate->Handle);
		}
	}
	// 如果没有找到技能或激活失败，返回false
	return false;
}
