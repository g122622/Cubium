# 红石电路深入测试

`block_behavior/src/tests/redstone/*CircuitTests.ts` 注册 `redstone_deep_*`，涵盖活塞推动与拉回、黏性分支、二极管延迟和模拟运算、铁轨有界传电与矿车检测、红石灯和火把时序、线路衰减及动态断接。

行为依据为截至 Java 1.21.11 的 Wiki 和本地源码。用例使用 Cubium 状态名及 `setBlockWithStates`，不要求在 BDS 上运行。共享工具位于 `tests/integrated/utils/block/redstone.ts`，共享结构 `redstone_lab` 为 24×8×16 空气实验区和玻璃底板；放置坐标从相对 y=2 起，不依赖结构外地形。

从仓库根目录构建后，仅运行本组：

```powershell
node tests/integrated/build.mjs
& ./build/bin/RelWithDebInfo/minecraft-server.exe --gametest --profiler_enabled=false `
  '--gametest_tests=redstone_deep_*' `
  "--gametest_report=$((Get-Location).Path)/build/redstone-report.xml" `
  "--gametest_world=$((Get-Location).Path)/build/redstone-world"
```

报告和世界路径使用仓库内绝对路径，避免相对路径被解析到游戏数据目录。用例按组件分批；精确时序断言基于先执行世界计划刻、后执行测试回调的调度顺序。

