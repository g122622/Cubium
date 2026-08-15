# 集成测试（GameTest）框架

Cubium 的集成测试基于 Minecraft 基岩版官方的 **GameTest 框架**（`@minecraft/server-gametest`）。用例以行为包形式编写，跑在真实的服务端进程里，能验证实体 AI、方块交互、命令等端到端行为。

本文涵盖三部分：
1. **写/跑 GameTest 用例** —— `tests/integrated` 的组织方式与构建。
2. **Cubium 无头跑 GameTest** —— `--gametest` 模式与 JUnit XML 报告。
3. **与官方基岩 BDS 自动对比** —— `scripts/test/run_diff.ts` 工具，以官方基岩为 ground truth 输出 Cubium 缺陷报告。

> TypeScript 化的背景与迁移细节见 [INTEGRATED_TEST_MIGRATION_TO_TS.md](./INTEGRATED_TEST_MIGRATION_TO_TS.md)。单元测试（C++ 侧 Catch2）见 [UNIT_TEST.md](./UNIT_TEST.md)。

---

## 1. 用例组织与构建

### 目录结构

`tests/integrated/` 下每个子目录是一个**独立行为包**，4 个包按主题划分：

| 包目录 | manifest name | 主题 |
|---|---|---|
| `starter` | Cubium Starter GameTests | 入门示例（`simpleMobTest`、`ExampleTests:alwaysSucceed` 内置） |
| `mob_behavior` | Cubium Mob Behavior GameTests | 生物 AI（僵尸追村民、铁傀儡竞技场、Zoglin 浮空等） |
| `command` | Cubium Command GameTests | 命令（`cloneBlocksCommand` 等） |
| `challenge` | Cubium Challenge GameTests | 综合挑战（`collapsing`、`minibiomes` 等） |

```
tests/integrated/
├── build.mjs              # 构建脚本（复制共享 utils 到各包 src/，再 tsc 编译）
├── package.json           # devDependencies: typescript + @minecraft 类型包
├── tsconfig.base.json
├── utils/                 # 共享工具源码（构建期复制进各包 src/utils/）
├── starter/               # 每个包：manifest.json + src/ + tsconfig.json + structures/
├── mob_behavior/
├── command/
└── challenge/
```

**关键约束**：行为包之间是独立模块，**不能跨包 import**。多个包需要的共享工具（`utils/`）由 `build.mjs` 在构建期复制到每个包的 `src/utils/` 下，再由各包 tsconfig 独立编译。这样每个包产出自包含的 `scripts/`（含 utils）。

### 构建

```bash
cd tests/integrated
node build.mjs          # 编译所有包（src/*.ts → scripts/*.js）
node build.mjs --clean  # 清理 scripts/ 与复制的 src/utils/
```

首次跑或 clone 后需先 `npm install`（`node_modules` 不入 git）。CMake 构建期也会自动调用 `build.mjs`（见 `src/server/test/facade` 的 `add_custom_command`）。

### 写一个 GameTest

用例注册形式（以 `mob_behavior` 为例）：

```typescript
import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// Shulker 的攻击会使 Zoglin 浮空至笼顶。
function zoglinFloat(test: Test): void {
  test.spawn("zoglin", { x: 5, y: 2, z: 5 });
  test.spawn("shulker", { x: 2, y: 2, z: 2 });

  test.succeedWhen(() => {
    // zoglin 是否已浮至笼顶？
    assertEntityInVolume(test, "zoglin", 1, 7, 1, 10, 10, 10);
  });
}

export function registerMobBehaviorTests(): void {
  GameTest.register("MobBehaviorTests", "zoglin_float", zoglinFloat)
    .batch("night")
    .structureName("gametests:mediumglass");
    // .maxTicks(810); // 可选，默认 210 ticks（约 10.5 秒）
}
```

要点：
- `GameTest.register(className, testName, fn)` 注册测试。**testName 不含 className**，对齐键是 `className:testName`（如 `MobBehaviorTests:zoglin_float`）。
- `.structureName("gametests:xxx")` 指定结构文件（`.mcstructure`，放包的 `structures/` 下）。
- `.batch("night")` 设定批次（部分测试需特定时间/环境）。
- `.maxTicks(N)` 设定超时（默认 210 ticks）。
- `test.succeedWhen(fn)` 设定通过条件；`test.succeed()` 立即通过；`test.runAtTickTime(t, fn)` 在指定 tick 执行回调。

更多 API 见 [`@minecraft/server-gametest` 官方文档](https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/server-gametest/minecraft-server-gametest)。

### manifest 模块版本

GameTest 框架整体属 **Beta APIs 实验**，`@minecraft/server-gametest` 只有 `1.0.0-beta`（无稳定版）。各包 manifest 依赖：

```json
"dependencies": [
  { "module_name": "@minecraft/server", "version": "2.0.0" },
  { "module_name": "@minecraft/server-gametest", "version": "1.0.0-beta" }
]
```

- `@minecraft/server` 写 `2.0.0`：官方基岩 1.26 捆绑稳定版 2.0.0–2.7.0，Cubium `supportedVersions={2.0.0, 1.17.0}`，**2.0.0 是两端最大公约数**。写 `1.x-beta` 会被官方基岩拒（invalid version）。
- 用例代码若只 import `server-gametest` 不直接用 `@minecraft/server` API，降版本对用例零影响。

---

## 2. Cubium 无头跑 GameTest

Cubium 的 `--gametest` 模式是无头（无世界、无玩家）跑 GameTest，跑完即退出，适合 CI。

```bash
./build/bin/RelWithDebInfo/minecraft-server.exe --gametest \
  --gametest_packs=E:/dev/minecraft-reborn-branch-1/tests/integrated \
  --gametest_report=E:/dev/minecraft-reborn-branch-1/build/cubium-report.xml
```

| flag | 说明 |
|---|---|
| `--gametest` | 启用无头 GameTest 模式 |
| `--gametest_packs=<dir>` | 行为包根目录（其下每个子目录是一个包） |
| `--gametest_report=<path>` | JUnit XML 输出路径（空=只写 stdout 日志） |
| `--gametest_tests=<pattern>` | 测试名过滤通配符（如 `simpleMobTest` 或 `simple.*`，**按 testName 非 className 匹配**，空=全部） |

### 双采集

`run_diff.ts` 跑 Cubium 时同时采集两路输出：
- **stdout 日志**：含 `[GameTest] Registered test '<className>.<testName>' (structure=...)` 注册日志（`GameTestRegistry.cpp:25`），用于建 testName→className 映射——因为 JUnit XML 的 `testcase.name` 只有 testName、`classname` 存的是 structure 而非 className，className 只能从 stdout 补。
- **JUnit XML**：结构化结果，`<testcase>` 含 `name`/`time`，`<failure>`/`<skipped>` 标记状态。`time`（秒）× 20 = ticks。

### 已知现象

- Cubium `--gametest` 偶发退出码 `3221225477`（0xC0000005 访问冲突）。这是关闭/析构阶段的问题，**stdout 与 XML 在崩溃前已写完**，不影响结果采集。直接命令行跑通常是退出码 1（有测试失败）或 0（全过）。

---

## 3. 与官方基岩 BDS 自动对比

`scripts/test/run_diff.ts` 是单文件全流程编排工具：让 Cubium 与官方基岩 BDS 跑**同一套** `tests/integrated` 用例，以官方基岩为 ground truth，输出 Cubium 的差异/缺陷报告。

**设计原则**：零外部依赖（仅 `node:` 内置模块）、node>=22 原生类型剥离、ESM 单文件。

### 为什么 Cubium（~6 秒）比基岩（~90 秒）快这么多

这不是基岩"慢"，是 Cubium **刻意加速**——两者跑的根本不是同一种时间。实测数据：

| | Cubium | Bedrock BDS |
|---|---|---|
| wall-clock 总耗时 | ~6.4 秒 | ~90 秒 |
| 测试逻辑 tick 总量 | 1145 ticks | 1145 ticks（同一套源码） |
| 每 tick 实际墙钟 | ~5.6 ms | 50 ms（真实 20 tps） |
| tick 与墙钟关系 | **脱钩**（虚拟 tick） | **绑定**（真实时间） |

**根因**：Cubium `--gametest` 是**无头门面**（`ServerApplicationEntry`："无头批量自动跑门面，自含行为包加载/JS 模块注册/runner 构造"），其 `GameTestServer::run()` 的 tick 循环（`src/server/test/facade/GameTestServer.cpp:374`）**没有 `sleep`/帧率同步**：

```cpp
while (m_running.load() && !m_runner->isComplete()) {
    tickOnce();      // 世界 tick → GameTestTicker → runner
    ++tickCount;
}
```

CPU 全速推进，一个 tick 不必等真实 50ms。这是对齐 Java `GameTestMainUtil` 的 CI 友好设计。官方基岩 BDS 是真实服务端，严格 20 tps，`iron_golem_arena` 的 `maxTicks(810)` 真实就是 810×50ms = 40.5 秒，雷打不动。

**公平性**：两端跑**完全相同的 `tests/integrated` 源码**，maxTicks 一致。Cubium 不是少跑 tick，是用更短墙钟跑完相同 tick 数。pass/fail 判定基于游戏逻辑（tick 内事件），与墙钟速度无关，**对比公平性不受影响**。

**次要加成**：Bedrock 端还有工具层每个测试间 1.5 秒串行间隔（避免结构方块冲突）+ 串行 `gametest run` 命令往返，8 测试 × 1.5s ≈ 12 秒纯等待。

### 基岩能加速吗？——不能（1.26.43.1 BDS 无官方途径）

Java 版有 `/tick rate N` / `/tick step` 命令加速 GameTest，但基岩 BDS **没有这条命令**。实测基岩 1.26.43.1：

```
help tick     → Syntax error: Unexpected "tick"
tick          → Unknown command: tick
tick rate     → Unknown command: tick
```

文档仓库（minecraft-creator）`creator/Commands/commands/` 下也无 `/tick` 收录。其他途径同样无效：
- gamerule 只有 `randomTickSpeed`（方块随机 tick，如作物生长，**不影响主循环 tps**）。
- server.properties 有 `tick-distance`（区块 ticking 距离，性能调优非速率）、watchdog 阈值（卡顿检测非加速）——都控制不了 tps。

**原因**：基岩 BDS 是真实多人服务端，tps 绑定真实时间是为了保证客户端同步、物理一致性、网络公平。给"加速 tick"会破坏这些不变量。Cubium 的无头门面能加速，是因为它不服务客户端、不维持真实帧率，测试跑完即退。

**对工具的影响**：Bedrock 端的 ~90 秒是**物理下限**，无法用命令加速。可行的优化只剩：
1. **减小串行间隔**：当前每测试间 1.5 秒（防结构方块冲突），可试探降到 0.5 秒省 ~8 秒，但有 `Could not find StructureBlockActor` 假失败风险，需实测。
2. **并行跑独立测试**：不同 className 的结构方块不冲突，理论上可并行——但基岩单进程 GameTest 是否支持并发 run 未验证，风险高。
3. **接受现状**：90 秒是基岩真实游戏时间，对比公平性不受影响。

最务实是 **#3 接受 + 可选 #1 微调**。基岩 tps 是设计约束，不是缺陷。

### 前置：一次性环境准备

```bash
node scripts/test/setup.ts
```

`setup.ts` 幂等地做三件事（可重复跑）：
1. **构建产物**：`cd tests/integrated && node build.mjs`，产物留原位不拷贝。
2. **建 Windows junction**：把官方基岩 `development_behavior_packs/<pack>` 链到 `tests/integrated/<pack>`（整个 pack 目录）。junction 不需管理员权限，基岩扫描时直接读仓库内产物，**改用例后重跑 `build.mjs` 即生效，无需 cp**。已存在则先 rmdir（junction 的 rmdir 安全，只删链接不删源）再重建。
3. **改 server.properties**：`level-name=gametest-diff`（专用世界）、`allow-cheats=true`（`/gametest` 要求）、`allow-list=false`、`content-log-file-enabled=true`。

setup **不启动基岩**。世界由基岩首次启动自动生成。

> 官方基岩 BDS 安装在 `D:/Minecraft/bedrock-server-1.26.43.1`（路径硬编码在 `setup.ts`/`run_diff.ts` 顶部常量，搬迁时改两处）。

### 跑对比

```bash
node scripts/test/run_diff.ts                 # 全流程（默认）
node scripts/test/run_diff.ts --step cubium   # 只跑 Cubium 侧
node scripts/test/run_diff.ts --step bedrock  # 只跑基岩侧（仍先跑 Cubium 拿测试列表）
```

### 流水线

```
tests/integrated (4 packs)
   ├── node build.mjs → scripts/*.js（产物留原位）
   │
   ├─[Cubium] minecraft-server.exe --gametest --gametest_packs --gametest_report
   │     stdout 注册日志 + JUnit XML
   │     → 归一化 JSON (source=cubium) + 测试列表
   │
   └─[Bedrock] bedrock_server.exe
         spawn → 注入 level.dat Beta APIs → 等 "Server started."
         → 逐个 stdin: gametest run <Class:name>（严格串行）
         → 解析 onTestPassed/onTestFailed 日志
         → 归一化 JSON (source=bedrock)
   \_____________________ _____________________/
                        ↓
         按 fullName 对齐 + L1/L2/L3 分级对比
                        ↓
         build/gametest-diff-report.md + CI 退出码
```

### 基岩侧的关键机制

**1. Beta APIs 实验开关（level.dat NBT 注入）**

`/gametest` 命令要求 Beta APIs 实验，但 BDS 无法经 server.properties / GUI 开实验，必须改 `worlds/gametest-diff/level.dat`。`run_diff.ts` 的 `enableBetaApiExperiment()` 是**零依赖手写 NBT 注入器**：
- Bedrock level.dat 格式：8 字节头（version u32 LE + length u32 LE）+ little-endian NBT payload。
- 在根 compound 的 `experiments` compound 里注入三个 byte tag：`gametest=1`（字段名是 `gametest` 不是 `beta_api`）、`experiments_ever_used=1`、`saved_with_toggled_experiments=1`。
- **必须重算头部 length 字段**（`header.writeUInt32LE(newPayload.length, 4)`），否则 Bedrock 截断读取。
- 改前基岩需先启动一次生成世界目录（`run_diff.ts` 的 `generateBedrockWorld()` 自动做）。
- 开启成功后基岩日志出现 `Experiment(s) active: gtst`。

**2. 逐个串行 `gametest run`**

- `gametest runset` 默认按 tag `suite:default` 跑，用例没打此 tag → `No tests found for tag 'suite:default'`，**不能用 runset 批量**。
- 改用 `gametest run <className>:<testName>` 逐个跑（测试名必须带 className 前缀，冒号分隔）。测试列表由 Cubium 报告提供（Cubium 跑全部注册测试，基岩按相同列表逐个跑）。
- **严格串行**：等前一个的 `onTestPassed`/`onTestFailed`/`Could not find test` 出现再发下一个，否则结构方块冲突致 `Could not find StructureBlockActor` 假失败。测试间隔 1.5 秒避免残留冲突。
- 基岩找不到的测试（如 Cubium 内置的 `ExampleTests:alwaysSucceed`，基岩无此注册）→ 报 `Could not find test with name '...'`，工具标 bedrock-missing（仅一端运行）。

**3. 基岩日志格式**

```
onTestStructureLoaded: <className>:<testName>   — 测试开始（结构加载）
onTestPassed: <className>:<testName>            — 通过
onTestFailed: <className>:<testName> - <reason> — 失败，' - ' 后是原因
```

日志无 tick 数，基岩侧 ticks 字段为 null（L3 tick 对比仅在有 tick 的一方有效）。完成信号即每个测试的 onTestPassed/onTestFailed，无全局汇总行。

### 对比分级

按 `fullName`（`className:testName`）对齐两端，分三级：

| 级别 | 类别 | 条件 | 严重度 |
|---|---|---|---|
| L1 状态 | `cubium-defect` | 基岩 pass + Cubium fail | **P1** |
| L1 状态 | `suspicious` | 基岩 fail + Cubium pass（Cubium 可能误判通过） | **P1** |
| L1 状态 | `stub-defect` | Cubium fail 且错误含 `MethodNotImplemented`/`not implemented`/`stub`/`TODO` | P1（已知 stub，单独归类不阻塞 CI 待定） |
| L1 状态 | `one-sided` | 仅一端运行（如 ExampleTests 基岩没有） | P2 |
| L2 错误 | `error-mismatch` | 两端都 fail 但 errorType 或归一化后 errorMessage 不同 | P2 |
| L2 错误 | `match` | 两端都 fail 且错误一致 | 一致 |
| L3 tick | `tick-drift` | 两端状态一致但 tick 差 > 20（1 秒） | P3 偏差（非缺陷） |
| — | `match` | 两端都 pass 且 tick 一致 | 一致 |

errorMessage 归一化：去坐标/tick 数、统一大小写、trim，再做语义比较。

### 产出

跑完在 `build/` 下生成：
- `gametest-diff-report.md` —— Markdown 报告（摘要 + 按类别分节表格）。
- `cubium-report.xml` —— Cubium JUnit XML。
- `cubium-stdout.log` / `bedrock-stdout.log` —— 两端原始 stdout（离线排查用）。

**CI 退出码**：
- `0` = 无 P1
- `1` = 有 P1（`cubium-defect` + `suspicious` + `stub-defect`）
- `2` = 流水线错误（基岩/Cubium 启动失败/超时/无结果，含报需 Beta APIs 实验开关）

### 报告样例

```markdown
## 摘要
- 基岩用例: 7 (passed 7, failed 0, skipped 0)
- Cubium用例: 8 (passed 7, failed 1, skipped 0)
- 对比结果: 一致 6, Cubium缺陷 1, ...

## Cubium 缺陷 (P1: 基岩通过 Cubium失败)
| fullName | 基岩 | Cubium | 说明 |
| MobBehaviorTests:zoglin_float | passed | failed (211t)<br>Test timed out after 210 ticks | Cubium failed: Test timed out... |

## 仅一端运行
| ExampleTests:alwaysSucceed | — | passed (1t) | only ran on Cubium |

## 一致
| ChallengeTests:collapsing | passed | passed (127t) | both passed |
...
```

---

## 4. 重要：GameTest 的非确定性

**官方基岩与 Cubium 的 GameTest 都有非确定性**，单次对比结果不可靠。用同一套 8 个用例跑三次的实测：

| 运行 | Cubium | Bedrock | P1 |
|---|---|---|---|
| 1 | 8/8 | 7/7 | 0 |
| 2 | 7/8（zoglin_float 超时） | 7/7 | 1 Cubium缺陷 |
| 3 | 8/8 | 5/7（zoglin+simpleMob 失败） | 2 可疑 |

原因：用例依赖 mob AI、实体生成、随机 tick 时序，两端都有随机性。典型：
- `MobBehaviorTests:zoglin_float`：两端都不稳。Cubium 偶尔超时（shulker 攻击/levitation 时序敏感），基岩偶尔 "Entity of type 'zoglin' was not found"。
- `StarterTests:simpleMobTest`：基岩偶尔报 `Did not expect Entity of type 'minecraft:chicken<>'`（基岩自己生成多余鸡），Cubium 通过反而合理。
- `MobBehaviorTests:zombie_villager_chase`：基岩偶尔报村民被僵尸杀光的时序错误。

**实践建议**：
- **单次 `run_diff.ts` 结果不能定性 Cubium 缺陷**。需多次跑（如 5 次）取稳定模式：某测试在 Cubium 上**连续失败**而基岩**连续通过**才是真缺陷；两端都偶尔失败属测试本身敏感。
- 报告里 `suspicious`（基岩挂 Cubium 过）分类很重要——多数是基岩随机性误判，不要当 Cubium 缺陷去修。
- 工具当前未做多次采样聚合，是已知改进点（可加 `--repeat N` 取众数/最稳结果）。

---

## 5. 故障排查

| 现象 | 原因 / 解决 |
|---|---|
| 基岩 `No tests found for tag 'suite:default'` | 正常。用例未打此 tag，工具已改用逐个 `gametest run`，忽略此提示。 |
| 基岩 `Could not find test with name '...'` | 该测试是 Cubium 内置（如 `ExampleTests:alwaysSucceed`），基岩无此注册。工具自动标 bedrock-missing。 |
| 基岩 `Beta APIs experiment is not enabled` | level.dat 注入失败。检查 `build/bedrock-stdout.log`，确认 `Experiment(s) active: gtst` 出现。`enableBetaApiExperiment` 依赖 8 字节头 + length 重算。 |
| 基岩卡在第 1 个测试不推进 | `runBedrock` 状态机索引 bug 已修：`sendNext` 发命令后必须立即 `testIdx++`，否则 onOutput 的 `testIdx > 0` 守卫挡住首个测试的完成检测。 |
| Cubium 退出码 3221225477 | 0xC0000005 访问冲突，关闭阶段问题。stdout/XML 已写完，不影响结果。 |
| 基岩 `Could not find StructureBlockActor associated to this test` | **基岩 BDS 1.26 自身的时序竞态**，非测试缺陷。结构放置后 `StructureBlock` 方块实体完成服务端同步有延迟，`gametest run` 若在同步完成前查询即报此错。基岩有**自动重试**机制——同一次 `gametest run` 内会先 `onTestFailed` 再重试，重试通过即 `onTestPassed`（日志里同一测试名出现 Failed 后又 Passed）。与测试逻辑、结构格式、是否用 `spawnSimulatedPlayer` 无关：纯 mob + `glass_pit` 的 `enderman_takes_water_damage`/`magmacube_immune_to_fire` 同样偶发。**对策**：见 6.3 节，用 `_bedrock_single.ts` 多次跑取稳定模式，或加大测试间串行间隔让结构方块充分同步。 |
| 基岩 `Failed to spawn test structure with path 'structures/.../xxx.mcstructure'` | mcstructure 文件格式损坏，基岩严格解析失败（Cubium 容错能读）。最常见是 `size` 的 NBT list 缺 count 字段（应为 `<tagType><int32 count><items>`，损坏时只有 `<tagType>` 没有后续 count）。见 6.4 节「mcstructure 格式陷阱」。 |
| 报告摘要出现 `undefined` | 已修：`countByCategory` 对未出现类别返回 undefined，用 `?? 0` 兜底。 |

---

## 6. 基岩单测工具与经验沉淀

本节沉淀「在官方基岩 BDS 上单独跑指定测试」的实践经验。当 `run_diff.ts` 全量对比（约 2 分钟、78 测试）过重、只想快速验证某几个测试在基岩的行为时，用单测工具。

### 6.1 `_bedrock_single.ts`：基岩单测工具

`scripts/test/_bedrock_single.ts` 启动基岩 BDS，等世界加载完成后逐个 `gametest run <Class:name>` 串行跑指定的测试，采集 `onTestPassed`/`onTestFailed` 日志。

```bash
# 用法：从仓库根目录跑（注意 cwd，脚本路径相对仓库根）
node scripts/test/_bedrock_single.ts "MobBehaviorTests:llama_spits_at_attacker" "MobBehaviorTests:llama_defends_against_wolf"
```

- 前置：`node scripts/test/setup.ts` 已完成环境准备（junction + level.dat Beta APIs 开关 + server.properties）。
- 基岩世界就绪（`Server started.`）后逐个发命令，每个测试等 `onTestPassed`/`onTestFailed` 后再发下一个。
- 总超时 180 秒；基岩 20 tps 真实时间，单测试约 10–30 秒。
- 输出直出 stdout，便于 `> /tmp/xxx.log 2>&1` 抓取后 grep `onTest`。

> 该工具**不是临时文件**，作为基岩侧快速验证的常备工具保留。`run_diff.ts` 是全量对比工具（含 L1/L2/L3 分级报告），`_bedrock_single.ts` 是轻量单测探针，两者互补。

### 6.2 单测过滤：Cubium `--gametest_tests` 是正则全匹配

Cubium 无头跑单测用 `--gametest_tests=<regex>` 过滤，**过滤器对 testName 做正则匹配**。常见坑：

```bash
# ❌ "llama_" 不匹配——过滤器需要匹配 testName 子串，但实际是全匹配语义
--gametest_tests=llama_          # → "no tests selected for filter 'llama_'"

# ✅ 用 .* 包裹做通配
--gametest_tests=.*llama.*       # 匹配 llama_spits_at_attacker / llama_defends_against_wolf
```

完整命令（从仓库根目录）：

```bash
./build/bin/RelWithDebInfo/minecraft-server.exe --gametest \
  "--gametest_packs=tests/integrated" \
  "--gametest_tests=.*llama.*" > /tmp/llama.log 2>&1
```

退出码 1 通常是「有测试在过滤子集里跑完」的正常退出（GameTestServer 跑完即退），**不代表失败**——看日志里 `PASSED:`/`FAILED:` 计数判断。

### 6.3 `Could not find StructureBlockActor`：基岩时序竞态，非测试缺陷

**现象**：基岩 `gametest run <test>` 后报 `Could not find StructureBlockActor associated to this test`，日志里 `onTestStructureLoaded`（结构加载成功）紧接着 `onTestFailed`。

**根因**：基岩 BDS 1.26 自身的时序竞态。结构放置后 `StructureBlock` 方块实体完成服务端同步有延迟，`gametest run` 若在同步完成前查询测试锚点即报此错。

**关键证据（推翻若干常见误判）**：
- 与测试逻辑无关：纯 mob + `glass_pit` 的 `enderman_takes_water_damage`、`magmacube_immune_to_fire`、`zombified_piglin_immune_to_fire`（都不用 `spawnSimulatedPlayer`）同样偶发此错。
- 与结构格式无关：`glass_pit`（格式正确）也偶发。
- 与 `spawnSimulatedPlayer` 无关：用 SimulatedPlayer 的 `blaze_shoots_fireball_at_player` 同样偶发，但它能在基岩自动重试中通过。

**基岩有自动重试**：同一次 `gametest run` 内，首次 `onTestFailed` 后基岩会重试，重试通过即 `onTestPassed`。日志里同一测试名出现 `onTestFailed` 后紧接 `onTestPassed` 即重试成功（如 `blaze`）。但重试并非必过——时序更糟时（如 `llama_spits_at_attacker` 连跑 3 次全 Failed）需更多重试或更大间隔。

**对策**：
- 单次失败不定性，用 `_bedrock_single.ts` 多次跑取稳定模式。
- 加大测试间串行间隔让 StructureBlock 充分同步（`_bedrock_single.ts` 当前 ~1.5s，可调大）。
- 这是基岩框架的固有非确定性，**不应通过改测试逻辑或 Cubium C++ 去修**——符合第 4 节「GameTest 非确定性」的总原则。

### 6.4 mcstructure 格式陷阱：`size` list 必须含 count 字段

**现象**：某 mcstructure 在 Cubium 能正常加载，基岩报 `Failed to spawn test structure with path 'structures/.../xxx.mcstructure'`。

**根因**：mcstructure 是基岩 little-endian NBT。`size` 字段是 NBT list，格式为 `<elementType:1byte><count:int32LE><items...>`。若生成脚本漏写 count（只写 `<elementType>` 后直接跟 items），Cubium 容错解析能读，基岩严格解析把 items 首字节当 count 读，后续字段全部错位，最终越界/解析失败。

**诊断方法**（node，零依赖）：对比能加载的结构与失败结构的 `size` 字段字节：

```bash
node -e "
const fs=require('fs');
const dir='tests/integrated/mob_behavior/structures/gametests/';
for(const name of ['creeper_pit','glass_pit']){
  const d=fs.readFileSync(dir+name+'.mcstructure');
  const i=d.indexOf('size');
  console.log(name, 'size bytes:', Array.from(d.slice(i+4,i+18)).map(b=>b.toString(16).padStart(2,'0')).join(' '));
}
# 正确: 03 03 00 00 00 07 00 00 00 05 00 00 00 07 ...  (type=3/int, count=3, 然后 [7,5,7])
# 损坏: 03 00 00 00 07 00 00 00 05 00 00 00 07 ...     (type=3/int, count=0 ←错, 字段错位)
"
```

**修复**：参照 `scripts/test/_rebuild_creeper_pit.ts`（保留作为格式参考样板），用正确的 little-endian NBT 序列化重建。关键点：
- root compound 无名，含 `format_version`(int=1)、`size`(list<int>)、`structure`(compound)。
- `structure.palette.default.block_palette` 是 list<compound>，每项 `{name, states:{}, version:17879555}`（version 是基岩方块版本号，对齐已知可加载结构）。
- `structure.block_indices` 是 list<list<int>>，两层：layer0=方块索引，layer1=-1（液体层全空）。
- 方块索引公式：`index = (x * size_y + y) * size_z + z`，遍历顺序 x→y→z（外层 x，内层 z）。

> `_rebuild_creeper_pit.ts` 保留作为「正确 mcstructure 格式」的可运行参考样板，不删除。

### 6.5 跨服务端兼容垫片：Cubium 专有 GameTest 链式方法

**背景**：Cubium 在官方 `@minecraft/server-gametest` 的 `RegistrationBuilder` 之上扩展了 `skyAccess`（对齐 Java GameTest TestData 字段，让结构上方露天列使 `canSeeSky=true`）等链式方法，类型声明在 `tests/integrated/mob_behavior/src/cubium-gametest-augment.d.ts`。官方基岩 BDS 的 `RegistrationBuilder` 没有 `skyAccess`，测试代码调用 `.skyAccess(true)` 时抛 `TypeError: not a function`，发生在 `main.js` 顶层 register 阶段，致**整个行为包加载失败、所有测试都无法注册**。

> 注意：`setupTicks` 是基岩 `RegistrationBuilder` **原生方法**（见 `index.d.ts`），两端都有，无需降级；仅 `skyAccess` 是 Cubium 专有。

**垫片方案**（`tests/integrated/mob_behavior/src/gametest-shim.ts`，在 `main.ts` 最顶部 import 触发副作用）：

1. **主策略——`RegistrationBuilder.prototype` 注入**：基岩 `RegistrationBuilder` 是导出 class，prototype 可扩展。在 prototype 上注入 `skyAccess` no-op（返回 this 保持链式）。基岩实例自身无 `skyAccess`，沿原型链命中注入的 no-op；Cubium 实例自身有 C++ 绑定的 `skyAccess`，覆盖 prototype，无副作用。**不替换 `GameTest.register`，最可靠**。
2. **备用策略——`register` Proxy 包装**：若 prototype 注入失败（如基岩 prototype 冻结），退而用 Proxy 包装 `GameTest.register` 返回的 builder，对 `skyAccess` 降级 no-op。基岩 ESM namespace 属性可能 read-only（赋值抛 TypeError 或静默失败），用 try/catch 兜底。

**跨服务端可写性差异**（核心认知）：
- 基岩 BDS：`@minecraft/server-gametest` 是标准 ESM 模块，namespace 属性可写。
- Cubium：`GameTest.register` 是 C++ 绑定的 read-only 属性，赋值抛 `TypeError: 'register' is read-only`。

早期失败的垫片方案是「直接 `GameTest.register = Proxy`」且无 try/catch——在 Cubium 侧抛 read-only 异常未捕获，导致垫片模块自身加载失败、连带整个包崩。正确做法是 prototype 注入（根本不碰 register）+ try/catch 兜底。

### 6.6 mcstructure 必含 `structure_world_origin` 字段

**现象**：基岩 BDS 1.26 严格校验 mcstructure，缺失 `structure_world_origin` 字段时报：
```
[Structure] Loading structure 'gametests:creeper_pit' from behavior pack: ... | "structure_world_origin" field, a required field, is missing from the structure.
```
随后 GameTest 报 `Could not find StructureBlockActor associated to this test`（与 6.3 的时序竞态同款报错，但根因不同：这里是结构压根没加载成功）。Cubium 容错能读缺该字段的结构，故仅基岩侧受影响。

**字段格式**：`structure_world_origin` 是 root compound 的一个顶层字段，类型 TAG_LIST of TAG_INT，长度 3，表示结构在世界中的原点坐标 `[ox, oy, oz]`。GameTest 场景下基岩按 test 位置 + 结构内相对坐标放置，origin 值 `[0,0,0]` 即可（基岩对具体数值不敏感，只要字段存在）。已知可加载的 `glass_pit`/`mediumglass` 用 `[10,10,10]`，`[0,0,0]` 同样工作。

**修复**：用 `scripts/test/_fix_structure_origin.mjs` 为缺字段的结构注入 `structure_world_origin`（默认 `[0,0,0]`），保持其他字段字节不变：
```bash
node scripts/test/_fix_structure_origin.mjs tests/integrated/mob_behavior/structures/gametests/creeper_pit.mcstructure 0 0 0
```
本次修复了 `creeper_pit`、`grass_pen` 两个结构（`glass_pit`、`mediumglass` 原本就有该字段）。

> 排查要点：当基岩报 `Could not find StructureBlockActor` 时，先检查结构文件是否含 `structure_world_origin`（用 6.4 的解析思路或 `_fix_structure_origin.mjs` 的检测逻辑），排除字段缺失后再考虑 6.3 的时序竞态。

### 6.7 `minecraft:equippable` 组件的跨平台差异（mob 卸装备）

**现象**：`Entity.getComponent("minecraft:equippable")` 对骷髅等 mob：
- **Cubium**：对所有 `LivingEntity` 返回有效组件（善意扩展，`MinecraftModuleFactory.cpp` setEquipment 绑定，第二参数 `undefined` 清空槽位 → `living->setEquipment(slot, ItemStack::EMPTY)`）。
- **基岩 BDS**：返回 `undefined`。基岩 `minecraft:equippable` 是 BP 实体组件，**只有显式声明了它的实体才有**（马科/驼科的鞍/地毯槽），原版骷髅无此组件。骷髅主手弓来自生成期 `minecraft:equipment` 战利品表（只读，脚本不可改）。

**根因**：这是 Cubium 对基岩 API 语义的偏离（让 mob 都返回 equippable 是善意扩展，方便测试操作 mob 装备）。基岩官方 `EntityEquippableComponent` 文档明确"只存在于 player 实体"。

**对测试的影响**：需要给 mob 卸装备的测试（如 `skeleton_fights_in_melee_without_bow` 给骷髅卸弓验近战）存在跨平台不可调和差异：
- Cubium 唯一路径：`equippable.setEquipment("Mainhand", undefined)`。
- 基岩无 equippable；唯一脚本化卸 mob 装备的途径是 `Entity.runCommand("replaceitem entity @s slot.weapon.mainhand 0 air")`（基岩 mob 主手槽名 `slot.weapon.mainhand`，带 `slot.` 前缀）。
- **但 Cubium 未绑定 `Entity.runCommand`/`Dimension.runCommand`**（脚本侧无命令执行入口），故 runCommand 路径在 Cubium 是死路。
- 平台分流方案：运行时按 `equippable` 组件存在性检测——Cubium 有 equippable 走 setEquipment，基岩无 equippable 走 runCommand。Cubium 的 `/replaceitem entity` 命令虽存在但仅支持玩家目标（`EntityArgumentType::players()`），无法卸 mob 弓，故 Cubium 不能走命令路径。

> 决策原则：当某行为在 Cubium 可测但在基岩受 API 限制无法脚本触发时（非测试逻辑缺陷），测试以 Cubium 验证为准，基岩差异在测试注释中标注。基岩行为本身存在（骷髅无弓时会近战，由 `has_ranged_weapon` 触发器切换组件组），只是无法通过脚本在基岩复现该前置条件。

### 6.8 GameTest spawn 的 Mob 必须设持久化（enablePersistence）

**现象**：`wither_rose_spare_undead`（骷髅踩凋灵玫瑰应免疫、保持满血）在 Cubium 全量跑中报 `skeleton disappeared`——骷髅在测试判定时刻（t=120）已不存在。单独跑该测试却稳定通过（骷髅 t=119 仍存活满血）。

**根因（两层，须分清）**：

1. **诊断代码强制失败（测试逻辑层，真凶）**：排查初期在测试里插入 `DIAG3` 诊断块，末尾带 `test.assert(false, ...)` 强制失败以打印快照。**忘记移除该 `assert(false)`**，导致测试永远失败，而诊断快照显示 `lastSeen=t119 hp20 goneAt=t-1`（骷髅从未消失）。即：测试失败的唯一原因就是诊断代码本身。教训：**带 `assert(false)` 的诊断代码必须 100% 移除后再判定**，否则诊断工具自身成为失败源。

2. **DespawnManager 误清测试 Mob（框架层，治本修复）**：全量跑 `mob_behavior` 时框架注入 Survival SimulatedPlayer（远程攻击测试需要），`DespawnManager::shouldDespawn`（`src/server/world/spawn/DespawnManager.cpp`）在"有玩家且实体距玩家 >128 格且 `canDespawn()`"时（line 141）立即 `remove()`。未设持久化的测试 Mob（自身无玩家陪伴，距 SimulatedPlayer 远）会在 spawn 后首个 despawn tick 即被移除。

   - 关键分支顺序：line 114 Peaceful 分支 → **line 119 `isNoDespawnRequired()` 短路保留** → line 125 无玩家保留 → line 141 >128 格立即移除 → line 146 随机移除。persistence=true 命中 line 119 短路，绕过所有 despawn。
   - 基岩靠 `DespawnComponent` 的"附近有可交互玩家则不 despawn"前置门控避免清 mob；本项目对齐 Java 的 `DespawnManager`（全局 despawnDistance=128），故按 Java `GameTestHelper.spawn`（`GameTestHelper.java:170`）方式在 spawn 处对 Mob 设 persistence。

**治本修复**：`GameTestHelper::spawnEntity`/`spawnAtLocation`（`src/server/test/facade/GameTestHelper.cpp`）对所有 `MobEntity` 调 `enablePersistence()`，使 `isNoDespawnRequired()==true`，DespawnManager 短路保留。`m_persistenceRequired` 全仓无重置点（仅 set true + NBT 读写），一旦设置测试 Mob 永不自然消失。

**排查要点**：
- 测试报"实体消失"但单独跑通过、全量跑失败 → 先查全量跑是否注入 SimulatedPlayer（mob_behavior 远程攻击测试会注入），DespawnManager 会在 >128 格清除未持久化 Mob。
- **诊断代码纪律**：任何 `test.assert(false, ...)` / `throw` 形式的强制失败诊断块，验证完必须立即删除，不能残留到提交。提交前 grep `assert(false` / `assert(!` / `DIAG` 确认无残留。
- 验证修复时用"区域限定 + runAtTickTime(延迟判定)"：`getEntities({type, location, volume})` 限定到结构范围（排除并行测试污染），`runAtTickTime(120)` 强制等到足够长时间证明全程未掉血/未消失（`succeedWhen` 每 tick 检查会在条件首帧满足时立即 succeed，无法验证"持续 N tick 仍成立"）。

### 6.9 block_behavior 测试结构跨包依赖 + 全量跑验证方法

**现象**：`--gametest_packs` 指向 `tests/integrated/block_behavior` 单独全量跑时，大量测试报 `structure 'gametests:glass_pit' not found in TemplateManager`，仅 fall_tower 相关测试通过。

**根因**：`block_behavior` 包自带的结构只有 `fall_tower`；`glass_pit`/`mediumglass`/`grass_pen`/`creeper_pit` 都在 `mob_behavior` 包的 `structures/gametests/` 下。block_behavior 测试依赖这些结构，全量跑必须同时加载 mob_behavior（或把结构复制进 block_behavior 包）。`--gametest_packs` 指向的目录是"包目录的父目录"（`loadPlugins` 扫描其下含 `manifest.json` 的子目录），直接指向包目录本身会扫不到子包。

**全量跑 block_behavior 的隔离验证方法**（不引入 mob_behavior 的 SimulatedPlayer/已知崩溃）：
```bash
# 1. 建临时父目录，复制 block_behavior 包
rm -rf /tmp/blk_only && mkdir -p /tmp/blk_only
cp -r tests/integrated/block_behavior /tmp/blk_only/
# 2. 把 block_behavior 依赖的共享结构从 mob_behavior 复制进来（自给自足）
cp tests/integrated/mob_behavior/structures/gametests/glass_pit.mcstructure \
   tests/integrated/mob_behavior/structures/gametests/mediumglass.mcstructure \
   /tmp/blk_only/block_behavior/structures/gametests/
# 3. 全量跑（指向临时父目录）
./build/bin/RelWithDebInfo/minecraft-server.exe --gametest --gametest_packs "/tmp/blk_only"
# 预期：20/20 passed（17 block + 3 内置），exit 0，无崩溃
```

**框架清理行为（重要约束）**：`BaseGameTestBatchRunner::tick`（`src/common/test/framework/batch/BaseGameTestBatchRunner.cpp`）在批完成时仅 `m_currentBatchInstances.clear()`（析构实例对象），**不清世界中残留实体**；`BaseGameTestInstance::succeed/fail` 仅跑 `onFinish` 回调 + 通知监听器，**不调 killAllEntities**。`killAllEntities` 只经 JS 绑定（`ScriptTestHelper.cpp`）暴露给测试作者手动调用，且只清结构范围内实体。结合 6.8 的 enablePersistence（测试 Mob 永不自然消失），**测试 Mob 一旦 spawn 且未被测试逻辑杀死，会持续累积**。block_behavior 测试多为短时序 + 实体被测行为致死（摔死/烧死/窒息），累积风险低；但长时序 mob 测试全量跑时需注意此约束。

> 排查要点：`structure not found in TemplateManager` 先查该结构在哪个包（`find tests/integrated -name "<name>.mcstructure"`），确认全量跑加载了提供该结构的包。`--gametest_packs` 传父目录（含多个包子目录），不传包目录本身。

---

## 参考

- [`@minecraft/server-gametest` Module — Microsoft Learn](https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/server-gametest/minecraft-server-gametest)
- [`/gametest` Command — Microsoft Learn](https://learn.microsoft.com/en-us/minecraft/creator/commands/commands/gametest)
- [Enabling experiments via NBT — wiki.bedrock.dev](https://wiki.bedrock.dev/nbt/enabling-experiments)
- 工具源码：`scripts/test/setup.ts`、`scripts/test/run_diff.ts`、`scripts/test/_bedrock_single.ts`（基岩单测探针，见 6.1）、`scripts/test/_rebuild_creeper_pit.ts`（mcstructure 格式参考样板，见 6.4）
- 跨服务端兼容垫片：`tests/integrated/mob_behavior/src/gametest-shim.ts`（见 6.5）、`tests/integrated/mob_behavior/src/cubium-gametest-augment.d.ts`（Cubium 专有方法类型声明）
- Cubium GameTest 实现：`src/server/test/facade/GameTestServer.{hpp,cpp}`、`src/common/test/framework/registry/GameTestRegistry.cpp`、`src/server/test/runner/reporter/JUnitTestReporter.cpp`
