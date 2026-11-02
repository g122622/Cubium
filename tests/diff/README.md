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
│   ├── crop_growth.cpp          # 作物一次随机刻（生长速率 / 抽芽 / 结果 / 耕地保墒）
│   ├── redstone_signal.cpp      # 红石信号（六方向 × 强 / 弱 × 关掉自我屏蔽）
│   ├── light_occlusion.cpp      # 方块状态的光照阻挡属性（lightBlock 等四项）
│   ├── block_movement.cpp       # 方块对移动的作用（速度/跳跃因子）与随机刻/红石开关
│   ├── can_harvest.cpp          # 每种手持工具能否获得方块掉落（requiresCorrectToolForDrops）
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
│   │   ├── crop_growth.ts       # 作物随机刻的比对口径（速率 / 结果 / 邻居 / 脚下）
│   │   ├── redstone_signal.ts   # 红石信号的比对口径（方向表统一键序）
│   │   ├── light_occlusion.ts   # 光照阻挡的比对口径（状态表按键名重排）
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
| `block_sound_type` | 原版 jar `BlockSoundTypeOracle` | 1167（全部方块 + 饰纹陶罐裂纹状态） | 全部一致 | 见下文 |
| `crop_growth` | 原版 jar `CropGrowthOracle` | 26 个手工地形场景 | 全部一致 | 见下文 |
| `redstone_signal` | 原版 jar `RedstoneSignalOracle` | 41 个手工地形场景 | 全部一致 | 见下文 |
| `light_occlusion` | 原版 jar `LightOcclusionOracle` | 1166 个方块 | 全部一致 | 见下文 |
| `block_movement` | 原版 jar `BlockMovementOracle` | 1166 个方块 | 全部一致 | 见下文 |
| `can_harvest` | 原版 jar `CanHarvestOracle` | 1166 个方块 × 32 种手持工具 | 全部一致 | 见下文 |
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
| `enchantment_provider` | 原版 jar `EnchantmentProviderOracle` | 7 项 × 7 种难度乘数 = 49 行输入（按 `provider` 折叠成 7 个比较单元） | 全部一致 | Cubium 未实现，探针直接读数据包（见下文） |
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
| `blockpos_codec` | 原版 jar `BlockPosCodecOracle` | 21 个打包/偏移用例 | 全部一致 | 纯计算，超范围坐标静默翻转 |
| `axis_plane` | 原版 jar `AxisPlaneOracle` | 30 个轴/平面判定 | 全部一致 | 纯计算，+Z 指向南 |
| `mth_lerp` | 原版 jar `MthLerpOracle` | 42 个插值用例 | 全部一致 | 纯计算，含 `lerpInt` 的 floor 与 `lerpDiscrete` 的 +1 |
| `mth_bit` | 原版 jar `MthBitOracle` | 45 个位运算用例 | 全部一致 | 纯计算，非正输入返回 0 |
| `mth_rot` | 原版 jar `MthRotOracle` | 33 个旋转插值用例 | 全部一致 | 纯计算，rotLerp 不归一化、triangleWave 负输入越界 |
| `mth_sin` | 原版 jar `MthSinOracle` | 33 个正弦/余弦用例 | 全部一致 | 纯计算，查表量化 + `sin(π)≠0` |
| `mth_approach` | 原版 jar `MthApproachOracle` | 33 个逼近/钳制用例 | 全部一致 | 纯计算，负步长被取绝对值、`clamp` 不校验 min≤max |
| `region_sector` | 原版 jar `RegionSectorOracle` | 27 个索引/寻址用例 | 全部一致 | 纯计算，负坐标 `>>5` 与 `/32` 不同 |
| `blockpos_dist` | 原版 jar `BlockPosDistOracle` | 26 个距离/迭代用例 | 全部一致 | 纯计算，迭代 X 最内层、`float` 距离 |
| `enchantment_storage` | 原版 jar `EnchantmentStorageOracle` | 21 个操作序列 | 全部一致 | 纯计算，`set(0)` 删除 / `upgrade(0)` 不动 |
| `collide_sweep` | 原版 jar `CollideSweepOracle` | 23 个碰撞扫描用例 | 全部一致 | 纯计算，Y 轴优先、主轴最后推进 |
| `shapes_boolean` | 原版 jar `ShapesBooleanOracle` | 18 个布尔组合用例 | 全部一致 | 纯计算，16 种 `BooleanOp` 只有 8 种可用 |
| `fluid_spread` | 原版 jar `FluidSpreadOracle` | 30 个流体用例 | 全部一致 | 纯计算，下落覆盖等级为 7、岩浆分维度 |
| `fluid_spread_dir` | 原版 jar `FluidSpreadDirOracle` | 29 个方向选择用例 | 全部一致 | 纯计算，`best` 无条件赋值导致方向选择不对称 |
| `blockpos_bfs` | 原版 jar `BlockPosBfsOracle` | 14 个遍历用例 | 全部一致 | 纯计算，去重先于状态、`SKIP` 永久 |
| `scheduled_tick` | 原版 jar `ScheduledTickOracle` | 29 个排序/去重用例 | 全部一致 | 纯计算，去重只认 type+pos |
| `random_tick_pos` | **复刻**（`Level` 不可构造） | 16 个采样用例 | 全部一致 | 纯计算，独立 LCG 状态 |
| `mth_color` | 原版 jar `MthColorOracle` | 27 个 HSV/murmur 用例 | 全部一致 | 纯计算，`h=1.0` 得黄色不是红色 |
| `block_transform` | 原版 jar `BlockTransformOracle` | 49 个变换用例 | 全部一致 | 纯计算，Y 轴不受旋转、镜像轴与名字相反 |
| `bounding_box` | 原版 jar `BoundingBoxOracle` | 29 个判定用例 | 全部一致 | 纯计算，闭区间相交、构造器自动归一化 |
| `enchantment_compat` | 原版 jar `EnchantmentCompatOracle` | 155 个附魔配对 | 全部一致 | 纯计算，附魔互斥性（`areCompatible`） |
| `section_pos` | 原版 jar `SectionPosOracle` | 40 个坐标换算用例 | 全部一致 | 纯计算，`>>4` 向下取整、22/20 位编码 |
| `level_height` | 原版 jar `LevelHeightOracle` | 41 个高度范围用例 | 全部一致 | 纯计算，maxY 含上界、分区数按跨区块算 |
| `chunk_pos` | 原版 jar `ChunkPosOracle` | 33 个编码/区域用例 | 全部一致 | 纯计算，32 位无符号拼接、区域 32 区块 |
| `mth_clamp` | 原版 jar `MthClampOracle` | 42 个钳制/映射用例 | 全部一致 | 纯计算，NaN 穿透、`square(int)` 溢出 |
| `chat_formatting` | 原版 jar `ChatFormattingOracle` | 39 个格式码用例 | 全部一致 | 纯计算，strip 字符集有限、`getById` 负数返回 RESET |
| `dye_color` | 原版 jar `DyeColorOracle` | 37 个染色值用例 | 全部一致 | 纯计算，`byId` 越界返回 WHITE、四色独立 |
| `equipment_slot` | 原版 jar `EquipmentSlotOracle` | 47 个槽位用例 | 全部一致 | 纯计算，ARMOR 含 BODY、id 有空洞 |
| `heightmap_types` | 原版 jar `HeightmapTypesOracle` | 33 个高度图判定用例 | 全部一致 | 纯计算，液体/树叶的两类差异 |
| `mth_int_div` | 原版 jar `MthIntDivOracle` | 51 个整数除法用例 | 全部一致 | 纯计算，`floorDiv` 向下取整、`smallest(0)=0` |
| `direction_plane` | 原版 jar `DirectionPlaneOracle` | 24 个方向数据值用例 | 全部一致 | 纯计算，2D/3D id 两套编号、负数回绕 |
| `rotation_info` | 原版 jar `RotationInfoOracle` | 45 个旋转枚举用例 | 全部一致 | 纯计算，`180` 序列化名、`WRAP` 越界 |
| `blockpos_geom` | 原版 jar `BlockPosGeomOracle` | 34 个坐标变换用例 | 全部一致 | 纯计算，`containing` 向负无穷取整 |
| `vec3i_arith` | 原版 jar `Vec3iArithOracle` | 37 个整数向量用例 | 全部一致 | 纯计算，`compareTo` 优先级 Y→Z→X、`distManhattan` 用 float |
| `aabb_contains` | 原版 jar `AabbContainsOracle` | 30 个包围盒用例 | 全部一致 | 纯计算，`contains` 半开区间、`contract` 单侧 |
| `fluid_height` | 原版常量基准 `FluidHeightOracle` | 16 个流体高度用例 | 全部一致 | 纯计算，分母 9 但满值 8 |
| `mth_inverse_lerp` | 原版 jar `MthInverseLerpOracle` | 22 个插值用例 | 全部一致 | 纯计算，不 clamp、`from==to` 产生 NaN、`lerp3` 的 d11 是死参数 |
| `mth_angle_lerp` | 原版 jar `MthAngleLerpOracle` | 34 个角度插值用例 | 全部一致 | 纯计算，`wrapDegrees` 区间、`rotLerp` 短路、`catmullrom` 过冲 |
| `mth_waveform` | 原版 jar `MthWaveformOracle` | 26 个波形用例 | 全部一致 | 纯计算，`triangleWave` 负输入越界、`smoothstep` 不 clamp |
| `mth_hsv` | 原版 jar `MthHsvOracle` | 17 个 HSV 转换用例 | 全部一致 | 纯计算，`hue=1.0` 得黄色、alpha 恒 0 |
| `argb_ops` | 原版 jar `ArgbOpsOracle` | 37 个颜色运算用例 | 全部一致 | 纯计算，`alpha` 用无符号右移、两种 `scaleRGB` 重载差 1 |
| `direction_nearest` | 原版 jar `DirectionNearestOracle` | 29 个方向反推用例 | 全部一致 | 纯计算，全零返回 north、平局由枚举顺序决定 |
| `mth_seed` | 原版 jar `MthSeedOracle` | 22 个坐标种子用例 | 全部一致 | 纯计算，`x` 乘法 int 溢出、`z` 是 long 乘法、`>> 16` 保留符号 |
| `mth_pack_degrees` | 原版 jar `MthPackDegreesOracle` | 29 个角度压缩用例 | 全部一致 | 纯计算，`packDegrees` 不 clamp、`180` 回绕成 `-128` |
| `mth_degrees` | 原版 jar `MthDegreesOracle` | 29 个角度差与平滑用例 | 全部一致 | 纯计算，`rotateIfNecessary` 从 target 反推、`sign(NaN)` 得 -1 |
| `mth_frac` | 原版 jar `MthFracOracle` | 27 个小数部分用例 | 全部一致 | 纯计算，`frac(float)` 的 `floor` 是 int、大输入饱和失效 |
| `mth_sqrt` | 原版 jar `MthSqrtOracle` | 24 个平方根用例 | 全部一致 | 纯计算，`fastInvSqrt` 误差 1e-3 且无特殊值保护 |
| `string_util` | 原版 jar `StringUtilOracle` | 39 个字符串工具用例 | 全部一致 | 纯计算，首个字符串能力；截断条件、`\v` 匹配 `\n`、颜色码范围 |
| `identifier_parse` | 原版 jar `IdentifierParseOracle` | 33 个标识符用例 | 全部一致 | 纯计算，命名空间禁 `/`、空串合法、首个分隔符 |
| `identifier_path` | 原版 jar `IdentifierPathOracle` | 17 个派生用例 | 全部一致 | 纯计算，`toLanguageKey` 保留 `/`、`withPrefix` 会重新校验 |
| `blockpos_iterate` | 原版 jar `BlockPosIterateOracle` | 10 个遍历顺序用例 | 全部一致 | 纯计算，x 最快 z 最慢、两参版本归一化、六参版本 `min>max` 无限迭代 |
| `blockpos_spiral` | 原版 jar `BlockPosSpiralOracle` | 8 个螺旋遍历用例 | 全部一致 | 纯计算，点数 `(2r+1)²`、段长 `leg/2+1` 成对、交换方向镜像 |
| `aabb_factory` | 原版 jar `AabbFactoryOracle` | 25 个 AABB 构造/方法用例 | 全部一致 | 纯计算，构造器归一化、`getSize` 是标量平均、`intersect` 不相交得跨盒 |
| `front_and_top` | 原版 jar `FrontAndTopOracle` | 44 个组合用例 | 全部一致 | 纯计算，`(name, front, top)` 参数序、36 种组合仅 12 合法 |
| `structure_orient` | 原版 jar `StructureOrientOracle` | 14 个结构朝向用例 | 全部一致 | 纯计算，UP/DOWN 走 default、NORTH 反向 Z、WEST/EAST 交换 X/Z |
| `bounding_box_encap` | 原版 jar `BoundingBoxEncapOracle` | 18 个封装族用例 | 全部一致 | 纯计算，构造器有条件归一化、空集合返回 empty、起点是首点折叠盒 |
| `chunk_region` | 原版 jar `ChunkRegionOracle` | 25 个区域换算用例 | 全部一致 | 纯计算，算术右移向下取整、`& 31` 取非负余数；`isValid` 因依赖 `ChunkPyramid` 未覆盖 |
| `portal_relative` | 原版 jar `PortalRelativeOracle` | 12 个传送门相对位置用例 | 全部一致 | 首個跨维度传送领域；宽度不足取 0.5、高度不足取 0.0 |
| `relative_mask` | 原版 jar `RelativeMaskOracle` | 29 个位掩码用例 | 全部一致 | 纯计算，`rotation` 首参是 `Y_ROT`、`Y_ROT` 位是 8（`X_ROT` 是 16）、高位忽略 |
| `position_move_rotation` | 原版 jar `PositionMoveRotationOracle` | 12 个传送目标用例 | 全部一致 | `xRot` 钳到 ±90（`yRot` 不钳）、相对/绝对语义、`ROTATE_DELTA` 旋转速度 |
| `style_apply` | 原版 jar `StyleApplyOracle` | 14 个样式应用用例 | 全部一致 | `applyFormat` 换色保格式、`applyLegacyFormat` 换色清零（`false` 非 `null`）、`applyTo` 是 `this` 优先 |
| `text_color` | 原版 jar `TextColorOracle` | 27 个颜色解析用例 | 全部一致 | 纯计算，`#f00` 是 `0x000F00` 不是 RGB 缩写、`fromRgb` 静默截断 |
| `click_event` | 原版 jar `ClickEventOracle` | 23 个点击事件用例 | 全部一致 | `OPEN_FILE` 是唯一禁用动作、页码从 1 开始 |
| `hover_event` | 原版 jar `HoverEventOracle` | 14 个悬停事件用例 | 全部一致 | 3 个动作全部允许来自服务端、`toString` 是 `<action xxx>` |
| `identifier_resolve` | 原版 jar `IdentifierResolveOracle` | 27 个构造入口用例 | 全部一致 | `withDefaultNamespace` 不解析 `:`、`parse` 按第一个冒号拆分 |
| `plain_text_contents` | 原版 jar `PlainTextContentsOracle` | 18 个纯文本内容用例 | 全部一致 | 空串折叠为 `EMPTY` 单例、`toString` 是裸词 `empty` |
| `mth_absmax` | 原版 jar `MthAbsMaxOracle` | 27 个绝对值与距离用例 | 全部一致 | 纯计算，`absMax(MIN_VALUE)` 溢出、`chessboardDistance` 是 2D |
| `display_slot` | 原版 jar `DisplaySlotOracle` | 31 个显示槽用例 | 全部一致 | 纯计算，团队槽用点分隔、越界 `byId` 返回 `LIST`、`RESET` 不是颜色 |
| `objective_criteria` | 原版 jar `ObjectiveCriteriaOracle` | 28 个计分判据用例 | 全部一致 | 判据名是 camelCase、`EXPERIENCE` 叫 `xp`、只有血量用 `hearts` |
| `team_rule` | 原版 jar `TeamRuleOracle` | 27 个队伍规则用例 | 全部一致 | 序列化名混合全小写与驼峰、两个枚举翻译键前缀不同 |
| `boss_bar` | 原版 jar `BossBarOracle` | 25 个 Boss 栏用例 | 全部一致 | PINK 对应 RED、RED 对应 DARK_RED、overlay 保留下划线 |
| `sound_source` | 原版 jar `SoundSourceOracle` | 23 个声音通道用例 | 全部一致 | RECORDS/BLOCKS/PLAYERS 三个复数枚举映射到单数名 |
| `difficulty` | 原版 jar `DifficultyOracle` | 29 个难度用例 | 全部一致 | `BY_ID` 用 `WRAP` 回绕（`id=5` 区别于 `ZERO`）、`.info` 追加在末尾 |
| `permission_level` | 原版 jar `PermissionLevelOracle` | 26 个权限等级用例 | 全部一致 | `CLAMP` 越界策略（第三种）、`isEqualOrHigherThan` 是 `>=` |
| `text_formatting_color` | 原版 jar `TextFormattingColorOracle` | 44 个格式化颜色用例 | 全部一致 | **探针对接 Cubium 真实实现**；暴露并修复 `getFormattingColor` 哨兵值缺陷 |
| `text_formatting_name` | 原版 jar `TextFormattingNameOracle` | 37 个格式化名字用例 | 全部一致 | **探针对接 Cubium 真实实现**（`toName`/`fromName`）；未发现缺陷 |
| `style_tristate` | 原版 jar `StyleTristateOracle` | 26 个样式三态用例 | 全部一致 | **探针对接 Cubium 真实实现**；暴露并修复颜色序列化为名字的缺陷 |
| `team_rule_strings` | 原版 jar `TeamRuleStringsOracle` | 24 个队伍规则字符串用例 | 全部一致 | **探针对接 Cubium 真实实现**；暴露并修复 `fromString` 静默回退缺陷 |
| `render_type_strings` | 原版 jar `RenderTypeStringsOracle` | 11 个渲染类型用例 | 全部一致 | **探针对接 Cubium 真实实现**；确认 `byName` 双参数重载与 `team_rule_strings` 语义相反 |
| `uuid_strings` | 原版 JDK `UUID.fromString` | 18 个 UUID 用例 | 11 一致，7 处已登记 | Cubium 架构用 32 位无连字符 UUID，刻意接受；非缺陷 |
| `game_type` | 原版 jar `GameTypeOracle` | 27 个游戏模式用例 | 全部一致 | `DEFAULT_MODE` 是别名非枚举项；`isSurvival` 对 `ADVENTURE` 为 `true` |
| `map_decoration_type` | 原版注册表 `MapDecorationTypes` | 43 个地图装饰用例 | 42 一致，1 处已登记 | **探针对接 Cubium 真实实现**；修复 5 个村庄序列化名用错纹理名 |
| `block_state_enum_strings` | 原版 jar `BlockStateEnumStringsOracle` | 34 个方块状态枚举用例 | 全部一致 | `Half`(top/bottom) 与 `DoubleBlockHalf`(upper/lower) 完全不通用 |
| `block_state_enum_strings_2` | 原版 jar `BlockStateEnumStrings2Oracle` | 78 个方块状态枚举用例 | 全部一致 | `PistonType.DEFAULT` 序列化名是 `normal`；`SlabType` 顺序与 `Half` 相反 |
| `block_state_enum_strings_3` | 原版 jar `BlockStateEnumStrings3Oracle` | 79 个方块状态枚举用例 | 全部一致 | `NoteBlockInstrument` 23 项，`BASEDRUM` 序列化名是 `basedrum`（无下划线）|
| `boolean_property_index` | 原版 jar `BooleanPropertyIndexOracle` | 50 个布尔属性 × 7 单元 | 250 处差异（**已溯源，判定无害**）| Cubium 的 `allowedValues()` 是 `{false,true}`，原版是 `[true,false]`；网络层按状态名映射，不泄漏 |
| `enum_property_order` | 原版 jar `EnumPropertyOrderOracle` | FACING × 2 属性共 36 单元 | 全一致，**修复 2 处缺陷** | `FACING`/`HORIZONTAL_FACING` 值顺序错，调试棒行为与 Java 版不符 |
| `enum_property_order_all` | 原版 jar `EnumPropertyOrderAllOracle` | 全部 40 个枚举属性 × 3 单元 | 全一致，**修复 3 处缺陷** | `SLAB_TYPE`/`VERTICAL_DIRECTION`/`SIDE_CHAIN_PART` 顺序错，调试棒循环方向相反 |
| `face_sturdy` | 原版 jar `FaceSturdyOracle` | 1166 方块 × 6 方向 × 3 类型 = 20988 | **部分通过**：625 → 20 处差异 | 已修 12 处方块形状缺陷；剩余主因是 `CollisionShape` 只能存轴对齐盒并集，无法表达带空洞拓扑（见下文）|
| `world_border` | 原版 jar `WorldBorderOracle` | 5 配置 × (1 extent + 10 block + 6 aabb) = 85 | 全部一致，**修复 3 处缺陷** | `setSize` 钳制、`contains` 下界开闭、`BlockPos` 重载多查 +1（见下文）|
| `easing_type` | 原版 jar `EasingTypeOracle` | 32 种缓动 × 9 采样点 = 288 | 275 一致、13 处 1 ulp 差异 | **修复 3 处**（elastic 误用 `mthSin`）；剩余为 `-ffast-math` 的 FMA 收缩（见下文）|
| `mth_interpolation` | 原版 jar `MthInterpolationOracle` | catmullrom 5 + smoothstep 7 = 12 | 全部一致 | Cubium 缺 `catmullrom`/`lerp2`，`smoothstep` 三参数版与 `Mth` 单参数版语义不同（见下文）|
| `legacy_random` | 原版 jar `LegacyRandomOracle` | 12 条序列（种子 × 操作组合）| 全部一致 | 覆盖 2 的幂快速路径、拒绝采样、负种子、i64 最大种子、`consumeCount` |
| `custom_stats` | 原版 jar `CustomStatsOracle`（反射枚举）| 77 个自定义统计 | 全部一致，**修复 14 处缺失常量** | 统计 ID 是存档持久键，缺项会让玩家已有统计被静默丢弃 |
| `sound_events` | 原版 jar `SoundEventsOracle`（反射枚举）| 1637 个音效 ID | 全部一致，**补齐 549 个缺失 ID** | 音效 ID 是 sounds.json 键与网络包内容，缺项会让行为完全无声 |
| `particle_type` | 原版 jar `ParticleTypeOracle`（注册表顺序）| 115 个粒子类型 | 110 一致、5 处已登记命名差异 | 枚举值直接是网络协议 ID，错一位会让后续全部错位且不报错；**索引已验证完全对齐** |
| `fuel_values` | 原版 jar `FuelValuesOracle` | 1505 个物品的燃料时长 | 全部一致 | 需服务端世界，独占组 `loot-world` |
| `recipe_book_category` | 原版 jar `RecipeBookCategoryOracle` | 1470 个配方的分页 | 全部一致 | 配方书分类页归属 |
| `walk_path_type` | 原版 jar | — | — | 缺少文档，待补 |
| `path_finder` | 原版 jar | — | — | 缺少文档，待补 |
| `fly_path_finder` | 原版 jar | — | — | 缺少文档，待补 |
| `villager_trades` | 原版 jar | — | — | 缺少文档，待补 |
| `enchantment_selection` | 原版 jar `EnchantmentSelectionOracle` | 168 个单元 | 105 一致，63 已知差异 | 附魔选择 |
| `repair_cost` | 原版 jar `RepairCostOracle` | 14 种代价组合 | 全部一致 | 纯计算，铁砧修复代价 |
| `anvil_durability` | 原版 jar `AnvilDurabilityOracle` | 16 种耐久组合 | 全部一致 | 纯计算，铁砧合并耐久 |
| `item_stackable` | 原版 jar `ItemStackableOracle` | 23 个「物品 × 损伤」 | 22 一致，1 已知差异 | 可堆叠判定 |
| `item_same_components` | 原版 jar `ItemSameComponentsOracle` | 12 组配对 | 全部一致 | 组件相等判定 |
| `furnace_tick` | 原版 jar `FurnaceTickOracle` | 12 种状态组合 | 全部一致 | 纯计算，熔炉状态机 |
| `brewing_tick` | 原版 jar `BrewingTickOracle` | 12 种状态组合 | 全部一致 | 纯计算，酿造台状态机 |
| `beacon_effects` | 原版 jar `BeaconEffectsOracle` | 10 种层级 × 效果 | 全部一致 | 信标效果 |
| `hopper_cooldown` | 原版 jar `HopperCooldownOracle` | 10 种冷却状态 | 全部一致 | 纯计算，漏斗冷却时序 |
| `mth_rounding` | 原版 jar `MthRoundingOracle` | 21 个取整输入 | 19 一致，2 已知差异 | 纯计算，`Mth` 取整 |
| `mth_modulo` | 原版 jar `MthModuloOracle` | 31 个取模/角度输入 | 全部一致 | 纯计算，`Mth` 取模 |
| `direction_rotate` | 原版 jar `DirectionRotateOracle` | 43 个方向/角度输入 | 全部一致 | 纯计算，方向旋转 |
| `aabb_ops` | 原版 jar `AabbOpsOracle` | 19 个几何运算 | 全部一致 | 纯计算（Cubium 无 AABB 类） |
| `spawner_delay` | 原版 jar `SpawnerDelayOracle` | 15 种延迟状态组合 | 全部一致 | 纯计算，`-1` 哨兵、区间退化 |

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

`crop_growth` 比的是**作物一次随机刻的结果**：生长速率、是否抽芽、梗是否结果、
耕地保墒。单元是 26 个手工编的场景——每场景给一片 7×7 地形（行优先，中心是作物），
两侧按同一份字符表重建世界，跑一次随机刻，再比对作物本体、上方、四个水平邻居与脚下耕地。

为什么必须手工编地形：生长速率不是方块自己的属性，而是**周围 3×3 地形**的函数——脚下
九格的耕地与湿度、左右前后有没有同类作物、四个斜角有没有。这类错误不报错、不崩溃，
只是「这块地的作物长得比原版慢」。地形用字符编码（`.` 空气、`0`-`7` 耕地带湿度、
`c` 同种作物、`b` 竹子），两侧各自写死同一张表，任何一侧改了地形都会立刻暴露。

随机数两侧同算法同种子（LCG），所以「这一次到底长没长」是确定的——不一致意味着掷骰
顺序或分支条件与原版不同，而不只是运气。

实现上有两处值得记下来的坑：

- **临时世界的方块写入会被判非法**：走 `Level.setBlock` 会先过 `updateShape` 的合法性
  判断，地形逐格铺时顺序稍不对（耕地先于地基）整块就会被回滚成空气。oracle 改用
  `LevelChunk#setBlockState` 直写区块存储，绕开校验。
- **天空光照在临时世界里恒为 0**（线程化光照引擎不跑 tick、`runLightUpdates` 直接抛
  异常）。光照是环境输入而非被测逻辑，所以场景不靠光照区分明暗，而是用「上方盖一块
  实心方块」表达暗处——两侧同一条约定。

`redstone_signal` 比的是**红石信号**：给定同一片地形，某个方块朝六个方向各输出多少
强度。单元是 41 个手工编的场景（拉杆的八种附着方式、红石块、红石火把、观察者、
石头 / 玻璃 / 台阶三种「导体候选」、红石线的八种邻里关系），每个场景给一片 5×5×3 的
薄片，主语固定在中心格，紧贴它的六个邻居随场景变化。

为什么值得比：红石是一套「方块问邻居要信号」的协议，强度、方向、强 / 弱之分，任何
一处与原版不一致，玩家搭出来的电路就会整体失灵——中继器不亮、门不开、活塞不推。
它不会崩溃、不会报错，只是「做得和原版不一样」。

字段严格对应原版的三条查询入口，不自创口径：

- `signal` → `SignalGetter#getSignal`：先问方块自己，若它是红石导体再叠加
  `getDirectSignalTo`。**这是电路真正「看到」的那个值**。
- `weak` → `BlockBehaviour#getSignal`（常说的「弱信号」）。
- `strong` → `BlockBehaviour#getDirectSignal`（常说的「强信号」）。

三者不是同一件事的不同名字：红石块 `weak` 处处 15 而 `strong` 处处 0；被火把点亮
的石头 `weak` 处处 0 而 `strong` 处处 15。电路失效恰恰常发生在这两个混淆上。

首轮跑通查出三处差异，全部修掉：

- **红石导体判据**：Cubium 原用「实体且不透明」，把单层台阶、楼梯误判为导体，于是
  「火把在台阶正下方」时凭空向四周输出满功率（原版此处为 0）。改为「碰撞形状完整覆盖
  整格 **且** 不透明」——两个判据缺一不可（只看形状会把玻璃、红石块误判为导体，原版
  对这两者都显式注册成 never）。
- **场景地形的坐标约定**：两侧的测试地形最初把邻居写在「相对中心偏移两格」的位置，
  与探针的约定差一格，邻居实际不在主语的六个方向上，答案全是 0。

另外两处是实现上的坑，记下来供后续能力复用：

- **临时世界的地形必须直写区块存储，且要跳过方块回调**。红石线的 `onPlace` 会走
  `updatePowerStrength`，而它第一条判定就是「脚下有没有支撑」——地形逐格铺时支撑可能
  还没摆上，红石线会被自己判为非法、原地变回空气。oracle 用
  `Block.UPDATE_SKIP_ALL_SIDEEFFECTS` 直写存储。
- **清空范围必须比地形本身更大一圈**。下层中心格在 y=-2、上层在 y=+2，清到 y∈[-2,1]
  会把上一次场景遗留的上层方块留在原地，个别场景因此莫名其妙地不一致。

`light_occlusion` 比的是**方块状态的光照阻挡属性**：`lightBlock`（每格衰减量）、
`propagatesSkylightDown`（天空光直通）、`useShapeForLightOcclusion`（按形状遮挡）、
`canOcclude`（遮挡总结开关）。单元是方块，每个方块一次返回它**全部状态**的取值。

为什么值得比：光照引擎按这四个值决定「一束光走过这一格还剩多少」。它们都是纯数据，
抄错没有编译期提示——多衰减一级屋子里就差一格看不清，树叶判成不透光树冠下就长不出草。
与 `block_properties` 的分工：那个能力比方块级的硬度/亮度/滑度，并明确把光照阻挡排除在外；
本能力专比状态级的这四个值。

首轮跑通查出四处系统性偏差，全部修掉：

- **树叶无衰减直通天空光**：`LeavesBlock` 重写 `propagatesSkylightDown()` 返回 true，注册属性
  里也加了 `propagatesSkylightDown()`。原版两者都没有——树叶遮挡形状是完整立方体，默认
  公式给出 false，天空光穿过衰减 1 级。早期实现让树冠下的地面比原版亮一级。
- **台阶/楼梯完全不透光**：`Block::getOpacity` 用「canOcclude && 材质不透明」近似
  `isSolidRender`，把单层台阶判成 15。原版判据是 `canOcclude && isShapeFullBlock(遮挡形状)`，
  台阶形状不完整 → 0。同一判定还用在 `propagatesSkylightDown`，两处都改为精确的形状判定。
- **37 个方块误加 `useShapeForLightOcclusion` 重写**：原版只有 15 个方块重写它，Cubium 有 46 个，
  栅栏、活板门、按钮、压力板、告示牌等原版都走默认 false。多余重写已删除，固化了旧行为的
  12 个单元测试同步改为断言默认值。
- **`noCollision()` 没有同步关掉 `canOcclude`**：原版这个开关会同时置 `canOcclude=false`，
  于是空气、花草这类无碰撞方块不会被当作遮挡体。

随后按根因逐类修复，一致数从 963 提升到 1166，全部对齐：

- **`refreshLightCache` 漏刷新 `useShapeForLightOcclusion`**：这是影响面最广的一处。三个
  光照属性都是由虚函数在状态构造期算出来的，而状态建表发生在方块对象构造完成之前——
  子类重写要等到构造之后才生效。缓存刷新函数只重算了 `opacity` 与 `propagatesSkylightDown`，
  漏掉的那一个让土径、耕地、讲台、切石机、附魔台、末地传送门框等所有重写该方法的方块
  运行时都退回默认值 false。补上后这些方块一次性对齐。
- **约 40 个方块缺 `noOcclusion()`**：旗帜、床、潜影盒、染色玻璃板、铜栏杆/锁链/灯笼/
  避雷针、铁链、梯子、脚手架、漏斗、炼药锅、遮光玻璃、宝库等，原版注册时都调用了这个开关。
  它们多为共用一份属性变量的染色体，改一处批量生效。
- **约 20 个实心方块被误加 `noOcclusion()`**：末地石、方解石、蘑菇柄、下界石英矿、
  蛙明灯、红石灯、加固深板岩等原版根本没有这个开关，Cubium 多加了，导致它们不再遮挡光线。
  这批问题此前一直被「`noCollision()` 不生效」掩盖，修好那个开关后才暴露出来。
- **墙面头颅错误继承 `noOcclusion()`**：原版墙面变体走 `Blocks.wallVariant(...)`，只继承
  掉落表与描述，不继承这个开关，因此它们的 `canOcclude` 保持 true。
- **铜箱子被错注成无遮挡**：注册处注释写着「vanilla 铜箱子注册 noOcclusion()」，但原版只注册了
  mapColor/strength/sound/requiresCorrectToolForDrops，`canOcclude` 保持默认 true。
- **玻璃缺天空光直通**：原版 `glass` 是 `TransparentBlock`，重写 `propagatesSkylightDown` 返回
  true（`lightBlock=0`）；Cubium 只注册了 `noOcclusion()`，让完整立方体的遮挡形状把天空光挡成
  1 级。注意染色玻璃板相反——原版它们不透光（`lightBlock=1`）。
- **六个方块缺 `useShapeForLightOcclusion` 重写**：讲台、切石机、末地传送门框、附魔台、
  白天探测器、活塞头，原版都返回 true。

第二轮清掉了收尾的 31 处，两类根因：

- **未重写 `getShape`，退回整格立方体**（多数）：原版为这些方块返回细柱、扁平、十字等
  非整格形状，光照引擎据此判 `propagatesSkylightDown=true` 与更小的 `lightBlock`。
  涉及末地烛、下界菌、火焰/灵魂火、拉杆、活塞/活塞头/移动中的活塞、末影箱、附魔台、
  白天探测器、高海葵、潮涌核心、遮光玻璃、两种下界藤的茎段等。修法是按原版源码里的
  `Shapes.box/column/boxZ` 尺寸逐个换算成像素盒——注意 `Block.boxZ(w, y1, y2)` 与
  `column(d, y1, y2)` 都靠居中计算展开，直接照抄参数会得到完全不同的盒子。
- **注册属性写错，与形状无关**：气泡柱与水把 `lightBlock` 显式写成 0（原版是 1，含流体
  的方块每格衰减 1 级）；浮冰多写了 `propagatesSkylightDown()`；遮光玻璃的 `opacity(15)`
  与默认哨兵值相同因而无效，必须靠专用子类给出 15；中继器/比较器/苔藓地毯多写了
  `noCollision()`，而它会连带清掉 `canOcclude`；幽匿脉络多写了
  `useShapeForLightOcclusion`。

两处需要额外说明的语义陷阱：

- **幽匿尖啸体**：原版只把碰撞形状与遮挡形状覆盖成 8 像素台子，`getShape` 仍是整格，
  而 `propagatesSkylightDown` 的默认公式取的是 `getShape`。Cubium 的默认公式为了绕开
  `getShape` 不看 `hasCollision` 的问题而改取 `getOcclusionShape`，在这里会算出相反结果，
  因此该方块必须自己重写 `propagatesSkylightDown`。把默认公式整体改成 `getShape` 或
  `getCollisionShape` 都会破坏另外十个方块（蜘蛛网、蜂蜜块、雪、泥巴、细雪等），已验证不可行。
- **两种下界藤的茎段**：原来用 `SimpleBlock` 注册，除了形状是整格外，还丢掉了「头部断裂
  时茎段变回头部」的行为。改为新增的 `TwistingVinesPlantBlock`/`WeepingVinesPlantBlock`
  （继承 `GrowingPlantBodyBlock`），形状分别为 `column(8,0,16)` 与 `column(14,0,16)`。
  顺带修正了扭曲藤头部的高度（原写 9 像素，原版是 15 像素）。

至此该能力的 `knownDifferences` 登记表已清空并删除。

`block_movement` 比的是**方块对实体移动的作用**，外加三个决定「这格方块会不会自己
变化」的开关：`speedFactor`（行走速度系数，原版 `Block#getSpeedFactor`）、`jumpFactor`
（跳跃高度系数）、`ticksRandomly`（是否参与随机刻）、`signalSource`（是否输出红石信号）、
`analogOutput`（比较器能否从它读数）。单元是方块，取默认状态。

为什么值得比：这五项都是纯数据，抄错不会有编译期提示。少写一个 `jumpFactor(0.5)`
玩家就会「在蜂蜜块上能正常跳」；漏标随机刻的作物永远不生长。这两类错误都不会崩溃、
不会报错，只能逐条与原版比对发现。

首轮跑通 1166 个单元，查出 46 处差异，全部修掉。其中最有价值的一处是**一个被同名重载
遮蔽的虚函数**：

- **`Block::ticksRandomly()` 定义了两次**——一次在属性访问区（非虚），一次在虚函数区。
  非虚版本把虚函数**隐藏**了：`block->ticksRandomly()` 在静态类型是 `Block*` 时直接绑定
  到非虚版本，只返回注册时写死的标志。于是所有「按状态决定随机刻」的子类和「用
  `ticksRandomly()` 重写而不是 `BlockProperties::tickRandomly()` 注册」的方块，在随机刻
  调度与区块序列化里全部失效。修法是删掉非虚版本，并补上带状态的
  `BlockState::isRandomlyTicking()`（对齐原版 `BlockBehaviour.BlockStateBase`），把调度
  路径改成走它。

其余按类：

- **红石矿按 LIT 决定随机刻**：原版 `RedstoneOreBlock#isRandomlyTicking` 返回
  `state.getValue(LIT)`，只有被点亮的矿石才去随机刻熄灭。这也正是必须有状态参数的原因。
- **蜡烛/营火/浮冰/重生锚/幽匿块被多标了随机刻**：原版这些方块都没有 `randomTicks()`，
  也不重写 `isRandomlyTicking`。其中蜡烛与营火的「含水熄灭」走的是流体 tick。
- **下界传送门与冰块缺随机刻**：两者原版都注册了 `randomTicks()`。传送门的随机刻用来
  生成僵尸猪灵（Cubium 尚未实现该逻辑，已留 TODO）。
- **「Unaffected」被误当成「不氧化」**：`copper_chest` / `copper_golem_statue` /
  `lightning_rod` 三个基础方块原本注册成不可氧化的基类，但原版它们是
  `WeatheringCopper*` 的未氧化实例——`WeatheringCopper.NEXT_BY_BLOCK` 的链条正是
  Unaffected → Exposed → Weathered → Oxidized，「Unaffected」只是链条起点。
- **铁轨/绊线/命令方块被错当成信号源**：铁轨只是「被充能后改变形状或启动矿车」，
  并不向周围输出信号；而探测铁轨（`detector_rail`）恰恰相反，原版明确重写了
  `isSignalSource`，之前跟着父类一起被删掉后又要补回来。
- **唱片机漏了信号源与弱信号输出**：原版播放唱片时输出强度 15。
- **信标与幽匿催化体多实现了比较器输出**：原版两者都没有 `hasAnalogOutputSignal`。
  「信标输出金字塔等级」「催化体按 BLOOM 输出 15」都是常见误解。
- **合成器被错当成信号源**：原版只重写了 `hasAnalogOutputSignal`。
- **比较器「只实现读数、漏实现开关」**：幽匿感测体、雕纹书架、合成器都有
  `getComparatorInputOverride`，但缺 `hasComparatorInputOverride`——比较器会先问
  「这格能不能读」，默认 false 就直接跳过，读数实现写了也永远调不到。
- **炼药锅/发射器/蜂巢/末地传送门框架缺比较器读数**：分别对应水位、容器填充率、
  蜂蜜等级、是否放了末影之眼。

`can_harvest` 比的是**每种手持工具能否从方块上获得掉落**（原版
`Player#hasCorrectToolForDrops`，即 `!requiresCorrectToolForDrops || isCorrectToolForDrops`）。
单元是方块 × 32 种手持物（空手 + 6 档 × 5 类工具 + 剪刀）。

为什么值得比：这是生存模式最直接的判定，抄错的表现是「白挖」——掉落为空、没有报错、
没有崩溃。它同时依赖两半数据：方块声明「需要什么工具、哪一档」，工具声明「自己能挖什么」。
原版这两半都放在**数据包标签**里（`mineable/<tool>` 与 `needs_{stone,iron,diamond}_tool`），
由 `Tool.Rule` 按标签匹配求值。

首轮跑通查出 1166 个单元全部判错。根因与修法：

- **采集判定被写成了「材质硬编码」**：`PickaxeItem::canHarvestBlock` 原先是
  「方块材质是 ROCK/IRON/ANVIL 就返回 true」。原版按**方块标签**匹配，两者不等价：
  绿宝石矿、涂蜡铜块、深板岩矿石都只能镐挖，材质各不相同；反过来同材质方块
  （黏土、泥土）却空手可挖。
- **修法**：新增 `HarvestToolTable`，在**方块标签加载完成后**（服务端 `BlockTagLoader`
  之后、客户端 `VanillaBlocks::initialize` 末尾）从数据包标签派生
  「方块 → [(工具, 最低档位)]」，`ToolItem`/`SwordItem`/`ShearsItem` 统一查它。
- **`requiresCorrectToolForDrops` 无法从数据包推导**：`mineable/*` 标签并集是 833 个方块，
  其中只有 **411** 个真正要求正确工具，其余（按钮、门、活板门、泥土、树叶……）空手挖掉
  照样掉落。该属性在原版是注册时硬编码的，数据包里没有文件表达它，因此以原版 jar 实测
  快照内嵌在 `HarvestToolTable` 中，并在注释里说明原因与来源。
- **金工具与木工具同档**：原版 `INCORRECT_FOR_GOLD_TOOL` == `INCORRECT_FOR_WOODEN_TOOL`
  （都含 needs_stone + needs_iron + needs_diamond），金镐挖不动 needs_stone 那一档
  （铁矿、青金石矿、铜块系列），与石镐不同。
- **蜘蛛网只有剑能正确采**：剑的 `Tool.Rule` 里只有一条
  `minesAndDrops(COBWEB, 15.0F)`，数据包里没有对应标签，因此以硬编码规则并入表。
- **命令方块/结构方块/拼图方块**：要求正确工具、却不在任何 `mineable` 标签里，
  原版表现为「任何工具都挖不动」。`ToolItem::canHarvestBlock` 对「要求工具但无规则」
  必须返回 false，不能放行。

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

[`aabb_ops`](../../docs/test/diff/aabb_ops.md)、[`aabb_contains`](../../docs/test/diff/aabb_contains.md)、[`fluid_height`](../../docs/test/diff/fluid_height.md)、[`mth_inverse_lerp`](../../docs/test/diff/mth_inverse_lerp.md)、[`mth_angle_lerp`](../../docs/test/diff/mth_angle_lerp.md)、[`mth_waveform`](../../docs/test/diff/mth_waveform.md)、[`mth_hsv`](../../docs/test/diff/mth_hsv.md)、[`argb_ops`](../../docs/test/diff/argb_ops.md)、[`direction_nearest`](../../docs/test/diff/direction_nearest.md)、[`mth_seed`](../../docs/test/diff/mth_seed.md)、[`mth_pack_degrees`](../../docs/test/diff/mth_pack_degrees.md)、[`mth_degrees`](../../docs/test/diff/mth_degrees.md)、[`mth_frac`](../../docs/test/diff/mth_frac.md)、[`mth_sqrt`](../../docs/test/diff/mth_sqrt.md)、[`string_util`](../../docs/test/diff/string_util.md)、[`identifier_parse`](../../docs/test/diff/identifier_parse.md)、[`identifier_path`](../../docs/test/diff/identifier_path.md)、[`blockpos_iterate`](../../docs/test/diff/blockpos_iterate.md)、[`blockpos_spiral`](../../docs/test/diff/blockpos_spiral.md)、[`aabb_factory`](../../docs/test/diff/aabb_factory.md)、[`front_and_top`](../../docs/test/diff/front_and_top.md)、[`structure_orient`](../../docs/test/diff/structure_orient.md)、[`bounding_box_encap`](../../docs/test/diff/bounding_box_encap.md)、[`chunk_region`](../../docs/test/diff/chunk_region.md)、[`portal_relative`](../../docs/test/diff/portal_relative.md)、[`relative_mask`](../../docs/test/diff/relative_mask.md)、[`position_move_rotation`](../../docs/test/diff/position_move_rotation.md)、[`style_apply`](../../docs/test/diff/style_apply.md)、[`text_color`](../../docs/test/diff/text_color.md)、[`click_event`](../../docs/test/diff/click_event.md)、[`hover_event`](../../docs/test/diff/hover_event.md)、[`identifier_resolve`](../../docs/test/diff/identifier_resolve.md)、[`plain_text_contents`](../../docs/test/diff/plain_text_contents.md)、[`mth_absmax`](../../docs/test/diff/mth_absmax.md)、[`display_slot`](../../docs/test/diff/display_slot.md)、[`objective_criteria`](../../docs/test/diff/objective_criteria.md)、[`team_rule`](../../docs/test/diff/team_rule.md)、[`boss_bar`](../../docs/test/diff/boss_bar.md)、[`sound_source`](../../docs/test/diff/sound_source.md)、[`difficulty`](../../docs/test/diff/difficulty.md)、[`anvil_durability`](../../docs/test/diff/anvil_durability.md)、[`banner_pattern`](../../docs/test/diff/banner_pattern.md)、[`beacon_effects`](../../docs/test/diff/beacon_effects.md)、[`brewing_tick`](../../docs/test/diff/brewing_tick.md)、[`dimension_type`](../../docs/test/diff/dimension_type.md)、[`direction_rotate`](../../docs/test/diff/direction_rotate.md)、[`enchantment_provider`](../../docs/test/diff/enchantment_provider.md)、[`enchantment_selection`](../../docs/test/diff/enchantment_selection.md)、[`entity_attributes`](../../docs/test/diff/entity_attributes.md)、[`entity_dimensions`](../../docs/test/diff/entity_dimensions.md)、[`explosion_damage`](../../docs/test/diff/explosion_damage.md)、[`explosion_ray`](../../docs/test/diff/explosion_ray.md)、[`explosion_seen_grid`](../../docs/test/diff/explosion_seen_grid.md)、[`food_stats`](../../docs/test/diff/food_stats.md)、[`food_tick`](../../docs/test/diff/food_tick.md)、[`furnace_tick`](../../docs/test/diff/furnace_tick.md)、[`hopper_cooldown`](../../docs/test/diff/hopper_cooldown.md)、[`item_same_components`](../../docs/test/diff/item_same_components.md)、[`item_stackable`](../../docs/test/diff/item_stackable.md)、[`mob_effect_merge`](../../docs/test/diff/mob_effect_merge.md)、[`mth_modulo`](../../docs/test/diff/mth_modulo.md)、[`mth_rounding`](../../docs/test/diff/mth_rounding.md)、[`repair_cost`](../../docs/test/diff/repair_cost.md)、[`spawner_delay`](../../docs/test/diff/spawner_delay.md)、[	rial_spawner_config](../../docs/test/diff/trial_spawner_config.md)、[`vec3_math`](../../docs/test/diff/vec3_math.md)、[`blockpos_codec`](../../docs/test/diff/blockpos_codec.md)、[`axis_plane`](../../docs/test/diff/axis_plane.md)、[`mth_lerp`](../../docs/test/diff/mth_lerp.md)、[`mth_bit`](../../docs/test/diff/mth_bit.md)、[`mth_rot`](../../docs/test/diff/mth_rot.md)、[`mth_sin`](../../docs/test/diff/mth_sin.md)、[`mth_approach`](../../docs/test/diff/mth_approach.md)、[`region_sector`](../../docs/test/diff/region_sector.md)、[`blockpos_dist`](../../docs/test/diff/blockpos_dist.md)、[`enchantment_storage`](../../docs/test/diff/enchantment_storage.md)、[`collide_sweep`](../../docs/test/diff/collide_sweep.md)、[`shapes_boolean`](../../docs/test/diff/shapes_boolean.md)、[`fluid_spread`](../../docs/test/diff/fluid_spread.md)、[`fluid_spread_dir`](../../docs/test/diff/fluid_spread_dir.md)、[`blockpos_bfs`](../../docs/test/diff/blockpos_bfs.md)、[`scheduled_tick`](../../docs/test/diff/scheduled_tick.md)、[`random_tick_pos`](../../docs/test/diff/random_tick_pos.md)、[`mth_color`](../../docs/test/diff/mth_color.md)、[`block_transform`](../../docs/test/diff/block_transform.md)、[`bounding_box`](../../docs/test/diff/bounding_box.md)、[`enchantment_compat`](../../docs/test/diff/enchantment_compat.md)、[`section_pos`](../../docs/test/diff/section_pos.md)、[`level_height`](../../docs/test/diff/level_height.md)、[`chunk_pos`](../../docs/test/diff/chunk_pos.md)、[`mth_clamp`](../../docs/test/diff/mth_clamp.md)、[`chat_formatting`](../../docs/test/diff/chat_formatting.md)、[`dye_color`](../../docs/test/diff/dye_color.md)、[`equipment_slot`](../../docs/test/diff/equipment_slot.md)、[`heightmap_types`](../../docs/test/diff/heightmap_types.md)、[`mth_int_div`](../../docs/test/diff/mth_int_div.md)、[`direction_plane`](../../docs/test/diff/direction_plane.md)、[`rotation_info`](../../docs/test/diff/rotation_info.md)、[`blockpos_geom`](../../docs/test/diff/blockpos_geom.md)、[`vec3i_arith`](../../docs/test/diff/vec3i_arith.md)、[`fuel_values`](../../docs/test/diff/fuel_values.md)、[`recipe_book_category`](../../docs/test/diff/recipe_book_category.md)、[`text_formatting_color`](../../docs/test/diff/text_formatting_color.md)、[`text_formatting_name`](../../docs/test/diff/text_formatting_name.md)、[`style_tristate`](../../docs/test/diff/style_tristate.md)、[`team_rule_strings`](../../docs/test/diff/team_rule_strings.md)、[`render_type_strings`](../../docs/test/diff/render_type_strings.md)、[`uuid_strings`](../../docs/test/diff/uuid_strings.md)、[`game_type`](../../docs/test/diff/game_type.md)、[`map_decoration_type`](../../docs/test/diff/map_decoration_type.md)、[`block_state_enum_strings`](../../docs/test/diff/block_state_enum_strings.md)、[`block_state_enum_strings_2`](../../docs/test/diff/block_state_enum_strings_2.md)、[`block_state_enum_strings_3`](../../docs/test/diff/block_state_enum_strings_3.md)、[`boolean_property_index`](../../docs/test/diff/boolean_property_index.md)、[`enum_property_order`](../../docs/test/diff/enum_property_order.md)、[`enum_property_order_all`](../../docs/test/diff/enum_property_order_all.md)、[`face_sturdy`](../../docs/test/diff/face_sturdy.md)、[`world_border`](../../docs/test/diff/world_border.md)、[`easing_type`](../../docs/test/diff/easing_type.md)、[`mth_interpolation`](../../docs/test/diff/mth_interpolation.md)、[`legacy_random`](../../docs/test/diff/legacy_random.md)、[`custom_stats`](../../docs/test/diff/custom_stats.md)、[`sound_events`](../../docs/test/diff/sound_events.md)、[`particle_type`](../../docs/test/diff/particle_type.md)
[`aabb_ops`](../../docs/test/diff/aabb_ops.md)、[`aabb_contains`](../../docs/test/diff/aabb_contains.md)、[`fluid_height`](../../docs/test/diff/fluid_height.md)、[`mth_inverse_lerp`](../../docs/test/diff/mth_inverse_lerp.md)、[`mth_angle_lerp`](../../docs/test/diff/mth_angle_lerp.md)、[`mth_waveform`](../../docs/test/diff/mth_waveform.md)、[`mth_hsv`](../../docs/test/diff/mth_hsv.md)、[`argb_ops`](../../docs/test/diff/argb_ops.md)、[`direction_nearest`](../../docs/test/diff/direction_nearest.md)、[`mth_seed`](../../docs/test/diff/mth_seed.md)、[`mth_pack_degrees`](../../docs/test/diff/mth_pack_degrees.md)、[`mth_degrees`](../../docs/test/diff/mth_degrees.md)、[`mth_frac`](../../docs/test/diff/mth_frac.md)、[`mth_sqrt`](../../docs/test/diff/mth_sqrt.md)、[`string_util`](../../docs/test/diff/string_util.md)、[`identifier_parse`](../../docs/test/diff/identifier_parse.md)、[`identifier_path`](../../docs/test/diff/identifier_path.md)、[`blockpos_iterate`](../../docs/test/diff/blockpos_iterate.md)、[`blockpos_spiral`](../../docs/test/diff/blockpos_spiral.md)、[`aabb_factory`](../../docs/test/diff/aabb_factory.md)、[`front_and_top`](../../docs/test/diff/front_and_top.md)、[`structure_orient`](../../docs/test/diff/structure_orient.md)、[`bounding_box_encap`](../../docs/test/diff/bounding_box_encap.md)、[`chunk_region`](../../docs/test/diff/chunk_region.md)、[`portal_relative`](../../docs/test/diff/portal_relative.md)、[`relative_mask`](../../docs/test/diff/relative_mask.md)、[`position_move_rotation`](../../docs/test/diff/position_move_rotation.md)、[`style_apply`](../../docs/test/diff/style_apply.md)、[`text_color`](../../docs/test/diff/text_color.md)、[`click_event`](../../docs/test/diff/click_event.md)、[`hover_event`](../../docs/test/diff/hover_event.md)、[`identifier_resolve`](../../docs/test/diff/identifier_resolve.md)、[`plain_text_contents`](../../docs/test/diff/plain_text_contents.md)、[`mth_absmax`](../../docs/test/diff/mth_absmax.md)、[`display_slot`](../../docs/test/diff/display_slot.md)、[`objective_criteria`](../../docs/test/diff/objective_criteria.md)、[`team_rule`](../../docs/test/diff/team_rule.md)、[`boss_bar`](../../docs/test/diff/boss_bar.md)、[`sound_source`](../../docs/test/diff/sound_source.md)、[`difficulty`](../../docs/test/diff/difficulty.md)、[`permission_level`](../../docs/test/diff/permission_level.md)、[`anvil_durability`](../../docs/test/diff/anvil_durability.md)、[`banner_pattern`](../../docs/test/diff/banner_pattern.md)、[`beacon_effects`](../../docs/test/diff/beacon_effects.md)、[`brewing_tick`](../../docs/test/diff/brewing_tick.md)、[`dimension_type`](../../docs/test/diff/dimension_type.md)、[`direction_rotate`](../../docs/test/diff/direction_rotate.md)、[`enchantment_provider`](../../docs/test/diff/enchantment_provider.md)、[`enchantment_selection`](../../docs/test/diff/enchantment_selection.md)、[`entity_attributes`](../../docs/test/diff/entity_attributes.md)、[`entity_dimensions`](../../docs/test/diff/entity_dimensions.md)、[`explosion_damage`](../../docs/test/diff/explosion_damage.md)、[`explosion_ray`](../../docs/test/diff/explosion_ray.md)、[`explosion_seen_grid`](../../docs/test/diff/explosion_seen_grid.md)、[`food_stats`](../../docs/test/diff/food_stats.md)、[`food_tick`](../../docs/test/diff/food_tick.md)、[`furnace_tick`](../../docs/test/diff/furnace_tick.md)、[`hopper_cooldown`](../../docs/test/diff/hopper_cooldown.md)、[`item_same_components`](../../docs/test/diff/item_same_components.md)、[`item_stackable`](../../docs/test/diff/item_stackable.md)、[`mob_effect_merge`](../../docs/test/diff/mob_effect_merge.md)、[`mth_modulo`](../../docs/test/diff/mth_modulo.md)、[`mth_rounding`](../../docs/test/diff/mth_rounding.md)、[`repair_cost`](../../docs/test/diff/repair_cost.md)、[`spawner_delay`](../../docs/test/diff/spawner_delay.md)、[	rial_spawner_config](../../docs/test/diff/trial_spawner_config.md)、[`vec3_math`](../../docs/test/diff/vec3_math.md)、[`blockpos_codec`](../../docs/test/diff/blockpos_codec.md)、[`axis_plane`](../../docs/test/diff/axis_plane.md)、[`mth_lerp`](../../docs/test/diff/mth_lerp.md)、[`mth_bit`](../../docs/test/diff/mth_bit.md)、[`mth_rot`](../../docs/test/diff/mth_rot.md)、[`mth_sin`](../../docs/test/diff/mth_sin.md)、[`mth_approach`](../../docs/test/diff/mth_approach.md)、[`region_sector`](../../docs/test/diff/region_sector.md)、[`blockpos_dist`](../../docs/test/diff/blockpos_dist.md)、[`enchantment_storage`](../../docs/test/diff/enchantment_storage.md)、[`collide_sweep`](../../docs/test/diff/collide_sweep.md)、[`shapes_boolean`](../../docs/test/diff/shapes_boolean.md)、[`fluid_spread`](../../docs/test/diff/fluid_spread.md)、[`fluid_spread_dir`](../../docs/test/diff/fluid_spread_dir.md)、[`blockpos_bfs`](../../docs/test/diff/blockpos_bfs.md)、[`scheduled_tick`](../../docs/test/diff/scheduled_tick.md)、[`random_tick_pos`](../../docs/test/diff/random_tick_pos.md)、[`mth_color`](../../docs/test/diff/mth_color.md)、[`block_transform`](../../docs/test/diff/block_transform.md)、[`bounding_box`](../../docs/test/diff/bounding_box.md)、[`enchantment_compat`](../../docs/test/diff/enchantment_compat.md)、[`section_pos`](../../docs/test/diff/section_pos.md)、[`level_height`](../../docs/test/diff/level_height.md)、[`chunk_pos`](../../docs/test/diff/chunk_pos.md)、[`mth_clamp`](../../docs/test/diff/mth_clamp.md)、[`chat_formatting`](../../docs/test/diff/chat_formatting.md)、[`dye_color`](../../docs/test/diff/dye_color.md)、[`equipment_slot`](../../docs/test/diff/equipment_slot.md)、[`heightmap_types`](../../docs/test/diff/heightmap_types.md)、[`mth_int_div`](../../docs/test/diff/mth_int_div.md)、[`direction_plane`](../../docs/test/diff/direction_plane.md)、[`rotation_info`](../../docs/test/diff/rotation_info.md)、[`blockpos_geom`](../../docs/test/diff/blockpos_geom.md)、[`vec3i_arith`](../../docs/test/diff/vec3i_arith.md)、[`fuel_values`](../../docs/test/diff/fuel_values.md)、[`recipe_book_category`](../../docs/test/diff/recipe_book_category.md)、[`text_formatting_color`](../../docs/test/diff/text_formatting_color.md)、[`text_formatting_name`](../../docs/test/diff/text_formatting_name.md)、[`style_tristate`](../../docs/test/diff/style_tristate.md)、[`team_rule_strings`](../../docs/test/diff/team_rule_strings.md)、[`render_type_strings`](../../docs/test/diff/render_type_strings.md)、[`uuid_strings`](../../docs/test/diff/uuid_strings.md)、[`game_type`](../../docs/test/diff/game_type.md)、[`map_decoration_type`](../../docs/test/diff/map_decoration_type.md)、[`block_state_enum_strings`](../../docs/test/diff/block_state_enum_strings.md)、[`block_state_enum_strings_2`](../../docs/test/diff/block_state_enum_strings_2.md)、[`block_state_enum_strings_3`](../../docs/test/diff/block_state_enum_strings_3.md)、[`boolean_property_index`](../../docs/test/diff/boolean_property_index.md)、[`enum_property_order`](../../docs/test/diff/enum_property_order.md)、[`enum_property_order_all`](../../docs/test/diff/enum_property_order_all.md)、[`face_sturdy`](../../docs/test/diff/face_sturdy.md)、[`world_border`](../../docs/test/diff/world_border.md)、[`easing_type`](../../docs/test/diff/easing_type.md)、[`mth_interpolation`](../../docs/test/diff/mth_interpolation.md)、[`legacy_random`](../../docs/test/diff/legacy_random.md)、[`custom_stats`](../../docs/test/diff/custom_stats.md)、[`sound_events`](../../docs/test/diff/sound_events.md)、[`particle_type`](../../docs/test/diff/particle_type.md)

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
