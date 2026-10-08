# Warrior：第三人称动作 RPG

UE 5.6 / C++ / Blueprint / GAS / BT / EQS / Motion Warping

[演示视频](https://www.bilibili.com/video/BV1EYk5BLEdP)

这个项目是我学习 Vince Petrelli 的 [Unreal Engine 5 C++: Advanced Action RPG](https://www.udemy.com/course/unreal-engine-5-advanced-action-rpg/) 课程后继续完善的 Demo。基础部分包括轻重连招、锁定、完美格挡、怒气、敌人 AI 和生存波次。后续主要补了输入缓冲、局内强化和敌人对象池。

课程作者的项目：[vinceright3/WarriorRPG](https://github.com/vinceright3/WarriorRPG)。代码与素材的来源说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 后续加的功能

### 轻击输入缓冲

处理上一刀还没结束时，第二次按键被吞掉的问题。

轻击正在执行时，ASC 保存一次额外输入；技能正常结束后，如果输入还没过期，就在下一次 TimerManager Tick 尝试接下一刀。默认缓冲时间为 0.45 秒，重复点击只更新一个缓存槽。被打断、超时或卸下武器时清理缓存。

代码：[WarriorAbilitySystemComponent.cpp](Source/Warrior/Private/AbilitySystem/WarriorAbilitySystemComponent.cpp)

### 波次间强化选择

每波结束、下一波开始前，从符合条件的强化中随机给出至多三项，玩家选一项。已选强化不会再次出现，有前置要求的卡需要先获得对应强化。

目前做了四项：

| 强化 | 效果 |
| --- | --- |
| 重击破甲 | 重击命中后施加限时破甲 |
| 轻击追击 | 对破甲目标的轻击额外增伤，需要先获得重击破甲 |
| 完美格挡返怒 | 完美格挡成功后额外恢复怒气 |
| 连段余裕 | 连段保留时间从 0.3 秒延长至 0.55 秒 |

卡牌配置保存在 GameMode，GameplayTag 记录本局已获得的强化。UI 动态创建卡牌，点击后把选中的 Tag 交给 GameMode 校验，强化生效后继续下一波。没有可选强化时直接推进。

代码：[WarriorSurviveGameMode.cpp](Source/Warrior/Private/GameModes/WarriorSurviveGameMode.cpp) · [破甲条件增伤](Source/Warrior/Private/AbilitySystem/GEExecCalc/GEExecCalc_DamageTaken.cpp)

### 敌人对象池

用 `UWorldSubsystem` 按敌人类型保存可复用实例，供波次刷新和敌人召唤使用。

回收时停止 AI、移动和攻击碰撞，清除命中记录与破甲效果，隐藏敌人和武器。下次取出时恢复生命、碰撞、动画和 AI，避免把上一轮的状态带回来。

还保留了对象池测试入口，用来检查状态复位，并比较复用与 Spawn/Destroy 两条路径。使用方式见 [对象池测试说明](Docs/Pool_Benchmark_Guide_CN.md)。

代码：[WarriorObjectPoolSubsystem.cpp](Source/Warrior/Private/ObjectPool/WarriorObjectPoolSubsystem.cpp) · [敌人复位与回收](Source/Warrior/Private/Characters/WarriorEnemyCharacter.cpp)

## 仓库内容

包含 `Source/`、必要的 `Config/`、`Warrior.uproject` 和对象池测试脚本。

模型、动画、地图及蓝图资源没有上传，因此这个仓库不能直接运行完整游戏。强化 UI、GE 和连段计时的一部分逻辑在蓝图中，接法整理在 [蓝图说明](Docs/BlueprintIntegration.md)。

更详细的实现过程见 [Extensions.md](Docs/Extensions.md)。
