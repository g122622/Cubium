---
name: mc-add-integrated-test
description: 基于现有集成测试框架，为项目增加集成测试，以测试与原版MC行为是否对齐
---

# 1. 任务简介

根据用户指定的范围，基于现有集成测试框架，为项目增加集成测试，以测试与原版MC行为是否对齐。

# 2. 详细过程

1. 调用 `docs\minecraft-wiki-source\.claude\skills\minecraft-wiki-retrieval\SKILL.md` 中的 `minecraft-wiki-retrieval` 技能，根据用户指定的范围，检索原版MC的行为、数据、逻辑等的wiki原文，并将检索结果作为参考。
2. 思考要设计怎样的集成测试。设计准则见下面章节。
3. 开始着手编写typescript测试用例。必须参考现有测试用例的语法和结构，以及随时查询官方ScriptAPI的文档：E:\dev\MC\Mods\minecraft-creator\creator\ScriptAPI\minecraft\server-gametest
4. 编写完测试用例后，必须构建并运行验证，确保测试用例在Cubium上能跑通。如果跑不通，必须参照文档和本地的java版源码进行补全。【重要】你时间充足，上下文也充足，必须始终选择最彻底、最耗时、最正确、最准确、最完整的做法。
5. 有时候测试失败、崩溃的原因可能不在于测试用例或者被测代码本身，而是项目的集成测试框架、脚本系统等的bug导致的（这里的bug多种多样，比如：api实际行为与官方文档不一致或与实际基岩版api不一致、api未完整实现等）。这种情况下请修复、补全集成测试框架、脚本系统等的bug，确保测试用例能够顺利运行，然后继续编写测试用例。脚本系统可从src\common\mod\bedrock\addon\modules\MinecraftModuleFactory.cpp入手。
6. 如果遇到困难，可以与官方基岩 BDS 单独跑这同一套用例对比，以检验你编写的测试用例的正确性（以基岩版行为为唯一权威！）；如果遇到困难，你还可以参考集成测试完整文档：`docs/test/INTEGRATED_TEST.md`（用例组织、构建、Cubium 无头跑、基岩对比、非确定性、故障排查），这个文档必须阅读！
7. 建议同一时间只处理一个错误：不要同时调查好几类错误，而是集中精力攻克一个错误，直到解决后再处理下一个错误。这样可以避免混淆和力量分散。

# 3. 集成测试设计准则和案例

## 3.1 设计准则

- 尽可能覆盖原版MC wiki中的大部分行为、数据、逻辑等，确保与原版MC行为对齐。
- 【重要】有的特性可能基岩版和Java版的行为不一致、或者官方文档中没有明确说明行为。这种情况下请以Java版为唯一权威真相源，设计集成测试！
- 测试只覆盖截止到1.21.11版本的特性，不允许编写基于新版本特性的测试。
- 对于随机性较强的行为（如生物 AI、掉落物概率），可增加maxTicks参数，延长测试时间，确保测试结果的可靠性，并在需要多次重复运行。
- 写测试的目的就是为了充分暴露cubium的缺陷并彻底修复和完善cubium！对于Cubium 与 vanilla 不一致的行为，必须编写测试用例暴露出来，并修复Cubium的缺陷并完善Cubium逻辑，确保与原版MC行为对齐。

## 3.2 代码准则

- 优先查阅 tests\integrated\utils 和 当前\tests\integrated\xxx\src\utils 目录下已有的辅助函数，尽可能做到代码复用；对于应该复用的代码，也应该提升到utils目录下，避免重复造轮子。
- 为每个测试添加清晰的注释，说明其验证的原版 Wiki 章节（如 // Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_僵尸.txt#自然生成），方便追溯
- 单一职责：每个测试只验证一个“行为点”，避免过度复杂导致失败时无法定位原因

## 3.3 测试目录组织规范

测试源码按**主角生物的 Cubium 实体分类**（`src\common\entity\entities` 的目录结构）拆分到子目录。这样未来为每个生物加行为测试时，放入对应分类目录即可，不会把所有测试堆在一个大文件里。

分类目录对照 `src\common\entity\entities`：
- `monster/undead`（僵尸、骷髅、幻翼等亡灵）、`monster/nether`（疣猪兽等地狱生物）、`monster/basic`（苦力怕、史莱姆）、`monster/end`（末影人、潜影贝）、`monster/illager`（灾厄村民）等
- `passive/golem`（铁傀儡、雪傀儡）、`passive/tamable`（狼、猫）、`passive/basic`（猪、牛、羊）、`passive/special`（狐狸、熊猫）等
- `villager`、`boss`、`projectile` 等

每个测试文件命名 `<生物>Tests.ts`（如 `ZombieTests.ts`），导出 `register<生物>Tests()` 函数。`main.ts` 聚合调用各 register 函数。

当前 `mob_behavior` 包结构示例：
```
tests/integrated/mob_behavior/src/
├── main.ts                          # 入口，聚合调用各 register
├── tests/
│   ├── monster/undead/ZombieTests.ts       # zombie_villager_chase
│   ├── monster/nether/ZoglinTests.ts       # zoglin_float
│   ├── monster/basic/PhantomTests.ts       # phantoms_should_fly_from_cats
│   └── passive/golem/IronGolemTests.ts     # iron_golem_arena
└── utils/                           # build.mjs 构建期从 tests/integrated/utils 复制
```

> 注意 import 路径：测试文件在 `src/tests/<cat>/<sub>/` 下，到 `src/utils/` 需回退 3 层 `../../../utils/...`。

## 3.4 实际案例

下面是一个单生物测试文件示例（拆分后每个文件只含一个生物的测试）。能调用的脚本 API 远不止这些，测试手段也远不止这些：

```ts
// tests/integrated/mob_behavior/src/tests/monster/undead/ZombieTests.ts
// 僵尸行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { addFourNotchedWalls } from "../../../utils/block/build.js";

// 僵尸在有缺口的砖墙间追逐村民，验证僵尸寻路 AI。
function zombieVillagerChase(test: Test): void {
  const villagerType = "villager_v2";
  const zombieType = "zombie";

  addFourNotchedWalls(test, "minecraft:brick_block", 2, 1, 2, 4, 6, 4);

  test.spawn(villagerType, { x: 1, y: 3, z: 1 });
  test.spawn(zombieType, { x: 5, y: 3, z: 5 });

  test.runAtTickTime(180, () => {
    test.assertEntityPresentInArea(villagerType, true);
    test.succeed();
  });
}

export function registerZombieTests(): void {
  GameTest.register("MobBehaviorTests", "zombie_villager_chase", zombieVillagerChase)
    .batch("night")
    .structureName("gametests:glass_pit")
    .maxTicks(2000);
}
```

`main.ts` 聚合注册：
```ts
// tests/integrated/mob_behavior/src/main.ts
import { registerZombieTests } from "./tests/monster/undead/ZombieTests.js";
import { registerIronGolemTests } from "./tests/passive/golem/IronGolemTests.js";
// ...

registerZombieTests();
registerIronGolemTests();
```

# 4. 测试世界与运行环境

写测试前必须了解 GameTest 跑在什么样的世界里、批次如何调度、失败如何重跑。这些由 `GameTestServer`（`src/server/test/facade/GameTestServer.{hpp,cpp}`）决定。

## 4.1 超平坦测试世界（minecraft:flat）

测试世界是**超平坦**（classic_flat 预设，`FlatChunkGenerator`），不是 normal 噪声地形。坐标语义：

- flat 地形层：bedrock（Y=0）+ dirt（Y=1..2）+ grass（Y=3），即地表在 **Y=3**。
- 测试结构方块放 **Y=4**（`gridStartY=4`），结构内容从 **Y=5** 起。
- 生物群系全图固定为 plains（flat 预设的 `biome` 字段）。

对测试作者的影响：
- 测试内相对坐标 `y=1` 是结构内容首层（世界 Y=5），站在"地面"上的实体脚踩 grass（Y=3）。
- `skyAccess` 的清空逻辑会清掉 padding 扩展范围内的 flat 层制造露天列，属预期行为。
- 依赖地形起伏/洞穴/特定群系的测试**不可写**（超平坦世界没有这些），只能依赖结构文件 + 程序化放置。

## 4.2 批内切片（MAX_TESTS_PER_BATCH=50）

同一批次超过 50 个测试时，`_selectAndBuildRunner` 会把该批切为多个子批次依次执行：

- 切片命名 `原批次名_1`/`原批次名_2`/...（1-based）。
- 切片名保留原批次名前缀，`getEnvForBatch` 的前缀匹配（`night` 前缀 → 18000 夜晚 / 其他 → 6000 白天）自动保持环境语义——即 `night_2` 仍是夜晚环境。
- 每个切片独立执行 beforeBatch/afterBatch 回调与环境 setup/teardown。

## 4.3 失败测试隔离重跑

`FailedTestCollector`（`TestReporter` 子类）在跑完后收集所有失败 testName。外层协调脚本 `scripts/test/run-gametests.ts` 用它编排两轮：

1. Round 1：全量跑，解析 JUnit XML 收集失败测试列表。
2. Round 2：对每个失败测试**独占一个进程**（隔离世界目录）重跑确认。

重跑语义：**重跑通过算通过**（最终判定为通过，从失败集合中移除）。这解决了"与其他测试并行时会因跨结构干扰而失败"的确认问题。

## 4.4 批次环境约定

- `night` 前缀批次：TimeOfDayEnvironment(18000) 夜晚。
- `day` 前缀及其他：TimeOfDayEnvironment(6000) 正午白天。
- 所有 day/night 批次强制晴天（WeatherEnvironment::Clear），消除天气随机性导致的 flaky。

---

# 5. 构建、运行与验证

写完/改完测试后必须构建并运行验证：

```bash
# 1. 构建所有行为包（src/*.ts → scripts/*.js，并复制 utils 到各包）
cd tests/integrated && node build.mjs

# 2. 跑 Cubium 无头 GameTest 验证用例注册与执行
node scripts/test/run_diff.ts --step cubium
# 预期：8 tests registered，无 "Module not found" / entry 加载失败
```

验证要点：
- Cubium 日志 `[GameTest] Registered test '<className>.<testName>'` 中 className/testName 与预期一致（改目录结构不应改 className，否则对比工具的 fullName 对齐会断）。
- 若报 `Module not found: .../scripts/./XxxTests.js`，检查 `main.ts` 的 import 路径与文件实际位置是否一致（注意子目录层级对应的 `../` 数量）。
- 若有ts类型问题，看 node_modules 里的类型定义（如`node_modules/@minecraft/server-gametest/index.d.ts`，这些文件较大，请避免全量读取）

# 6. 与官方基岩 BDS 对比测试

集成测试除了"在 Cubium 上能跑通"，还要**与官方基岩 BDS 跑同一套用例对比**，以官方基岩为 ground truth，自动发现 Cubium 的行为偏差/缺陷。这是验证"与原版 MC 对齐"的最权威手段。因此你写的测试不仅要在 Cubium 上跑通，还要在官方基岩上跑通。

## 5.1 工具与流程

对比工具：`scripts/test/run_diff.ts`（单文件，零依赖）。完整文档见 `docs/test/INTEGRATED_TEST.md`。

```bash
# 一次性环境准备（建 junction 链基岩 + 改 server.properties，幂等）
node scripts/test/setup.ts

# 全流程对比（跑 Cubium + 跑基岩 + 对比 + 报告）
node scripts/test/run_diff.ts
# 单步：--step cubium | bedrock
```

流程：Cubium 跑全部注册测试拿测试列表 → 基岩按相同列表逐个 `gametest run <Class:name>` 串行跑 → 按 fullName 对齐两端结果，L1 状态/L2 错误/L3 tick 分级对比 → 输出 `build/gametest-diff-report.md` + CI 退出码（0 无 P1 / 1 有 P1 / 2 流水线错误）。

## 5.2 对比结果分类

| 类别 | 条件 | 含义 |
|---|---|---|
| `cubium-defect` | 基岩 pass + Cubium fail | **Cubium 真缺陷（P1）**，优先修 |
| `suspicious` | 基岩 fail + Cubium pass | Cubium 可能误判通过，需人工复核 |
| `stub-defect` | Cubium fail 且错误含 `not implemented`/`stub` | 已知未实现，单独归类 |
| `error-mismatch` | 两端都 fail 但错误不同 | 行为偏差（P2） |
| `tick-drift` | 状态一致但 tick 差 > 20 | 时序偏差（P3，非缺陷） |
| `one-sided` | 仅一端运行 | 如 Cubium 内置测试基岩没有 |
| `match` | 两端状态一致 | 对齐 |

## 5.3 重要：GameTest 的非确定性

**官方基岩与 Cubium 的 GameTest 都有非确定性**，单次对比不可靠。同一套用例跑三次可能出现：Cubium 偶尔超时、基岩偶尔自己挂（如 `simpleMobTest` 基岩偶尔报多余鸡）。实践：
- **单次 run_diff 结果不能定性缺陷**。需多次跑（如 5 次）取稳定模式：某测试在 Cubium 上**连续失败**而基岩**连续通过**才是真缺陷。
- `suspicious`（基岩挂 Cubium 过）多数是基岩随机性误判，不要当 Cubium 缺陷去修。
- 详见 `docs/test/INTEGRATED_TEST.md` 的"非确定性"章节。

## 5.4 性能预期

- Cubium `--gametest` 是无头门面，tick 脱钩墙钟，8 测试约 **6 秒**跑完。
- 官方基岩 BDS 严格 20 tps（真实时间），同样 8 测试约 **90 秒**。这是基岩的设计约束（**无 `/tick rate` 命令加速**），不是缺陷，对比公平性不受影响（两端跑相同 tick 数）。
- 全流程 `run_diff.ts` 会有一定的时间开销（数分钟起步），请耐心等待。

# 6. 参考资源

- 集成测试完整文档：`docs/test/INTEGRATED_TEST.md`（用例组织、构建、Cubium 无头跑、基岩对比、非确定性、故障排查）这个文档必须阅读
- TS 迁移背景：`docs/test/INTEGRATED_TEST_MIGRATION_TO_TS.md`
- 官方 ScriptAPI 文档：`E:\dev\MC\Mods\minecraft-creator\creator\ScriptAPI\minecraft\server-gametest`
- wiki 检索技能：`docs\minecraft-wiki-source\.claude\skills\minecraft-wiki-retrieval\SKILL.md`
- 可能会用到的mcstructure文档：docs/test/STRUCTURES.md 里面系统性阐述了各个结构的特征和组成，你不再需要手动解析结构文件的nbt！
