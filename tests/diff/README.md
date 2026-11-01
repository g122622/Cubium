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

### 新能力 mth_modulo：`wrapDegrees` 的端点不对称与负零

第 92 个能力，31 个取模/角度输入全部一致。与上一轮的 `mth_rounding` 同属 `Mth` 数值工具。

#### 原版实现

```java
public static int positiveModulo(int x, int y)        { return Math.floorMod(x, y); }
public static float positiveModulo(float x, float y)  { return (x % y + y) % y; }
public static double positiveModulo(double x, double y) { return (x % y + y) % y; }

public static int wrapDegrees(int deg) {
    int i = deg % 360;
    if (i >= 180)  { i -= 360; }
    if (i < -180)  { i += 360; }
    return i;
}
```

#### 三个关键点

**1. 整数版与浮点版的 `positiveModulo` 不是同一个函数。** 整数走 `Math.floorMod`，浮点
走 `(x % y + y) % y`。实测：

| 单元 | 结果 |
|---|---|
| `modi\|-7\|3` | **2** |
| `modi\|7\|-3` | **-2** |
| `modi\|-6\|3` | **0**（整除时不能变成 3） |

结果的符号**跟随除数 `y`**，不是恒为正——写成「负数就加 `y`」会在 `y < 0` 时出错。

**2. `wrapDegrees` 的区间是 `[-180, 180)`，端点不对称。**

| 输入 | 结果 |
|---|---|
| `180` | **-180** |
| `-180` | **-180** |
| `179.9` | 179.9 |
| `-179.9` | -179.9 |

`180` 被调整成 `-180`，而 `-180` 保持不变。这与函数名暗示的「对称」不符，但正是两个
`if` 条件（`>= 180` 与 `< -180`）的直接结果。

**3. `wrapDegrees` 用两个独立 `if`，依赖 `% 360` 的结果严格落在 `(-360, 360)`。**
正因为这个前提，两次调整至多命中一次，所以 `if`、`if/else if`、`while` 在有限输入下等价。
本能力用 `±720`、`±900`、`±1000000` 把这一前提钉住：

| 单元 | 结果 |
|---|---|
| `-720` | `-0`（双精度负零）/ `0`（整数版） |
| `-900` | `-180` |
| `1000000` | `-80` |

#### 一处实现差异（行为等价）

Cubium 的 `MathUtils::wrapDegrees` 用的是 `if / else if`，原版是两个独立 `if`。由于两个
分支互斥，**行为完全等价**——本能力的 31 个单元把它验证出来了，无需修改。

Cubium 目前**没有** `positiveModulo`，那部分是待实现的行为基准。

#### 负零的保留

`wrap|-720` 在双精度版本得到 `-0`、整数版本得到 `0`。`std::fmod(-720.0, 360.0)` 返回负零，
后续的 `if` 都不命中，负零一路保留到输出。两侧格式化后都显示为 `-0`，一致。

### 新能力 mth_rounding：`Mth.floor` 不是 `std::floor`

第 91 个能力，21 个取整输入。19 个一致，2 个记为已知差异。`Mth` 的取整函数被全项目
广泛调用，但从未被系统性比对过。

#### 原版实现

```java
public static int floor(float f)  { int i = (int) f; return f < i ? i - 1 : i; }
public static int floor(double d) { int i = (int) d; return d < i ? i - 1 : i; }
public static int ceil(float f)   { int i = (int) f; return f > i ? i + 1 : i; }
public static int ceil(double d)  { int i = (int) d; return d > i ? i + 1 : i; }
```

**不是 `Math.floor`**，而是「先向零截断成 int，再比较」。在射程内两者结果相同，但：

#### 负数的取整方向

| 输入 | `floor` | `ceil` |
|---|---|---|
| `-0.5` | **-1** | **0** |
| `-1.5` | **-2** | **-1** |
| `-2.9` | **-3** | **-2** |

常见错误写法是 `static_cast<i32>(x)`——它对负数**向零截断**，等价于 `ceil` 而不是
`floor`。把 `floor` 写成它就是错的。

#### 超界输入：语言层面的差异，不是实现缺陷

| 输入 | 原版 | 探针 |
|---|---|---|
| `3000000000` | `2147483647` | `-2147483648` |
| `-3000000000` | `2147483647` | `-2147483648` |

- **Java** 的 `(int)` 转换对超界值是**饱和**的（一律 `Integer.MAX_VALUE`）
- **C++** 的 `static_cast<i32>(double)` 对超界值是**未定义行为**（本平台实测得到 `MIN_VALUE`）

这不是 Cubium 的实现缺陷——C++ 标准就没有定义这个行为。因此登记为**已知差异**而非待修
问题。（注意 `-3000000000` 在 Java 侧也得到 `MAX_VALUE`，这是 `(int)` 饱和的规则，与符号
无关。）

#### 这条能力为什么值得存在

`Mth.floor` / `ceil` 的正确性影响面极广（区块坐标转换、方块位置计算、渲染插值）。把
21 个代表性输入的取整结果固定下来，可以在有人把它「优化」成 `std::floor` 时立刻发现。
`std::floor(-0.5)` 得 `-1`（与 `Mth.floor` 相同），但 `std::floor` 的返回类型是浮点，
`static_cast<i32>(std::floor(2.9))` 与 `Mth.floor(2.9f)` 在边界值上仍可能因浮点精度差异
而不同。

### 新能力 spawner_delay：`-1` 是哨兵值，「先重置再递减」

第 90 个能力，15 种延迟状态组合全部一致。刷怪笼的生成延迟此前没有被比对过。

#### 原版代码

```java
private void delay(Level level, BlockPos pos) {
    if (this.maxSpawnDelay <= this.minSpawnDelay) {
        this.spawnDelay = this.minSpawnDelay;                       // 区间退化
    } else {
        this.spawnDelay = this.minSpawnDelay
            + level.random.nextInt(this.maxSpawnDelay - this.minSpawnDelay);
    }
}

// serverTick
if (isNearPlayer(...) && level.isSpawnerBlockEnabled()) {
    if (this.spawnDelay == -1) { this.delay(level, pos); }
    if (this.spawnDelay > 0) { this.spawnDelay--; }
    else { ... 尝试生成 ... }
}
```

#### 三个关键点

**1. `-1` 是「需要重置」的哨兵值**，不是「还剩 -1 tick」。`spawnDelay == -1` 时先调
`delay()` 取值，**然后继续走正常流程**——同一个 tick 内「先重置、再减 1」。实测
`tick|-1|true|true|200|800|0` 得到 `reset=true` 且 `delay=199`（= 200 - 1）。

**2. 区间退化时直接取 `minSpawnDelay`**，条件是 `maxSpawnDelay <= minSpawnDelay`
（**包含相等**）。实测 `200|200` 得 200、`300|200` 也得 300，都不调用随机数。
若写成 `<`，两者相等时会去调 `nextInt(0)`。

**3. `nextInt` 的上界是 `max - min`**，即区间左闭右开 `[min, max)`。默认 200/800 时
`roll=599` 得到 **799**——**永远取不到 800**。

#### 前置条件是「整 tick 都不动」

`isNearPlayer(...) && isSpawnerBlockEnabled()` 不满足时，**既不重置也不递减**：

| 单元 | `delay` | `reset` | `decremented` |
|---|---|---|---|
| `tick\|-1\|false\|...` | **-1** | false | false |
| `tick\|20\|true\|false\|...` | **20** | false | false |

容易写成「玩家走远了仍然倒计时」或「−1 仍然会被重置」——两者都与原版不符。

#### 一个 tick 内的动作次序

| 单元 | `delay` | `decremented` | `shouldSpawn` |
|---|---|---|---|
| `tick\|1\|true\|...` | 0 | **true** | **false** |
| `tick\|0\|true\|...` | 0 | false | **true** |

减到 0 的那个 tick **不生成**，要等到下一个 tick 才生成。这与 `hopper_cooldown` 那轮的
边界一致：递减在前、判定在后。

#### 验证方法

延续前三轮的做法，跑完逐条打印实际输出核对分支，15 条全部命中。

#### 未覆盖范围

实际生成（`spawnCount` 次尝试、`spawnRange` 内的位置随机、附近实体数量上限），需要世界
与实体注册表。

### 新能力 hopper_cooldown：递减发生在冷却判定之前

第 89 个能力，10 种冷却状态组合全部一致。漏斗的传输冷却时序此前没有被比对过。

#### 原版代码

```java
public static void pushItemsTick(...) {
    hopper.cooldownTime--;                        // 无条件递减
    hopper.tickedGameTime = level.getGameTime();
    if (!hopper.isOnCooldown()) {
        hopper.setCooldown(0);
        tryMoveItems(level, pos, state, hopper, () -> suckInItems(level, hopper));
    }
}

if (!hopper.isOnCooldown() && state.getValue(HopperBlock.ENABLED)) {
    if (!hopper.isEmpty())  { flag = ejectItems(...); }
    if (!hopper.inventoryFull()) { flag |= supplier.getAsBoolean(); }
    if (flag) {
        hopper.setCooldown(8);                    // MOVE_ITEM_SPEED
    }
}

private boolean isOnCooldown() { return this.cooldownTime > 0; }
```

#### 三个关键点

**1. 递减发生在「是否冷却」判定之前。** `cooldownTime == 1` 的那个 tick：先减到 0，
然后 `isOnCooldown()` 为假，**本 tick 就能传输**。实测：

| 单元 | `after` | `attempted` | `moved` |
|---|---|---|---|
| `1\|true\|true` | 8 | **true** | **true** |
| `2\|true\|true` | 1 | **false** | false |

也就是说「设置冷却 8」之后，第 2..8 个 tick 递减、第 9 个 tick 再次传输——间隔恰好 8 个 tick。

**2. 传输成功才设置冷却。** `1|true|false`（无物品可动）得到 `after=0, cooldownSet=false`
——没有传输就不设冷却，下一个 tick 会立刻再试。

**3. 递减是无条件的**，`cooldownTime` 会一路减到 `-1` 并停在那里（`-1` 是构造器的初始值）。

#### 一处「实现不同但行为等价」的差异

Cubium 的 `HopperEntity::tick` 写的是：

```cpp
if (m_transferCooldown > 0) { --m_transferCooldown; }
```

原版是无条件的 `cooldownTime--`。差别在于冷却为 0 时：原版减到 -1，Cubium 停在 0。

**行为上看不出区别**——因为 `isOnCooldown()` 只在 `> 0` 时为真，两者都不冷却。但 NBT 里
存下的 `TransferCooldown` 值会不同（原版 -1、Cubium 0）。本能力比较的是内部状态值，因此
这个差异在校验范围内被记录下来；它不影响玩法，但会影响存档数据的一致性。

#### 验证方法

延续前两轮的做法，跑完逐条打印实际输出核对分支，10 条全部命中（见上表前两行即为边界用例）。

#### 未覆盖范围

- `ejectItems` / `suckInItems` 各自的槽位遍历顺序与方向选取（需要世界与容器）

### 新能力 beacon_effects：半径与时长两个容易记错的常数

第 88 个能力，10 种层级 × 效果组合全部一致。Cubium 目前**只有客户端 `BeaconRenderer`**，
没有服务端信标方块实体——本能力提供的是待实现的行为基准。

#### 原版公式

```java
double d0 = levels * 10 + 10;                       // 范围半径
int i = 0;
if (levels >= 4 && Objects.equals(primary, secondary)) {
    i = 1;                                          // 满级且主副相同 -> 主效果 II
}
int j = (9 + levels * 2) * 20;                      // 时长（tick）
```

#### 四个关键点

**1. 半径 = `层数 × 10 + 10`**：1 级 20、2 级 30、3 级 40、4 级 50。写成 `层数 × 10`
会整体少 10 格。

**2. 时长基数 9**：`(9 + 层数 × 2) × 20` tick → 1 级 220、4 级 340。这个 9 容易记成 10。

**3. 升到 II 级需要「满级且主副效果相同」**：只有 4 级才有 `amplifier = 1`，3 级及以下
永远是 0。实测 `3|true|false` 得 `amp=0`，`4|true|false` 得 `amp=1`。

**4. 副效果与「主效果升 II」互斥**：

| 单元 | `amplifier` | `appliesSecondary` |
|---|---|---|
| `4\|true\|true`（主副相同、有副） | **1** | **false** |
| `4\|false\|true`（主副不同、有副） | **0** | **true** |

注意第一行：即使「有副效果」，只要主副相同就走升 II 那条路，**不再单独施加副效果**。
`4|true|true` 与 `4|true|false` 的输出完全相同——这一条如果只凭规则描述去实现，很可能
写成「有副就施加副」，从而让 4 级同效果的信标多施加一个 0 级效果。

#### 验证方法

延续上一轮的做法，跑完逐条打印实际输出核对分支。10 条全部命中：

```
0|false|false -> radius=10 dur=180 amp=0 secondary=False
1|false|false -> radius=20 dur=220 amp=0 secondary=False
4|true|true   -> radius=50 dur=340 amp=1 secondary=False   <- 互斥
4|false|true  -> radius=50 dur=340 amp=0 secondary=True    <- 独立施加
```

#### 未覆盖范围

- `updateBase` 的底座方块检测（需要世界）
- 信标光束的可见性（纯渲染，属客户端）

### 新能力 brewing_tick：四处口径差异，与烘焙台同类问题

第 87 个能力，12 种状态组合全部一致。酿造台每 tick 的状态机此前没有被比对过。

#### 原版状态机

```java
if (fuel <= 0 && fuelSlot.is(ItemTags.BREWING_FUEL)) {
    fuel = 20;                       // 设为 20，不是加 20
    消耗一个燃料;
}

boolean brewable = isBrewable(...);
boolean brewing = brewTime > 0;
if (brewing) {
    brewTime--;
    if (brewTime == 0 && brewable) {
        doBrew(...);
    } else if (!brewable || !ingredientSlot.is(this.ingredient)) {
        brewTime = 0;                // 中途条件破坏 -> 取消
    }
} else if (brewable && fuel > 0) {
    fuel--;                          // 开始酿造时就扣燃料
    brewTime = 400;
    this.ingredient = ingredientSlot.getItem();
}
```

#### 四处差异

Cubium 已有 `BrewingStandEntity::tick`，但结构与原版不同：

| 项 | 原版 | Cubium |
|---|---|---|
| 燃料种类 | `ItemTags.BREWING_FUEL` 标签 | 硬编码 `Items::BLAZE_POWDER` |
| 燃料补充方式 | `fuel = 20`（覆盖写） | `m_fuel += FUEL_PER_BREW`（累加） |
| **燃料扣除时机** | **开始酿造时** `fuel--` | **酿造完成时** `_consumeFuel()` |
| 不可酿造时 | 取消只发生在酿造中；未开始时什么都不做 | 开头直接 `m_brewTime = 0; return;` |

第三条影响最实质：**若在完成时才扣燃料，中断的酿造不会消耗燃料**——玩家可以反复启动、取消来白嫖。原版在 `brewTime = 400` 的同时就扣掉了。

第四条也有行为差异：原版在「未在酿造且不可酿造」时**不触碰** `brewTime`；Cubium 主动清零。

#### 本轮的验证方法调整

前两轮连续出现「用例看起来合理、实际没打到目标分支」（`99|50|99|50` 被钳到 0、`hasFuel` 字段位写错）。这轮改进了做法：**跑完后把每一条单元的实际输出逐条打出来，对照单元表注释核验它走了哪个分支**。

12 条全部确认命中目标：

| 单元 | 预期分支 | 实际输出 |
|---|---|---|
| `0 / 0 / true / true / true` | 补燃料后开始 | `fuelAdded=true, fuel=19` |
| `0 / 10 / true / false / true` | 开始酿造 | `fuelConsumed=true, fuel=9, brewTime=400` |
| `1 / 10 / true / false / true` | 本 tick 完成 | `brewed=true` |
| `100 / 10 / true / false / false` | 换试剂取消 | `brewTime=0` |
| `200 / ...` / `2 / ...` | 正常递减 | `199` / `1` |

这一步成本很低（一条命令），但能直接暴露「单元没生效」的问题。后续新能力会固定带上这一步。

#### 未覆盖范围

- `doBrew` 的实际产物转换（属 `potion_brewing` 的职责）
- 药水瓶位的 `HAS_BOTTLE` 方块状态同步（需要世界）

### 新能力 furnace_tick：熄灭后进度是「减 2」不是「归零」

第 86 个能力，12 种状态组合全部一致。熔炉每 tick 的状态机此完全没有被比对过。

#### 状态机骨架

```java
boolean wasLit = isLit();
if (isLit()) { litTimeRemaining--; }

if (isLit() || hasFuel && hasInput) {
    if (!isLit() && canBurn(...)) {          // 2a 点燃
        litTimeRemaining = getBurnDuration(fuel);
        litTotalTime = litTimeRemaining;
    }
    if (isLit() && canBurn(...)) {           // 2b 推进烹饪
        cookingTimer++;
        if (cookingTimer == cookingTotalTime) { cookingTimer = 0; 产出; }
    } else {
        cookingTimer = 0;                     // ← 块内：直接归零
    }
} else if (!isLit() && cookingTimer > 0) {
    cookingTimer = Mth.clamp(cookingTimer - 2, 0, cookingTotalTime);   // ← 块外：减 2
}
```

#### 四个易错点

**1. 两条退进度路径的处理不同**，这是本轮最重要的发现：

| 情形 | 走哪条 | `cookingTimer` |
|---|---|---|
| 燃烧中但条件不满足 | 块内 `else` | **归零** |
| 完全熄火 | 块外 `else if` | **减 2** |

实测 `0|0|10|200|...` 得到 `cookingTimer=8`（减 2），而 `100|200|5|200|false|...` 得到 `cookingTimer=0`（归零）。把两处合并成一处，熄火后的进度衰减会完全错掉——**减 2 意味着熄火后进度是缓慢回退，不是立刻清零**。

**2. 点燃与烹饪在同一 tick 内可连续发生。** 两个 `if` 是顺序关系而非互斥。实测刚点燃的单元得到 `litTimeRemaining=200` 且 `cookingTimer=1`——点燃当 tick 就开始计进度。

**3. 熄灭判定在读减之后。** `wasLit` 在 tick 开头记录，比较发生在末尾，用来决定方块状态是否更新。

**4. `cookingTimer == cookingTotalTime` 是相等判断**，不是 `>=`。

#### 又一次「用例写错导致分支没被覆盖」

我设计单元表时给「点燃」那条写的是 `0|0|0|200|true|true|false|200`，注释写「有燃料有原料」——但字段顺序是 `canBurn|hasInput|hasFuel`，第七位 `false` 意味着 **`hasFuel=false`**，根本不会点燃。

这条单元跑出来 `litTimeRemaining=0`、`stateChanged=false`，看起来「通过」了，实际上**点燃分支一次都没被执行**。是我核对输出时发现注释与结果对不上，才回头查出字段位置写错。

修正为 `...|true|true|true|200` 后，得到 `litTimeRemaining=200, cookingTimer=1, stateChanged=true`，点燃分支才真正被覆盖。

**这是连续第二轮遇到同一类问题**（上轮是 `99|50|99|50` 被钳到 0，这轮是字段位写错）。两次都是「用例看起来合理、实际没打到目标分支」。教训要再收紧一层：**单元表写完后必须逐条核对「这条单元预期走哪个分支」，而不是写完就算。**

### 新能力 item_same_components：判空必须在比物品之后

第 85 个能力，12 组配对全部一致。Cubium **没有** `isSameItemSameComponents`，最接近的是
`ItemStack::canMergeWith`——后者语义不同，会影响容器合并、配方匹配、掉落物堆叠等多处。

#### 原版逻辑

```java
public static boolean isSameItemSameComponents(ItemStack a, ItemStack b) {
    if (!a.is(b.getItem())) {
        return false;
    }
    return a.isEmpty() && b.isEmpty() ? true : Objects.equals(a.components, b.components);
}

public static boolean matches(ItemStack a, ItemStack b) {
    if (a == b) {
        return true;
    }
    return a.getCount() != b.getCount() ? false : isSameItemSameComponents(a, b);
}
```

#### 三个关键点

**1. 两个空栈被视为「相同」。** `a.isEmpty() && b.isEmpty()` 这一支返回 `true`——空栈之间
没有组件可比，语义上就是相等。

**2. `matches` 比 `isSameItemSameComponents` 多一道数量判定。** 实测：

| 输入 | `sameItemSameComponents` | `matches` |
|---|---|---|
| 泥土 ×1 vs 泥土 ×5（同损伤） | **true** | **false** |
| 钻石剑（损伤 0）vs 钻石剑（损伤 1） | false | false |
| 空 vs 空 | **true** | **true** |

两者用途不同：前者回答「能否堆叠到一起」，后者回答「是否完整相等」（如配方匹配）。

**3. 组件相等是深比较。** 损伤值、附魔、自定义名称、lore 都属于组件；任意一项不同就不相等。
「同物品、不同损伤」的两件钻石剑**不能**堆叠。

#### 本轮自己踩的坑：判空顺序写反了

我第一版探针是这么写的：

```cpp
const bool sameItem = !left.isEmpty() && !right.isEmpty() && left.getItem() == right.getItem();
```

它要求**两边都非空**才算「同物品」，于是「两空」这一支永远走不到，空栈配对得到 `false`。
差分比对立刻把这个单元指出来了（原版 `true` / 探针 `false`）。

正确写法是**先判两空**，再比物品：

```cpp
if (left.isEmpty() && right.isEmpty()) {
    sameItemSameComponents = true;
} else if (!left.isEmpty() && !right.isEmpty() && left.getItem() == right.getItem()) {
    sameItemSameComponents = left.getDamage() == right.getDamage();
}
```

原版之所以能写成那样，是因为在它那里「空栈的物品就是 AIR」，`a.is(b.getItem())` 对两个空栈
天然为真，判空分支只是在短路组件比较。**Cubium 用 `nullptr` 表示空，这个前提不成立，直接
照抄表达式就会错。** 这是表示法差异第一次真正影响到逻辑复刻——`item_stackable` 那轮只是
输出值不同，这次是**控制流走错**。

#### 与 Cubium 的口径差异

`canMergeWith` 在任一为空时返回 `false`（原版两空为 `true`），且只比较了损伤、修复成本、
名称、lore 等若干字段，不是完整的组件深比较。已在探针头记 TODO：待 Cubium 实现完整深比较后
改为直接调用。

### 新能力 item_stackable：可损坏 ≠ 不可堆叠

第 84 个能力，23 个「物品 × 损伤」组合。22 个一致，1 个记为已知差异。

#### 原版三段逻辑

```java
public int getMaxStackSize() {
    return this.getOrDefault(DataComponents.MAX_STACK_SIZE, 1);   // 默认 1！
}

public boolean isDamageableItem() {
    return this.has(DataComponents.MAX_DAMAGE)
        && !this.has(DataComponents.UNBREAKABLE)
        && this.has(DataComponents.DAMAGE);
}

public boolean isStackable() {
    return this.getMaxStackSize() > 1 && (!this.isDamageableItem() || !this.isDamaged());
}
```

#### 两个反直觉的结论

**1. `MAX_STACK_SIZE` 的默认值是 1，不是 64。** 没有这个组件的物品（工具、盔甲）一律不可
堆叠。默认值写成 64 会让所有工具都被判为可堆叠。

**2. 可损坏 ≠ 不可堆叠。** 实测对照：

| 物品 | 上限 | 可损坏 | 已损伤 | `stackable` |
|---|---|---|---|---|
| 泥土 | 64 | false | false | **true** |
| 雪球 | 16 | false | false | **true** |
| 钻石剑（未损伤） | 1 | true | false | **false** |
| 钻石剑（已损伤） | 1 | true | true | **false** |

钻石剑在「未损伤」时仍不可堆叠——因为 `maxStack > 1` 这一项就已经为假，`isDamageableItem`
那半边根本没机会生效。**真正决定工具不可堆叠的是上限 1，不是「可损坏」这个属性。**

**3.（未覆盖）`UNBREAKABLE` 会让物品重新变得可堆叠。** `isDamageableItem()` 里有
`!has(UNBREAKABLE)`，不可破坏的钻石剑在组件层面不再是「可损坏物品」，于是
`isStackable()` 直接放行。Cubium 的 `isDamageable()` 已经排除了 unbreakable，逻辑等价；
但 `ItemStack` 目前没有设置该组件的公开入口，因此单元表无法覆盖这一支，已在探针头
记 TODO。

#### 一处表示法差异

`minecraft:air` 单元暴露了空栈表示法的差异：

| 字段 | 原版 | Cubium |
|---|---|---|
| `empty` | true | false |
| `maxStackSize` | 1 | 64 |
| `stackable` | false | true |

根因是两边用**不同的东西**表示空栈：原版 `isEmpty()` 是
`this == EMPTY || item == Items.AIR || count <= 0`，把 AIR 物品本身当作空；Cubium 用
`nullptr` 表示空，`ItemStack(Items.AIR)` 是一个**非空的、装着空气的栈**。

这是既有的设计选择（不是本能力引入的），因此登记为已知差异而非缺陷。但它提醒了一件事：
**下游任何 `if (stack.isEmpty())` 的分支，在两边的边界行为并不相同**。

### 新能力 anvil_durability：一个「测试用例本身不够敏感」的教训

第 81 个能力，16 种耐久组合全部一致。与上一轮的 `repair_cost` 相邻，覆盖铁砧合并同物品时
的耐久叠加。

#### 原版代码

```java
int l  = itemstack.getMaxDamage()  - itemstack.getDamageValue();
int i1 = itemstack2.getMaxDamage() - itemstack2.getDamageValue();
int j1 = i1 + itemstack1.getMaxDamage() * 12 / 100;
int k1 = l + j1;
int l1 = itemstack1.getMaxDamage() - k1;
if (l1 < 0) { l1 = 0; }
if (l1 < itemstack1.getDamageValue()) {
    itemstack1.setDamageValue(l1);
    i += 2;
}
```

#### 三个易错点

**1. `* 12 / 100` 必须先乘后除。** 对钻石剑（1561）：正确得 187，先除后乘只得 180。

**2. 「有改善」才计入。** `l1 < itemstack1.getDamageValue()` 为假时，损伤值不变、代价也
不 +2。实测主物品满耐久（`1561|0`）得到 `repaired=false, costAdded=0`。

**3. 新损伤值钳到 0**，不能取负。

#### 本轮最大的教训：用例不够敏感会给出虚假的通过

我最初选的顺序敏感用例是 `99|50|99|50`。它**确实**能区分「先乘后除（11）」与
「先除后乘（0）」——但两侧都半损，总耐久远超上限，`newDamage` 一律被钳到 **0**。
于是「算错」和「算对」得到同样的输出，这条用例**完全没有起到验证作用**。

发现问题后我换成 `1561|1500|1561|1400`（主物品剩 61、副物品剩 161）：

| 算法 | 加成 | `newDamage` |
|---|---|---|
| 先乘后除（正确） | 187 | **1152** |
| 先除后乘（错误） | 180 | 1159 |

两者都不到 0，**差异才显现出来**。实测输出 `1152`，确认了实现用的是正确顺序。

这与之前 `explosion_damage` 那轮的教训是同一条：**测试输入必须避开被钳制/被短路吞掉
差异的区间**，否则能力会给出虚假的通过。区别在于那次是「射程内输入测不出钳制差异」，
这次是「钳到 0 吞掉了顺序差异」。

#### 溢出边界

`maxDamage * 12` 是 `int` 运算，理论上在 `maxDamage > 178956970` 时溢出。用例覆盖了
`178956970`（界内）与 `178956971`（`2147483652`，仍小于 `INT_MAX`）两点，两侧一致。

### 新能力 repair_cost：一个几乎无法从规则描述推出的条件

第 80 个能力，14 种代价组合全部一致。铁砧的修复代价此前完全没有被比对过。

#### 原版代码

```java
int i3 = itemstack1.getOrDefault(DataComponents.REPAIR_COST, 0);
if (i3 < itemstack2.getOrDefault(DataComponents.REPAIR_COST, 0)) {
    i3 = itemstack2.getOrDefault(DataComponents.REPAIR_COST, 0);
}
if (k != i || k == 0) {
    i3 = calculateIncreasedRepairCost(i3);
}

public static int calculateIncreasedRepairCost(int cost) {
    return (int) Math.min(cost * 2L + 1L, 2147483647L);
}
```

`i` 是已消耗的修复材料数，`k` 是经验代价。

#### 三个反直觉的点

**1. 取两者中较大的代价**，不是相加。主物品与副物品的 `REPAIR_COST` 只取 `max`。

**2. `k != i || k == 0` 会让代价「跳过增长」。** 当经验代价**恰好等于**消耗的材料数时，
`k != i` 为假；若此时 `k` 也不是 0，整条判断为假，代价**不翻倍**。实测：

| 输入 | `nextCost` | `increased` |
|---|---|---|
| `3\|0\|2\|2`（代价 3、材料 2、经验 2） | **3**（不变） | false |
| `0\|0\|0\|0`（三者全 0） | **1**（仍增长） | true |

第二行说明 `k == 0` 那一半是为了兜住「全都为 0」的情形——否则新物品永远拿不到代价 1。

**3. 翻倍用 `long`，结果截断到 int 上限。** `cost * 2L + 1L` 是 64 位运算，再
`Math.min(..., Integer.MAX_VALUE)`。用 32 位直接算会在 `cost > 1073741823` 时溢出成负数。
实测 `1073741823` 得到 `2147483647`，正是截断的结果。

#### 能力边界

本能力只看**代价这一层**，不涉及 `createResult` 的其余部分（材料修复的伤害削减、同物品
合并、附魔合并、重命名、`cost >= 40` 的封禁）。那些与槽位、附魔表强耦合，属另一层。

### 新能力 enchantment_selection：同一类错误第三次出现

第 79 个能力，14 物品 × 12 等级 = 168 个单元。105 个一致，63 个记为已知差异。

附魔领域此前已有 7 个能力全部覆盖**定义层**（代价曲线、适用性、标签、效果、宝藏枚举）。
本能力补上**选择逻辑**：给定物品与等级，最终挑出哪些附魔、各为几级。

#### 原版逻辑

```java
stream.filter(e -> e.value().isPrimaryItem(stack) || flag)   // flag = 是不是书
      .forEach(e -> {
          for (int i = e.value().getMaxLevel(); i >= e.value().getMinLevel(); i--) {
              if (level >= e.value().getMinCost(i) && level <= e.value().getMaxCost(i)) {
                  list.add(new EnchantmentInstance(e, i));
                  break;      // 每个附魔最多一项
              }
          }
      });
```

#### 三类差异，同一根因

| 差异 | 表现 | 根因 |
|---|---|---|
| 缺宝藏附魔 | `mending` / `vanishing_curse` / `binding_curse` / `frost_walker` / `soul_speed` 从不出现 | `canApplyAtEnchantingTable` 里硬编码了 `if (isTreasure()) return false;` |
| 多 `thorns` | 盔甲类多处多出荆棘 | 荆棘的主物品判定被放宽 |
| 多 `sharpness` 等 | `diamond_axe` 能出锋利 / 亡灵杀手 / 节肢杀手 | 斧被归入 `EnchantmentType::Weapon` 粗分类 |

共同根因：**Cubium 用粗粒度类型枚举代替了原版的物品级主物品表**，并把原版由**调用方**
负责的宝藏过滤内移到了被调方。

#### 「闸门前移」是第三次出现

原版 `getAvailableEnchantmentResults` 不判断宝藏——是否包含宝藏附魔由**传进来的
`Stream<Holder<Enchantment>>`** 决定。Cubium 把这道闸门写进了
`canApplyAtEnchantingTable`，于是任何调用方都拿不到宝藏附魔。

这与前几轮的 `explosion_damage`（把调用方的 `inRange` 闸门前移进伤害公式）、
`food_stats`（`add` 缺下界）属于同一类问题：**把「调用方的职责」内移到「被调方的
公式/判定」里**。这类错误的共同特征是：

- 单独看被调方，逻辑「看起来更自洽」；
- 但对**所有**调用方都生效，而原版只对**部分**路径生效；
- 差分测试是唯一能发现它的手段——因为没有哪个单点测试会同时覆盖两条调用路径。

#### 顺序差异的处理

原版返回顺序由附魔注册表决定，Cubium 的遍历顺序不同。顺序本身不是本能力要验证的口径
（它属于注册表加载顺序，已有能力覆盖），因此探针与 oracle **两侧都按附魔 id 排序**后输出，
让差异集中在「哪些附魔、各几级」上。

### 新能力 food_tick：四个互斥分支与两个阈值

第 78 个能力，21 种「状态 × 难度」组合全部一致。上一轮的 `food_stats` 覆盖了
`add`/`eat`/`addExhaustion`，本能力补上 `FoodData.tick` 的状态机。

#### 分支结构

```
1) exhaustionLevel > 4.0F  ->  -=4.0F，然后扣饱和（有）或扣饥饿（无饱和且非 PEACEFUL）
2) regen && sat>0 && hurt && food>=20  -> 每 10 tick 回 min(sat,6)/6 点，加等量 exhaustion
3) regen && food>=18 && hurt           -> 每 80 tick 回 1 点，加 6 exhaustion
4) food<=0                             -> 每 80 tick 判定一次是否造成 1 点饥饿伤害
5) 以上都不满足                        -> tickTimer 归零
```

三个容易写错的地方：

**1. exhaustion 消耗在所有分支之前，且只判定一次。** 它不是 `while` 循环——即使
`exhaustionLevel` 是 100，一次 tick 也只减 4。

**2. 饥饿伤害是否真的扣血取决于生命值与难度的组合**：

```java
health > 10.0F || difficulty == HARD || (health > 1.0F && difficulty == NORMAL)
```

实测对照：生命 1 + NORMAL 得 `damage=0`，生命 1 + HARD 得 `damage=1`。这个组合条件
写成「生命低于某值就扣血」会全错。

**3. 默认分支会把 tickTimer 归零。** 条件不满足时计时器**不保留**——这意味着再生
条件一旦中断，之前的累积就白费。实测 `15|2|0|50` 输入得到 `tickTimer=0`。

#### 一个跨分支的时序陷阱

快再生的 `addExhaustion(min(sat, 6))` 会立刻抬高 exhaustion，**下一次 tick** 就可能
触发最上面的消耗分支，从而把饱和扣掉。也就是说「回血」与「扣饱和」在连续两个 tick
里是联动的。场景表最后一条（`20|8|3|9`）就是压这种同一 tick 内多分支叠加的情形。

#### 为什么用复算而不是直接调用原版

`FoodData.tick` 需要真实的 `ServerPlayer`（读难度、游戏规则、受伤状态、生命值，并调用
`heal`/`hurtServer`），而 `exhaustionLevel` 只有私有字段没有公开写访问器。因此 oracle
按原版方法体逐行复算，把玩家侧的可变因素作为输入显式传入。

### 新能力 food_stats：一处下界钳制差异，与一次被源码纠正的误判

第 77 个能力，20 种变化组合全部一致。

#### 差异：`add` 缺少下界钳制

原版 `FoodData.add`：

```java
this.foodLevel       = Mth.clamp(food + this.foodLevel, 0, 20);
this.saturationLevel = Mth.clamp(saturation + this.saturationLevel, 0.0F, (float) this.foodLevel);
```

Cubium 的 `FoodStats::addStats` 写的是 `std::min(m_foodLevel + food, MAX_FOOD_LEVEL)`——
**只钳上界，没有下界**。`food + foodLevel < 0` 时会得到负饥饿值，而原版是 0。

实测 `add|-25|0|10|3` 得到 `foodLevel=0, saturationLevel=0`，正是下界把它抬回来的结果。

#### 另一处差异：两次 clamp 的顺序

饱和度上界用的是**刚更新过的** `foodLevel`，不是旧的。先算饱和度再更新饥饿值会得到不同
结果。场景表里的 `add|-20|5|10|3` 专门压这条：饥饿值先被钳到 0，饱和度上界随之变成 0。

#### 一次被源码纠正的误判

我最初只读到 `addExhaustion` 的一段片段，据此写下「原版没有上限，Cubium 加了 `min(x, 40)`
是多余限制」。后来完整读 `FoodData.java` 才发现原版那一行就是：

```java
public void addExhaustion(float e) {
    this.exhaustionLevel = Math.min(this.exhaustionLevel + e, 40.0F);
}
```

**Cubium 在这里是对的**，是我的片段阅读出了错。探针注释已改正。

这是这一系列工作中第三次出现「只读片段就下结论」的问题（前两次分别是要塞的常量种子与
扫描步长）。**读源码必须读完整的方法体**，否则「发现差异」本身可能是幻觉。

### 新能力 mob_effect_merge：`hiddenEffect` 的降级回退

第 76 个能力，16 种叠加组合全部一致。Cubium 的 `EffectInstance` **没有 `update()`**，
本能力因此直接按原版公式计算，输出的是待实现的行为基准。

#### 三层分支里最容易漏的两处

原版 `MobEffectInstance.update()` 的分支耦合度很高，本轮实测踩到其中一个：

**1. 等级更高时无条件采用新时长**，哪怕新的更短——此时旧效果被压进 `hiddenEffect`，
等新效果过期后重新生效。

| 输入 | 结果 | hiddenEffect |
|---|---|---|
| `1|300` ← `2|100` | `amplifier=2, duration=100` | `1|300` |
| `2|300` ← `1|500` | 不变（`changed=false`） | `1|500` |

第二行是反向情形：等级更低时**只把新值放进 hiddenEffect**，当前值完全不动。

**2. ambient 更新时也会置 `flag = true`**——这是本轮发现的一处真实偏差。

原版这段的末尾有一句 `flag = true;`：

```java
if (!incoming.ambient && this.ambient || flag) {
    this.ambient = incoming.ambient;
    flag = true;          // <- 容易看漏
}
```

我第一版按「ambient 只影响 ambient 字段」实现，结果 `1|200|true` ← `1|100|false`
（等级相同时长更短、只有 ambient 变）被判成 `changed=false`，而原版是 `true`。
差分比对直接把这行漏掉的分支指出来了。

#### 一处 Java 侧的技术处理

`hiddenEffect` 是包内可见的私有字段，没有公开访问器，oracle 里用反射读取。不读它就无法
验证上面第一条「降级回退」——而这恰恰是最容易写错的一支。

### 新能力 entity_dimensions：三条容易写错的公式

第 75 个能力，12 种「尺寸 × 缩放 × 位置」组合全部一致。Cubium 目前**没有**
`EntityDimensions` 也没有 `EntityAttachments`，所以探针自带一份实现——本能力覆盖的是
**公式**（眼睛高度比例、缩放规则、包围盒展开），而不是「实体的宽高表是否正确」。

#### 三条容易写错的公式

**1. 眼睛高度只按高度缩放。** 原版：

```java
public EntityDimensions scale(float w, float h) {
    return new EntityDimensions(this.width * w, this.height * h,
                                this.eyeHeight * h, ...);   // <- 乘 h，不是 w
}
```

写成 `eyeHeight * w` 在宽高比不等于 1 的实体上立刻出错。实测验证：

| 输入 | `scaledEyeHeight` |
|---|---|
| `0.6 / 1.8` 只缩宽 0.5 | **1.53**（不变） |
| `0.6 / 1.8` 只缩高 0.5 | **0.765**（减半） |

**2. 包围盒以中心为基准，不是脚底某角。** X/Z 是 `x ± width/2`，Y 是 `[y, y + height]`
（脚底在 y）。宽扁体 `4 × 0.5` 的实测结果是 `bboxMinX=-2, bboxMaxX=2`——半径是
`width/2 = 2`，若误用 `width` 会得到 ±4。

**3. 默认眼睛高度 = `height * 0.85F`**，在构造器里算好，`scale` 时再按高度乘一次。

#### 一处精度细节

`f = width / 2.0F` 是 **float**，而位置 `x` 是 **double**。`x - f` 里 `f` 先提升到 double
再相减。C++ 侧必须保持同样的提升顺序，否则在大坐标（如 `-1234.5`）下会差 ulp。
场景表里专门放了两个大坐标用例来压这一点。

### 新能力 explosion_seen_grid：`-ffast-math` 与 2.78e-17

第 74 个能力，10 种碰撞箱全部一致。它是上一个能力 TODO 的直接延伸：`getSeenPercent`
的**采样网格生成**是纯计算，只有射线检测那一半需要世界。

#### 网格本身值得单独钉住的理由

采样点数 = `floor(1 / step) + 1`，这个「+1」在 `f` 的浮点累加误差下可能少一次或多一次。
Cubium 的 `Explosion::_getBlockDensity` 用 `f32` 做这一步，原版用 `double`——同样的碰撞箱
会得到不同点数，可见度分母随之改变，最终伤害也跟着变。

#### 一个花了不少时间的坑

网格逻辑照抄无误，但 `offsetX` 上持续差 `2.77556e-17`：

```
oracle: {"offsetX":"0",    "firstSample":[["0",...]]}
probe : {"offsetX":"2.77556e-17","firstSample":[["2.77556e-17",...]]}
```

`offsetX` 来自 `(1.0 - floor(1/step) * step) / 2.0`。只影响 `step` 恰为 1/3、1/6 的两个
箱子（完整方块与大尺寸）。我单独写了一个小程序验证 `1.0/3.0` 的往返，得到精确 `0`——
说明表达式本身没问题，是**编译环境**的差异。

根因是项目根 CMakeLists 全局开了 `-ffast-math`（`-fno-finite-math-only` 保留 NaN 语义，
但重结合仍被允许）。它可以把 `floor(1/step) * step` 折叠成 `(1/step) * step`，后者不保证
等于 1。

试过两条路都失败：

- `#pragma float_control(precise, on, push/pop)`——`push/pop` 只能出现在文件或命名空间作用域；
- 换成函数内 `on`——clang 在 `-ffast-math` 下对该 pragma 不生效。

最终改法是把判据显式写出来：

```cpp
const f64 covered = gridCount * step;
const f64 remainder = std::abs(covered - 1.0) < 1e-12 ? 0.0 : (1.0 - covered);
```

这与原版语义一致（能整除时余数就是 0），且不依赖编译器的浮点策略。

#### 教训

**「表达式与 Java 完全一致」不足以保证结果一致**，编译选项会改变求值结果。差分测试的价值
正在于此——它不需要我预先知道哪里有陷阱，只要两侧用同一份输入跑，差异自己会浮出来。
### 新能力 explosion_damage：两处口径差异，差 295 倍

第 73 个能力，6 半径 × 8 距离 × 5 可见度 × 2 击退抗性 = 480 个单元，全部一致。
它一建立就暴露出 Cubium 与原生版的**两处口径差异**。

#### 差异一：距离比例被钳制

原版 `ExplosionDamageCalculator.getEntityDamageAmount`：

```java
float f = explosion.radius() * 2.0F;
double d0 = Math.sqrt(entity.distanceToSqr(center)) / f;   // 裸比值，不钳制
double d1 = (1.0 - d0) * seenPercent;
return (float) ((d1 * d1 + d1) / 2.0 * 7.0 * f + 1.0);
```

Cubium 写的是 `std::min(1.0f, std::sqrt(distSq) / damageRadius)`。

「跳过超距实体」这件事在**调用方**完成（`ServerExplosion` 里的 `if (!(d0 > 1.0))`），
不在公式里。Cubium 把这道闸门前移进了公式，后果是超距实体不再得到「越过 0 之后
重新上升」的伤害：

| 输入 | 原版 | 若按钳制 |
|---|---|---|
| radius 1、distance 16、seen 1 | `ratio=8`、**damage=295** | `ratio=1` → `d1=0` → **damage=1** |

相差 295 倍。这只是公式层的复刻差异——是否真的打到这个伤害由调用方的 `inRange`
决定，而本能力把 `inRange` 单独输出，正是为了让「公式」与「闸门」的分工可验证。

#### 差异二：返回值被取整

原版是 `(float)` 直接转换，保留小数；Cubium 用了 `std::floor`。实测输出里
`damage="11.0898"`，取整后会是 `11`——几乎所有输入下两者都不同。

#### 单元设计

480 个组合刻意跨越 `distance == radius * 2` 这条边界（`inRange` 的分界），
并包含多个超距值。只有把超距值放进表里，差异一才会显示出来——如果只测射程内的
实体，两处实现是完全一致的。
### 新能力 explosion_ray：纯计算，一个「取错坐标」的 bug

第 72 个能力，6 个场景 × 8 个种子 = 48 个单元，全部一致。它是第一个**完全不依赖
数据包、不依赖世界**的能力：复刻 `ServerExplosion.calculateExplodedPositions()` 的
射线步进，用「以爆炸中心为原点的同心抗性壳」代替真实世界。

#### 把 world 型问题化成纯计算

原版这一步只依赖三样东西：爆炸中心、半径、每个方块位置的抗性。只要把「每个位置的
抗性」换成一张同心壳表，世界依赖就消失了。代价是不覆盖「方块抗性表本身是否与原版
一致」，但那部分由 `block_properties` 等能力从别的角度覆盖。

#### 一路踩到的四个坑

调试过程是有价值的，四处错误**每一处都让差异变小一点**：

| 顺序 | 错误 | 症状 |
|---|---|---|
| 1 | 射线位置用 `f32` 累积 | 探针系统性偏大（703 vs 678） |
| 2 | 抗性按**射线浮点位置**取，而不是方块坐标 | 边界方块被归到相邻层（162 vs 152） |
| 3 | 空气位置（抗性 0）被计入破坏集合 | 有空气层的场景偏大（206 vs 205） |
| 4 | `sqrt` 前用了 f32 平方和 | 与 Java 侧 `(double) x * x` 精度不一致 |

其中**第 2 条最关键**：原版查的是 `level.getBlockState(blockpos)`，`blockpos` 已经
`floor` 过。我用射线的浮点位置算距离，于是射线在 `(-2.9, …)` 时 `floor` 得方块 `-3`，
但距离按 2.9 判定，把这个方块归到了错误的抗性层。

`tnt_air` 场景（全空气，无抗性）**从一开始就完全一致**（1097 = 1097），正是这条
线索把问题范围从「射线步进」缩小到了「抗性取址」。

#### 三个容易写错的常量

- `0.22500001F` 不是 `0.225F`；
- `0.3F` 同时用于射线步长与抗性公式 `(res + 0.3) * 0.3`；
- 初始强度 `radius * (0.7 + rand * 0.6)`，每条射线各消耗一次 `nextFloat()`
  （1352 条射线 = 16³ − 14³，只取 16×16×16 网格的外壳）。
### 新能力 dimension_type：旧字段模型 vs attributes 映射

第 71 个能力，4 个维度类型全部一致。它有别于前三个能力的地方在于：Cubium **有**
`DimensionType` 类，但用的是**旧版字段模型**。

| 项目 | 原版 1.21.11 | Cubium |
|---|---|---|
| 游戏规则 | `attributes` 映射（`gameplay/bed_rule`、`gameplay/piglins_zombify`…） | 独立布尔字段（`ultraWarm` / `natural` / `bedWorks` / `piglinSafe`…） |
| `coordinate_scale` | `double` | `f32` |
| 数据包加载 | `data/<ns>/dimension_type/*.json` | **无加载器**，只有硬编码实现 |

全仓库搜不到任何读取 `dimension_type` 数据包的代码。因此本探针同样直接解析 JSON，
并把 `attributeCount` 作为可比量——这一项直接反映新旧模型的差距：主世界 9 个、
末地 8 个、下界 13 个，而旧模型的独立字段各维度都是固定那几个。

#### 三个踩到的格式坑

1. **`infiniburn` 的 `#` 前缀**。JSON 写 `"#minecraft:infiniburn_overworld"`，但原版
   `DimensionType.infiniburn()` 返回 `TagKey<Block>`，`location()` **不带** `#`。
   直接比原始字符串会让 4 个维度全部差一个字符。

2. **`monster_spawn_light_level` 的多态**。原版是 `IntProvider.codec(0, 15)`，既能写
   整数也能写带 `type` 的对象。原先打算「原样 dump JSON」，但 nlohmann 与 Gson 的键
   顺序不同会误报，改成固定格式 `类型[下界,上界]` 之后才稳定。

3. **`timelines` 是 `Either<TagKey, List<Holder>>`**。`unwrap()` 返回的 `Either` 没有
   `size()`，要分左右支取数。

#### 顺带验证的东西

原版 `createDirectCodec` 有三条校验（`height % 16 == 0`、`min_y % 16 == 0`、
`logical_height <= height`）。本能力把三条校验的结果也输出成字段，让「数据是否合法」
本身成为可比量——4 个维度全部通过，说明数据包与两侧的解析口径都自洽。

TODO 与前几个能力一致：待 Cubium 加入数据包加载器后，探针应改为读注册表。
### 新能力 trial_spawner_config：一次跑出两处真实差异

第 70 个能力，28 项 × 9 种玩家数 = 252 个单元，全部一致。它一上来就抓到了两处问题。

#### 差异一：四个数值字段被声明成整数

Cubium 的 `TrialSpawnerBlockEntity::Config`：

```cpp
i32 totalMobs = 6;
i32 totalMobsAddedPerPlayer = 2;
i32 baseSimultaneousMobs = 3;
i32 simultaneousMobsAddedPerPlayer = 1;   // <- 数据包里全是 0.5
```

而原版是 `f32`。数据包 28 项里 `simultaneous_mobs_added_per_player` **无一例外都是
`0.5`**，整数字段根本装不下。

这个偏差会直接改变刷怪数量，因为原版的派生方法是：

```java
calculateTargetSimultaneousMobs(n) = (int) Math.floor(simultaneousMobs + simultaneousMobsAddedPerPlayer * n)
```

乘数 0.5、玩家数 3 时：`1 + 0.5 * 3 = 2.5` → floor 得 **2**。若按整数加法先算成
`1 + 0 = 1`，就少一只。能力输出的实测值：

```json
{"config":"minecraft:trial_chamber/breeze/normal","playerCount":3,
 "simultaneousMobs":"1","simultaneousMobsAddedPerPlayer":"0.5","targetSimultaneousMobs":2}
```

本能力因此在 9 种玩家数下各算一遍，专门压这条边界。

#### 差异二：缺省字段的默认值不是空

探针第一次跑就报了：

```
oracle: {"lootTableCount":2}
probe : {"lootTableCount":0}
```

原版 `loot_tables_to_eject` 的缺省值来自 `DEFAULT.lootTablesToEject`，而那是个**含 2 项的
WeightedList**（`spawner_trial_chamber_consumables` 与 `spawner_trial_chamber_key`），
不是空列表。14 个配置没写这个字段，全都要回退到这两项。

这条比「字段缺失就按空处理」更容易错——空列表看起来是最自然的中性默认值，但原版不是。

#### 一条被验证的假设

`spawn_range` 在全部 28 项里都没有出现，必须回退到 `Builder` 的默认值 4。这条假设成立，
252 行里 `spawnRange` 全部为 4。

TODO 与 `banner_pattern` / `enchantment_provider` 一致：待 Cubium 改为数据包驱动后，
探针应改为读注册表。
### 新能力 enchantment_provider：把「类型分派 + 代价截断」单独钉住

第 69 个能力。数据包 `enchantment_provider/` 下 7 项，Cubium **一项都没有加载**
（全仓库搜不到任何实现）。原版有三种类型：

```java
single                  → record SingleEnchantment(Holder<Enchantment>, IntProvider level)
by_cost                 → record EnchantmentsByCost(HolderSet<Enchantment>, IntProvider cost)
by_cost_with_difficulty → record EnchantmentsByCostWithDifficulty(HolderSet, int minCost, int maxCostSpan)
```

#### 范围界定：只比到「代价区间」为止

原版 `enchant()` 的后半段是 `EnchantmentHelper.selectEnchantment(...)`，那属于附魔选择
算法，已由 `enchantment_applicability` 与 `enchantment_cost` 从别的角度覆盖。本能力聚焦
提供器**自身**那一层：JSON 的 `type` 分派到哪个实现类，以及代价区间的算法。

后者值得单独钉住，因为原版是：

```java
int i = Mth.randomBetweenInclusive(rng, minCost, minCost + (int)(f * maxCostSpan));
```

`(int)` 是**截断而非四舍五入**，且下界不随难度变化。乘数取 0.25 时
`5 + (int)(0.25 * 17) = 9`，取 0.5 时是 `13`——写错成四舍五入会得到 9 和 14，
差异一眼可见。

#### 单元数不等于 `list` 行数

本能力用「提供器 × 难度乘数」的笛卡尔积，7 个提供器 × 7 个乘数 = 49 个单元。
`computed` 单元拿不到 `list` 的输出，所以单元生成函数直接扫数据包目录；
它用的是 oracle 侧的 `vanillaDataPackDir()` 而不是直接读环境变量——**`computed`
单元在 runner 注入 `MC_DIFF_VANILLA_DATAPACK` 之前就会求值**，这里踩过一次。

注意报告里的 `units: 7` 是按比较键（`provider`）去重后的数，49 行输入实际逐行比过。

#### 结果

49 行全部一致。TODO 与 `banner_pattern` 相同：待 Cubium 实现该注册表后，
探针应改为读注册表而不是直接解析 JSON。
### 新能力 banner_pattern：绕开注册表直接读数据包

`banner_pattern` 是第 68 个能力，43 项全部一致。它有一个特殊的处理方式值得记录。

原版 1.21.11 已把旗帜图案改成数据包驱动的注册表：

```java
public record BannerPattern(Identifier assetId, String translationKey)
// Registries.BANNER_PATTERN，数据包 data/<ns>/banner_pattern/*.json
```

Cubium 侧却仍是写死的枚举：

```cpp
enum class BannerPatternType : u8 {
    Base = 0, SquareBottomLeft = 1, ...   // 43 个固定值
};
```

如果探针去读那个枚举，比对就退化成「枚举与枚举自己比」——数据包里改一个 `asset_id`，
Cubium 不跟随，这个能力也发现不了。所以这里**绕开 Cubium 的注册表，直接解析数据包
JSON**，与原版 oracle 走同一份数据源。

代价是：本能力当前只能验证「数据包内容与解析口径」，无法覆盖「注册表装载顺序」
「运行时覆盖」这类偏差。已在探针头部记 TODO：待 Cubium 迁移到数据包驱动注册表后，
探针应改回读注册表。

这也是一次有意为之的取舍：**先让能力跑起来并锁住数据基线**，而不是等 Cubium 完成
架构迁移。43 项全部一致意味着至少确认了「数据包里的 43 个图案能被正确解析并与原版
一一对应」，这是一个真实的、可回归的保证。
### 要塞：合并随机源后仍未命中，转向新增能力

上一轮记录的怀疑点是「`generateRingPositions` 内部的 rng 与外层 fork 用的 rng 不是同一个」。
原版确实在同一条流里边算起点边 fork：

```java
RandomSource randomsource = RandomSource.create();
randomsource.setSeed(concentricRingsSeed);
double d0 = randomsource.nextDouble() * PI * 2.0;
for (...) {
    double d1 = ... randomsource.nextDouble() ...;    // 半径扰动
    RandomSource randomsource1 = randomsource.fork(); // <- 同一个流上 fork
    d0 += (PI * 2) / k;
    if (换环) d0 += randomsource.nextDouble() * PI * 2.0;
}
```

已按此改造：`generateRingLayout` 在同一条随机流上取 fork 种子并随起点一起带出，
`StructurePlacementState` 直接用它构造搜索随机源。

**结果：仍是 47 处差异，要塞依然一个都不生成。**

到此为止，要塞问题已经排除了五个方向：

| 方向 | 结论 |
|---|---|
| 环容量递推 / 半径公式 | 正确（序列 3,6,10,15,21 正确，半径 107~1507 与原版量级一致） |
| `preferredBiomes` | 已修复（37 个，ID 空间一致且能命中） |
| 群系采样 `getNoiseBiome` | 已验证返回正确群系 |
| 扫描参数（方向 / 步长 / 边缘） | 已按 8 参数重载修正 |
| fork 随机源是否同一流 | 已合并，无变化 |

`structure_starts` 的要塞部分到此**暂停深挖**，改为记录状态、转向新增能力。理由：连续三轮
都在这一个能力上深挖，每轮都修掉真实缺陷，但差异数没有动过。持续新增能力才是这个任务的
本意，继续单点深挖的投入产出比已经不划算。

要塞仍然是一个明确的缺口，留给后续单独处理。
### 要塞搜索：三个参数猜错，逐一被源码否证

上一轮记下的 TODO 是「`k1` 的递增步长应为 `spread` 而非 1」。本轮逐参数对照原版
`BiomeSource.findBiomeHorizontal`，三处猜测逐一验证：

| 猜测 | 实际 | 结论 |
|---|---|---|
| 步长应为 `spread`（3） | 结构生成走**8 参数重载**，内部固定 `p_220565_ = 1` | **猜错**，改回 1 |
| 应从最大半径向内递减 | `j1 = findClosest ? 0 : k`，`findClosest = false` 时确实从 `k` 开始 | 已按此修正 |
| 只扫环边缘即可 | `if (findClosest) { ...只扫边缘... }`，`false` 时**扫满整个方形** | 已按此修正 |

三处修完后半径分布正确（107 ~ 1507 区块，与原版量级一致），坐标也从「几乎等于搜索
起点」变成有明显偏移。但 `(-524,-678)` 这个具体位置仍未命中——最近的候选是
`(-530,-410)` 与 `(-360,-655)`，同一象限、距离数百区块。

已确认不是以下原因：

- 环容量递推（按 `k += 2*k/(i1+1)` 复刻，序列 3,6,10,15,21,... 正确）
- 半径公式（`4*distance + distance*ringIndex*6 + ...`，最大半径 1507 与 128 个要塞匹配）
- `preferredBiomes`（37 个，已验证群系 ID 空间一致且能命中）
- 群系采样（`getNoiseBiome` 返回正确的群系）

剩下最可能的是**蓄水池抽样的随机数消耗时机**：原版对每个要塞 `fork()` 一次，
而 fork 的种子来自「此前所有起点已经消耗过随机数」的那个 `randomsource`。
我的复刻里 `generateRingPositions` 内部自己持有一个 rng（用于半径与角度扰动），
外面又新建了一个用于 fork，**两者不是同一个随机源**——这是一个明确的、可验证的怀疑点。

TODO: 把半径/角度扰动的 rng 与 fork 用的 rng 合并成同一个实例，让 fork 序列反映
起点生成过程中已经发生的消耗。

**教训（本轮第二次）**：上一轮我依据「原版有两个 `nextDouble` 调用」推断步长应为
spread，并写进了 TODO。实际那个参数由重载固定为 1，`spread` 在结构生成路径上根本不
参与。**读源码时推断出的结论，必须回到具体调用点确认参数值**，不能只看到形参名就下判断。
### 要塞：生物群系标签没加载 + 搜索太慢，两处基础设施缺陷

接着上一轮的要塞问题往下查，又发现两处**与算法无关**的基础设施缺陷。

#### 一、探针从未加载生物群系标签

`preferred_biomes` 引用 `#minecraft:stronghold_biased_to`（37 个群系），由
`StructureSetLoader` 去 `BiomeTags` 里查。探针的初始化序列里**没有**
`BiomeTagLoader::loadFromDataPackRepository`，于是：

```
[ss] strongholds preferredBiomes = 0    // 应为 37
```

`preferredBiomes` 恒为空 → `findBiomeHorizontal` 直接返回空 → 要塞永远退回原始环坐标。
这与上一轮修的环算法无关，是另一条独立的失效路径。

补上标签加载后 `preferredBiomes = 37`，并验证了后续链路可用：

```
[ss] 采样 quart(-33,0,-20) 群系 id = 4
[ss] 该群系是否在 preferred 中: 1        // 命中
[ss] preferred 前 5 个 id: 1 129 12 140 4
```

群系 ID 空间在两侧一致，`getNoiseBiome` 采样正常。

#### 二、128 次螺旋扫描被每个区块重跑一次

补上搜索后整个能力**跑不完**（1800 秒超时）。实测单个单元 2.3 秒——每个单元都要
跑 128 个要塞 × 29×29 个 quart 的螺旋扫描。

根因是缓存位置：原版把 `List<ChunkPos>` 缓存在 `ChunkGeneratorStructureState` 里，
**与放置规则同生命周期**；而 `StructurePlacementState` 是每个区块（探针里是每个单元）
新建的，只靠它的实例缓存等于没缓存。

改法：把改写后的最终坐标表回填到 `ConcentricRingsStructurePlacement` 上（按世界种子
缓存）。3 个单元从 6.9 秒降到 2.58 秒，542 个单元回到 170 秒量级。

#### 仍然存在的差距

要塞依然一个都不生成。已排除的原因：

- 环算法（上一轮已按原版修正三处）
- `preferredBiomes` 为空（本轮已修，37 个）
- 群系采样失效（已验证返回正确群系且命中偏好集）
- 搜索性能（已用缓存解决）

剩下的可能是**搜索结果的坐标与原版差一个数量级**：542 个候选单元来自原版 oracle
（原版能找到要塞的区块），如果我们的 `findBiomeHorizontal` 复刻有细节偏差（例如扫描
顺序、蓄水池抽样的调用时机），算出的坐标就会系统性偏离，从而全部错过这批候选。

TODO: 逐参数对照原版 `findBiomeHorizontal` 的扫描顺序——特别是 `k1` 的递增步长
（原版是 `k1 += spread`，而非 1）与内层 `l1`/`i2` 的起始值。
### 要塞环算法：三处偏差已修，生物群系搜索仍缺失

`structure_starts` 恢复运行后，47 处差异里最大的一块是**要塞**：9 个单元在原版有、
Cubium 完全没有。追下去发现 `ConcentricRingsStructurePlacement` 的环位置生成与原版
（`ChunkGeneratorStructureState.generateRingPositions`）有三处实质偏差：

| 项目 | 原版 | 修复前 | 状态 |
|---|---|---|---|
| 半径公式 | `4*distance + distance*ringIndex*6 + ...` | `distance + distance*ringIndex*6 + ...`（少了开头的 4 倍） | 已修 |
| 环容量递推 | `k += 2*k/(ringIndex+1)`，动态扩张 | 硬编码表 `3,6,10,15,21,28,36,9` | 已修 |
| 环间角度扰动 | 每换环再 `angle += random*2π` | 无 | 已修 |
| 生物群系搜索 | `findBiomeHorizontal(..., 112, ...)` 把要塞挪到 `#stronghold_biased_to` | 无 | **未修** |

三处都修完之后要塞仍然一个都不生成，说明**最后一项才是决定性的**：原版算出的环坐标
只是搜索起点，真正的要塞位置由半径 112 区块的生物群系搜索决定。

#### 一次被数据否定的猜想

读原版源码时发现 `createForNormal` 传入的 `concentricRingsSeed` 是**常量 0L** 而不是
世界种子，我据此判断「要塞分布与种子无关」，改成了常量并重新测——差异数纹丝不动。

然后我用 oracle 的输出证伪了这个猜想：

```
种子 0              : (-524,-678) (-573,1114) (1113,567) (612,910)
种子 9248738493917  : (-295,91) (-705,79) (1005,747) (174,-1053) (763,-1043) ...
```

两个种子的要塞位置**完全不重合**。如果种子恒为 0 且没有生物群系搜索，两者必须落在
同一批坐标上。所以常量 0 只用于生成「搜索起点」，最终坐标由生物群系搜索（其 fork
随机源来自世界种子）改写。改动已回退。

**教训**：只读源码得出的结论必须用数据验证。这次如果我停在「已按源码改成常量」，
就会写下一段看起来有依据、实际上使行为变差的改动。

TODO: 补齐生物群系搜索需要把 `IBiomeSource` 传进 `StructurePlacementState::ringPositions`，
算出每个环坐标后复刻 `findBiomeHorizontal(半径 112, preferredBiomes, fork 随机源)` 并改写
坐标。这是要塞对齐的最后一块拼图。

#### 其余差异

47 处差异的构成：要塞 9 处（完全缺失）、矿井 13 处（片段数普遍只有原版的 1/3~1/10）、
试炼密室 5 处、前哨站 / 海底废墟等各若干处。矿井片段数偏少是下一个独立问题。
### 一次静默失效：CMake 三张列表错位让 6 个能力链接到了别的探针

`structure_starts` 长期报 `key 'origin' not found`，看起来像是探针自身的缺陷。实际根因
在 `tests/diff/CMakeLists.txt`：

三张列表按**相同索引**配对，构建时用 `list(GET ...)` 取同一位置的 suffix 与源文件：

```cmake
list(GET MC_DIFF_PROBE_SUFFIXES ${index} targetSuffix)
```

历史上有一次提交在追加新能力时把 `structure_density` / `structure_starts` /
`villager_trades` 三项插在了 `walk_path_type` **前面**（SUFFIXES 里在**后面**），
于是从第 37 项起两者错开 3 位：

| 索引 | SUFFIXES（命名） | SOURCES（实际编译） |
|---|---|---|
| 37 | `walk_path_type` | `structure_density` |
| 38 | `path_finder` | `structure_starts` |
| 39 | `fly_path_finder` | `villager_trades` |
| 40 | `structure_density` | `walk_path_type` |
| 41 | `structure_starts` | `path_finder` |
| 42 | `villager_trades` | `fly_path_finder` |

后果是这 6 个可执行文件**各自链接了别人的探针源文件**。错位不会产生任何编译错误或
警告，只表现为「某个能力报了一个与它无关的错误」。`structure_starts` 之所以报
`origin`，是因为它当时跑的是别的探针的代码。

对齐后 6 项全部恢复正常：`structure_starts` 从「完全无法运行」变成 542 个单元、47 处差异；
`walk_path_type` / `path_finder` / `fly_path_finder` 各 282/282 全部一致。

**教训**：这组列表的正确性完全依赖三张列表的**顺序**，靠肉眼维护。新增能力时若只往
SUFFIXES 和 DOMAINS 尾部追加、却把源文件插到别处，就会静默错位。
`MC_DIFF_PROBE_COUNT` 只校验了长度相同，没有校验顺序对应关系。
TODO: 在 CMake 里加一条配置期断言，逐索引比对 `MC_DIFF_PROBE_SUFFIXES[i]` 与
`MC_DIFF_PROBE_SOURCES[i]` 的文件名主干是否一致，把这类错位在配置阶段就拦下来。
### 乐器：浮点字段与两个派生值，8/8 全一致

`instrument` 比对 8 种山羊角音色：音效 ID、吹奏时长、可听距离、描述翻译键，外加两个
**派生值**——`useDurationTicks` 与 `volume`。一次通过 **8/8**。

原版 `record Instrument(Holder<SoundEvent> soundEvent, float useDuration, float range,
Component description)` 只有四个字段，派生值不在记录里，但在 `InstrumentItem` 里各占
一行换算：

```java
getUseDuration -> Mth.floor(useDuration * 20.0F)
play           -> range / 16.0F
```

Cubium 侧同样实现了这两个派生（`useDurationTicks()` / `volume()`），把它们也纳入比对，
是因为**换算口径本身会错**：秒与 tick 的换算、范围与音量的比例，这两处一旦写错，
只比原始字段是发现不了的。

#### 浮点输出口径

三个浮点字段（`useDuration` / `range` / `volume`）在两侧都按「6 位有效数字，去掉尾随零」
格式化成**字符串**输出。这是 `food_properties` 与 `jukebox_song` 两个能力踩过同一个坑之后
定下的规矩：C++ 的 `%.6g` 与 Java 的 `%g` 行为不同——`178.0f` 一边给 `178`，另一边给
`178.000`。若直接按数字输出，Gson 与 nlohmann 的打印差异还会再叠一层
（`0.3` vs `0.30000001192092896`）。

#### 一个初始化顺序的坑

探针首轮 8 个单元全部返回 `missing`，注册表为空。原因是服务端 `RegistryBootstrap` 里
这个注册表有三步：

```
clear()                     -> 清掉可能残留的旧条目
initialize()                -> 登记写死的内置条目（8 种山羊角）
loadFromDataPackRepository  -> 用数据包覆盖同名条目
```

探针最初只做了第三步。数据包里的条目其实能注册成功，但 `get()` 走的是另一条路径，
表现为「找不到」。补齐前三步的顺序后立刻 8/8。

这提示了一个更一般的规律：**差分能力复刻的是原版的运行时状态，不是原版的输入数据**。
凡是注册表，都要照抄服务端的初始化顺序，而不是只加载数据包。### 动物变体：六类变体的出生条件，32/32 一次收敛

`animal_variant` 比对狼 / 猫 / 蛙 / 牛 / 鸡 / 猪六类变体，共 32 个单元。首轮 32 处全差，
排查后收敛到 **32/32 全一致**。

六类变体此前在 Cubium 中都没有实现，变体的贴图与出生条件全都无处可查。

#### 数据形状

六类的形状高度一致，因此共用一套加载器（`AnimalVariants`），差异由 `AssetField` 承载：

| 类别 | 贴图字段 | `model` 字段 | 数量 |
|---|---|---|---|
| `wolf_variant` | `assets: { wild, tame, angry }`（三张） | 无 | 9 |
| `cat_variant` / `frog_variant` | `asset_id`（一张） | 无 | 11 / 3 |
| `cow_variant` / `chicken_variant` / `pig_variant` | `asset_id`（一张） | 有，缺省 `normal` | 各 3 |

出生条件是 `spawn_conditions: [{ condition, priority }]`，条件只有三种：

- `biome` → `BiomeCheck(HolderSet<Biome>)`，字段 `biomes`
- `structure` → `StructureCheck(HolderSet<Structure>)`，字段 `structures`
- `moon_brightness` → `MoonBrightnessCheck(MinMaxBounds.Doubles)`，字段 `range: { min, max }`

#### 首轮全差的三个根因

| 现象 | 根因 |
|---|---|
| `entries` 全为空、`rangeMin` 全丢失 | 数据文件里 `type` 写作 `minecraft:structure`，而原版注册名是 `structure`。字符串比较不相等，整个条件解析分支被跳过 |
| 贴图 ID 两侧对不上 | 原版 `ClientAsset.ResourceTexture` 的构造器把 `asset_id` 变换成 `textures/` + path + `.png`。数据文件写 `entity/cat/all_black`，原版暴露 `textures/entity/cat/all_black.png` |
| `model` 为空 vs `normal` | 原版是 `optionalFieldOf("model", ModelType.NORMAL)`，缺省取 `normal`。但只有牛 / 鸡 / 猪三类有 model 概念，狼 / 猫 / 蛙必须保持空串 |

第一条尤其典型：`type` 带不带命名空间前缀是**数据文件写法**与**注册表 key** 的差异，
单看任何一侧都发现不了，只有两侧对着比才会暴露。

#### 两处刻意的口径

- **`entries` 保留 `#` 前缀**：原版 `HolderSet` 的标签引用与内联元素语义不等价
  （标签会随数据包内容变化），比对必须能区分。
- **狼的 `assets` 按 `AssetInfo(wild, tame, angry)` 的字段顺序输出**，而不是按 JSON 对象的
  遍历顺序；`selectors` 也保持数组原顺序——优先级链的书写顺序有语义。
### 盔甲纹饰：两类数据包注册表，一次通过

`armor_trim` 一次性比对 18 个纹饰图案与 11 种纹饰材料，**29/29 全一致**。

这个能力顺带补上了 Cubium 的一处空白：此前**完全没有** `trim_pattern` /
`trim_material` 这两层注册表——`ArmorTrim` 只以物品组件和数据组件类型枚举的形式存在，
图案的贴图 ID、描述、是否贴花，材料的贴图后缀、描述颜色、覆盖装备材质表全都无处可查。
本次新增 `TrimPattern`/`TrimPatterns`、`TrimMaterial`/`TrimMaterials` 两对类，
按 `PaintingVariants` 的既有模式从数据包加载。

比对项与原版结构的对应关系：

| 单元 | 比什么 | 对应原版结构 |
|---|---|---|
| `pattern/<id>` | 贴图 ID / 描述翻译键 / 是否贴花 | `record TrimPattern(Identifier assetId, Component description, boolean decal)` |
| `material/<id>` | 贴图后缀 / 描述翻译键 / 描述颜色 / 覆盖装备材质表 | `record TrimMaterial(MaterialAssetGroup assets, Component description)` |

两处沿用了此前踩过的坑的结论：

- **描述比翻译键而非渲染文本**（`TranslatableContents.getKey()`），这是唱片机歌曲能力里
  踩过 `getString()` 返回已渲染文本的坑之后定下的口径。
- **`overrides` 是无序表**（原版是 `Map<ResourceKey<EquipmentAsset>, AssetInfo>`），
  两侧都按装备材质键排序后输出，并额外用 `unorderedArrays` 声明一次，让比较器不依赖顺序。

`asset_name` + `override_armor_assets` 对应原版 `MaterialAssetGroup` 的
`base`（`AssetInfo`）与 `overrides`；`override_armor_assets` 在数据文件里是可缺省字段，
缺省即空表，与 `optionalFieldOf(..., Map.of())` 一致。
### 附魔适用性：枚举归纳 vs 标签判定（28 → 0）

`enchantment_applicability` 对每个附魔逐个物品问「能不能附上去」，比的是**最终物品集合**。
首次运行暴露出 **28 个附魔与原版不符**，两轮迭代后收敛到 **43/43 全绿**。

根因在表示方式：原版直接读数据包的 `supported_items` 物品标签引用（如
`#minecraft:enchantable/sharp_weapon`）；Cubium 的附魔定义是硬编码的，没有这个字段，
改用 `EnchantmentType` 枚举**归纳**原版标签，再由 `canApplyToItem` 按物品类型判定。
枚举归纳得粗糙，差异就藏在这一层。

#### 最隐蔽的一类错误：继承链假设错了

```cpp
const auto* toolItem = dynamic_cast<const item::tool::ToolItem*>(item);
return toolItem != nullptr && toolItem->getToolType() == item::tool::ToolType::Sword;
```

这段代码**看上去完全正确**，注释也写着「剑」。但：

| 物品类 | 实际继承 | 后果 |
|---|---|---|
| `SwordItem` | `TieredItem`（**不继承 `ToolItem`**） | `getToolType() == Sword` 永远不成立，**锋利附不到任何剑上** |
| `MaceItem` | `Item`（不继承 `ToolItem`） | 重锤附不了锋利、亡灵杀手、节肢杀手 |
| `ShearsItem` | `Item`（不继承 `ToolItem`） | 效率附不到剪刀上 |
| `SpearItem` | `tool::TieredItem`（独立类） | 矛被整体漏掉 |

同一模式的错误出现了**四次**。前三次是逐个差分发现的，第四次是在修完前三个之后才想到
「可能还有别的类也这样」。没有差分，这类问题几乎不可能靠读代码发现——代码没有语法错误，
逻辑看起来也自洽，只是前提假设错了。

#### 其余修复

| 问题 | 表现 | 修法 |
|---|---|---|
| `Breakable`/`Vanishable` 直接 `return true` | 经验修补、耐久能附到泥土、空气、刷怪蛋（各 1124 个物品） | 改用 `isDamageable()`，补上指南针、雕刻南瓜、头颅 |
| 四个护甲部位分支全都 `return item->isArmor()` | 水下速掘/呼吸能附在靴上，深海漫游/冰霜行者能附在头盔上 | 按 `isHelmet`/`isChestplate`/`isLeggings`/`isBoots` 分部位 |
| `isArmor()` 把动物铠甲算进护甲 | `wolf_armor` 被误判可附保护、耐久、经验修补、消失诅咒 | 护甲只认玩家四部件；`durability` 类排除动物铠甲 |
| 六个武器附魔共用一个 `Weapon` 类型 | 斧/重锤/矛各自错配 | 按原版标签拆成 `Weapon`/`SharpWeapon`/`MeleeWeapon`/`FireAspectWeapon`/`SweepingWeapon`/`LungeWeapon` |
| 时运、精准采集与效率共用 `Digger` | 剪刀被误判可附时运 | 新增 `DiggerLoot`（`#enchantable/mining_loot`，不含剪刀） |

#### 一处连带收益

修完这些之后，卡了好几轮的存量失败：

```
ContainerRequestTest.EnchantmentButtonRejectsInsufficientLevelAndCreativeDoesNotConsumeLapis
```

**自己消失了**。附魔台按钮能否点击，正是由「这个物品能不能附这个魔」推导出来的，
枚举判定错误一直在污染下游行为。我前几轮把它当成「无关存量问题」忽略，那个判断是错的。
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
