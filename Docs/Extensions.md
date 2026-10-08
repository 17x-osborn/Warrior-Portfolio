# 扩展实现阅读指南

基础 GAS、角色、AI 与生存波次框架来自课程学习实现，下面说明后续新增与调整的逻辑。

## 轻击输入缓冲

[实现](../Source/Warrior/Private/AbilitySystem/WarriorAbilitySystemComponent.cpp) · [声明与窗口配置](../Source/Warrior/Public/AbilitySystem/WarriorAbilitySystemComponent.h)

ASC 根据动态 InputTag 找到 AbilitySpec。斧头轻击已激活时，记录同一份 Spec 的 Handle 和过期时间；未激活时沿用原激活逻辑。第二次点击通常不产生第二份 Spec，缓存保存的是已授予能力的句柄。

`HandleAbilityEnded` 接收所有能力结束通知，先比较 Handle；仅对应轻击正常结束、未超时且 Spec 不再激活时消费缓存。清理后安排下一次 TimerManager Tick 重放，让当前结束调用栈先完成。

重放重新检查 Avatar、死亡 Tag、Spec 是否存在、是否已激活及 InputTag，然后调用 `TryActivateAbility`。请求仍受 GAS 激活条件约束。

当前时间语义是在能力结束回调时判断是否过期；通过检查后，输入已消费为一次待执行请求，延迟回调不再次检查过期时间。默认窗口 0.45 秒。

新轻击输入、取消/超时的轻击结束以及武器技能撤销会清理当前缓存或 Timer。重复点击只刷新一个槽位，不积累多刀。

## 局内强化

[声明](../Source/Warrior/Public/GameModes/WarriorSurviveGameMode.h) · [实现](../Source/Warrior/Private/GameModes/WarriorSurviveGameMode.cpp)

`FWarriorSurvivalUpgradeCard` 保存强化 Tag、前置 Tag、图标、标题与描述。配置位于 GameMode 默认值数组，并非一个通用技能树系统。

`GenerateUpgradeChoices` 排除无效、重复、已拥有及未满足前置 Tag 的卡，再打乱候选、保留至多三项。`PrepareUpgradeChoices` 每轮只生成一次，UI 读取 `CurrentUpgradeChoices`，避免界面重复读取导致重新随机。

波次结束进入 `ChoosingUpgrade`，没有候选则直接继续。`TryChooseUpgrade` 校验状态、当前展示列表、已拥有与前置条件，成功后赋予英雄 Tag 并推进波次。最后一波直接进入 `AllWavesDone`。

### 破甲联动

[GEExecCalc_DamageTaken.cpp](../Source/Warrior/Private/AbilitySystem/GEExecCalc/GEExecCalc_DamageTaken.cpp) 读取攻击类型、连击数和基础伤害，并捕获攻击力、防御力及来源/目标 Tag。

只有轻击来源拥有 `Player.Upgrade.ExploitArmorBreak` 且目标拥有 `Enemy.Status.ArmorBroken` 时，才应用可配置的额外伤害倍率。

重击施加破甲、完美格挡返怒和连段重置计时属于蓝图接入。拥有 Tag 不等于自动实现效果，见 [蓝图接入说明](BlueprintIntegration.md)。

## 敌人对象复用

[对象池](../Source/Warrior/Private/ObjectPool/WarriorObjectPoolSubsystem.cpp) · [敌人生命周期](../Source/Warrior/Private/Characters/WarriorEnemyCharacter.cpp) · [战斗状态](../Source/Warrior/Private/Components/Combat/PawnCombatComponent.cpp)

对象池按 `UClass` 分桶，获取时跳过失效项，复用闲置实例或 Spawn 新实例；回收时执行停用接口，再保留或走测试的 Destroy 分支，并检查重复回收。

停用流程停止 AI/移动、关闭攻击碰撞、清除命中记录和破甲、隐藏武器并清理死亡通知委托。激活流程恢复生命、移动、碰撞、动画及 AI 目标状态。

激活时再次清理破甲用于兜底：致命命中的后续流程可能在敌人已回收后才施加效果，不能只依赖停用清理。

源码包含统计与 CPU Trace 标记，以及生命周期测试 Actor。实战耗时或 GC 改善需要另行说明测量环境和方法；本展示版不提供未经说明的性能百分比。
