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
| 基岩 `Could not find StructureBlockActor` | 测试未串行，前一个结构方块未清理。工具已强制逐个串行 + 1.5s 间隔 + `gametest clearall`。 |
| 报告摘要出现 `undefined` | 已修：`countByCategory` 对未出现类别返回 undefined，用 `?? 0` 兜底。 |

---

## 参考

- [`@minecraft/server-gametest` Module — Microsoft Learn](https://learn.microsoft.com/en-us/minecraft/creator/scriptapi/minecraft/server-gametest/minecraft-server-gametest)
- [`/gametest` Command — Microsoft Learn](https://learn.microsoft.com/en-us/minecraft/creator/commands/commands/gametest)
- [Enabling experiments via NBT — wiki.bedrock.dev](https://wiki.bedrock.dev/nbt/enabling-experiments)
- 工具源码：`scripts/test/setup.ts`、`scripts/test/run_diff.ts`
- Cubium GameTest 实现：`src/server/test/facade/GameTestServer.{hpp,cpp}`、`src/common/test/framework/registry/GameTestRegistry.cpp`、`src/server/test/runner/reporter/JUnitTestReporter.cpp`
