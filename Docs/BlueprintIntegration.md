# 蓝图接入说明

本仓库不包含 `.uasset`。以下补充 C++ 的蓝图接入位置，不是随仓库发布的可运行蓝图。

## 强化配置

在 `BP_SurviveGameMode` 默认值的 `UpgradeCards` 中填写 Tag、图标、标题、描述与可选前置 Tag。

| 强化 Tag | RequiredUpgradeTag |
| --- | --- |
| `Player.Upgrade.ArmorBreak` | 无 |
| `Player.Upgrade.ExploitArmorBreak` | `Player.Upgrade.ArmorBreak` |
| `Player.Upgrade.PerfectBlockRage` | 无 |
| `Player.Upgrade.ComboGrace` | 无 |

## 界面与选择

监听 `OnSurvialGameModeStateChanged`，进入 `ChoosingUpgrade` 后创建 `WBP_UpgradeSelection`，加入 Viewport，设置 UI 输入模式并显示鼠标。

选择界面遍历 `CurrentUpgradeChoices`，逐项创建 `WBP_UpgradeCard`，调用 `SetCardData` 设置标题、描述与图标，绑定卡牌选择事件并添加到 `HB_Cards`。一个 Widget 模板对应多个实例。

按钮点击时，事件分发器携带自身 `CardData.UpgradeTag`。选择界面调用 `TryChooseUpgrade`，成功才移除界面、恢复 Game Only 输入并隐藏鼠标。推进波次由 GameMode 完成，UI 不另增波次计数。

## 效果接入

- 重击破甲：命中后检查英雄的 `Player.Upgrade.ArmorBreak`，满足才给目标施加 `GE_Hero_ArmorBreak`。GE 在有效期内赋予 `Enemy.Status.ArmorBroken` 并修改防御属性。
- 轻击追击：额外伤害倍率在 C++ ExecCalc 中根据来源/目标 Tag 计算。
- 完美格挡返怒：格挡成功分支先检查完美格挡，再检查 `Player.Upgrade.PerfectBlockRage`，满足后给自身施加怒气恢复 GE。
- 连段余裕：按 `Player.Upgrade.ComboGrace` 选择轻击结束后的重置 Timer 时间，基础 0.3 秒、强化 0.55 秒；下次激活轻击时清除旧 Timer，到时重置连段计数。

连段 Timer 控制连段保留时间，与 ASC 的 0.45 秒输入缓冲是两个独立机制。

完整工程还依赖角色/动画蓝图、Montage、GA/GE、BT/EQS、波次数据表、地图和 UI 图标。恢复课程资源后，仍需配置以上新增连接。
