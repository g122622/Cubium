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
│   ├── recipe_book_category.cpp # 每条配方落在配方书的哪个页签（类型 + 分类 → 归属）
│   ├── fuel_values.cpp          # 每种物品作为燃料的燃烧时长
│   ├── block_sound_type.cpp     # 方块声音组（五个音效事件 + 音量 + 音调）
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
│   │   ├── recipe_book_category.ts # 配方书页签归属的比对口径（补齐 type/page）
│   │   ├── fuel_values.ts       # 燃料燃烧时长的比对口径（0 表示不是燃料）
│   │   ├── block_sound_type.ts  # 方块声音组的比对口径（补齐七个字段）
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
| `recipe_book_category` | 原版 jar `RecipeBookCategoryOracle` | 1470（每条配方一项） | 全部一致 | 见下文 |
| `fuel_values` | 原版 jar `FuelValuesOracle` | 1505（全部已注册物品） | 全部一致 | 见下文 |
| `block_sound_type` | 原版 jar `BlockSoundTypeOracle` | 1167（全部方块 + 饰纹陶罐裂纹状态） | 全部一致 | 见下文 |
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
| `armor_trim` | 原版 jar `ArmorTrimOracle` | 18 个纹饰图案 + 11 种纹饰材料 | 全部一致 | — |
| `animal_variant` | 原版 jar `AnimalVariantOracle` | 32 个变体（狼 9 / 猫 11 / 蛙·牛·鸡·猪 各 3） | 全部一致 | — |
| `instrument` | 原版 jar `InstrumentOracle` | 8 种山羊角音色 | 全部一致 | — |
| `banner_pattern` | 原版 jar `BannerPatternOracle` | 43 | 全部一致 | Cubium 仍是硬编码枚举，探针直接读数据包（见下文） |
| `enchantment_provider` | 原版 jar `EnchantmentProviderOracle` | 7 项 × 7 种难度乘数 = 49 | 全部一致 | Cubium 未实现，探针直接读数据包（见下文） |
| `trial_spawner_config` | 原版 jar `TrialSpawnerConfigOracle` | 28 项 × 9 种玩家数 = 252 | 全部一致 | Cubium 用整数 Config，探针直接读数据包（见下文） |
| `dimension_type` | 原版 jar `DimensionTypeOracle` | 4 个维度类型 | 全部一致 | Cubium 无数据包加载器，探针直接读数据包（见下文） |
| `explosion_ray` | 原版 jar `ExplosionRayOracle` | 6 个场景 × 8 个种子 = 48 | 全部一致 | 纯计算，不依赖世界（见下文） |
| `explosion_damage` | 原版 jar `ExplosionDamageOracle` | 6 半径 × 8 距离 × 5 可见度 × 2 抗性 = 480 | 全部一致 | 纯计算，暴露两处口径差异（见下文） |
| `explosion_seen_grid` | 原版 jar `ExplosionSeenGridOracle` | 10 种碰撞箱 | 全部一致 | 纯计算，绕开 `-ffast-math`（见下文） |
| `entity_dimensions` | 原版 jar `EntityDimensionsOracle` | 12 种尺寸 × 位置组合 | 全部一致 | 纯计算，Cubium 无此类（见下文） |
| `mob_effect_merge` | 原版 jar `MobEffectMergeOracle` | 16 种叠加组合 | 全部一致 | 纯计算，Cubium 无 `update()`（见下文） |
| `food_stats` | 原版 jar `FoodStatsOracle` | 20 种变化组合 | 全部一致 | 纯计算，暴露一处下界钳制差异（见下文） |
| `food_tick` | 原版 jar `FoodTickOracle` | 21 种状态 × 难度组合 | 全部一致 | 纯计算，四个互斥分支（见下文） |
| `damage_type_tags` | 原版 jar `DamageTypeTagsOracle` | 33 个伤害类型标签 | 全部一致 | — |
| `enchantment_cost` | 原版 jar `EnchantmentCostOracle` | 43 个附魔的等级代价曲线 | 全部一致 | — |
| `enchantment_applicability` | 原版 jar `EnchantmentApplicabilityOracle` | 43 个附魔 × 全物品 | 全部一致（28 → 0） | — |
| `item_component_nbt` | 原版 jar `ItemComponentNbtOracle` | 1124 物品 × 3 变体（plain / suite / removed） | 3372/3372 一致 | 381 个物品未注册（方块物品，同 `known_missing_items.ts`） |
| `stack_codec` | 原版 jar `StackCodecOracle` | 3021 单元（1124 物品 × base / suite + 11 个数量与坏数据场景） | 2256/2259 一致；3 处已登记的已知差异 | 见下文 |
| `loot_function` | 原版 jar `LootFunctionOracle` | 41 个单函数场景（随机值域 / 时运加成 / 截断 / 伤害 / 附魔 / 组件 / 组合） | 全部一致 | 见下文 |
| `vec3_math` | 原版 jar `Vec3MathOracle` | 31 个向量运算 | 全部一致 | 纯计算，含 `1e-5F` 短路阈值 |

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

`block_sound_type` 比的是**每个方块的声音组**：破坏 / 踩踏 / 放置 / 击打 / 坠落五个
音效事件，加上音量与音调。单元是方块 ID（取默认状态），另有
`minecraft:decorated_pot#cracked=true` 这一个状态相关的变体。

这类错误不会崩溃、不影响平衡，只会让玩家听到不该有的声音——木头踩出石头声、羊毛摔出
金属声、陶罐碎了却还是普通破碎声。除了逐个方块比对，没有别的办法发现。

原版声音组是「方块属性上的一个常量」，Cubium 拷贝的是一张手写的常量表，于是错的形状
很集中：整族方块套了通用声音组（`Material` 默认值），以及几种「看着像 A 其实是 B」的
细节。首轮跑通查出 **273 处差异**，分四类全部修掉：

- **整族套错声音组**：木质告示牌/悬挂告示牌/木门/活板门/按钮/压力板按木种分（樱花、
  竹子、绯红各有自己的组）、珊瑚与珊瑚块、铁质方块（`IRON` 音调 1.0，`METAL` 是 1.5）、
  铁轨、下界砖、虫蚀方块、瓜梗……
- **原版按状态切换声音组**：`DecoratedPotBlock#getSoundType(BlockState)` 在裂隙状态
  用碎裂声。Cubium 的 `getSoundType()` 不带状态，整条链路缺失——本次给 `Block` 补了
  虚函数 `getSoundType(const BlockState&)`，破坏/放置音效改走状态入口。
- **空音效事件的名字**：`SoundEvents.EMPTY` 的注册名是 `minecraft:intentionally_empty`，
  而 Cubium 自造的 `minecraft:block.air.ambient` / `minecraft:empty` 都不存在。
- **物品音效写在方块命名空间下**：作物放置声是 `minecraft:item.crop.plant`、
  下界疣是 `minecraft:item.nether_wart.plant`，不是 `block.*`。

音量/音调是 f32：`0.3f` 提升成 double 会写成 `0.30000001192092896`，与原版的
`0.3` 文本不同。探针输出前统一收窄到「最短往返十进制」，让两侧写法一致。

`fuel_values` 比的是**每种物品作为燃料的燃烧时长**（原版 1.21.11 `FuelValues`）。
单元是物品 ID，两侧都只输出一个 `burnTime`（0 表示不是燃料）。

燃料表是「物品种类 → tick 数」的一张表，玩家对它的容错极低：少一种木材就烧不着，
数值差一倍整组矿的烧制时间就不对，而且这类错误不会崩溃也不会报错。原版 1.21.11 用
**标签聚合**表达这张表（`ItemTags.LOGS` 一条覆盖全部原木变体，末尾再
`.remove(NON_FLAMMABLE_WOOD)` 剔除绯红/诡异木），Cubium 侧是逐物品的 if 链——所有
乘除都作用在 **200 的基准**上而不是中间值，这是最容易抄错的地方。

首轮跑通查出 14 处差异，全部修掉：

- **12 种木质架子烧得太久（450 → 300）**：原版 `200 * 3 / 2 = 300`，Cubium 写成
  「基础木质 300 × 1.5 = 450」——拿已放大的 300 当基准再乘一次，凭空多出 150 tick。
- **绯红 / 诡异架子被误判为燃料**：这两族属于 `NON_FLAMMABLE_WOOD`，原版在表尾显式
  移除，Cubium 把它们一起算进了木质架子。
- **木长矛不认作燃料**：`Items.WOODEN_SPEAR` → 200，Cubium 的木制工具组里漏了它。
- **`minecraft:air` 被判成合法物品**：探针侧要按原版口径把它当作未注册。

`recipe_book_category` 比的是**每条配方落在配方书的哪个页签**。这个归属由两个维度
合成：配方**类型**（熔炼 / 高炉 / 烟熏 / 营火 / 切石 / 锻造各有一套页签）与数据包里的
`category` 字段（有序合成是 building / redstone / equipment / misc，烧制是 food /
blocks / misc）。原版 `Recipe#recipeBookCategory()` 把它们压成一个注册表键，例如
`minecraft:crafting_equipment`、`minecraft:smoker_food`。

只比这一个字段，就能同时发现两类错误：分类字段解析错了，以及类型映射错了。两个**不是**
机械对应的地方尤其值得压——烟熏炉只有「食物」一页（分类写 blocks 也落 `smoker_food`），
高炉没有食物页（食物类落 `blast_furnace_misc` 而不是凭空的 `blast_furnace_food`）。

首轮跑通查出 30 处差异，全部修掉：

- **12 种特殊配方的类型被笼统写成 `special`**：原版把它们注册成各自的序列化器
  （`crafting_special_armordye`、`crafting_special_bookcloning`……）。Cubium 的
  `SpecialRecipe` 其实已经持有精确 ID（`getSerializerId()`），只是探针没用它。
- **16 个收纳袋从装备页掉进杂项页**：`crafting_transmute` 的 `category: equipment`
  没有被解析——`TransmuteRecipe` 也是 `CraftingRecipe` 的实现，走同一套分类。
- **2 条红石高炉配方从 blocks 掉进 misc**：`parseBlastingRecipe` 等三个转换函数在
  「熔炼配方 → 高炉/烟熏/营火配方」的二次构造里**丢掉了分类**，`category` 字段因此
  在转换时消失。这一处是重构 RecipeBook 时才暴露的——旧模型根本没有分类这回事。

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

### 各能力的实现记录

状态总表之上的逐能力调查、踩坑与验证结论已拆分到 `docs/test/diff/`：

- 单能力文档：`docs/test/diff/<能力名>.md`
- 历史调查记录（跨多轮的排查过程）：[`_investigations.md`](../../docs/test/diff/_investigations.md)
- 跨能力共性规律（`-ffast-math`、门控下沉、用例不敏感等）：[`_patterns.md`](../../docs/test/diff/_patterns.md)

已拆出文档的能力：

[`aabb_ops`](../../docs/test/diff/aabb_ops.md)、[`anvil_durability`](../../docs/test/diff/anvil_durability.md)、[`banner_pattern`](../../docs/test/diff/banner_pattern.md)、[`beacon_effects`](../../docs/test/diff/beacon_effects.md)、[`brewing_tick`](../../docs/test/diff/brewing_tick.md)、[`dimension_type`](../../docs/test/diff/dimension_type.md)、[`direction_rotate`](../../docs/test/diff/direction_rotate.md)、[`enchantment_provider`](../../docs/test/diff/enchantment_provider.md)、[`enchantment_selection`](../../docs/test/diff/enchantment_selection.md)、[`entity_attributes`](../../docs/test/diff/entity_attributes.md)、[`entity_dimensions`](../../docs/test/diff/entity_dimensions.md)、[`explosion_damage`](../../docs/test/diff/explosion_damage.md)、[`explosion_ray`](../../docs/test/diff/explosion_ray.md)、[`explosion_seen_grid`](../../docs/test/diff/explosion_seen_grid.md)、[`food_stats`](../../docs/test/diff/food_stats.md)、[`food_tick`](../../docs/test/diff/food_tick.md)、[`furnace_tick`](../../docs/test/diff/furnace_tick.md)、[`hopper_cooldown`](../../docs/test/diff/hopper_cooldown.md)、[`item_same_components`](../../docs/test/diff/item_same_components.md)、[`item_stackable`](../../docs/test/diff/item_stackable.md)、[`mob_effect_merge`](../../docs/test/diff/mob_effect_merge.md)、[`mth_modulo`](../../docs/test/diff/mth_modulo.md)、[`mth_rounding`](../../docs/test/diff/mth_rounding.md)、[`repair_cost`](../../docs/test/diff/repair_cost.md)、[`spawner_delay`](../../docs/test/diff/spawner_delay.md)、[	rial_spawner_config](../../docs/test/diff/trial_spawner_config.md)、[`vec3_math`](../../docs/test/diff/vec3_math.md)

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

本节的完整内容已拆分：

- **逐能力的实现要点与踩坑** → `docs/test/diff/<能力名>.md`
- **各轮排查过程与结论** → [`_investigations.md`](../../docs/test/diff/_investigations.md)
- **跨能力规律与检查清单** → [`_patterns.md`](../../docs/test/diff/_patterns.md)

下面只保留「影响面大、需要长期跟踪」的缺口摘要。

### 初始化缺口：方块材质默认值

`block_properties` 目前只比较**已完全对齐**的字段（硬度 / 亮度 / 滑度 / 是否流体）。
以下三项仍来自材质默认值而不是逐方块注册：

| 字段 | 现象 | 根因 |
|---|---|---|
| `resistance`（爆炸抗性） | 几十个方块为 0 | `BlockProperties::hardness()` 不像原版那样同时作为抗性默认值 |
| `opacity` / `opaque` | 半砖、门、栅栏等被当成 15 档全遮挡 | `m_opacity` 默认 15，未逐类设置 |
| `solid` / `replaceable` | 门、栅栏门、树叶、植物等判定相反 | 取自材质 `m_isSolid` / `isReplaceable` |

### 实现缺口：`KNOWN_MISSING` 清单

`known_missing_items.ts`、`entity_shape.ts`、`enchantment_table.ts`、
`known_missing_entity_attributes.ts` 里的 `KNOWN_MISSING` 列的是「原版有、本项目尚未实现」
的条目——**实现缺口，不是参照数据缺失**。清单显式列出、每次运行输出条数，实现补齐后必须
删掉对应条目，否则会出现「豁免了原版也有的项」的假一致。

### 已定位但未修：`VoxelShape` 坐标压缩

`block_support` 有 61 处差异，根因是 Cubium 的 `VoxelShape` 坐标压缩方式与原版不同。
详见 [`_investigations.md`](../../docs/test/diff/_investigations.md)。

### 长期未收敛：`structure_starts`

要塞生成仍未命中，已排除五种假设，详见
[`_investigations.md`](../../docs/test/diff/_investigations.md)。

---

## 下一步候选能力

按「只读、无状态、易对齐」排序：

1. **方块状态 → NBT / 数字 ID 编解码** —— 直接落在存档与网络协议的 1:1 对齐上；
2. **命令解析树** —— 从原版导出 command tree（字面量/参数/重定向），与 `/help` 看到的
   树逐节点比对；`command_argument` 已经把参数层对齐了；
3. **实体 / 附魔缺失项** —— 补齐 `KNOWN_MISSING` 里那 3 个附魔与 32 个实体类型，
   清单随之删条目。

## 结构生成与村民交易：已查出的问题

这两部分的详细记录已拆分到
[`_investigations.md`](../../docs/test/diff/_investigations.md)。
