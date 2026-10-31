# 差分测试（differential tests）

同一份输入喂给 Cubium 与参照实现（原版数据或 Java oracle），要求两侧输出**逐字节相同**。

这是项目的第四类测试：

| 目录 | 回答什么 | 期望值从哪来 |
|---|---|---|
| `tests/unit` | 单个函数/类对不对 | 人写的断言 |
| `tests/integrated` | 行为包在真实服务端里跑得对不对 | GameTest 的通过条件 |
| `tests/e2e/bot` | 真客户端连上来能不能玩 | 冻结的快照基线 |
| **`tests/diff`** | **和原版比，同一输入下结果一样吗** | **原版自己跑一遍** |

前三类里「期望值」都由人决定，人写错就查不出来；差分测试的期望值由原版给出，不存在写错。

---

## 结构

```
tests/diff/
├── CMakeLists.txt               # probe 目标定义（开启差分选项后随常规构建编译）
├── probes/                      # C++ 侧小程序，只链 mc_common
│   ├── probe.hpp                # Probe 接口 + MC_DIFF_DEFINE_PROBE 宏
│   ├── runner_main.cpp          # 所有 probe 共用的 main（命令行、IO、错误处理）
│   ├── text_encode.cpp          # 方块状态的文本表示
│   ├── block_state_ids.cpp      # 方块状态的全局 ID（协议 / 存档编号）
│   ├── biome_climate.cpp        # 生物群系温度 / 降水 / 颜色
│   ├── damage_type.cpp          # 伤害类型缩放 / 体力消耗 / 附加效果
│   ├── block_tags.cpp           # 方块标签展开后的成员
│   ├── item_tags.cpp            # 物品标签展开后的成员
│   ├── item_summary.cpp         # 堆叠上限 / 食物属性
│   ├── item_components.cpp      # 耐久 / 稀有度 / 附魔能力
│   ├── item_component_nbt.cpp   # 物品组件补丁的 NBT 表示（逐组件 SNBT 比对）
│   ├── stack_codec.cpp          # 物品堆整体编解码（id / count / components 与失败边界）
│   ├── entity_shape.cpp         # 实体尺寸 / 分类
│   ├── enchantment_table.cpp    # 附魔等级与附魔台代价
│   ├── loot_table.cpp           # 战利品表掷骰（最小上下文：origin / 中性方块 / 空工具）
│   ├── loot_table_block.cpp     # 战利品表掷骰（挖掘上下文：方块状态 + 时运工具）
│   ├── loot_table_entity.cpp    # 战利品表掷骰（击杀上下文：this_entity + 攻击者 + 掠夺）
│   ├── loot_function.cpp        # 战利品函数逐函数比对（单池单条目 + 表级单函数）
│   ├── vanilla_datapack.hpp     # 探针共用：定位原版数据包、初始化注册表、载入数据包标签
│   ├── mob_category.cpp         # 生物分类的每区块上限 / 生成参数
│   ├── block_properties.cpp     # 硬度 / 亮度 / 滑度 / 是否流体
│   ├── block_experience.cpp     # 方块被破坏时掉落的经验（按区间采样序列）
│   ├── item_attribute_modifiers.cpp # 物品按装备槽位提供的属性修饰符
│   ├── damage_reduction.cpp     # 护甲减伤公式（含破甲附魔）
│   ├── gamerule.cpp             # 游戏规则的分类 / 类型 / 默认值 / 范围
│   ├── armor_material.cpp       # 护甲材质的耐久 / 护甲值 / 韧性与附魔能力
│   ├── tool_material.cpp        # 工具材质的耐久 / 效率 / 攻击伤害加成 / 附魔能力
│   ├── enchantment_effects.cpp  # 附魔各等级的伤害加成 / 保护点数 / 属性修饰符
│   ├── attribute_range.cpp      # 属性的默认值与取值上下限
│   ├── mob_experience.cpp       # 生物死亡掉落的基础经验值
│   ├── block_destroy_progress.cpp # 方块破坏速度与每 tick 进度（正确/错误工具）
│   ├── block_state_names.cpp    # 方块状态名全集
│   ├── entity_attributes.cpp    # 生物属性默认值
│   ├── random_sequence.cpp      # Xoroshiro / JavaLegacy 随机数接口序列
│   ├── status_effect.cpp        # 效果实例操作、共享隐藏链与时长推进
│   ├── command_argument.cpp     # 命令参数解析
│   ├── command_tree.cpp         # 命令图、参数编码、节点属性与完整协议字节
│   ├── advancement.cpp          # 进度定义
│   ├── biome_spawns.cpp         # 生物群系自然生成配置
│   ├── spawn_placement.cpp      # 实体放置类型、高度图与受控方块柱中的位置判定
│   ├── spawn_rules.cpp          # 生成谓词、位置检查与后续随机抽样
│   ├── recipe.cpp               # 配方表（类型 / 原料 / 结果 / 烹饪）
│   ├── crafting_match.cpp       # 合成配方在固定网格语料下的匹配与产出
│   ├── single_item_recipe.cpp   # 单物品配方的查询语义（熔炼/高炉/烟熏/营火/切石）
│   ├── villager_trades.cpp      # 村民交易：整表导出 / 生成抽取 / 价格公式 / 补货
│   ├── loot_world.hpp           # 战利品探针共用的空世界 / 掷骰入口
│   └── loot_probe_common.hpp    # 战利品探针共用的样本循环与摘要
│   ├── path_scene.hpp           # 寻路探针共用的内存方块表区域与受控实体
│   ├── walk_path_type.cpp       # 逐格路径类型（方块规则 / 脚下+邻域 / 实体包围盒三层）
│   ├── path_finder.cpp          # 地面 A*：起点、邻居枚举、完整路径与可达性
│   ├── fly_path_finder.cpp      # 飞行 A*：26 邻居与飞行起点枚举
├── java/dev/horizonlab/diff/    # 原版 Java oracle（直接调 1.21.11 服务端 jar）
│   ├── DataPackRegistries.java  # 像服务端一样加载原版数据包（数据驱动注册表）
│   ├── *Oracle.java / *Export.java
├── harness/                     # TypeScript 编排层（Node 24 原生 type stripping）
│   ├── runner.ts                # CLI 入口：run / build / graph，选择器、调度、状态
│   ├── worker.ts                # 执行进程：一个能力一个进程，装模块追踪、跑、回报 JSON
│   ├── core/                    # 与「怎么跑」无关的纯逻辑
│   │   ├── comparison.ts        # 字段投影与比较（严格数组长度，仅声明为无序的才忽略顺序）
│   │   ├── snbt.ts              # SNBT 文本的规范形（引号风格 / 浮点后缀 / 键序）
│   │   ├── hash.ts              # 内容哈希（纳秒 mtime + 大小的缓存）
│   │   └── types.ts             # 能力声明、运行上下文、共享输入清单
│   ├── oracle/                  # 原版 Java 一侧
│   │   ├── environment.ts       # 定位原版 jar、解包依赖库、拼 classpath
│   │   └── cache.ts             # 原版输出缓存（键 = oracle 源码闭包 + 工具链 + argv + 单元 + 输入）
│   ├── data/                    # 离线数据源
│   │   ├── vanilla.ts           # minecraft-data（少数能力的参照数据）
│   │   └── probes.ts            # 从 CMakeLists 读出探针清单（唯一事实来源）
│   ├── harnesses/               # 声明式能力表（纯元数据）
│   │   ├── index.ts             # HARNESSES 注册表 + 各能力的比对口径与已知缺口
│   │   ├── text_encode.ts       # 方块状态文本与属性摘要
│   │   ├── loot_context.ts      # 战利品三探针的上下文模板
│   │   ├── loot_container.ts    # 容器战利品场景与已知缺口
│   │   ├── loot_function.ts     # 战利品函数逐个场景（输入物品 / 函数 JSON / 上下文）
│   │   ├── recipe.ts            # 导出配方夹具，生成两侧共用的锻造三槽输入
│   │   ├── crafting_match.ts    # 合成配方的摆法语料（原样 / 放大 / 平移 / 反例）
│   │   ├── single_item_recipe.ts # 单物品配方查询的比对口径（补齐五个结果键）
│   │   ├── status_effect.ts     # 效果实例的共同边界语料与固定操作序列
│   │   └── known_missing_*.ts   # 实现缺口清单（物品 / 实体 / 方块工具 / 命令）
│   └── runner/                  # 运行层实现
│       ├── layout.ts            # 构建目录、产物路径、ninja 位置、仓库根
│       ├── graph.ts             # 依赖图：能力依赖哪些仓库文件、一次改动影响谁
│       ├── state.ts             # 状态文件（内容摘要、依赖清单、上次结果）
│       ├── select.ts            # 选择器：--all / --changed / --failed / 缺省增量
│       ├── scheduler.ts         # 并行调度（每能力一进程、最长优先、互斥组）
│       ├── session.ts           # 跑两侧、比对、写报告
│       ├── build.ts             # 只构建本次要用的探针
│       └── module-trace.ts      # 运行期模块追踪（记录能力实际加载了哪些 TS 文件）
├── tools/
│   ├── ninja.ts                 # 读 ninja 构建图：可执行文件的 .obj 闭包与逐目标依赖
│   └── export_entity_attributes_reference.ts   # 用 oracle 重算生物属性参考表
```

运行产物全部落在 `build/diff/<能力>/`：

| 路径 | 内容 |
|---|---|
| `run/inputs.jsonl` | 两侧共用的输入（每行一个单元） |
| `run/probe_outputs.jsonl` | 探针输出 |
| `run/report.json` | 比对报告：一致数、差异（裁剪到不相等的字段）、已知差异与理由 |
| `java/` | oracle 的类目录与工作文件（也是依赖记录里的「构建产物」，不与 `run/` 混放） |
| `../state/state.json` | 增量状态：每个能力的内容摘要、依赖清单、上次结论 |
| `../state/oracle/` | 原版输出缓存（按内容摘要命名） |

---

## 运行

```bash
# 缺省：只跑「依赖内容与上次不同」的能力（增量；全部已通过时几秒内结束）
node tests/diff/harness/runner.ts

# 全部能力，忽略通过记录
node tests/diff/harness/runner.ts --all

# 单个 / 几个
node tests/diff/harness/runner.ts item_components
node tests/diff/harness/runner.ts command_tree loot_table_entity

# 只跑受某次改动影响的能力（先看会跑哪些用 --list）
node tests/diff/harness/runner.ts --changed origin/main
node tests/diff/harness/runner.ts --changed-files changed.txt --list

# 只跑上次没通过的；查看每个能力上次的结论
node tests/diff/harness/runner.ts --failed
node tests/diff/harness/runner.ts --status

# 不复用原版输出缓存
node tests/diff/harness/runner.ts --all --fresh

# 只构建全部 / 指定的 probe
node tests/diff/harness/runner.ts build
node tests/diff/harness/runner.ts build item_components

# 打印依赖图（每个能力依赖多少仓库文件、多少运行期模块）
node tests/diff/harness/runner.ts graph

# 指定其它构建目录（默认 <仓库>/build）
MC_DIFF_BUILD_DIR=build-noprof node tests/diff/harness/runner.ts --all
```

常用参数：`-j <n>` 并发度（默认 CPU 核心数/3、上限 8）、`--no-build` 跳过构建、
`--timeout <秒>` 单能力超时（默认 1800）、`--samples <n>` 终端里展示的差异条数（默认 5）。

退出码：`0` 无意外差异（含「只有已登记的已知差异」）；`1` 存在意外差异；
`2` 环境问题（缺 probe、构建失败、缺原版数据）。

### 增量、缓存与选择

编排层的设计目标是「改了什么就跑什么，没改就不跑，跑过的不重跑」：

1. **内容摘要**：每个能力一份依赖清单（probe 的链接闭包 + oracle 源码闭包 + 运行期实际加载
   的 TS 模块 + probe 产物 + 环境指纹），按文件内容哈希成一个摘要写进
   `build/diff/state/state.json`。摘要没变且上次通过 → 直接跳过。
2. **快速路径**：缺省选择只哈希「上次记下的依赖清单」（有缓存时约 0.1 秒）；只有摘要变了才去
   建图（`ninja -t compdb` + 逐目标 `-t deps`，本机约 35 秒）。
3. **原版输出缓存**：oracle 的输出按 `oracle 源码闭包 + jar/依赖库 + argv + 单元 + 输入` 缓存，
   缓存命中时只跑探针。**命中却出现差异时不会直接采信**——会重跑一次 oracle，重跑仍不一致
   才算真差异，一致则判定缓存被污染并删除该键。
4. **只构建要用的探针**：`build <name>` 只构建该能力对应的 `diff_<探针>` 目标。
5. **选择器**（`runner/select.ts`）：`--changed` 用依赖图判定「哪些能力依赖改动的文件」，
   并且**不**用通过记录过滤——显式给出一批改动时，用户就是在要求验证它们。
   注意依赖图是**刻意过宽**的：C++ 侧按静态库边界展开，改一个 `mc_common` 的头会让所有
   probe 重跑；这比用符号表反推「真正被引用」稳，误触发的代价只是慢几十秒。
6. **并行**：一个能力一个子进程（模块解析钩子是进程级的，且能把崩溃/OOM 隔离在这一个能力里），
   上次耗时长的先跑。`exclusiveGroup` 声明的能力互不并发——战利品族 oracle 要在
   `java.io.tmpdir` 建原版服务端世界并持有区域文件锁，并行会直接锁死。

本机实测（28 核，Windows）：全量 37 个能力 `-j 8` 约 3 分 20 秒；无改动重跑约 3 秒
（`--all` 会全部重跑，约 3 分 20 秒）；改动一个 probe 源文件只跑该能力。

**前置**：

1. `MC_BUILD_DIFF_PROBES` 默认为 `OFF`，`runner.ts build` 会自动打开它；Windows 上它通过
   `scripts/configure.bat` 注入 VS 环境后再调 `cmake --build`，与人工构建共用同一个构建目录。
2. **JDK 21+**（`javac`/`java` 在 PATH 上）。Java oracle 直接拿原版 jar 当参照实现，
   不需要 Gradle。
3. 原版 jar 与依赖库。默认从 fabric-loom 缓存找：

   ```
   %USERPROFILE%/.gradle/caches/fabric-loom/minecraftMaven/net/minecraft/minecraft-merged/...
   %USERPROFILE%/.gradle/caches/neoformruntime/artifacts/minecraft_1.21.11_server.jar
   ```

   找不到时用环境变量显式指定：`MC_VANILLA_MERGED_JAR`（反混淆 merged jar）、
   `MC_VANILLA_SERVER_JAR`（bundler，用于解出 39 个依赖库），或直接给
   `MC_VANILLA_LIBRARIES`（已解包的依赖目录）。
4. `item_summary` 用 minecraft-data 作参照：`tests/e2e/bot/node_modules/minecraft-data`，
   未安装时 `cd tests/e2e/bot && npm install`。

---

## 一个 harness 的协议

C++ probe 只有两个子命令：

```
diff_<domain> list                          # 每行一个单元（方块状态文本）
diff_<domain> run <input.jsonl> <output.jsonl>   # 每行输入 → 一行 JSON 输出
```

`list` 枚举「这个能力能提供的全部单元」，`run` 按单元出结果，两者不必是同一个集合——
比对什么由能力声明决定。运行层负责按 `key` 对齐两侧输出、规范化、比对、写报告。

**比较口径**（`core/comparison.ts`）：

- 只比参照实现声明过的字段：参照没给的信息是「没得比」，不是「不一致」；
- 数组必须等长；只有能力显式声明为无语义的路径（`unorderedArrays`）才按规范序重排；
- 对象键序不参与比较（同一次判定同时接受 `{"a":1,"b":2}` 与 `{"b":2,"a":1}`）；
- 浮点的写法差异（Gson 写 f32 最短表示 `0.485`，nlohmann 写 `0.48500001430511475`）由能力的
  `normalise` 处理，**且只在被测一侧做**：运行层会校验规范化对参照侧是恒等映射
  （`normalisationIsFaithful`），防止「两侧都经过它」把真差异一起抹掉；
- 报告里的差异只保留**不相等的字段**（递归裁剪到第一层不等处、等长数组逐个对齐），
  否则命令树的 `wire` 一条就能刷掉几十 KB。

**已知缺口必须显式登记**，四档，按粒度从粗到细：

| 声明 | 含义 | 报告里的表现 |
|---|---|---|
| `oracleMissing` | 整条单元不参与比较（原版有、本项目没实现） | 只报条数 |
| `memberGaps` | 从**原版一侧**剔除某些成员（未注册的物品等） | 只报条数 |
| `projectOnly` | 从**被测一侧**剔除本项目独有的成员（自研命令等） | 只报条数 |
| `knownDifferences` | 差异照常产生，但登记在案、不判失败 | 条数 + 每条的理由 |

`knownDifferences` 的匹配规则：单元键完全相同（或用 `*` 表示所有单元），且（若给了
`field`）差异的顶层字段包含它。`*` 必须带 `field`——整单元放行的通配等于关掉整个能力，
不允许。已登记的已知差异在结果行里也会点名：

```
[36/37] loot_container   通过   116.3s  0/5296 一致；5296 处已登记的已知差异
```

一旦这条缺口被修好，必须删掉对应登记，否则新缺陷会被一并吞掉。

---

## 新增一个能力

1. 在 `probes/<domain>.cpp` 实现两个函数并调用宏：

   ```cpp
   void writeList(FILE* out);                        // 或不需要就传 nullptr
   std::string runLine(std::string_view inputLine);
   MC_DIFF_DEFINE_PROBE("<domain>", &writeList, &runLine)
   ```

   放在 `namespace mc::diff { ... }` 内（宏本身不开 namespace）。

2. 在 `tests/diff/CMakeLists.txt` 的三张平行列表**同一位置**各加一项：

   ```cmake
   set(MC_DIFF_PROBE_SUFFIXES text_encode encode_nbt)
   set(MC_DIFF_PROBE_SOURCES  probes/text_encode.cpp probes/encode_nbt.cpp)
   set(MC_DIFF_PROBE_DOMAINS  text_encode encode_nbt)
   ```

3. 在 `java/dev/horizonlab/diff/` 写 `<Domain>Oracle.java`（`list` / `run` 两个子命令），
   在 `harness/harnesses/index.ts` 的 `HARNESSES` 表里加一条声明：

   ```ts
   { name: "block_flags", probe: "block_flags", oracle: "BlockFlagsOracle", key: "block",
     units: { kind: "listing" }, input: (unit) => ({ block: unit }) }
   ```

   `.java` 由 `oracle/environment.ts` + `runner/session.ts` 在运行前编译（javac 幂等），
   不需要额外构建步骤；oracle 的同包 import 闭包会被依赖图自动追踪。

   声明的其余字段按需使用：`normalise`（浮点比较等）、`unorderedArrays`、`memberGaps`、
   `projectOnly`、`knownDifferences`、`oracleMissing`、`oracleArgs`（追加给 oracle 的参数，
   用于让参照侧跳过已知缺口）、`beforeUnits`（枚举单元前导出夹具，见 `recipe.ts`）、
   `exclusiveGroup`（与占用同一进程外资源的能力互斥，见上方「增量、缓存与选择」）。

   `units` 默认 `listing`（有 oracle 时由 oracle 的 `list` 枚举，否则由探针枚举）；
   两侧枚举口径不同时用 `computed` 自己组合（`text_encode` 用探针的状态全集提炼方块名，
   再用 oracle 的状态全集判定「原版有没有」）。

   读数据包的能力（战利品表、进度、群系生成、配方）：探针从 `MC_DIFF_VANILLA_DATAPACK`
   拿到与 oracle 同一个数据包目录，共用初始化在 `probes/vanilla_datapack.hpp`。

**确定性是硬要求**：同一输入必须永远产出同一输出。涉及无序容器、随机数、时间、
地址的，probe 必须在写出前自行收敛（排序、固定种子、去掉地址），否则差分没有意义。

完整的作业流程（含探针/oracle 骨架、已知缺口四档如何选、构建与收敛步骤、收尾清单与常见陷阱）
见 skill `mc-add-diff-test`（`.agents/skills/mc-add-diff-test/SKILL.md`）。

---

## 当前状态

以下能力已接入。结果描述最近一次验证；`spawn_placement` 单独比较实际位置判定，
不把静态群系配置通过视为运行时生成规则通过。

| harness | 参照实现 | 单元 | 结果 | 已知缺口 |
|---|---|---|---|---|
| `text_encode` | 原版 jar `BlockStateOracle` | 1166 个方块（状态全集 29671 条由各自的 `list` 枚举） | 全部一致 | 原版没有、Cubium 多注册的方块会被列为「probe 侧多出」 |
| `item_summary` | 原版 jar `ItemSummaryOracle` | 1124 | 全部一致 | 381 个物品未注册 |
| `item_components` | 原版 jar `ItemComponentsOracle` | 1124 | 全部一致 | 381 个物品未注册 |
| `entity_shape` | 原版 jar `EntityShapeOracle` | 125 | 全部一致 | 32 个实体类型未注册 |
| `enchantment_table` | 原版 jar `EnchantmentExport` | 43 | 全部一致 | — |
| `block_properties` | 原版 jar `BlockPropertiesOracle` | 1166 | 全部一致 | — |
| `mob_category` | 原版 jar `MobCategoryOracle` | 8 | 全部一致 | — |
| `entity_attributes` | 参考表 + 原版 jar `EntityAttributesOracle` | 73 | 全部一致 | 2 个实体类型未对齐（盔甲架、凋灵） |
| `block_state_names` | 原版 jar `BlockStateNamesOracle` | 1166 | 全部一致 | — |
| `block_state_ids` | 原版 jar `BlockStateIdsOracle` | 1166 | 全部一致 | — |
| `biome_climate` | 原版 jar `BiomeClimateOracle` | 65 | 全部一致 | — |
| `damage_type` | 原版 jar `DamageTypeOracle` | 50 | 全部一致 | — |
| `block_tags` | 原版 jar `BlockTagOracle` | 204 | 全部一致 | — |
| `item_tags` | 原版 jar `ItemTagOracle` | 198 | 全部一致 | 300 条成员引用了未注册物品 |
| `random_sequence` | 原版 jar `RandomSequenceOracle` | 24 | 全部一致 | — |
| `status_effect` | 原版 jar `StatusEffectOracle` | 2430 个场景 / 48,822 次操作 | 全部一致 | — |
| `command_argument` | 原版 jar `CommandArgumentOracle` | 162 | 全部一致 | — |
| `command_tree` | 原版 jar `CommandTreeOracle` | 1（全部根命令）| `tree`/`arguments`/`components` 一致；`wire` 为已登记的已知差异 | 9 条命令未实现（见下）；`/panel` 使整棵树编码不可比（见下） |
| `loot_table` | 原版 jar `LootTableOracle` | 1324 | 全部一致 | — |
| `loot_table_block` | 原版 jar `LootTableBlockOracle` | 1324 | 全部一致 | — |
| `loot_table_entity` | 原版 jar `LootTableEntityOracle` | 5288（1324 张表 × 4 个场景） | 5274 一致；14 处为已登记的已知差异（13 张表的 `#player` + `zombie_nautilus`） | 2 张表的实体类型建不出来（`creaking`、`mannequin`），探针注释待核对 |
| `loot_container` | 原版 jar `LootContainerOracle` | 5296（1324 张表 × 4 个场景） | 两段均为已登记的已知差异（见下） | `slots` 随机源未对齐、`drop` 字段集不一致 |
| `advancement` | 原版 jar `AdvancementOracle` | 1584 | 全部一致 | — |
| `biome_spawns` | 原版 jar `BiomeSpawnsOracle` | 65 | 全部一致 | — |
| `spawn_placement` | 原版 jar `SpawnPlacementOracle` | 157 种实体 × 332 场景 | 全部一致 | — |
| `spawn_rules` | 原版 jar `SpawnRulesOracle` | 157 种实体 / 168,876 个场景 | 全部一致 | — |
| `recipe` | 原版 jar `RecipeOracle` | 1470 | 全部一致 | — |
| `crafting_match` | 原版 jar `CraftingMatchOracle` | 4068（有序/无形状配方 × 6 种摆法） | 全部一致 | 见下文 |
| `single_item_recipe` | 原版 jar `SingleItemRecipeOracle` | 202 个输入物品（熔炼/高炉/烟熏/营火/切石） | 全部一致 | 见下文 |
| `block_experience` | 原版 jar `BlockExperienceOracle` | 1166 | 全部一致 | — |
| `item_attribute_modifiers` | 原版 jar `ItemAttributeModifiersOracle` | 1124 | 全部一致 | 381 个物品未注册 |
| `damage_reduction` | 原版 jar `DamageReductionOracle` | 2415 种伤害×护甲×韧性组合 | 全部一致（4 位小数） | — |
| `gamerule` | 原版 jar `GameRuleOracle` | 59 | 全部一致 | — |
| `armor_material` | 原版 jar `ArmorMaterialOracle` | 9 种材质 × 5 槽位 | 全部一致 | — |
| `tool_material` | 原版 jar `ToolMaterialOracle` | 7 | 全部一致 | 采集等级不比（原版用方块标签表达） |
| `enchantment_effects` | 原版 jar `EnchantmentEffectsOracle` | 43 | 全部一致 | 宝藏枚举与附魔台代价由 `enchantment_table` 覆盖 |
| `attribute_range` | 原版 jar `AttributeRangeOracle` | 35 | 全部一致 | 项目自研属性 `horse.jump_strength` 原版没有 |
| `mob_experience` | 原版 jar `MobExperienceOracle` | 125 | 全部一致 | 32 个实体类型未注册 |
| `block_destroy_progress` | 原版 jar `BlockDestroyProgressOracle` | 971 | 705 个一致 | 461 个方块的采集工具/requiresTool 未声明（见 `known_missing_block_tools.ts`） |
| `structure_density` | 原版 jar `StructureDensityOracle` | 采样点集 | 全部一致 | 覆盖 Beardifier 的四种地形适配（bury / beard_thin / beard_box / encapsulate） |
| `structure_starts` | 原版 jar `StructureStartsOracle` | 542 个区块（候选区块 + 邻居 + 空白区块，两个种子 × 主世界/末地） | 494 个单元逐片段一致；528/540 个单元选中同一结构 | 见下方「结构起点：已查出的问题」 |
| `villager_list` | 原版 jar `VillagerTradesOracle` | 13（每职业整张交易表，5 个等级的全部条目） | 全部一致 | — |
| `villager_price` | 原版 jar `VillagerTradesOracle` | 1137（基础代价 × 需求 × 乘数 × 特殊价格） | 全部一致 | — |
| `villager_restock` | 原版 jar `VillagerTradesOracle` | 600（单条交易 + 多条交易） | 全部一致 | — |
| `villager_gen` | 原版 jar `VillagerTradesOracle` | 390（13 职业 × 5 等级 × 6 种子） | 355/390 一致；35 个附魔内容差异已登记 | 见下方「村民交易：已查出的问题」 |
| `piston_movement` | 原版 jar `PistonMovementOracle` | 523 个场景（链条 / 粘性分支 / 随机混合台 / 收回） | 全部一致 | — |
| `experience` | 原版 jar `ExperienceOracle` | 440 个操作序列（公式阶梯 / 单次加点 / 扣点 / 加减等级 / 随机） | 全部一致 | — |
| `potion` | 原版 jar `PotionOracle` | 46 种药水（基础名 / 效果列表 / 瞬时效果 / 三种前缀翻译键） | 全部一致 | — |
| `potion_brewing` | 原版 jar `PotionBrewingOracle` | 874（46 种药水 × 19 种试剂） | 全部一致 | — |
| `block_support` | 原版 jar `BlockSupportOracle` | 1166 个方块 × 23 种底座 | 见下文（218 → 75 处） | 部分（见「已知缺口」） |
| `food_properties` | 原版 jar `FoodPropertiesOracle` | 44 种带食物的物品 | 全部一致 | — |
| `entity_type_tags` | 原版 jar `EntityTypeTagsOracle` | 46 个实体类型标签 | 全部一致 | — |
| `jukebox_song` | 原版 jar `JukeboxSongOracle` | 21 首唱片机歌曲 | 全部一致 | — |
| `enchantment_tags` | 原版 jar `EnchantmentTagsOracle` | 22 个附魔标签 | 全部一致 | — |
| `painting_variant` | 原版 jar `PaintingVariantOracle` | 51 种画变体 | 全部一致 | — |
| `damage_type_tags` | 原版 jar `DamageTypeTagsOracle` | 33 个伤害类型标签 | 全部一致 | — |
| `enchantment_cost` | 原版 jar `EnchantmentCostOracle` | 43 个附魔的等级代价曲线 | 全部一致 | — |
| `enchantment_applicability` | 原版 jar `EnchantmentApplicabilityOracle` | 43 个附魔 × 全物品 | 见下文（28 处） | 部分（见「已知缺口」） |
| `item_component_nbt` | 原版 jar `ItemComponentNbtOracle` | 1124 物品 × 3 变体（plain / suite / removed） | 3372/3372 一致 | 381 个物品未注册（方块物品，同 `known_missing_items.ts`） |
| `stack_codec` | 原版 jar `StackCodecOracle` | 3021 单元（1124 物品 × base / suite + 11 个数量与坏数据场景） | 2256/2259 一致；3 处已登记的已知差异 | 见下文 |
| `loot_function` | 原版 jar `LootFunctionOracle` | 41 个单函数场景（随机值域 / 时运加成 / 截断 / 伤害 / 附魔 / 组件 / 组合） | 全部一致 | 见下文 |

`loot_function` 把范围从「整张表掷出来什么」收窄到**单个函数**：两侧各现场造一张「1 个池
+ 1 个必定产出一条物品」的表，表级只挂一个函数，然后在同一批种子上掷 4 次。差异因此只
可能来自函数本身，而函数恰好是战利品体系里唯一大量消费随机源的一层——**取数顺序错一位**
（先判断再取数、区间端点写反）就会让整条链偏移，这类错误在手写期望值里几乎不可能发现。

参数集两侧都用 `block`（原版 `LootContextParamSets.BLOCK`，Cubium 侧
`LootParameterSets::block()`）：它是唯一同时允许 `tool` 与 `explosion_radius`、又不必造
玩家/伤害来源桩值的参数集。

- **不能改用 `entity`**：它不允许这两个键出现，而 `apply_bonus` 要从 `tool` 读附魔等级、
  `explosion_decay` 要读半径，两者都是「键在就执行、不在就原样返回」——oracle 侧会静默
  不执行，差异指向的是 oracle 缺键而不是 Cubium。
- **不能改用 `generic` / `ALL_PARAMS`**：原版把它十项全部登记为**必需**，`withParameter`
  传 `null` 会被直接拒绝，必须造出 `ServerPlayer` 与 `BlockEntity` 的桩值。

所谓「掠夺等级」就落在 `tool` 这个键上（原版 `ApplyBonusCount` 从
`getOptionalParameter(TOOL)` 取附魔等级），两侧都给一把带 `looting` 附魔的铁剑。

首轮跑通查出 41 处差异，逐个收敛到 0：

- **`set_count#uniform_float_bounds` 差一格**：原版 `UniformGenerator.getInt` 是
  `Mth.nextInt(rand, min.getInt(), max.getInt())`，端点走 `Math.round`（四舍五入），
  而 Cubium 的 `RandomValueRange::generateInt` 用的是截断。已在 `RandomRanges.hpp` 新增
  `javaRoundToInt` 并对齐——这不只是取整口径，它会平移整段随机数的取值区间。
- **`apply_bonus#binomial_with_bonus` 与 `explosion_decay#plain` 两侧产出不同**：如上，oracle
  原来用 `entity` 参数集，读不到 `tool` / `explosion_radius`，这两条函数静默不执行。改
  `block` 后两侧在同一批种子上逐位一致。
- **`set_custom_data` 解析失败**：原版这一项的 codec 是 `TagParser.LENIENT_CODEC`，字段
  形态是 **SNBT 文本**（`"{DiffProbe:{level:3}}"`），不是 JSON 对象。
- **`set_instrument` 标签查不到**：`goat_horn_sounds` 不存在，改用数据包里真实存在的
  `#minecraft:goat_horns`；同时给探针环境补上 `initializeVanillaInstrumentTags()`（内置
  标签必须先于从数据包补充的标签就位，否则加载器因「成员未知」整批失败）。
- **`filtered` / `set_lore` / `set_contents` / `set_loot_table` 解析失败**：字段名分别是
  `item_filter`（不是 `modifier`）、`mode`、`component`、`type`（后三者必填）。
- **条目数量多叠一层 `set_count`**：条目数量直接取输入 `count`，不再额外套
  `SetItemCountFunction`——多一层会让每个场景凭空多消费一次随机数，后面所有函数的取数
  全部错位。

`stack_codec` 首轮跑通共 7 处差异，修掉前三条后剩 3 处（数量越界，已登记为已知差异——原版
`orElse(1)` 对越界宽容，Cubium 按源码字面语义判为无效）。修掉的是「读存档时该失败的
地方没失败」：

- **count 越界被静默接受**（0 / 100 / -1）：`count <= 0 → 空堆` 让 100 与 -1 都能读进来。
  已按 1..99 校验（原版 `orElse` 是宽容的，两侧因此在这三个取值上仍分叉，见上）。
- **未知物品变成空堆**：原版 `Item.CODEC` 查不到注册项就是解码失败；Cubium 原来返回 `EMPTY`，
  坏存档会静默变成空气。已改为返回错误。
- **空气被当作合法物品堆**：原版 `ItemStack.CODEC` 明确拒绝 `minecraft:air`。已改为失败；
  `toNbt` 对空堆改写 `id=air, count=1`（count 0 在区间外，读回来必然失败）。

同一轮还修掉了两个组件层的崩溃/丢数据缺陷（都在 `DataComponentPayloadCodec` 与 `ItemStack`）：

- **lore / tool / can_place_on / can_break 从存档读不回来**：这些列表的承载类型取决于来源，
  数据包加载器造紧凑列表（`string_list_tag` / `compound_list_tag`），存档与网络读进来的是
  通用 `tag_list_tag`。代码按 `element_id()` 判断后强行 `dynamic_cast`，在后者上抛出
  `std::bad_cast`——整条物品堆读不回来。改为两种承载都认。
- **rarity 等于原型值时仍写进补丁**：原版 `asPatch()` 只报相对原型的差异，命令方块、鞘翅、
  龙头这类本来就非 common 的物品因此凭空多出一项 `minecraft:rarity`。改为与原型比较后再写。

`item_component_nbt` 首轮跑通查出并修掉的组件编码问题（全部是「能写进存档、但字节与原版不同」
或「读回来就丢」的静默错误）：

- **整数标量被窄化成 Byte**：组件载荷解析对非对象值走 `jsonToNbt`，它会按值域把 `7` 缩成
  `7b`，而 `minecraft:damage` 用的是 `Codec.INT`——整个组件因此被丢掉。改为统一的严格整数转换。
- **整数列表被提升成 double**：`_normalizeNumbers` 把所有数字一律转 double，`firework_explosion`
  的 `colors`/`fade_colors` 于是在 NBT 上变成 `[D; ...]`，原版按 `List<Integer>` 读，整组读空
  ——烟花颜色、渐变色的自定义数据在存档里静默消失。改为整数保持整数。
- **`custom_model_data` 的 `flags`**：布尔列表要落成字节列表；读回时还要还原成布尔，否则
  下一次编码会把 `true` 写成 0。
- **`enchantable` 的持久形态**：原版是 `{value: n}`，Cubium 写成裸整数，读回来判为「不是 map」。
- **`instrument` 的持久形态**：原版是 Holder 的完整定义（`sound_event` / `use_duration` /
  `range` / `description`），Cubium 只写资源位置字符串；`sound_event` 还是 `{sound_id: ...}`
  复合对象，`description` 是文本组件对象。
- **`custom_data` 里混入 `CustomPotionColor` / `CustomPotionEffects`**：PotionUtils 把药水的
  自定义颜色寄存在自定义数据里，导出组件补丁时没有剔除，存档因此多出原版没有的键。
- **`attack_range` / `firework_explosion` / `fireworks`**：原版都是 `optionalFieldOf`，
  等于默认值就省略；Cubium 一律写全，字节与原版不同。
- **组件的「显式移除」被丢弃**：`ItemStack` 只记录了「有值」的组件，`toComponentPatch` 从不
  产出 removed 半边，`set_components` / `copy_components` 里的 `"!name"` 写回时消失。
  载荷新增 `removedComponents` 记录这一状态。
- **文本组件的样式是两态而非三态**：`Style` 用 `bool` 记 bold/italic 等，无法区分「显式 false」
  与「未设置」。原版是 `Optional<Boolean>`：前者要写出 `italic:0b`，后者省略；继承时也只看
  子样式是否显式给出（`子 || 父` 会让显式 false 盖不住父样式的 true）。改为 `std::optional<bool>`。

`single_item_recipe` 比的是**单物品配方的查询语义**：单元是输入物品而不是配方 ID，
两侧各自回答「这个物品在熔炼炉 / 高炉 / 烟熏炉 / 营火里会烧出什么、切石机上有哪些选项」。

与 `recipe` 能力的分工在于「查询」这一层，按 ID 的静态导出看不见这些：

- **同一输入命中多条配方时实际返回哪一条**。查询走 findFirst，结果取决于注册顺序；
  顺序一变玩家烧出来的就是别的东西，而每条配方单看都正确。
- **配方有没有被装进正确的类型桶**。熔炼与烟熏的原料完全相同（都是生肉），只在
  cookTime 上不同——桶装错的话，高炉里能烧出生肉。
- **切石的可选产物漏没漏**。切石机界面列出该输入的全部产物，少一条就少一个选项，
  因此这里比的是整张列表而不是单条。

覆盖度：202 个输入物品中，156 个命中熔炼、62 个命中高炉、9 个命中烟熏、9 个命中营火，
58 个有切石产物（共 254 条）。两侧口径逐项对齐：输入堆数量固定为 1（原版
`SingleRecipeInput` 只取一个物品）；切石列表两侧都按配方 ID 排序后比对。

`crafting_match` 比的是**合成配方的匹配与产出**：每个有序/无形状配方配 6 种摆法
（原样、数量放大、平移、多一个物品、少一个物品、空网格），两侧用同一份网格各自调
`matches` / `assemble`。`recipe` 能力只比「配方加载成了什么」，且仅对锻造配方做过匹配；
玩家最常触碰的有序合成与无形状合成此前从未比过。

摆法由 oracle 的 `dump` 子命令导出（尺寸 + 逐格首选物品）后由 harness 展开：标签原料
在两侧的成员顺序可能不同，各取各的会把差异引到标签上，因此「一格放什么」只留一个答案。

**为什么不直接把网格喂给 `matches`**：原版玩家摆东西时，网格先过
`CraftingInput.ofPositioned` 裁掉四周空行空列，`ShapedRecipePattern.matches` 再要求
包围盒与配方尺寸**严格相等**。oracle 若不裁剪，2×2 配方摆在 3×3 网格里会被判否——
那是 oracle 自己造出来的假差异。两侧都用裁剪后的输入，口径才一致。

首轮跑通就查出一个会导致崩溃的缺陷：

- **有序配方的原料表被压紧，匹配时越界读取**：`_parsePatternIngredients` 只收录
  pattern 里的非空格原料（原意是让配方书拿到「非空序列」），而
  `ShapedRecipe::_checkMatch` 按 `relX + relY * m_width` 的下标取原料——下标基准是
  完整图案宽度。`m_ingredients` 因此比 `m_width * m_height` 短，**任何含空格的图案
  （镐、剑、弓、盔甲……）都会读到 vector 尾部之外**，表现为随机崩溃、凭空匹配成功，
  或「摆对了却合不出来」。修复是让原料表与 pattern 逐格对齐、空格填
  `Ingredient::EMPTY`；想拿非空序列的调用方自己过滤 `isEmpty()`。
  回归用例：`ShapedRecipeTest.Matches_PatternWithHole_RequiresHoleEmpty` 与
  `Matches_OneByTwoPattern_SlidesAcrossGrid`。

第一次跑通就查出并修掉的实打实的错误（都是手抄数据/公式出错）：

- 方块：`purpur_wall`、`quartz_wall` 在原版不存在（假方块），已删；
- 物品：4 个物品堆叠上限错、3 个鱼桶漏了食物属性、弩耐久 326→465、重锤耐久 250→500、
  不祥之瓶误挂了 MAX_DAMAGE；
- 稀有度：38 个物品不一致（现在是原版导出的表，注册期统一套用）；
- 附魔能力：52 个物品不一致（同上，生成 `VanillaItemEnchantable.cpp`）；
- 附魔代价：时运 / 抢夺 / 精准采集的 `getMaxCost` 错用了基类公式，高等级时最大代价反而更低；
- 方块属性（`block_properties` 首轮就查出 320 处）：
  - 彩色陶瓦 1.25/4.2 被写成 1.4/4.2，釉面陶瓦抗性应是 1.4 而非 4.2；
  - 远古残骸 30 写成 50、虫蚀方块硬度全是 0、紫水晶芽/紫颂植物 0、竹笋 0、雪 0.2；
  - 台阶规则：原版沿用基方块硬度（石砖台阶除外，是 +0.5），此前按材质硬编码抵消；
  - 水/岩浆 0、刷怪笼 -1（生存模式永远挖不动）、命令/结构/拼图方块本应不可破坏；
  - 幽匿感测体与宝库的 1/6 级光照未注册、只有红蘑菇发光 1（红蘑菇被误设 1）。

`enchantment_effects` 首轮查出并修掉的问题（附魔效果此前完全没有被比对过）：

- 水下速掘、效率、深海探索者、横扫之刃、迅捷潜行**根本没有** `getAttributeModifiers`，
  这五个附魔在游戏里只靠手写常量近似生效，属性面板、多件叠加、与原版逐位一致的数值
  全都没有；它们的属性（`mining_efficiency`、`submerged_mining_speed`、
  `water_movement_efficiency`、`sweeping_damage_ratio`、`sneaking_speed`）在项目中
  也完全没有注册，因此补附魔修饰符之前先把属性补齐并接通消费点；
- 效率加成写在`ToolItem::getDestroySpeed`里，导致持附魔工具的僵尸、掉落物、发射器
  也挖得更快；原版只给玩家（`Player.getDestroySpeed` 读属性），已下移；
- 深海探索者用两个手调常量近似（阻力增量 + 输入速度增量），改成原版的插值公式
  `f += (0.54600006 - f) * efficiency`、`f1 += (getSpeed() - f1) * efficiency`；
- 横扫比例从`EnchantmentHelper::getSweepingDamageRatio`（只看主手一件物品）改为读属性；
- 爆炸保护与火焰保护漏了身体槽（马铠/狼铠/鹦鹉螺铠甲），原版的 `slots="armor"`
  对 body 同样成立；
- `AttributeInstance::addModifier` 原来是 `push_back` 不去重，而原版按 id 覆盖——
  同一附魔在多件装备上会重复叠加（三件迅捷潜行 = +0.45 而不是 +0.15）。

`mob_experience` 首轮查出并修掉的经验值问题（7 处）：

- 守卫者、远古守卫者、唤魔者经验值停在基类 Monster 的 5（原版 10）；
- 恼鬼 5 → 3；猪灵蛮兵 5 → 20；
- 史莱姆与岩浆怪：原版只在 `setSize` 里写 `xpReward`，构造函数只定义初始尺寸，
  因此「刚建出来、还没生成」的史莱姆经验值是 0；Cubium 在建属性时就写了 1，
  且 `setSlimeSize` 在尺寸未变时直接返回，导致 `setSlimeSize(1)` 拿不到经验值；
- 顺带查出分裂逻辑的错误：子史莱姆调用了 `finalizeSpawn`，尺寸被重新随机成
  1/2/4，而原版 `convertTo` 只执行 AfterConversion（`setSize(parent/2)`），
  分裂出来的小史莱姆可能和父体一样大。

`attribute_range` 首轮查出并修掉的属性定义问题（13 处，全部是属性表本身写错）：

- **12 个原版属性根本没有注册**（`block_break_speed`、`camera_distance`、`gravity`、
  `max_absorption`、`mining_efficiency`、`scale`、`sneaking_speed`、`step_height`、
  `submerged_mining_speed`、`sweeping_damage_ratio`、`tempt_range`、
  `water_movement_efficiency`）——按名查找、`/attribute` 命令、附魔修饰符注入
  在这些属性上全部静默失效；
- `max_absorption` 仍用 1.16.5 的 `generic.` 前缀（属性常量与工厂函数都是），
  按 `minecraft:max_absorption` 查找落空；
- `jump_strength` 默认值写成 double 字面量 `0.42`，原版是 `0.42F` 再拓宽，
  差 1ulp；
- 重力沿用 Forge 的 `forge.entity_gravity` 且范围 ±8.0，原版 1.21 起是
  `minecraft:gravity`、范围 ±1.0；
- 补注册的 5 个属性（`block_break_speed` / `step_height` / `scale` / `tempt_range` /
  `camera_distance`）尚无消费点，已就地加 `TODO` 注明接入位置。

`entity_attributes` 首轮查出并修掉的属性错误（全部是手抄/想当然的默认值）：

- 鱼（鳕鱼/鲑鱼/河豚/热带鱼）、鱿鱼、蝙蝠、流浪商人：1.21 起原版不再覆盖移动速度，
  沿用 `LivingEntity` 默认的 0.7，Cubium 还停留在 0.25~0.35 的旧值；
- 蜜蜂跟随范围 48 → 16、巨人 40 → 16、羊驼/商人羊驼 40 → 16、幻翼 64 → 16 且速度
  0 → 0.7、潜影贝 18 → 16 且速度 0 → 0.7、猪灵蛮兵 16 → 12；
- 史莱姆/岩浆怪的默认属性就是 20 血、2 伤害、速度 0.7 / 0.2（尺寸缩放是在
  `finalizeSpawn` 里另算的），此前把「尺寸 1」的值当成了默认值；
- 恶魂：飞行速度 0.9 → 0.06，移动速度 0 → 0.7；
- 鱼、鱿鱼之外还有 8 种生物（美西螈、海豚、鹦鹉螺、熊猫、北极熊、山羊、青蛙、
  凋灵骷髅）**没有注册** `ATTACK_DAMAGE`，`setBaseValue` 静默失败，攻击伤害落在
  属性缺失的默认值上；狼的驯服加成把 4 改成了 2；
- 马族：默认值应是属性工厂的 53 血 / 0.225 速度（驴/骡/羊驼 0.175），随机化发生在
  `finalizeSpawn`；僵尸马固定 25 血 / 0.225 速度；随机公式也对齐了原版
  `generateSpeed` / `generateMaxHealth` / `generateJumpStrength`；
- 僵尸猪灵沿用僵尸的跟随范围 35 与护甲 2。

`block_state_names` 首轮查出 33 个方块的**默认状态**与原版不同（状态集合本身全对）：

- 避雷针（含 8 个氧化/打蜡变体）默认朝北，原版朝上；
- 珊瑚扇与墙扇（含 20 个死亡变体）默认 `waterlogged=false`，原版是 `true`；
- 气泡柱默认 `drag=false`，原版 `true`；苍白垂苔默认 `tip=false`，原版 `true`；
- 鬼木之心默认轴是 X，原版 Y；结构方块默认 `mode=data`，原版 `load`；
- 测试方块默认 `mode=fail`，原版 `start`。

`random_sequence` 首轮就查出随机数生成器的两处实现错误（Xoroshiro 序列前半段完全一致，
只有 nextDouble 与窄位宽抽取不同——最容易漏的一类）：

- `XoroshiroRandomSource.nextDouble()` 的乘数在原版是 **f64** `1.1102230246251565E-16`，
  Cubium 照抄了 BitRandomSource 的 f32 写法，53 位精度被砍成 24 位；
- `LegacyRandomSource.nextDouble()` 用的是 java.util.Random 的 53 位公式（f64 乘数），
  Cubium 误用了 BitRandomSource 的 next(26)/next(27) + f32 组合；
- `IRandom::nextBits(32)` 取的是 nextU64() 的高 32 位，原版 `next(32)` 取低 32 位。

`command_argument` 首轮跑出 54 个不一致，逐条对齐后挖出了这些真问题：

- **数值参数的「读完再校验」语义**：brigadier 的整数/浮点参数先把整段单词读完再解析，
  `1a` 读成 1（原地留下 `a`）、`1.0` 与 `1.5.5` 整体失败、`2147483648` 溢出报错、`+1` 非法；
  Cubium 之前是「读到第一个非数字就停」，于是 `1.0` 被静默接受、` 1` 却被接受（原版拒绝）；
- **坐标读取（`WorldCoordinate.parseDouble`）**：可选 `-`、数字、至多一个小数点，遇到字母
  立即停（`1e3` → 1，`e3` 留给单位）；前导空格不报错而是 **0 且游标不动**；游标已读尽才报错。
  `/time set 1.5d` 之前多解析了一位小数之外还漏掉了单位；
- **游戏模式**：原版 1.13 起只认 `survival`/`creative`/`adventure`/`spectator` 全名且大小写
  敏感；Cubium 还接受 `s`/`c`/`a`/`sp`/`0`/`1` 并转小写，`/gamemode s` 在原版里是语法错误；
- **物品栏槽位**：原版只认名字表，没有纯数字写法；且护甲号是 head=103、chest=102、
  legs=101、feet=100（与枚举顺序一致），Cubium 之前是反的——`/replaceitem ... armor.head`
  实际会写到脚上；
- **资源位置**：原版按 `Identifier.read` 贪婪读 `[a-z0-9_.-]`/`/` 与 **一个** `:`，不读引号、
  不认大写、第二个 `:` 直接报错；Cubium 用 `readString()`，`"stone"` 读成 `stone`、
  `a:b:c` 被静默接受；
- **时间单位**：单位区分大小写、`+1` 非法、`-1` 合法（最小值由命令自己指定）；
- **NBT 路径**：空输入在原版里是合法的空路径（不报错）；未引用键名要读到 `.`/`[`/`{`/空白
  才停，`*`（匹配任意键）此前根本解析不了。

这一批修复顺带改正了一个**真实玩法 bug**：`ReplaceItem` 的护甲槽号翻转。

### 标签改成数据驱动

`block_tags` / `item_tags` 首轮分别只有 100/204 与 164/198 一致。原因是标签表是**手写**的：
原版方块标签 200 个、物品标签 198 个，靠 `tags[x] = std::make_unique<BlockTag>(...)` 一条条
抄，和 `data/minecraft/tags/**` 之间必然漂移——首轮查出方块侧 82 个标签完全缺失、22 个标签
成员不全（`logs` 少 4 个、`sculk_replaceable` 少 20 个），另外 6 个标签
（`dripstone_replaceable` / `non_flammable_wood` / `powder_snow_walkable_moved` / `stone` /
`tall_flowers` / `valid_sweet_berry_bush_ground`）在 1.21.11 里根本不存在。

现在两张表都由原版数据包生成（`VanillaBlockTags.gen.cpp` / `VanillaItemTags.gen.cpp`），
`#tag` 引用保留为引用（与原版 `TagEntry` 一致），由 `linkAndExpand` 在运行期展开成传递
闭包。做 `recipe` 时发现物品侧在生成期展开了一层：`logs_that_burn` 写的是
`#minecraft:acacia_logs` 等引用，生成器只认物品名，引用成员全部丢失，`cherry_planks`
等配方因此原料为空、加载失败。运行期统一展开后，标签成员就是原版 `Tag#getValues()`
的传递闭包。手写的 500 多行 `add(...)` 在更早一轮已全部删除。

顺带按原版改了三处主调者：

- 滴石改用原版的 `#dripstone_replaceable_blocks`；
- 蜜蜂采蜜的「高花朵」判断改用 `#flowers`（原版 `#flowers` 已包含 `#small_flowers`）；
- 甜浆果丛可种植地面改用原版 `VegetationBlock.mayPlaceOn` 的语义：`#dirt` 或耕地。

`item_tags` 的差异全部来自 381 个尚未注册的物品，逐条列进 `known_missing_items.ts` 既写不全
也读不出重点；因此能力声明里有 `memberGaps` 选项，按成员名从**原版一侧**剔除并报告条数
（probe 多出来的成员永远算差异）。

### `biome_climate`：首轮只有 9/65 一致

挖出的问题分三类：

- **海洋温度**：1.18 起原版把所有海洋的 `temperature` 统一成 0.5（冷/暖只体现在水体颜色），
  Cubium 还留着 1.16 的 0.3 / 0.6 / 0.8；`deep_frozen_ocean` 写的是 0.0，原版是 0.5、
  由 FROZEN 修饰器把派生温度压到冰点；`snowy_plains` 是 0.0（不是 -0.5）；
  `sparse_jungle`（Cubium 旧名 `jungle_edge`）是 0.95（不是 0.8）；
- **雾色 / 天空色**：1.21.11 把这两项挪进 `EnvironmentAttributeMap`，属性默认值就是 0，
  群系 JSON 里显式写了才是覆盖色。Cubium 的 `BiomeEffects` 默认值还是 1.20 的浅蓝常量，
  于是「没覆盖」的群系全都多了一组并不存在的颜色。默认值改成 0，并按原版 JSON 逐条补齐
  55 个群系的 `water_color` / `water_fog_color` / `fog_color` / `sky_color`；
- **群系名**：`Biome::m_name` 沿用 1.16.5 旧名（`mountains`、`giant_tree_taiga`……），
  1.18 起原版改成 `windswept_hills`、`old_growth_pine_taiga`。probe 按
  `JavaBiomeRegistryIdMap` 的同一张别名表归一化后再比对；
- 顺带修掉几处手抄错的颜色：冷水海洋 `#3D57E6`→`#3D57D6`、暖水海洋水下雾
  `#041F33`、沼泽水下雾 `#232817`→`#232317`、樱桃林水色改 `#5DB7EF`、
  红树林沼泽用的是另一套色（`#3A7A6A` / `#4D7A60` + 树叶 `#8DB127`）。

### 战利品表：三个上下文

`loot_table` / `loot_table_block` / `loot_table_entity` 各自用同一批单元（数据包里
1324 张表），区别只在上下文：最小上下文（`origin` + 中性方块状态 + 空工具）、挖掘
上下文（方块状态 + 时运工具）、击杀上下文（`this_entity` + 攻击者 + 掠夺等级 + 着火）。
每个单元从固定种子出发掷 4 次、每次先推进 `sample × SAMPLE_STRIDE` 次
（对齐 `LootRoll.randomForSample`）。

对齐过程中修掉的真问题：

- **条件只在 `expand` 阶段判定**：原版 `LootPoolEntryContainer#expand` 先用条件筛
  候选，`createItemStack` 不再判定。此前在 `generate` 里再判一次，带随机数的条件
  （`random_chance`、`table_bonus`）因此多消耗一次随机数，后续抽取全部错位；
- **空堆不在任何一层过滤**：条目 / 池 / 表三层的 `decorate` 对空堆照样执行函数链，
  丢弃发生在收集端 `createStackSplitter`。此前在条目层提前过滤，导致池级、表级函数
  与 `enchanted_count_increase` 的随机数消耗与原版错位（`bogged` 的第一个样本
  整堆丢失就是这么暴露的）；
- **`ItemStack` 的数量语义**：`setCount` 只改数量、不丢物品类型，`isEmpty()` 只看数量。
  `set_count` 把数量设成 0 之后，`enchanted_count_increase` 仍能在原物品上把数量加
  回来——原版正是如此。此前数量归零会清空 `m_item`，链式函数因此产出空堆；
- **`set_count` 不做空堆早退**：原版 `SetItemCountFunction#run` 对空堆同样量产数量；
- **掠夺等级来自装备**：原版没有「掠夺等级」上下文键，`enchanted_count_increase` 与
  `random_chance_with_enchanted_bonus` 都从 `ATTACKING_ENTITY` 的装备读附魔。
  探针与 oracle 都把掠夺附魔真实写到攻击者主手上；
- **参数集必需键对齐**：`BLOCK` = {block_state, origin, tool}、`ENTITY` =
  {this_entity, origin, damage_source}、`FISHING` = {origin, tool}、`SELECTOR` =
  {origin, this_entity}。此前缺键只打警告，现在与原版 `LootParams.Builder#create`
  一样是硬校验。

### 已知未解决（带 TODO 标注）

- **`loot_table_entity` 的 14 处 `#player` 差异**：这 14 种生物在「击杀者是玩家」的上下文里
  Cubium 多掉一部分物品（蜘蛛眼、幻翼膜、吹风机碎片、湿海绵……），换成 `#default`
  （击杀者是僵尸）就一致；根因未定位。已在 `harnesses/index.ts` 的
  `ENTITY_LOOT_KNOWN_GAPS` 登记为已知差异（逐单元 + `samples` 字段），修好后整段删除。
- **`loot_container` 的两段已知差异**：`slots` 的随机源未对齐 `random_sequence` 派生；
  `drop` 段 oracle 只导出 `id/count` 而探针多导出了 `customName/container`（探针注释判断为
  oracle 加载旧字节码，尚未查清）。两条都在 `harnesses/loot_container.ts` 的 `KNOWN_GAPS`
  里登记，并在探针文件头以 `TODO` 说明修法。
- **`command_tree` 的 `wire`**：`/panel` 这个自研命令改变了整棵树的编码字节，使逐字节比对
  恒不相等。`tree`/`arguments`/`components` 三组已全部一致，`wire` 登记为已知差异；
  修复方向是让自研命令不进入可编码的命令集合。
- **三个「数量为 2 的不可堆叠物品」单元测试**（`ItemPredicateTest.CombinedCountAndPotion`、
  `EquipItemIfPossibleTest.PickUpItem_LimitsArmorToSingleItem`、
  `ArmorStandEntityTest.InteractAt_WithChestplate_CountGreaterThanOne_MovesOne`）：`ItemStack::setCount`
  会把数量裁剪到最大堆叠数（对齐原版），「数量 2 的铁甲」构造出来只有 1 个，测试前提不成立。
- **`DensityEnchantmentTest.Properties`**：`density` 的 `isTreasure` 与原版数据不符。

### 游戏规则与材质表

`gamerule` 与 `armor_material` / `tool_material` 比对的都是「手写常量表」——这类
表写错不会报任何错，只会让游戏安静地按错误数值运行。首轮查出：

- **`lava_source_conversion` 的默认值是错的**：原版默认 `false`（岩浆不会把旁边的流体
  转化掉），Cubium 写成了 `true`——玩家建好的水池会被岩浆「吃掉」，这是直接改变玩法
  的数值错误；
- **三条规则的分类错位**：`allow_entering_nether_using_portals`（应为 misc）、
  `mob_explosion_drop_decay` 与 `tnt_explosion_drop_decay`（应为 drops）。分类只影响
  整理与未来 UI 分组，但这正是手抄表最容易漂移的地方；
- **海龟壳材质缺防御值**：只登记了头盔 2，护腿/胸甲/靴子都是 0。原版
  `makeDefense(2, 5, 6, 2, 5)` 覆盖全部槽位；
- **犰狳鳞甲的胸甲与护腿值对调**：原版 `makeDefense(3, 6, 8, 3, 11)` 是
  boots/leggings/chest/helmet/body 顺序，Cubium 按 chest=6、legs=8 登记，正好互换。

`tool_material` 首轮即全部一致，说明工具材质表此前就是对的（此前修过的那批工具
攻击伤害问题出在「注册处数值 + 构造期又加材质加成」的叠加，不是材质表本身）。

### 方块破坏经验与物品属性修饰符

`block_experience` 比对的是**采样序列**而不是取值范围：固定值区间（原版 `ConstantInt`）
不消耗随机数，闭区间（`UniformInt`）消耗一次，只比范围查不出「本该固定却写成了区间」。
首轮对比暴露出原先的实现根本没走这条路——它用「坐标 + 玩家」拼出一个自造种子另起
随机源，与该 tick 世界随机数的走向完全无关。现改为 `Block::dropExperience` 按方块
登记的区间采样，16 个矿石与幽核照抄 `Blocks.java`。

`item_attribute_modifiers` 首轮 1124 个物品里有 93 个与原版不一致，逐条对齐后归零。
挖出的问题：

- **属性名还停在 1.16.5 的 `generic.*`**：所有按名查找（数据组件、属性注册表、命令）
  都与原版 `minecraft:*` 对不上。统一改名后 `normalizeName` 仍接受短名、`minecraft:`
  前缀与旧的 `generic.` / `zombie.` 写法；
- **攻击伤害被重复叠加**：注册处给基础值，构造期又加了一次材质加成。原版组件里就是
  最终值，因此构造期不再叠加，注册处填原版绝对值（斧 6/8/8/8/6/8/9、镐 1..5、
  锄全 0、长矛全 0 等一并修正）——这是一批**玩家实际感受到数值变化**的修正；
- **漏掉的修饰符**：马铠/鹦鹉螺铠甲没有身体槽位的护甲与韧性条目（下界合金缺击退抗性）、
  三叉戟没有武器条目、雕刻南瓜与各类头颅缺「隐藏导航点」条目；
- **长矛攻速被存成 f32**：`-2.4615384340286255` 这类值过一趟 f32 就与原版对不上。

顺带查出两个真缺陷：恶魂的 `registerAttributes` 覆写没调父类，`follow_range` 从未
注册、`setBaseValue` 静默失败（追踪范围一直是 0）；骆驼的 `step_height` 属性同样没
登记，已加 TODO 待补。

### 护甲减伤

`damage_reduction` 以「伤害 × 护甲 × 韧性」的乘积为单元（2415 个组合），比较最终伤害
与破甲 1..4 级的结果。两侧末位会差 1 ulp：JVM 与 clang 对 f32 中间结果的求值顺序不同
（先乘后除与先除后乘在 25 这类除数上会落到不同的 f32）。把中间结果逐步按 f32 舍入
复算，结果与探针完全吻合，说明公式一致、差异只来自舍入，因此比对按固定 4 位小数进行
（不能用有效位舍入：「……5」这类恰好落在进位边界上的值会因末位差 1 翻到相邻有效位）。

### 进度定义：1584 个全部对齐

`advancement` 把数据包里每一个进度（`data/<ns>/advancement/<path>.json`）解析成结构化摘要：
父进度、显示信息（图标物品 + 数量 + 组件、标题/描述文本、框架、背景纹理、toast/公告/隐藏）、
条件（名称 → 触发器 ID）、需求矩阵、奖励（经验 / 战利品表 / 配方 / 函数），与原版
`ReloadableServerResources` 加载出的 `AdvancementHolder` 逐一比对。

对齐过程中修掉的真问题：

- **`Criterion` 的触发器 ID 来自数据包字面量**：此前从实例类型反推（`ICriterionInstance::getId()`），
  而多个触发器共用同一实例类型，`slept_in_bed` / `hero_of_the_village` / `voluntary_exile` /
  `location` 全被读成 `minecraft:location`；`tick` / `impossible` 等无玩家谓词的触发器被读成
  `impossible`。更糟的是**未注册**的触发器被静默替换成 `impossible` 实例：`kill_mob_near_sculk_catalyst`、
  `construct_beacon`、`shot_crossbow`、`spear_mobs`、`target_hit`、`allay_drop_item_on_block`、
  `crafter_recipe_crafted`、`levitation`、`thrown_item_picked_up_by_player` 这 9 个根本没有注册，
  数据包里 1584 个条件共 2700 余条全部退化成 `impossible`。现在 ID 取自数据包，实例按触发器
  自己的 Codec 字段解码（新增 `GenericTriggers`），`ImpossibleTrigger` 一并删除；
- **同一条件结构不能共用实例类型**：`fall_from_height` / `nether_travel` / `ride_entity_in_lava`
  曾共用 `DistanceTriggerInstance`，三个触发器的条件 ID 因此统一变成 `fall_from_height`。拆成
  三个独立实例类型，`ServerPlayer::triggerDistance` 按触发器 ID 分派到各自的模板方法；
- **`item_used_on_block` 与 `default_block_use` 是两个触发器**：此前 `ItemUsedOnBlockTrigger`
  继承 `AnyBlockUseTrigger`，ID 被基类覆盖，11 个进度的条件因此对不上；
- **背景是 `ClientAsset.ResourceTexture`**：数据包里的 `background` 是资源 ID
  （`minecraft:gui/advancements/backgrounds/stone`），纹理路径按约定补全为
  `minecraft:textures/gui/advancements/backgrounds/stone.png`；
- **图标是完整的 `ItemStack`**：`{"id", "count", "components"}`，3 个进度的图标带组件
  （装饰陶罐的 `pot_decorations`、不祥旗帜的 `banner_patterns` + `item_name` + `rarity` +
  `tooltip_display`）。`banner_patterns` 的 JSON 用图案 ID 与染料名，此前没有 JSON 解码
  路径（只有 NBT 往返），现补齐 `BannerPatternLayers`。

### 生物群系生成：65 个全部对齐

`biome_spawns` 把数据包 `worldgen/biome/*.json` 的 `spawners` / `creature_spawn_probability` /
`spawn_costs` 解码成 `MobSpawnInfo`，与原版 `Biome#getMobSettings()` 逐字段比对：每个
`MobCategory` 的生成条目（生物类型、权重、成组数量）、每区块上限、动物生成概率、
带生成成本的实体。

对齐过程中修掉的真问题：

- **生成条目的实体类型必须能解析**：原版 `MobSpawnSettings.SpawnerData.CODEC` 用
  `BuiltInRegistries.ENTITY_TYPE.byNameCodec()`，未注册的类型直接解码失败、整份群系作废。
  此前跳过坏条目并保留其余；`BiomeLoader::loadFromJson` 现在与其它字段一样，失败即整体失败；
- **`creature_spawn_probability` 的缺省值是 Codec 字段默认值 0.1**，不是「工厂方法的设定」：
  `deep_dark`、`the_void` 在数据包里只有空 `spawners`，原版取 0.1，Cubium 的
  `MobSpawnInfo::createEmpty()` 却给了 0；`snowy_taiga` 同样错用了雪原的 0.07
  （数据包里只有 `snowy_plains`、`ice_spikes` 写了 0.07）；
- **`spawn_costs` 之前只在「两项都为正」时生效**：原版 `charge` 缺省 0、`energy_budget`
  缺省 1，且只要求非负，零值也保留；
- **解码逻辑上移到 common**：`BiomeLoader` 与探针共用 `MobSpawnInfoJson`，探针测的就是
  服务端真正跑的那段代码。

### 生成位置：共同场景逐项比较

沿用 Horizon 的 `tools/test_spawn_rules_parity.py` 与 `VanillaSpawnRulesOracle` 的方法：
共同输入描述方块柱，两侧实际调用生产代码。`SpawnPlacementOracle` 直接调用 1.21.11 的
`SpawnPlacements.isSpawnPositionOk`，不移植 26.3 的预期结果。

全部 157 种实体均比较放置类型、高度图、332 种位置场景，合计 52,124 次判定。
同一种方块状态分别放到脚下、脚部、头部和水面上方，覆盖半砖/楼梯、单箱/双箱、
树叶、危险方块、信号源、禁入标签、流水、含水方块、水下植物、边界内外与建筑高度外位置。
没有实体豁免。Java 世界只提供用例明确给出的数据，意外访问其他能力直接失败。

该能力不调用生成谓词，因此不覆盖光照、难度、生成原因和随机数消耗；这些属于
`spawn_rules` 中 Horizon `checkSpawnRules` 与后续 `nextInt(1000)` 对应的验证。
公共探针入口使用完整 JSON 行读取，允许一条输入携带超过 4095 字节的共同场景集。

### 生成谓词：结果与随机数消耗一起比较

`spawn_rules` 沿用 Horizon 的受控 `ServerLevel`：从共同输入设置方块柱、光照、雷雨、
海平面、世界种子、群系、地表史莱姆环境属性和附近玩家，直接运行 1.21.11 的
`SpawnPlacements.checkSpawnRules`，随后记录同一随机源的 `nextInt(1000)`。
位置检查独立运行，即使位置不合格也比较谓词，防止前置拒绝掩盖分支或随机消耗差异。

覆盖全部 157 种实体、19 种原因、三种维度的光照门槛、四种难度、多个海平面边界与
玩家 5 格距离边界。普通实体各 1,068 个场景；史莱姆与僵尸各追加 600 个区块坐标和世界种子
场景，总计 168,876 个。原版世界使用显式环境属性值，群系标签在两侧都来自同一个数据包。
输出完整比较 `positionOk`、`rulesOk`、`nextRandom`，不设置实体或字段豁免。

生产代码将 82 个注册谓词与位置检查分开，运行期世界和生成区域共用读取逻辑。区块首次
生成额外传入生成器的实际海平面；地表史莱姆读环境属性，区块史莱姆复用 JavaLegacyRandom，
其种子计算保留 Java 在提升为 long 之前的 int 回绕。

### 配方：定义与装配结果

`recipe` 把数据包 `recipe/*.json` 交给服务端的 `RecipeLoader::loadRecipeJson`，再从
`RecipeManager` 按 ID 取回做结构化摘要，与原版 `RecipeManager#getRecipe` 的结果比对：
序列化器 ID（原版配方注册表的键）、分组、每个原料的候选物品集合（标签展开成成员，
与原版 `Recipe#placementInfo().ingredients()` 同口径）、有序配方的宽高、结果物品与数量
（原版侧跑一次真实 `assemble`）、烹饪时间/经验。

锻造配方沿用 Horizon 的两阶段方法：先由 oracle 导出原料并选取资源 ID 最小的候选，
再把同一份具体三槽输入写进共同 JSONL。30 个锻造配方比较输入、`matches` 和两侧实际
`assemble` 的结果物品/数量，避免标签成员迭代顺序影响样本。原版直接复用已加载资源的
注册表，无需构造世界；异常直接失败，不再转成空结果。结果摘要尚不比较数据组件，
纹饰组件的写入仍由业务源码中的 TODO 跟踪。

对齐过程中修掉的真问题：

- **物品标签带嵌套引用时整张标签表是空的**：`logs_that_burn` 在数据包里写的是
  `#minecraft:acacia_logs` 等引用，但生成的内置标签快照把这些引用展开时只认物品名，
  引用成员全部丢失——`charcoal`、`campfire` 等配方的原料只剩一半，`cherry_planks` /
  `mangrove_planks` / `pale_oak_planks` 甚至因为原料全空而加载失败。标签现在保留
  `#tag` 引用（原版 `TagEntry` 的语义），由 `ItemTags::linkAndExpand` 统一展开成传递
  闭包；展开在遍历中写入自身集合会导致迭代器失效，`ItemTag::forEachItem` 先复制成员；
- **数据包是物品标签的唯一来源**：服务端启动时此前先初始化内置快照、再把数据包标签
  追加进去，两份来源互相覆盖，`replace` 语义还会错误地清掉内置成员。现在启动时清空标签
  表后只从数据包构建（与方块标签一致）；探针同样只读数据包；
- **有序配方的原料表含空格占位**：`Ingredient::EMPTY` 被塞进原料列表，`placementInfo`
  比原版多出空槽（`acacia_boat` 6 个原料 vs 原版 5 个），装配时也会把空槽当成「该格必须为空」。
  现在与 `ShapedRecipePattern#ingredients` 一样只收 pattern 里实际用到的符号；
- **标签 + 显式物品并存的原料匹配不到显式物品**：`["#minecraft:planks", "minecraft:oak_planks"]`
  解析成「保留标签引用 + 追加显式物品」，但 `Ingredient::test` 在标签分支里提前 return，
  显式物品永远匹配不上；`getAllMatchingItems` 同样只返回标签成员；
- **特殊配方的类型**：数据包里每个特殊配方都有自己的序列化器 ID
  （`crafting_special_armordye`、`crafting_decorated_pot`……），Cubium 一律返回
  `minecraft:special`。`SpecialRecipe::getSerializerId` 由装载器按数据包写入；地图扩展的
  放置信息（3×3 外圈纸、正中已填充地图）由 `RecipeSerializers::placementIngredients` 给出；
- **纹饰配方的样本必须相同**：此前 Java 取标签首个物品为皮革靴，C++ 则可能取到钻石胸甲，
  导致 18 个结果差异。现在三槽输入由同一份夹具给定，匹配失败时两侧都输出空结果；
- **樱桃 / 红树 / 苍白橡木的原木在物品里缺失**：`BlockItemRegistry` 注册这三组原木时
  用的方块指针在建表阶段还是空指针；现在与其它树种同一路径注册后，相关标签与配方才能
  正常展开。

### 状态效果实例：操作序列与共享身份

`status_effect` 沿用 Horizon `test_status_effect_parity.py` 的共同操作序列：构造、覆盖、复制、
别名、缩放、相等、到期查询和时长回调。每一步比较返回值、回调次数及完整节点图，跨步骤保留
身份编号，检查共享隐藏链、零和无限时长、等级夹取、整数回绕，以及 NaN / 无穷等浮点位模式。
回调可以通过真实 update 修改自身别名后失败，失败前的修改仍须保留。

Java 直接调用 1.21.11 实例 API；隐藏链由反射读取，纯时长推进调用服务端实际使用的
`tickDownDuration` / `downgradeToHiddenEffect`。C++ 探针调用生产实例类，时长推进同时用于
真实服务端 tick；隐藏效果恢复的属性重建和网络刷新由相关单测验证。
两侧均不比较客户端 BlendState，也不模拟实体上的效果伤害、治疗等具体行为。
未注入绕过构造器的原始字段；外部回调修改通过公开 update 完成。

对象仅比较原版声明过的字段；数组必须等长。公共投影器不再截取数组前缀，避免漏报多出的
隐藏节点、回调和操作步骤。

严格数组校验后的 25 项回归中，24 项通过；`loot_table_entity` 的 `magma_cube`、`pillager`
暴露额外掉落。此前原版的空数组会吞掉 probe 的非空结果，这两处差异仍保留在报告中。

### 命令树的比较口径

`command_tree` 使用原版 `Commands(DEDICATED, ...)` 注册入口，比较四组数据：

| 字段 | 比较内容 |
|---|---|
| `tree` | 根命令及前两层名字、参数类型摘要 |
| `arguments` | 每个参数的注册表数值 ID、properties 十六进制载荷、建议提供器 ID；仅叶子路径带类型后缀 |
| `components` | 全部节点的类型、可执行性、受限性、重定向目标、修改器模式和建议提供器；路径不带类型后缀 |
| `wire` | 完整命令包的字节数和十六进制字节；Java 侧另用原版 codec 解码验证 |

完整字节比较前，两侧均按「字面量在前、参数在后，同类按名字」排序，再以 BFS 编号。
只沿子节点收集规范路径，重定向保留为目标引用。无回调的相等字面量和 Brigadier
基础参数可复用网络索引；不同执行修改器仍在原始节点属性中单独比较。

参数类型 ID 取自 `ArgumentTypeInfos.bootstrap` 的实际注册顺序，不能由类型名字猜测。
`entity` 的单目标及玩家限制由 properties 标志决定；类型名一致不代表编码相同。
建议提供器取真实注册名：声音为 `minecraft:available_sounds`，可召唤实体为
`minecraft:summonable_entities`，其余自定义提供器为 `minecraft:ask_server`。
Oracle 序列化失败会直接终止，不删除 payload 字段或豁免该节点。

`execute` 修改器及条件的目标是 `execute` 节点，`run` 的目标是根；修改器是否分叉
取 `isFork()`，不能用回调实现类推断。注册图和协议一致不代表命令执行语义完整；
执行语义的未完成部分仍由相应命令测试和源码 TODO 跟踪。

**仍未实现（9 条，见 `known_missing_commands.ts`）**：`fetchprofile`、`fillbiome`、
`item`、`place`、`random`、`test`、`tick`、`transfer`、`waypoint`。这 9 条通过
`oracleArgs` 让原版在注册阶段就跳过——它们的子节点会把同一批参数类型节点撑成不同形状，
留在参照侧会让其余节点也变成伪差异。

**`wire` 目前是已登记的已知差异**：`/panel` 是自研命令，原版没有，它会让整棵树的编码
字节整体不同。`tree`/`arguments`/`components` 三组已经全部一致，因此结构级偏差（节点顺序、
重定向下标）仍被覆盖；`wire` 恢复可比需要让自研命令不进入可编码集合（见
`harnesses/index.ts` 的 TODO）。

### 已知缺口清单

`known_missing_items.ts`、`entity_shape.ts`、`enchantment_table.ts`、
`known_missing_entity_attributes.ts` 里的 `KNOWN_MISSING`
列的是「原版有、本项目尚未实现」的条目——**实现缺口，不是参照数据缺失**。清单显式列出、
每次运行输出条数，实现补齐后必须删掉对应条目，否则会出现「豁免了原版也有的项」的假一致。

---

## 写一个 oracle 时要注意

- **协议通道不能用 stdout/stderr**：原版 log4j 的配置会把 stdout 拿去记日志，`System.err`
  也会被套成 `[time] [main/INFO]: [STDERR]: ` 前缀行。所以 `list` 子命令把单元写到
  参数给定的文件（`list <units.txt>`）——**唯一例外**是 `BlockStateOracle`：它没有文件参数、
  直接写 stdout，运行层在文件不存在时读 stdout（只认 `minecraft:` 开头的行）。
  `run` 一律写输出文件。
- **进程外固定资源要声明互斥**：需要在 `java.io.tmpdir` 下建服务端世界、或占用固定端口/
  固定目录的 oracle，必须给能力声明 `exclusiveGroup`，否则并行运行时会互相锁死
  （战利品族在 Windows 上实测会报 `another process has locked a portion of the file`）。
- **错误要带上 stdout**：JVM 的异常栈常写在 stdout，只截 stderr 会只剩半句
  `Exception in thread "main"`。运行层已经两侧都带。
- **数据包注册表**：1.21 起附魔、生物变种等不再写在代码里。只调 `Bootstrap.bootStrap()`
  拿不到它们，要像服务端启动那样加载原版数据包——见 `DataPackRegistries.java`
  （内部使用 `RegistryDataLoader` + `ReloadableServerRegistries`；Java 类名对应 1.21.11
  的 `WORLDGEN` 层与 `SYNCHRONIZED_REGISTRIES`）。
- **比较只按 oracle 声明过的字段**（`project()`，会递归进数组内部）：oracle 没给的信息
  不是「不一致」，而是「没得比」，避免逼着人去伪造对侧输出。

---

## 未对齐项（已知缺口）

`block_properties` 目前只比较**已完全对齐**的字段（硬度 / 亮度 / 滑度 / 是否流体）。
以下三项在 Cubium 里仍来自材质默认值而不是逐方块注册，是初始化缺口，补齐前不进
本 harness 的通过条件：

| 字段 | 现象 | 根因 |
|---|---|---|
| `resistance`（爆炸抗性） | 几十个方块为 0 | `BlockProperties::hardness()` 不像原版那样同时作为抗性默认值 |
| `opacity` / `opaque` | 半砖、门、栅栏等被当成 15 档全遮挡 | 同上：`m_opacity` 默认 15，未逐类设置 |
| `solid` / `replaceable` | 门、栅栏门、树叶、植物等判定相反 | 同上：取自材质 `m_isSolid` / `isReplaceable` |

### 随机数：已对齐到哪一层

`random_sequence` 直接比对两个**内层**生成器（`Xoroshiro128ppRandom`、
`JavaLegacyRandom`）的接口序列，全部逐位一致。上层的 `WorldgenRandom`（原版把它包在
生成器外面、用自己的 `next(bits)` 重写所有抽取）现在全部路径都与原版逐位一致：

| 路径 | 状态 |
|---|---|
| `nextInt(bound)` / `nextFloat` / `nextDouble` | 与原版逐位一致（golden 测试通过） |
| 裸 `nextLong()` / `nextInt()` | 与原版逐位一致（`WorldgenRandomVanillaGoldenTest` 全绿） |

此前 `nextLong()` 与装饰种子存在逐位差异，根因有两处，均已修复：

1. `nextLong()` 把两个半部按 `u64` 拼接。原版是
   `((long) next(32) << 32) + next(32)`，而 `next(32)` 返回带符号 `int`，两半都要**符号扩展**。
2. `IRandom::nextBits` 对 `bits >= 32` 特判成截断取低位，与
   `BitRandomSource.next(bits) = (i32)(nextLong() >>> (64 - bits))` 的取高位语义相反。

`BlendedNoiseVanillaGoldenTest` / `DensityAstBaselineTest` 仍有约 1e-7 量级残余差异，
已与随机序列无关（修复前误差在 1e-6 量级且量级随种子放大），属噪声求值的浮点精度缺口，
待单独排查。

### `entity_attributes` 的比对基准

生物属性不能简单拿 oracle 输出当基准：原版不少生物在「创建」时改写属性，
`DefaultAttributes` 的默认供应器与实体实例并不相同（史莱姆尺寸 1 只有 1 点血，
而默认供应器是 20）。所以这条 harness 的基准是
`harness/harnesses/entity_attributes_reference.ts`：

- 表里 73 个生物的期望值取自原版 `DefaultAttributes`（`javap -c` 读字节码常量，
  与 `Minecraft wiki` 一致），是**属性工厂的默认值**；
- `RUNTIME_DIVERGENT`（史莱姆/岩浆怪/幻翼）的运行时值以尺寸/状态改写，oracle 只报
  `missing`，基准仍取参考表；
- 表外的生物（例如 Cubium 有、原版没有的类型）仍由 oracle 的默认供应器兜底；
- 只比较两侧都建模的 6 个属性（生命/移速/跟随/攻击/护甲/击退抗性）；原版别的属性
  （`SPAWN_REINFORCEMENTS_CHANCE`、`ATTACK_KNOCKBACK`……）Cubium 尚未建模，不混入比对；
- 表面数据可用 `node tests/diff/tools/export_entity_attributes_reference.ts` 用 oracle
  重新导出（不带 `--write` 只打印，便于 diff；带 `--write` 只改数据段，注释与
  `RUNTIME_DIVERGENT` 原样保留）。`RUNTIME_DIVERGENT` 与实现缺口清单里的静态条目在
  oracle 侧没有输出，导出时沿用旧值。

### 活塞推动结构：一次推动的纯计算

`piston_movement` 覆盖的是**一次推动的结构求解**：给定一小块方块台、活塞位置、朝向与是否伸出，
比较「能否推动」「移动链」「破坏链」。它调用 Cubium 的 `PistonStructureHelper`（只读世界）与原版
`PistonStructureResolver`，两侧读同一份方块描述字符串。

逐刻动画（`MovingPistonBlock` 推进、方块实体 tick、收回阶段）必须真实驱动世界，属于 world 型
能力，当前不可行，仍由 `block_behavior` 的 GameTest 覆盖。判据见 skill `mc-add-diff-test` 的
「world 型能力当前不可行」一节。

首轮 523 个场景中有 61 处差异，全部指向同一类问题——**方块的「推动反应」（pushReaction）没被读取**：

- **`Block::getPushReaction` 恒返回 `Normal`**：Cubium 把 pushReaction 登记在材质上
  （`Material::m_pushReaction`），但基类实现直接 `return Normal`，从不读材质。于是火把、树苗、
  铁轨等原版登记为 `DESTROY` 的方块，被活塞推着走而不是被破坏；
- **`Material::REPLACEABLE_PLANT` 漏登记 `Destroy`**：修好基类读取后，植物类材质本身的值仍是
  默认 `Normal`，树苗继续被推动；
- **水/岩浆登记成 `Block`**：原版流体是 `DESTROY`，活塞推到水应当把它破坏掉；登记成 `Block`
  会让活塞被水完全挡住。

三处修好后 523/523 全部一致。这个例子说明：**手写的材质表里漏一个字段，不会有任何编译期提示，
只会让游戏安静地按错误规则运行**。

### 玩家经验值：公式与状态机的两处错位

`experience` 覆盖三层语义：等级 ↔ 累计经验的换算公式、加点/扣点的状态机（原版
`Player#giveExperiencePoints` 的两段 while）、以及进度条这个 f32 字段的逐位比较。

首轮 455 个单元里有 333 处差异，收敛后全部一致。查出两个缺陷，都不是「公式抄错」，
而是**状态维护路径**出了问题：

- **`_validateState()` 越权重算等级**：它在末尾有 `while (progress >= 1) { progress -= 1; level++; }`，
  把进度当成「整级数量」扣除，而不是按容量折算。这个函数本应只做越界夹取，却作用在
  `addExperience` 刚算好的、已经收敛的状态上，把合法进度强行截断。后果很重：**加 100 点
  会得到 94 级，而原版是 7 级**；
- **`addLevels` 无条件重算累计经验**：方法的注释写着「正等级时不重新计算
  `totalExperience`」，下一段却无条件用 `getExperienceForLevel(level) + progress × capacity`
  重算。原版 `giveExperienceLevels` **从不**改写 `totalExperience`（它只由加点与归零分支
  改写），于是「只加减等级」也会把累计经验改成与玩家实际获得经验不符的值。

另外，`addExperience` 原本给负数数量单独写了一套「用等级起点 + 进度重算 `total`」的路径，
与正数分支的累加路径并存且互斥。现在两者合并为原版的单一路径：
`clamp(total + amount, 0, INT_MAX)`。

修复后 440/440 一致；`ExperienceManagerTest`、`VillagerTradeXpTest` 等 184 个单测全绿。

### 一个注意事项：单次加点量要落在合理范围

加点会**逐级**推进（每次 1 级），单次给出 `2147483647` 点会让两侧都陷入数千万次循环——
那是输入不合理，不是行为差异。场景里把单次加点上限设为 10000（覆盖击杀经验的量级）。

### 药水与酿造：名前缀缺失 + 初始化顺序

`potion` 比较 46 种药水的基础名、效果列表（两侧都先按效果 ID 排序——原版存的是 `List`，
顺序没有玩法语义）、是否含瞬时效果，以及三种前缀下的翻译键。首轮 18 处差异，全部是
**`baseName` 为空**：`Potions.cpp` 里短变体药水一律写成 `Potion("", {...})`，翻译键退化成
`".effect.minecraft."`。14 处显式空串 + `empty/water/mundane/thick/awkward` 五个无参构造，
补上自身 ID 后 46/46 一致。

`potion_brewing` 比较「药水 × 试剂」的笛卡尔积（874 组），每个单元回答三件事：试剂是不是
酿造材料、能否酿造、酿造出哪种药水。首轮 93 处差异，查出两个缺陷：

- **多出一条原版没有的规则**：`尴尬药水 + 发酵蛛眼 → 虚弱`。原版只有「水瓶 + 发酵蛛眼」，
  多出的这条让玩家能凭空调出虚弱药水；
- **整组起始酿造缺失**：原版的 `PotionBrewing.Builder#addStartMix` 会为每种材料同时注册
  `水 + 材料 → 平凡` 与 `尴尬 + 材料 → 目标`，共 5 组（微风棒 / 黏液块 / 石头 / 蛛网 /
  岩浆膏）。Cubium 只登记了岩浆膏那一组，蓄风、黏稠、盘丝、渗浆四种药水**完全酿不出来**。

补齐后仍剩 46 处（全部是 `breeze_rod`），根因埋在**初始化顺序**里：

> `Items::initialize()` 里 `PotionBrewing::initialize()` 排在 `_registerPotions()` 之后、
> `_registerTrialChamberItems()` 之前。而「微风棒」正是试炼密室物品，注册得比酿造配方晚
> 24 步——配方注册时 `Items::BREEZE_ROD` 还是 `nullptr`，`_addMix` 直接静默丢弃了这条规则。

把 `PotionBrewing::initialize()` 移到 `_registerTrialChamberItems()` 之后，874/874 一致。
这类缺陷没有任何编译期或运行期提示：只有配方表少了一条，微风棒静静地「不是酿造材料」。

### 方块支撑判定：一类「根本没写」的功能缺失

`block_support` 对 1166 个方块、每个配 23 种底座，比较「能否留在这个位置」
（原版 `BlockState#canSurvive` ↔ Cubium `Block::isValidPosition`）。

场景摆放有一个容易摆错的地方：附着方式有**两类**，底座必须同时放在正下方与四个水平
侧面。只放下方的话，墙上悬挂告示牌、墙面头颅、按钮、火把、灯笼、珊瑚这些侧挂方块
会整体被判成「不支持」——那是探针没摆对场景，不是实现差异。两侧必须用完全相同的摆法。

首轮 1166 个单元里 211 处差异。修掉三处后（按钮、压力板、苔藓材质）差异降到 218 处，
但统计口径给出了本轮最有用的结论：

- **在 12 种完整方块底座上，没有任何一个方块存在差异**——正常情况全部正确；
- 218 处差异**全部**落在台阶、栅栏、漏斗、箱子、地毯、耕地、水、岩浆、空气这些
  「不能支撑」的底座上。

也就是说，问题不是「判定算错」，而是**大量方块根本没有实现支撑判定**，继承了
`Block::isValidPosition` 的默认实现（永远返回 true）。这一点用单个方块的用例很难发现。

已修复的三处（都是「缺失」而非「算错」）：

- **按钮**：`AbstractButtonBlock` 有 `getAttachDirection` 等辅助方法，但从未覆写
  `isValidPosition`，按钮能悬浮在空气中。按原版
  `FaceAttachedHorizontalDirectionalBlock#canSurvive` 补上（`isFaceSturdy(FULL)`）；
- **压力板**：同样缺失。按原版 `BasePressurePlateBlock#canSurvive` 补上
  （`canSupportRigidBlock || canSupportCenter(UP)`）；
- **苔藓块材质**：`makeMossMaterial()` 写的是 `solid(false)`，而苔藓块与苍白苔藓块都是
  完整固体方块。这让 `BlockState::isSolid()` 为 false，所有用 `isSolid()` 判支撑的方块
  （站立告示牌等）都认为苔藓块撑不住——单这一项就贡献了 179 处差异；
- **站立告示牌**：判定写成 `isSolidSide(world, below, UP)`，比原版
  `getBlockState(below).isSolid()` 严格一档，玻璃、箱子这类「有碰撞但上表面不完整」的
  方块撑不住告示牌。

已确认但**未修**的全局议题：原版 `isSolid()` 由 `calculateSolid()` 从碰撞形状推导
（碰撞盒体积 ≥ 0.729 或 Y ≥ 1.0），Cubium 靠注册时手写 `notSolid()`（全项目 234 处）。
玻璃、树叶、冰这类「有整格碰撞却被标成非固体」的方块因此与原版不一致。修它要动全部
234 个调用点，属于独立议题。TODO: 对齐 `calculateSolid` 后把玻璃/树叶/冰加回底座清单。

### 第二轮：把 218 处差异压到 85

第一轮修完之后，差异的分布指向一个更具体的病灶——**`isSolidSide` 与 `isFaceSturdy`
被混用**。前者只判「是固体且有碰撞」，后者判「这一面在形状上铺满整格」。原版里需要
「面坚固」的地方几乎都用后者，Cubium 却常常用前者，于是任何侧面不满的方块
（箱子、漏斗）在所有方向上都被当成坚固。

这一轮修掉的六处：

- **箱子**：注册时带 `notSolid()`，但原版箱子的 `legacySolid` 为 true（forceSolidOn），
  它能撑住站立告示牌。去掉多余的 `notSolid()`；
- **红石火把 / 墙上红石火把 / 拉杆 / 钟**：都没有 `isValidPosition`，能悬浮在空中。
  按原版 `BaseTorchBlock`、`WallTorchBlock`、`FaceAttachedHorizontalDirectionalBlock`、
  `BellBlock` 的对应实现补上；
- **门**：下半部分支撑判定用 `isSolidSide(UP)`，原版是 `isFaceSturdy(UP)`；
- **珊瑚 / 珊瑚扇 / 墙上海扇**：三处 `canAttachTo` 都用了 `isSolidSide`，原版是
  `isFaceSturdy(FULL)`；
- **漏斗形状**：这是本轮最隐蔽的一处。原版顶部是
  `Shapes.join(column(16,10,16) ∪ column(8,4,10), column(12,11,16), ONLY_FIRST)`，
  `ONLY_FIRST` 即「a 减 b」，所以顶部不是实心 16×16，而是**外框 16×16、中间挖空
  12×12 的环形**。Cubium 写成了实心全宽，于是上表面被当成完整面——凡是「下方需坚固」
  的方块（门、蜡烛、花、海龟蛋）都能悬在漏斗上。单这一项贡献了 108 处误判。

顺带发现 `CollisionShape::combine` 只实现了 `OR`，`AND` / `NOT` 会**静默返回空形状**，
既不报错也无提示。已在源头补上 TODO；漏斗的环形改用四条边显式拼出，不依赖差集。

### 附魔适用性：枚举归纳 vs 标签判定，28 个附魔对不上

`enchantment_applicability` 对每个附魔逐个物品问「能不能附上去」，比的是**最终物品集合**。
43 个附魔里 **28 个与原版不符**——这是目前发现的最大一处系统性偏差。

根因在表示方式：原版直接读数据包的 `supported_items` 物品标签引用（如
`#minecraft:enchantable/sharp_weapon`）；Cubium 的附魔定义是硬编码的，没有这个字段，
改用 `EnchantmentType` 枚举**归纳**原版标签，再由 `canApplyToItem` 按物品类型判定。

差异分四类：

| 模式 | 附魔 | 规模 | 根因 |
|---|---|---|---|
| 过度宽泛 | `binding_curse`、`vanishing_curse`、`mending`、`unbreaking` | 仅 Cubium 有 1000+ | `EnchantmentType::All` 直接 `return true` |
| 漏掉武器 | `sharpness`、`smite`、`bane_of_arthropods`、`fire_aspect`、`knockback`、`looting` | 仅原版有 ~14-22 | `Weapon` 只认剑与三叉戟，不认斧、矛 |
| 护甲细分未落实 | `aqua_affinity`、`respiration`（应限头盔）；`depth_strider`、`frost_walker`、`soul_speed`、`swift_sneak`（应限靴子） | 仅 Cubium 有 ~22-23 | 统一按「是护甲」放行 |
| 少量偏差 | `breach`、`density`、`wind_burst`、`efficiency`、四种保护 | 各 1 | 个别物品判定 |

前两类的玩法后果很直观：

- **给任意物品都能附耐久/经验修补/诅咒**——包括泥土、空气、刷怪蛋。原版里这些只能附在
  可损坏物品上。
- **斧子附不了锋利**。原版 `#enchantable/melee_weapon` 含斧与矛，Cubium 的 `Weapon`
  类型只匹配剑和三叉戟。

修这一类需要把 `EnchantmentType` 枚举换成真实的 `supported_items` 物品标签引用，
是附魔系统的结构性改动，会波及附魔台、铁砧、交易、战利品等所有调用方。

TODO: 按上表四类逐步替换 `canApplyToItem` 的类型判定为标签判定。优先级：先修
`EnchantmentType::All` 的兜底（影响面最大），再补武器类，最后处理护甲细分。

### 附魔代价曲线：43 个附魔逐级比对，全对

`enchantment_cost` 比较 43 个附魔的最大等级、铁砧代价，以及**每一级**的附魔台等级
区间（`minCost` / `maxCost`）。一次通过 **43/43**。

这条曲线来自数据包的 `min_cost` / `max_cost`：

```json
"min_cost": { "base": 1,  "per_level_above_first": 11 }
```

即第 L 级（从 1 起）的代价是 `base + per_level_above_first * (L - 1)`。Cubium 的
`Enchantment::getMinCost(level)` / `getMaxCost(level)` 与之逐级吻合，包括
`sharpness` 这种 5 级附魔的完整 5 个采样点。

附魔台"刷不刷得出"、铁砧"能不能合"全由这几个数决定，写错一处就会让某些附魔在特定
等级下永远刷不出来——而且不会报错。此前没有任何自动化覆盖。

比对时**按等级顺序**取值（数组有序，不能声明为无序）：代价曲线的形状本身就是要验证
的东西，排序会把它抹平。

### 伤害类型标签：一次「本来就对」的验证

`damage_type_tags` 比较 33 个伤害类型标签各自包含哪些伤害类型，一次通过 **33/33**。

这次的结论是「Cubium 本来就对」——`DamageTypeTags` 与 `DamageTypeTagLoader` 早就存在，
加载流程也与 `EntityTypeTagLoader` 同构（`initialize()` 建对象、加载器填成员）。
能力本身只做了两件事：把标签纳入差分校验范围，以及确认成员集合与数据包一致。

值得记的是它的价值不在于「发现缺陷」，而在于**把一块原本没人验证的区域变成有护栏的**。
这批标签几乎全是玩法开关：

| 标签 | 影响 |
|---|---|
| `bypasses_armor` | 能否穿甲 |
| `is_fall` | 落地保护、摔落靴判定 |
| `burns_armor_stands` | 会不会烧掉装备架 |
| `avoids_guardian_thorns` | 是否触发守卫者荆棘 |

漏一个成员就有一类伤害行为走偏，而且不会有任何东西报错。之前它们没有任何自动化覆盖。

顺带记一个 API 细节：原版 `TagKey` 用 `location()`，而 `ResourceKey` 用 `identifier()`，
两者在同一个方法体里一起出现时很容易写混（这次就写错了一次，javac 直接报出来）。

### 画变体：纯数据包注册表，加载路径踩了两个坑

Cubium 原先完全没有画变体这一层（`painting_variant` 只在物品组件里作为一个类型枚举出现），
画的尺寸、贴图、标题、作者全都无处可查。本能力为此新建了 `PaintingVariants` 注册表，
从数据包 `painting_variant/` 读取全部 51 种，结果 **51/51 一致**。

加载器本身不难，但踩了两个坑，都属于「路径拼错就静默零结果」：

1. **`listResourceStacks` 的目录参数必须带命名空间前缀**。我一开始传的是
   `"painting_variant"`，一个文件都列不到；正确写法是遍历
   `getResourceNamespaces()`，对每个命名空间传 `"<ns>/painting_variant"`。
   参考 `GenericTagLoader` 才发现的。

2. **`ResourceVersion` 是结构体而非字符串**。它是
   `{ packName, content }`，`nlohmann::json::parse` 不能直接吃它，要取
   `.content`；多数据包叠加时优先级最高的是**末尾**那个（`versions.back()`，
   与 `GenericTagLoader` 的 `rbegin()` 一致）。

两个坑的共同表现都是「注册表为空 → 探针全部 missing → 51 处差异」，而不是报错。
这类静默失败在差分能力里很容易被误读成「数据全错」。

### 附魔标签：补上了一个结构性缺口

`entity_type_tags` 那次是「探针漏了初始化」的假缺陷，`enchantment_tags` 这次则是**真的缺东西**：
Cubium 原先只用 `Enchantment::isTreasure()` / `isCurse()` 两个硬编码虚函数表达标签语义，
等于把数据包里的 22 个标签压成了 2 个开关。其余 20 个——附魔台能不能刷出
（`in_enchanting_table`）、能不能交易（`tradeable`）、战利品与生物装备各自允许哪些、
`exclusive_set/*` 的互斥组——都没有对应物。

本能力为此新建了 `EnchantmentTags` 注册表，直接复用通用的两阶段加载器
（`GenericTagLoader`），后者已经处理好了 `#` 引用的递归展开与循环依赖检测：

```
double_trade_price.json = { "values": ["#minecraft:treasure"] }
```

补完之后 **22/22 一致**，一个成员都没错。

过程中确认了一件值得记的事：`exclusive_set/` 是**子目录**，第一次用
`Get-ChildItem -Filter *.json` 只列了顶层 15 个文件，误以为全集就是 15 个。
`GenericTagLoader` 是递归扫描的，那 7 个互斥组标签被自动发现并创建——
这反过来验证了「加载器遇到未预注册的标签要按需 `registerTag`」这条改动是必要的：
没有它，这 7 个标签即使文件在也永远是 `missing`。

TODO: `tooltip_order` 的**顺序**语义当前没有承载（成员集是无序 set）。它决定提示框里
附魔的排列次序，与其它标签的集合语义不同，若后续要用到需另存一份有序数组。

### 唱片机歌曲：两处「比较口径」而非数据缺陷

`jukebox_song` 比较 21 首歌的声音事件、描述翻译键、时长、比较器输出。首轮 21/21
全差异，但**没有一处是数据错的**，全是比较口径没对齐：

1. **描述该取翻译键而不是渲染结果**。我最初用 `Component.getString()`，它会把组件
   解析成当前语言下的显示文本（`"C418 - 11"`），而 Cubium 存的是原始键
   `jukebox_song.minecraft.11`。本能力比的是数据本身，不是渲染结果，改为取
   `TranslatableContents.getKey()`。

2. **`%g` 在两个语言里的尾零处理不同**。同一个 `178.0f`，C++ 的 `%.6g` 打成 `"178"`，
   Java 的却打成 `"178.000"`。统一为「6 位有效数字再去掉尾随零」，即 Java
   `Float.toString` 的口径。这个坑在 `food_properties` 里已经踩过一次，这里是同一个
   问题换了个字段——凡是比较 f32 的地方都得留意。

### 方块支撑：用「探针直接打印」代替几何推理

`block_support` 从 218 处一路收敛，其中台阶侧面那 54 处卡了很久：我反复读源码推导
`isFaceSturdy` 该不该为真，始终不得要领。转折点是**放弃推理、让探针把判定矩阵打出来**——
把同一批方块摆到 `facing=north` 与 `facing=south` 两种台阶上，一眼就能看出是哪一侧判反了。

这一步之后连查出两个独立的方位错误：

- **楼梯形状方位整体错开**。`SHAPE_INDEX_MAP` 的 STRAIGHT 段是 `{12, 5, 3, 10}`，
  而原版 `SHAPE_BOTTOM_STRAIGHT` 在 NORTH 下是北半侧。方位错开后北侧面只剩半格，
  贴上去的墙火把、墙上海扇、墙面头颅全被判为无法存活。
- **按钮附着方向反了**。`canSurvive` 是 `canAttach(world, pos, getConnectedDirection(state).getOpposite())`，
  Cubium 少做一次取反，`facing=north` 的按钮去问了北边的方块，而它实际贴在南边。

两处修完 75 → 61。值得记的是第二个错误**只有在第一个修好之后才浮出来**：台阶两侧
坚固性不同，方向一反就对不上，而先前台阶形状本身就错，掩盖了它。

### 已定位但未修的根因：VoxelShape 的坐标压缩方式与原版不同

`block_support` 剩下的主要差异（箱子 42、下半砖 41、耕地 40、漏斗 39、地毯 38）
指向同一个底层问题。

原版 `VoxelShape` 的坐标集来自**形状实际用到的边界**并补上 padding：

```
箱子 column(14,0,14) 的 Y 坐标集 = [0, 0.875, 1.0]（尺寸 2）
findIndex(Y, 0.9999999) -> 1
SliceShape 取 [p_83179_, p_83179_+1] = [0.875, 1.0]
```

Cubium 的 `VoxelShape(std::shared_ptr<DiscreteVoxelShape>)` 把坐标点按 `i / size`
**均匀分布**，同一个箱子得到 14 个 Y 层，`findIndex` 返回 13，切片变成第 13 层。

层数不同导致 `getFaceShape` 取到的切片范围不同，进而 `SupportType.CENTER`
（`Block.column(2.0, 0.0, 10.0)`）是否被完全包含的结论也不同。这解释了为什么灯笼、
紫水晶簇这类「只需中心一小块坚固」的方块会在箱子、耕地、半砖上判错，而完整方块上正常。

TODO: 让 `VoxelShape` 的坐标集改用「实际边界 + padding」而非均匀分布。这是
`VoxelShape` 的根基改动，影响光照遮挡、形状布尔运算等所有使用方，需要独立评估与
全量回归，不适合夹在差分能力的迭代里做。

### 实体类型标签：一次「假缺陷」

`entity_type_tags` 比较 46 个实体类型标签各自包含哪些实体。第二轮迭代后才全绿，中间
那版曾得出一个**错误结论**，值得记下来。

首版 14 处差异全部是「成员缺失」，缺了 13 个标签（`boat`、`can_equip_saddle`、
`followable_friendly_mobs`、`illager_friends`、`non_controlling_rider`…），看起来
就是 Cubium 少声明了一大批标签。我一度据此写下「补齐这 13 个标签」的 TODO。

真正的根因在**探针自己身上**：它只调了 `EntityTypeTags::initialize()`，而这个函数
只负责把内建的若干标签对象建出来；**成员集来自数据包文件**
（`data/minecraft/tags/entity_type/*.json`）。服务端启动时 `RegistryBootstrap`
会在 `initialize()` 之后接着调 `EntityTypeTagLoader`——探针漏了第二步。

于是同一个现象有两层成因：

- 数据包里的 46 个文件都在，但没被加载；
- 即便加载，`resolveEntityTypeTagWithDependencies` 里是
  `auto* tag = getTag(location); if (tag != nullptr) {...}`——**找不到就静默跳过**，
  不会按需创建。这条在加载器里也补了（改为 `getTag` 空则 `registerTag`），
  属于真实健壮性缺陷，但不是本次差异的主因。

补上数据包加载后 46/46 一致，13 个标签一个不少。

教训：**先确认探针和被测系统走的是同一条初始化路径**，再谈「被测系统有缺陷」。
这条能力里，探针漏一步初始化就会凭空造出 13 个「缺陷」。

另一个独立踩坑：全集**不能**用 oracle 的 `list` 枚举。原版
`BuiltInRegistries.ENTITY_TYPE.getTags()` 在只 Bootstrap、未加载数据包时只返回 4 个
内联标签，拿它当全集会让能力只覆盖那一小撮（表现为「4 个单元全部差异」）。
本能力在 TS 侧显式列出原版的 46 个标签。

### 食物属性：同一个量放在了两套字段语义里

`food_properties` 比较 44 种食物物品的饥饿值、饱和度修正、能否饱腹后继续吃。全绿，
但过程里踩了三个坑，都属于**比较口径**而非实现缺陷：

1. **字段语义不同**。原版 `FoodProperties.saturation` 存的是**已经换算过的饱和值**
   （1.21 组件化后直接写死乘积），Cubium 的 `Food::getSaturationModifier()` 存的是
   **修正系数**，换算发生在 `FoodStats::addStats`（`food × modifier × 2`，与原版
   `FoodConstants.saturationByModifier` 一致）。两侧「吃下去会怎样」完全相同，只是把
   同一个量放在了不同字段里。在探针侧正算 `nutrition × modifier × 2` 会与两侧写死的
   f32 常量差最后一个 ulp（`2.4f` vs `2.4000001f`），最终改为**直接比系数**。

2. **f32 的打印精度**。同一个 `0.3f`，Java 经 Gson 输出 `0.3`，C++ 经 nlohmann 输出
   `0.30000001192092896`。统一按 6 位有效数字输出。

3. **`%.6g` 在两侧语义不同**。C++ 的 `%.6g` 把 0.3 打成 `"0.3"`，Java 的却打成
   `"0.300000"`。统一为「6 位有效数字再去掉尾随零」，即 Java `Float.toString` 的口径。

经验：**先确认两侧比较的是同一个量**，再谈数值相等。这三步里前两步如果没拆开排查，
很容易误判成「Cubium 食物数据全错」。

### 一个经验：先用「完整方块底座」把真实缺陷筛出来

218 处差异里，只有 12 个方块在完整的固体底座上也出错，其余 188 个只在
台阶、栅栏、漏斗、箱子这类「不能支撑」的底座上出错。这个切分很有用：它把
**「判定算错」**与**「根本没实现判定」**分开了。前者要读逻辑，后者只要照原版补方法。

### 第三轮：方向传反（85 → 75）

珊瑚的 `canAttachTo(world, pos, direction)` 里，`direction` 是**从珊瑚指向底座**的
方向（下方珊瑚传 `Down`）。但检查时直接把 `direction` 当成了「底座朝向珊瑚的面」，
于是实际查的是底座的**下表面**。后果是上下半砖的结果完全颠倒：

| 底座 | 原版 | 修复前 |
|---|---|---|
| `oak_slab[type=top]` | 撑得住（真） | 撑不住 |
| `oak_slab[type=bottom]` | 撑不住 | 撑得住 |

改成 `Directions::opposite(direction)` 后 85 → 75。这类「方向取反」的错很难靠读代码
发现——两个方向都"看起来合理"，只有把上下半砖这种**镜像底座**摆进用例才暴露。

剩余 75 处集中在台阶侧面（54）与箱子/漏斗（42/39，彼此有重叠）。已确认其中一部分是
`CollisionShape::getFaceShape` 的问题：它对「触碰该面」的判定用 `maxCoord ≈ 1.0`，
而原版会把触碰面的盒子**投影成满格**再判完整。台阶这类分段拼接的形状因此判错。
TODO: 对齐 `getFaceShape` 的投影语义后继续收敛。

## 下一步候选能力

按「只读、无状态、易对齐」排序：

1. **方块状态 → NBT / 数字 ID 编解码** —— 直接落在存档与网络协议的 1:1 对齐上；
2. **命令解析树** —— 从原版导出 command tree（字面量/参数/重定向），与 `/help` 看到的
   树逐节点比对；`command_argument` 已经把参数层对齐了；
3. **实体 / 附魔缺失项** —— 补齐 `KNOWN_MISSING` 里那 3 个附魔与 32 个实体类型，
   清单随之删条目。

## 结构起点：已查出的问题

`structure_starts` 首轮跑通就查出并修掉的结构生成缺陷（每一条都对着原版源码核实过）：

- **结构集条目抽签不是加权轮盘**：原版 `StructureTemplatePool.getRandomTemplate` 是在
  **按权重逐份展开**的列表上取下标，Cubium 原来用的是"随机数落在哪个权重区间"的另一种
  抽法——起始块因此选错（村庄整座偏移十几格）。现已按下标语义重写；
- **单条目结构集不得消耗抽签随机数**：原版 `list.size() == 1` 时直接生成，Cubium 原来
  一律走抽签，多消耗一次随机数，末地城等结构随之整体错位；
- **村庄不该有专用装配流程**：村庄的结构定义 JSON 与拼图结构完全同构，原版也走同一条
  `JigsawPlacement.addPieces`（含 `start_jigsaw_name` 起始连接点选择）。Cubium 单写的
  `VillageStructure` 先抽旋转再抽起始块、且不按名字过滤起始块，起始块与装配走向都错。
  该类已删除，村庄改走通用 `JigsawStructure`；
- **拼图结构的生物群系校验要用候选生成点**：原版在 `GenerationStub` 的位置（起始块中心、
  投影后的地面线）采样，Cubium 在区块中心 + y=0 采样，群系边界处会把合法结构整片误杀
  （古代城市首轮被误杀就是这个原因）。裸 `JigsawStructure` / 试炼密室 / 前哨站现均按
  候选点校验，且校验位于装配之后（与原版 `findValidGenerationPoint` 的调用位置一致）；
- **掠夺者前哨站的起始高度**：原版是 `start_height: absolute 0` + `project_start_to_heightmap:
  WORLD_SURFACE_WG`，Cubium 原来硬编码成 `nearTerrain` 特判（起始高度 0 或 64），
  只有 1/16 的情形恰好等于投影结果；
- **废弃传送门的模板要在登记起点前加载**：原来模板由 `generate` 惰性加载，起点先登记，
  包围盒于是退化成单点；现在登记前加载并断言模板存在，缺模板会直接报错而不是静默消失。

仍未对齐（各自已是独立的结构实现缺口，不属于装配框架问题）：

- **要塞**：`ConcentricRingsStructurePlacement` 只是按固定环数硬编码落点，没有原版按
  群系搜索后还要求"不是海洋"的那一步；语料里的要塞单元全部缺失；
- **废弃矿井 / 试炼密室 / 末地城 / 海底废墟 / 埋藏宝藏**：构件生成逻辑与原版不同
  （矿井的起始房间与分段规则、海底废墟的模板选取、末地城的塔楼递归），需要逐个按
  1.21.11 重写；
- **村庄 / 古代城市 / 前哨站的构件数已一致，但个别连接点取值仍有差异**（`structure_starts`
  逐片段比对可直接定位）。

`structure_starts` 的探针与 oracle 都只读"本区块内的结构起点"，不写方块、不依赖邻区，
因此可以逐区块独立复算；语料由 oracle 按原版放置规则枚举候选区块后等距取样，再补每格的
邻居区块与若干空白区块，用来覆盖"候选判定不通过"这条路径。

## 村民交易：已查出的问题

村民交易拆成四个能力，各管一段：

- `villager_list` 导出**整张交易表**（每职业 5 个等级的全部条目）。它不经过抽签，所以每一处
  差异都能定位到"哪一级、第几条、哪个字段"，是查表数据最快的手段；
- `villager_gen` 验证**抽取过程**：同一 seed 下抽中哪两条、每条内部抽到什么；
- `villager_price` 单独验证 `MerchantOffer.getAdjustedBuyPrice()` 的价格公式；
- `villager_restock` 单独验证 `VillagerEntity::restock()` 的先更新需求再重置用量。

`villager_list` 首轮跑通就查出并修掉的交易表数据错误（都对着 1.21.11 的 `VillagerTrades` 核实过）：

- 农民的面包 / 南瓜派 / 曲奇 / 蛋糕、牧师的萤石、牧羊人的剪刀：`maxUses` 用了 16，
  原版这几个是 `DEFAULT_SUPPLY`（12）；
- 屠夫的煤炭：经验记为"卖"档，原版是 `XP_LEVEL_1_BUY`（2）；
- 制图师的探险地图装饰类型：`village` / `ocean_explorer` 之类的自造 id 换成原版的
  `village_plains` / `village_desert` / `village_savanna` / `village_taiga` / `village_snowy` /
  `monument` / `mansion` / `jungle_temple` / `swamp_hut` / `trial_chambers`。

`villager_gen` 在表数据对齐之后，又暴露出随机序列的四处错位，同样已修：

- **整批交易共用一条随机流**：原版 `addOffersFromItemListings` 把同一个 `RandomSource` 既用于
  "抽哪一条"、又传给条目的 `getOffer`。项目原来给每条派生独立子流，两段随机互相独立，
  抽取序列整体不同；现在整批共用一条；
- **离线随机源必须是 `RandomSource.create(seed)` 对应的 `LegacyRandomSource`**（Java 48 位
  LCG），不是 Xoroshiro；
- **附魔池的顺序就是标签的声明顺序**：原版按下标取标签内容，项目早先按 id 排序过一遍，同一
  随机数抽到了别的附魔；`non_treasure` / `on_traded_equipment` 的顺序本身是数据驱动写死的，
  不能重排；
- **附魔书的价格公式**：`2 + nextInt(5 + level*10) + 3*level`，命中
  `#double_trade_price`（经验修补）再翻倍、封顶 64。

仍未对齐：**附魔装备 / 附魔书的附魔内容**（390 个单元里的 35 个）。根因是原版逐附魔维护
`supported_items` / `primary_items` / `exclusive_set` / `weight` 四个数据字段，而项目把附魔
写死在代码里、只有粗粒度的 `EnchantmentType` 分类，于是候选列表（哪些附魔可能出现在这件
装备上）与各附魔的等级区间都和原版不同。修法是把附魔定义搬成数据驱动（或至少补齐这四张表），
属于独立的一轮工作；这些单元已在 `villager_gen` 里登记为已知差异，抽取序列、价格、次数、
经验仍然逐字段严格比对。
