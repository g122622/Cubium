---
name: mc-add-diff-test
description: 为项目新增一个差分测试（tests/diff）能力项，用原版实现作为参照逐字节比对
---

# 1. 任务简介

差分测试（`tests/diff`）是本项目的第四类测试：**同一份输入分别喂给 Cubium 与原版实现，要求两侧输出逐字节相同**。它的期望值不是人手写的，而是原版自己跑出来的，因此不存在「人写错期望值」这一类漏检。

本 skill 描述「为 Cubium 新增一个差分能力项」的标准作业流程（SOP）。适用于两类需求：

1. 用户点名某个子系统（如红石、流体、计划刻、附魔、命令），要求补齐它的原版对齐验证；
2. 你在做别的任务时发现某个纯数据表/纯计算逻辑没有任何参照，想为它补一层防护。

本 SOP 的产物是：C++ 探针 + Java oracle + TS 能力声明三件套，全部接入既有编排层，并能实际跑出报告。

---

# 2. 开工前必须读的东西

按顺序读完，缺一不可：

| 文件 | 读它是为了 |
|---|---|
| `tests/diff/README.md` | 全量机制说明：结构、运行方式、比对口径、已知缺口四档声明、增量缓存 |
| `tests/diff/harness/core/types.ts` | `HarnessDeclaration` 的**全部**字段语义（这是能力声明的契约） |
| `tests/diff/probes/probe.hpp` | 探针的 `list` / `run` 协议与 `MC_DIFF_DEFINE_PROBE` 宏 |
| `tests/diff/harness/harnesses/index.ts` | 现有能力表，照着它的写法声明新能力 |
| `tests/diff/CMakeLists.txt` | 三张平行列表 + 按能力加链接依赖的位置 |
| `docs/CODE_CONVENTIONS.md`、`docs/PROJECT_CONVENTIONS.md` | 代码规范；差分产物同样受约束 |

同时至少精读**一个结构最接近你目标的能力**的完整实现（探针、oracle、声明三件套），例如纯数据类看 `block_properties`，带导出夹具类看 `recipe`，带多场景展开类看 `loot_container`。

---

# 3. 先判断：这个能力该不该做成差分测试

## 3.1 适合做差分测试

- **纯函数/纯查表**：输入是 ID 或参数，输出是数值、字符串、结构体；
- **确定性计算**：给定相同输入必然得到相同输出（含固定种子的随机数流）；
- **两侧都能无副作用地跑同一段逻辑**：Cubium 侧能通过 `mc_common` 直接调用，原版侧能用 oracle 直接调用。

## 3.2 不适合做差分测试

- 依赖真实玩家输入、网络时序、渲染的；
- 强依赖多 tick 状态演进且无法在两侧构造出**完全一致**的初始状态的（这类优先用 GameTest 集成测试，见 skill `mc-add-integrated-test`）；
- 原版行为本身不确定、或两侧浮点误差无法收敛且无法用 `normalise` 等价归一化的。

**判断不了就先问用户**，不要凭猜测开工。

## 3.3 与集成测试的区别（重要）

| | `tests/integrated`（GameTest） | `tests/diff` |
|---|---|---|
| 跑在哪 | 真实服务端 + 真 tick | 独立进程，只链 `mc_common` |
| 期望值 | 人手写的断言 | 原版自己算出来的 |
| 适合 | 跨 tick 的完整机制、实体行为、交互 | 纯数据、单步计算、可枚举的全集 |
| 覆盖 | 手写场景（易漏） | 全量枚举（不会漏，但难覆盖时序） |

二者互补，**不要用差分测试去重复集成测试已经覆盖的场景**，也不要用集成测试去覆盖本可以全量枚举的数据表。

---

# 4. 标准作业流程（七步）

## 第 1 步：定义「单元」与「比对字段」

先想清楚三件事，再动手写代码：

1. **单元（unit）是什么**：一个方块？一个物品？一个「表 × 场景」组合？单元键会出现在报告里，必须能唯一定位问题；
2. **输出字段有哪些**：只列**真正承载语义**的字段。不确定的字段不要加，宁缺勿滥——多一个不可靠的字段会制造大量伪差异；
3. **哪些字段属于已知缺口**：Cubium 尚未实现的，必须显式登记（见第 6 步），不能靠「干脆不比」来掩盖。

## 第 2 步：写 C++ 探针 `tests/diff/probes/<domain>.cpp`

骨架（照抄并按需裁剪，文件头必须带上项目的 MIT 版权注释）：

```cpp
#include <cstdio>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/** 只初始化一次的重型注册表（方块/物品等）。探针是单次进程，静态标志足够。 */
void ensureRegistries()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        VanillaBlocks::initialize();
        s_initialized = true;
    }
}

void writeList(FILE* out)
{
    ensureRegistries();
    // 必须**排序去重**：编排层会直接比对两侧列表，顺序不稳会制造伪差异。
}

std::string runLine(std::string_view inputLine)
{
    ensureRegistries();
    const json input = json::parse(inputLine);
    // 只输出确定性字段；无序容器必须先排序收敛。
    return json{{"block", input.at("block").get<std::string>()}, /* ... */}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("<domain>", &writeList, &runLine)

} // namespace mc::diff
```

**硬性约束**：

- **确定性**：同一输入永远产出同一输出。涉及无序容器（`unordered_map`/`unordered_set`）、指针地址、时间、未固定随机种子的，写出前必须自行收敛：排序、固定种子、去掉地址。
- **输出必须是一行 JSON**，不允许内嵌换行。
- **只链 `mc_common`**（必要时按第 5 步给单个探针追加源文件）。探针不启动服务端、不连网络、不创建真实世界。
- 浮点优先输出**位模式**（`0x%08x` of `memcpy` 到 `u32`），而不是十进制字符串——两侧格式化差异会让真差异看起来相同。
- 探针里抛异常是允许的：`runner_main.cpp` 会把它转成该行的 `{"error": ...}`，不会中断整体运行。但**不要用它来跳过单元**。

## 第 3 步：写 Java oracle `tests/diff/java/dev/horizonlab/diff/<Domain>Oracle.java`

骨架（`list` 把单元写进参数给定的文件；`run` 读 JSONL 写 JSONL）：

```java
package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * <能力>的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.&lt;Domain&gt;Oracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 */
public final class <Domain>Oracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private <Domain>Oracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();
        if (args.length == 2 && args[0].equals("list")) {
            // 枚举单元，**排序后**写文件。
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            // 逐行读、逐行算、逐行写。
            return;
        }
        System.err.println("usage: <Domain>Oracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }
}
```

**oracle 的硬性约束**（全部来自 `tests/diff/README.md`，这里重申最容易踩的几条）：

- **不能用 stdout 传协议数据**：原版 log4j 会占用 stdout。`list` 写文件，`run` 写文件。
- **需要数据驱动的注册表时**用 `DataPackRegistries.load()`（附魔、生物变种、战利品表、世界生成等都不在代码里）；只调 `Bootstrap.bootStrap()` 拿不到。
- **同包复用**：公共逻辑（如加载数据包、构造世界）放进同包的辅助类，`harness/runner/graph.ts` 的 `oracleSources()` 会自动解析同包闭包并纳入缓存键。但**只认真实存在的 `.java` 文件**，别指望动态生成。
- **新增 oracle 必须能独立 `main` 运行**，方便人工单独调试。
- **不要捕异常后继续**：oracle 内部错误应当直接抛出并终止，让编排层报环境问题，而不是静默产出空结果（否则会变成伪装成「一致」的假通过）。

## 第 4 步：登记到 `tests/diff/CMakeLists.txt`

在**三张平行列表的同一位置**各追加一项（顺序必须严格对应，CMake 末尾有长度一致性检查）：

```cmake
set(MC_DIFF_PROBE_SUFFIXES ... <domain>)
set(MC_DIFF_PROBE_SOURCES  ... probes/<domain>.cpp)
set(MC_DIFF_PROBE_DOMAINS  ... <domain>)
```

如果该探针需要额外的服务端源文件（例如要构造某个子系统），在 `mc_add_diff_probe()` 里按 `targetSuffix` 分支追加 `target_sources`。参考现有的 `command_tree`、`loot_table*` 分支写法。**不要**为此把整个 `mc_server` 链进来。

> 注意：`harness/data/probes.ts` 直接从这份 CMake 清单读域名，因此**只需改 CMake 一处**，编排层自动认识新探针。

## 第 5 步：在 `harness/harnesses/index.ts` 里声明能力

最小声明：

```ts
{ name: "block_flags", probe: "block_flags", oracle: "BlockFlagsOracle", key: "block",
  units: { kind: "listing" }, input: (unit) => ({ block: unit }) },
```

按需使用的字段（语义见 `core/types.ts`，此处是选择指引）：

| 字段 | 什么时候用 |
|---|---|
| `units: { kind: "computed", units }` | 单元不是单纯枚举，而是「表 × 场景」展开，或两侧枚举口径不同 |
| `beforeUnits` | 枚举单元前要先让 oracle 导出夹具（如 `recipe` 的锻造槽位） |
| `normalise` | 浮点格式差异（用现成的 `floatComparer()`）；**必须在被测侧是等价变换**，运行层会校验 |
| `unorderedArrays` | 语义上无序的数组路径（只对确实无序的用） |
| `oracleMissing` | 原版有、Cubium 未实现的**整单元**缺口 |
| `memberGaps` | 原版侧的**成员级**缺口（如未注册物品） |
| `projectOnly` | Cubium 独有的成员，从被测侧剔除 |
| `knownDifferences` | 已定位根因、修法已写 TODO 但需单独一轮的**已知差异**（不剔除数据，只在报告里点名） |
| `oracleArgs` | 让原版在注册阶段跳过 Cubium 的缺口，避免污染其余节点 |
| `exclusiveGroup` | oracle 要占用进程外固定资源（世界目录、端口），必须串行 |

**`knownDifferences` 的纪律**：`key: "*"` 必须带 `field`，禁止整单元放行（那等于关掉整个能力）。每一条都必须写明 `reason`，并对应源码里的 `TODO` 注明修法。**修好之后必须删掉登记**，否则新缺陷会被一并吞掉。

如果新增/修改了**所有能力共用**的文件（`core/comparison.ts` 等），记得同步更新 `core/types.ts` 里的 `SHARED_INPUTS`——它决定缓存失效范围，漏了会让「上次通过」的结论不可信。

## 第 6 步：构建并实跑

```bash
# 只构建本次要用的探针（会自动打开 MC_BUILD_DIFF_PROBES）
node tests/diff/harness/runner.ts build <domain>

# 只跑这一个能力
node tests/diff/harness/runner.ts <domain>

# 不复用原版输出缓存（改完 oracle 后建议）
node tests/diff/harness/runner.ts <domain> --fresh

# 查看报告（差异会裁剪到不相等的字段）
#   build/diff/<domain>/run/report.json
```

**构建规范**：Windows 上只允许 `./scripts/configure.sh build`（或让 runner 自己驱动构建）。构建等待时间必须 30 分钟以上，期间不允许并行做别的事。禁止自行 `git worktree add` 建第二棵树编译基线。

## 第 7 步：收敛结果

第一次跑出来有差异是**正常的、也是这个能力的意义所在**。处理顺序：

1. **先确认差异是真的**。逐条读报告，对照原版源码（`D:\Minecraft\MC研究\Minecraft1.21.11源码`）判断：
   - Cubium 写错了 → **改 Cubium**，这是本能力的核心价值；
   - 探针/oracle 写错了 → 改探针或 oracle；
   - 两侧格式差异 → 用 `normalise` 做**等价**归一化；
   - Cubium 确实没实现 → 登记 `oracleMissing` / `memberGaps` / `knownDifferences`，并补 `TODO`。
2. **不要为了让测试变绿而删字段或放宽比对**。如果某字段确实无法可靠比对，写明原因后登记，而不是静默去掉。
3. 每修好一批，重新跑，直到「无意外差异」或「只剩已登记的已知缺口」。

---

# 5. 已知缺口的四档登记（务必分清）

| 声明 | 含义 | 报告表现 | 修好后 |
|---|---|---|---|
| `oracleMissing` | 整条单元不参与比较（原版有、本项目没有） | 只报条数 | **删掉该条目** |
| `memberGaps` | 从**原版一侧**剔除某些成员 | 只报条数 | 删掉该成员 |
| `projectOnly` | 从**被测一侧**剔除本项目独有的成员 | 只报条数 | 删掉该成员 |
| `knownDifferences` | 差异照常产生、照常记录，只是不判失败 | 条数 + 每条理由 | 删掉整条登记 |

**共同纪律**：这些是「实现缺口清单」，不是「永久豁免」。清单必须每次运行输出条数；实现补齐后必须删条目，否则会出现「豁免了原版也有的项」的**假一致**，比不测更危险。

---

# 6. 新增能力时的常见陷阱

1. **探针与 oracle 的单元集合不一致**。两侧各自 `list` 得到不同集合时，运行层会报「probe 侧多出/缺少」。若差异源于 Cubium 未实现，用 `oracleMissing` 显式登记；不要靠改 `list` 去凑齐。
2. **浮点直接比十进制字符串**。JVM 的 `Float.toString` 与 C++ 的格式化最短表示不同，同一 f32 会印成不同文本。必须用位模式，或用 `floatComparer()` 并在被测侧做等价变换。
3. **数组顺序**。只有**语义上无序**的集合才可以声明 `unorderedArrays`。有序的数组（如命令树节点、路径节点序列）严格按项比对。
4. **oracle 缓存污染**。oracle 输出按「源码闭包 + jar + argv + 单元 + 输入」缓存。改了 oracle 却忘记 `--fresh` 时，运行层会检测「缓存命中却出现差异」并自动重跑一次；但改完 oracle 建议直接 `--fresh` 省事。
5. **探针退出崩溃**。持有实体、世界等全局状态的探针必须用 `MC_DIFF_DEFINE_PROBE_WITH_SHUTDOWN` 显式释放，否则静态析构阶段会访问已销毁的单例（表现为 `0xC0000005`，会被误判成环境问题）。
6. **把不可比的东西塞进比对条件**。例如 Cubium 里某字段来自材质默认值而非逐方块注册，这类系统性缺口应先补齐或登记为已知缺口，不要让它们混进通过条件（参考 `block_properties` 的做法）。
7. **在探针里做本属于编排层的事**。探针只负责「输入 → 输出」，用例生成、场景展开、排序策略属于 TS 侧（`harnesses/*.ts`），否则增量缓存和报告都失去意义。

---

# 7. 收尾清单

完成一个能力后逐项确认：

- [ ] 探针源文件带 MIT 版权头，放在 `namespace mc::diff` 内，用 `MC_DIFF_DEFINE_PROBE` 注册；
- [ ] `list` 输出稳定有序，`run` 输出单行 JSON 且确定性；
- [ ] oracle 用文件传协议数据，同包复用逻辑已抽成辅助类；
- [ ] `CMakeLists.txt` 三张列表**同一位置**各加一项，长度一致；
- [ ] `harnesses/index.ts` 声明齐全，`knownDifferences` 每条都有 `reason` 与对应 `TODO`；
- [ ] `node tests/diff/harness/runner.ts build <domain>` 与 `... <domain>` 都能跑通；
- [ ] 报告里没有未登记的意外差异，或每个差异都已定性并处理；
- [ ] 更新 `tests/diff/README.md` 的「当前状态」表格：能力名、参照实现、单元数、结果、已知缺口；
- [ ] 如果首轮查出了 Cubium 的真错误，把「查出什么、怎么修的」写进 README 的对应章节（这是差分测试最有价值的部分）；
- [ ] 修改过的 C++ 文件已用 `node scripts/format/clang_format_all.ts` 格式化（Node 24）；
- [ ] 若新增了共用文件，`core/types.ts` 的 `SHARED_INPUTS` 已同步。

---

# 8. 参考：已有的能力形态

| 形态 | 参考能力 | 看点 |
|---|---|---|
| 纯枚举 + 单字段 | `block_properties` | 最小实现，适合起步 |
| 全量状态集合 | `block_state_names` / `block_state_ids` | `list` 枚举的量级可以很大 |
| 导出夹具后比对 | `recipe` | `beforeUnits` + oracle 导出共同输入 |
| 表 × 场景展开 | `loot_table_entity` / `loot_container` | `units: computed` + `exclusiveGroup` |
| 浮点归一化 | `damage_reduction` | `floatComparer()` 与「末位 1 ulp」处理 |
| 成员级缺口 | `item_tags` | `memberGaps` 的用法 |
| 项目独有成员 | `attribute_range` | `projectOnly` 的用法 |