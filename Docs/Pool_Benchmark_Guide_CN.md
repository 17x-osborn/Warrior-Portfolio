# 对象池生命周期测试

入口依赖完整项目的测试地图、Guardian 蓝图与破甲 GE，源码展示版缺少资源，不能独立运行。

## 范围

- `validate`：检查生命、死亡 Tag 与破甲残留的清理。
- `reuse`：批量生成、回收后复用同一批实例。
- `fresh`：执行相同停用回调后 Destroy，下轮重新 Spawn。

记录生命周期次数、批次耗时、帧时间、进程内存与敌人/AI/武器数量，在结束时写 CSV，可选生成 CPU Trace。检查通过不代表完整战斗状态全部验证，也不能直接推导实战帧率或 GC 改善。

## 独立进程

先构建当前 C++，保存并关闭编辑器。在完整项目根目录执行，替换为实际引擎路径：

```powershell
.\Tools\Run-PoolBenchmark.ps1 -EngineRoot 'D:\YourUnreal\UE_5.6' -Mode validate -Headless -WorkspaceCache
```

`-Headless` 仅用于逻辑检查，不用于性能结论。`validate` 固定使用一只敌人进行三次获取/回收。

对照比较启用渲染，每次独立进程：

```powershell
.\Tools\Run-PoolBenchmark.ps1 -EngineRoot 'D:\YourUnreal\UE_5.6' -Mode reuse -Batch 20 -Cycles 10 -Warmup 2 -WorkspaceCache
.\Tools\Run-PoolBenchmark.ps1 -EngineRoot 'D:\YourUnreal\UE_5.6' -Mode fresh -Batch 20 -Cycles 10 -Warmup 2 -WorkspaceCache
```

保持机器负载、地图、摄像机、画质、分辨率、帧率限制及 Trace 开关一致，交替重复运行。第一次资源准备耗时不能当作稳定复用收益。

报告在 `Saved/Profiling/PoolBenchmark/`，日志在 `Saved/Logs/`，均不上传。

## PIE 观察

完整项目的单机 PIE 控制台：

```text
open /Game/Maps/GameModeTestMap?game=/Script/Warrior.WarriorPoolBenchmarkGameMode
warrior.PoolTest.Start validate
```

每次对照前重载地图，避免沿用上次的闲置池。PIE 观察不代替独立进程最终测量。
