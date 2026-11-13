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
4. **构建粒度**：全部能力编入同一个 `diff_runner` 目标，构建它等于构建全部探针。
   增量由 ninja 按源文件粒度处理：只改一个探针源文件时只重编那一个 .o，再重链接一次。
5. **选择器**（`runner/select.ts`）：`--changed` 用依赖图判定「哪些能力依赖改动的文件」，
   并且**不**用通过记录过滤——显式给出一批改动时，用户就是在要求验证它们。
   注意依赖图是**刻意过宽**的：C++ 侧按静态库边界展开，改一个 `mc_common` 的头会让所有
   probe 重跑；这比用符号表反推「真正被引用」稳，误触发的代价只是慢几十秒。
6. **并行**：一个能力一个子进程（模块解析钩子是进程级的，且能把崩溃/OOM 隔离在这一个能力里），
   上次耗时长的先跑。`exclusiveGroup` 声明的能力互不并发——战利品族 oracle 要在
   `java.io.tmpdir` 建原版服务端世界并持有区域文件锁，并行会直接锁死。

本机实测（28 核，Windows）：全量 37 个能力 `-j 8` 约 3 分 20 秒；无改动重跑约 3 秒
（`--all` 会全部重跑，约 3 分 20 秒）；改动一个 probe 源文件只跑该能力。

> **多个 Java oracle 能力并发时偶发 `jar xf ... META-INF/libraries/` 失败**：多个 worker 会同时把
> 同一个 bundler jar 的依赖解到同一目录，先解完的会把目录换掉，其余 worker 拿到 ENOENT。
> 表现为该能力「错误」而不是「差异」。处理：重跑一次；仍然失败就 `-j 1` 串行跑这几个能力
> （`structure_*`、`loot_table*` 等）。

**前置**：

1. `MC_BUILD_DIFF_PROBES` 默认为 `OFF`，`runner.ts build` 会自动打开它；Windows 上它通过
   `scripts/configure.bat` 注入 VS 环境后再调 `cmake --build`，与人工构建共用同一个构建目录。
   Windows 用 `windows-clang-relwithdebinfo-server` 预设（`MC_BUILD_CLIENT=OFF`）：客户端已停止
   维护、`vcpkg.json` 不再提供 glfw3 等客户端依赖，含客户端的预设会在 configure 阶段失败；
   差异探针本身不依赖 `src/client`。
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

全部能力编入**同一个**可执行文件，第一参数选能力，之后是两个子命令：

```
diff_runner <domain> list                          # 每行一个单元（方块状态文本）
diff_runner <domain> run <input.jsonl> <output.jsonl>   # 每行输入 → 一行 JSON 输出
```

为什么合用一个进程：每个能力单独链接一份 `mc_common` 会让产物体积与 Debug 信息按能力
数倍增（257 个探针合计 1.4 GB 可执行文件 + 11 GB `.pdb`），且改一行公共代码要重链接
257 次。合并后只有一份 66 MB 可执行文件与一份 PDB。各探针源文件用
`MC_DIFF_DEFINE_PROBE(<domain>, &writeList, &runLine)` 在静态初始化期把自己登记进注册表。

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
| `item_summary` | minecraft-data（`reference`，不再走 Java oracle） | 1124 | 全部一致 | 381 个物品未注册 |
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
| `structure_starts` | 原版 jar `StructureStartsOracle` | 542 个区块（候选区块 + 邻居 + 空白区块，两个种子 × 主世界/末地） | 509/542 个单元逐片段一致 | 见 `docs/iterations/structure_starts差分-交接.md` |
| `mineshaft_pieces` | 原版 jar `MineshaftPiecesOracle` | 24 个单元（两个种子下真的落了矿山的候选区块） | 9 个一致；15 处差异 |
| `end_city_pieces` | 原版 jar `EndCityPiecesOracle` | 10 个单元（两个种子下真的落了末地城的候选区块） | 全部一致 | 逐构件比模板名 / 包围盒 / 旋转 / genDepth |
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
| `fluid_lava_react` | 原版 jar `FluidLavaReactOracle` | 岩浆源/流动岩浆 × 四方位 × 邻居 = 24 | **全部一致** | 岩浆与相邻流体/方块相遇的产物（`LiquidBlock#shouldSpreadLiquid`）：**岩浆源+水→黑曜石**、**流动岩浆+水→圆石**、**岩浆+蓝冰+灵魂土→玄武岩**，成功凝固时发 **`levelEvent 1501`**（嗤声与烟雾）。两个易错点：① **源与流动的产物不同**——写成一个，玩家浇岩浆造石头时得到黑曜石，或造黑曜石时只得到圆石；② **只看水平四个方向不看正上方**——原版 `POSSIBLE_FLOW_DIRECTIONS` 是 DOWN + 四水平，往岩浆正上方倒水不会当场凝固。**修复**：`triggerMixEffects` 原来只手工播放音效并自造粒子、**没有发 `levelEvent 1501`**，与客户端逻辑重复且两端口径不一致 |
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
| `face_sturdy` | 原版 jar `FaceSturdyOracle` | 1166 方块 × 6 方向 × 3 类型 = 20988 | **全部一致**（625 → 0）| 已修 12 处方块形状缺陷；剩余差异随 `getBlockSupportShapeAt` 的位置偏移修正一并收敛 |
| `world_border` | 原版 jar `WorldBorderOracle` | 5 配置 × (1 extent + 10 block + 6 aabb) = 85 | 全部一致，**修复 3 处缺陷** | `setSize` 钳制、`contains` 下界开闭、`BlockPos` 重载多查 +1（见下文）|
| `easing_type` | 原版 jar `EasingTypeOracle` | 32 种缓动 × 9 采样点 = 288 | 275 一致、13 处 1 ulp 差异 | **修复 3 处**（elastic 误用 `mthSin`）；剩余为 `-ffast-math` 的 FMA 收缩（见下文）|
| `mth_interpolation` | 原版 jar `MthInterpolationOracle` | catmullrom 5 + smoothstep 7 = 12 | 全部一致 | Cubium 缺 `catmullrom`/`lerp2`，`smoothstep` 三参数版与 `Mth` 单参数版语义不同（见下文）|
| `legacy_random` | 原版 jar `LegacyRandomOracle` | 12 条序列（种子 × 操作组合）| 全部一致 | 覆盖 2 的幂快速路径、拒绝采样、负种子、i64 最大种子、`consumeCount` |
| `custom_stats` | 原版 jar `CustomStatsOracle`（反射枚举）| 77 个自定义统计 | 全部一致，**修复 14 处缺失常量** | 统计 ID 是存档持久键，缺项会让玩家已有统计被静默丢弃 |
| `sound_events` | 原版 jar `SoundEventsOracle`（反射枚举）| 1637 个音效 ID | 全部一致，**补齐 549 个缺失 ID** | 音效 ID 是 sounds.json 键与网络包内容，缺项会让行为完全无声 |
| `particle_type` | 原版 jar `ParticleTypeOracle`（注册表顺序）| 115 个粒子类型 | 110 一致、5 处已登记命名差异 | 枚举值直接是网络协议 ID，错一位会让后续全部错位且不报错；**索引已验证完全对齐** |
| `shape_face` | 原版 jar `ShapeFaceOracle` | 15 种边界形状 × 6 方向 | 全部一致 | 把 slice/findIndex/isCubeLikeAlong 单独拎出来验证；**结论：切片机制本身正确**，face_sturdy 剩余差异在方块侧 |
| `block_shape_all` | 原版 jar `BlockShapeAllOracle` | 29671 个状态 × 三种形状 | **全部一致** | 遍历全部属性组合；`block_shape` 只测默认状态，本能力抓非默认状态下的形状错误。收敛路径 13612 → 0 |
| `axis_cycle` | 原版 jar `AxisCycleOracle` | 75（3 循环 × 3 轴 + 3 inverse + 54 分量重排 + 9 between）| **全部一致** | `AxisCycle` 是 `VoxelShape.collide` 与 `DiscreteVoxelShape` 边/面遍历的底层坐标变换。**两个 `cycle` 重载语义不同**：`cycle(axis)` 推轴序（X→Y→Z），`cycle(x,y,z,axis)` 重排分量后取轴（FORWARD 时 X 取 z）。分量刻意取互不相等且含负数，取错分量会立即显形 |
| `entity_type_size` | 原版 jar `EntityTypeSizeOracle` | 157 个实体类型（Cubium 已注册 126 个）| **全部一致**，31 个已知缺口 | 比对宽 / 高 / 眼睛高度 / 是否固定——这四项是原版硬编码在 `EntityType.Builder` 里的注册数据，不在数据包中。**浮点输出位模式**（`0x%08x` 的 IEEE-754），避开 Java `Float.toString` 与 C++ `%.6g` 的格式差异。缺口清单见 `known_missing_entity_types.ts`（船/筏 21、展示实体 3、marker/interaction 2、投掷药水 2、breeze_wind_charge/command_block_minecart/creaking/mannequin 4）|
| `entity_peaceful` | 原版 jar `EntityPeacefulOracle` | 双方共有的 126 个实体类型 | **全部一致** | 实体类型在和平难度下是否允许生成/召唤（`EntityType#isAllowedInPeaceful`）。原版**默认允许**，需显式 `notInPeaceful()` 才禁止。反直觉之处：大多数 `MONSTER` 类别实体确实禁止，但**并非全部**——潜影贝、僵尸猪灵、猪灵、猪灵蛮兵、疣猪兽等都不带该标记。写反的后果玩家可直接观察到却不会崩溃：该禁止的没禁止（和平难度刷出敌对生物），该允许的被禁止（`/summon` 与刷怪蛋失效）。原版另有 30 个实体类型 Cubium 尚未实现，已由 `known_missing_entity_types.ts` 豁免 |
| `entity_serialize_summon` | 原版 jar `EntitySerializeSummonOracle` | 双方共有的 126 个实体类型 | **全部一致** | 实体类型的存档序列化与召唤开关（`EntityType#canSerialize` / `canSummon`）。两个都是**默认值写反**型缺陷：`serialize` 默认 `true`（显式 `noSave()` 才关：拴绳结/闪电/玩家/浮标），`summon` 默认也是 `true`（`noSummon()` 只有玩家与浮标用）。写反不会崩溃不会报错，只是某些实体**悄悄失去存档或召唤能力**——玩家造的东西重进世界全没了，或者 `/summon` 对某些实体静默失效。**修复 29 处**：① 4 个实体缺 `noSave()`，拴绳结/闪电/玩家/浮标会被写进存档；② `Builder` 的 `CanSummon` 默认值写成不置位，导致 25 个实体（含矿车、箭、烟花、画框、末影之眼等）`/summon` 失效 |
| `entity_type_flags` | 原版 jar `EntityTypeFlagsOracle` | 双方共有的 126 个实体类型 | **全部一致** | 实体类型的刷怪分类、网络追踪参数与火焰免疫（`MobCategory` / `clientTrackingRange` / `updateInterval` / `fireImmune`）。**分类是重灾区**：原版 8 类的生物上限完全不同（monster 70 / creature 10 / ambient 15 / axolotls 5 / underground_water_creature 5 / water_creature 5 / water_ambient 20 / misc -1），把美西螈错归进水生生物就会和鱿鱼抢名额，表现为「某种生物怎么都不刷」。`trackingRange` 太小则远处实体突然消失，太大浪费带宽；`updateInterval` 为 `INT_MAX` 表示**从不主动更新**（物品框/特效云/拴绳结这类静态或由其他机制驱动的实体）。**修复 51 处**：31 个 `trackingRange` 偏差（末影龙 128→10 最严重）+ 33 个 `updateInterval` 偏差 |
| `dispense_behavior` | 原版 jar `DispenseBehaviorOracle` | 1505 个物品（Cubium 1124，缺口 381）| **985/1124 一致**，139 处已知缺口，**修复 4 处缺陷** | 比对每个物品的发射行为标签 + `isSuccess()` 初值。原版 169 条注册里 109 条是匿名子类（类名是 `DispenserBlock$1` 序号），跨实现不可比，故比对归一化标签。缺口清单见 `known_dispense_gaps.ts`（潜影盒 17、矿车 6、剪刀 1、带箱船的船 10、刷怪蛋 87、其它 18）|
| `attribute_modifier_apply` | 原版 jar `AttributeModifierApplyOracle` | 22 个场景 | **全部一致**（7 → 0）| 属性修饰符三阶段计算。**修复 2 处**：① `ADD_MULTIPLIED_BASE` 乘的是**经过 ADD_VALUE 之后的基数**而非原始 base；② `sanitizeValue` 对 **NaN 返回 minValue**（普通 clamp 会放行 NaN）|
| `entity_pose_dimensions` | 原版 jar `EntityPoseDimensionsOracle` | 16 实体 × 18 姿态 = 288 | **253/288 一致**，35 处已知缺口，**修复 11 处缺陷** | 核心：`LivingEntity.getDimensions(Pose)` 是 public final，`SLEEPING` 恒为 (0.2,0.2,0.2) 与类型无关。**修复**：`width/height/eyeHeight` 默认实现从硬编码玩家尺寸改为查注册表（影响所有未覆写者）；睡眠特判；pig/frog 的 0.85 系数误抄成 0.4/0.45；bat/breeze/creeper/warden/phantom 眼高常量；phantom/goat/pufferfish 覆写绕过睡眠特判 |
| `mob_spawn_light_level` | 原版 jar `MobSpawnLightLevelOracle` | 19 个场景 | **全部一致** | `Monster.isDarkEnoughToSpawn` 三段判定。**三个易错点**：天空光与 `nextInt(32)` 的随机阈值比而非常量；方块光上限仅在 `blockLightLimit<15` 时才检查；最后比的是综合亮度而非方块光。**本能力同时补上了 Cubium 缺失的该判定实现**。注意随机源必须是 `JavaLegacyRandom`——原版 `RandomSource.create()` 返回 Legacy LCG，而 `math::Random` 是 Xoroshiro128++ |
| `dimension_type_params` | 原版 jar `DimensionTypeParamsOracle` | 3 维度 × 14 字段 = 42 | **36/42 一致**，6 处已知缺口，**修复 9 处缺陷** | 数据包驱动的 `dimension_type` 注册表。**修复**：末地误用主世界常量（`min_y` -64→0、`height` 384→256、`has_skylight` false→true、`ambient_light` 0→0.25）；下界 `height` 把 logical_height 填进了物理高度（128→256）；主世界 `logical_height` 256→384；`infiniburn` 补 `#` 前缀；补建 `monster_spawn_light_level` / `monster_spawn_block_light_limit` 三个字段 |
| `beacon_pyramid_level` | 原版 jar `BeaconPyramidLevelOracle` | 11 个金字塔布局 | **全部一致** | `BeaconBlockEntity.updateBase`。要点：第 j 层是 `(2j+1)²` 的**整片**而非只有边框、必须**连续**（某层一破即止）、**低于世界底即停止**。布局字符串可精确表达「整层缺一格」「层间不连续」「世界底截断」。**修复**：`_updateLevels` 缺 `getMinBuildHeight` 边界检查；`computeLevel` 抽成纯函数供世界读取与测试共用 |
| `conduit_frame_level` | 原版 jar `ConduitFrameLevelOracle` | 8 个框架布局 | **全部一致** | `ConduitBlockEntity.updateShape`。要点：中心 3×3×3 必须**全是水**否则立刻失效、只认**框架环**（42 格）而非 5×5×5 整个外壳（98 格）、两档门槛 16 与 42。**重构**：`countFrameBlocks` 抽成纯函数供世界读取与测试共用 |
| `campfire_cooking` | 数据包 JSON | 18 配方 + 17 查询 = 26 | **25/25 一致** | `campfire_cooking` 类型的全部配方。营火只认该类型，**不读熔炉/烟熏机配方**，三者数据文件独立。`cookTime` 就是营火实际烹饪耗时。**修复**：`addItem` 缺配方校验（原版 `placeFood` 查不到配方就拒绝放置），此前任意物品都能放上营火、永不熟且被销毁；`cookTime <= 0` 时改为取配方值，避免第一 tick 瞬熟 |
| `cooking_recipe_types` | 数据包 JSON | 16 输入 × 4 设施 = 64 | **全部一致** | 四类烹饪配方的横向对照（熔炉/烟熏机/高炉/营火）。四套配方在数据包里各自独立、`cookingtime` 也写在 JSON 里。语义差异：熔炉通用、烟熏机只做食物且快一倍、高炉只做矿石且快一倍、营火只做食物且慢三倍。最容易出错的是**串配方**，不会崩溃只会让物品在错误设施里可加工或耗时差一倍。**注意 ingredient 可以是标签引用**（如玻璃配方用 `#minecraft:smelts_to_glass`），不展开会误判成"无配方" |
| `game_event_radius` | 原版 jar `GameEventRadiusOracle` | 60 个游戏事件 | **全部一致** | `GameEvent.notificationRadius`。60 个里 57 个是默认 16，只有 `jukebox_play`/`jukebox_stop_play`（10）与 `shriek`（32）不同。半径决定事件能被多远的监听器收到——幽匿感测体、尖啸体、监守者、悦灵这些「听声音」的机制全靠它，抄成默认 16 不会崩溃、只会让监守者的尖啸只惊动半张地图。单元由 `allGameEventIds()` 生成，**事件名集合的差异也会直接体现为单元缺失/多出** |
| `container_analog_signal` | 原版 jar `ContainerAnalogSignalOracle` | 15 个容器布局 | **全部一致** | 容器红石信号（比较器读箱子）。三个易错点：每格按**各自的 `maxStackSize`** 归一化而不是统一按 64、`f > 0` 才 +1 所以非空最低为 1 而空箱为 0、先逐格累加再整体除以槽位数。**修复**：空桶 `maxStackSize` 16→1（原版不可堆叠）|
| `item_max_stack_size` | 原版 jar `ItemMaxStackSizeOracle` | 1505 个物品 | **全部一致** | 全部物品的 `maxStackSize`。上限是散落在上千处注册调用里的小整数（64/16/1），抄错不崩溃但影响玩法：本该 1 的会在一个格子叠一摞、本该 64 的让物品栏凭空多占格子、而比较器按 `count/maxStackSize` 折算时会让**信号强度整体偏移**。**修复**：`air` 64→1、`anvil`/`chipped_anvil`/`damaged_anvil` 1→64；并**回退了上一轮对空桶的误改**（原版空桶可堆叠 16，不是 1）|
| `item_container_fit` | 原版 jar `ItemContainerFitOracle` | 全部 1505 个物品 | **全部一致** | 物品能否放进容器类物品（`Item#canFitInsideContainerItems`）。`Item` 默认 `true`，`BlockItem` 覆写为**潜影盒返回 `false`**。它防的是**递归存储**：潜影盒能装物品，若潜影盒也能装潜影盒，就能做出无限嵌套容器——既是无限容量漏洞，也是存档体积炸弹。判据挂在 `BlockItem` 上而非按物品 id 白名单，所以新增颜色/材质变体自动被覆盖 |
| `block_light_emission` | 原版 jar `BlockLightEmissionOracle` | 29671 个方块状态 | **全部一致** | 方块状态的发光强度（`getLightEmission`）。与 `light_occlusion` 互补：后者量「光穿过这一格后还剩多少」，本条量「这一格自己发出多少光」。**修复 10 个方块**：铜灯 4 种氧化等级未区分（全 15，应 15/12/8/4，且 `CopperBulbBlock::getLightLevel` 硬编码 15 把属性完全遮蔽）；`trial_spawner` 全 0（应 inactive/cooldown=0、waiting=4、其余=8）；`vault` 全 6（应 inactive=6、激活后=12）。**注意**：状态的属性顺序两侧不同，必须先规范化排序再配对 |
| `block_hardness` | 原版 jar `BlockHardnessOracle` | 1166 个方块 | **全部一致** | 全部方块的硬度与爆炸抗性（`getDestroySpeed` / `getExplosionResistance`），f32 位模式比对。与 `block_destroy_progress` 互补：后者验公式，本条验公式的输入。**修复 200 处方块抗性**——分三类：漏写 `.resistance()`（软方块全为 0，黑曜石会被一颗 TNT 清空）、值写错（`fence_gate` 2→3、`trial_spawner`/`vault` 1200→50、铜栏杆 5→6、铜灯笼 6→3.5）、以及批量脚本自身造成的重复调用覆盖 |
| `block_map_color` | 原版 jar `BlockMapColorOracle` | 29671 个方块状态 | **全部一致** | 全部方块状态的地图颜色（`getMapColor`）。地图上每一格的颜色都由它决定，只影响地图渲染、不影响任何游戏逻辑，所以抄错了既不崩溃也不会让玩家觉得哪里不对。**修复**：此前 29671 个状态**全部返回 AIR(0)**——代码里那句 `TODO(map-color)` 描述的就是这件事，导致填充地图完全不渲染地表。现内嵌原版快照表（`BlockMapColorTable.gen.cpp`，按**状态**而非按方块记录，因为床/原木/含水方块的颜色随状态变化）|
| `block_friction` | 原版 jar `BlockFrictionOracle` | 1166 个方块 | **全部一致** | 移动手感三件套：摩擦（`getFriction`）、速度倍率（`getSpeedFactor`）、跳跃倍率（`getJumpFactor`），f32 位模式比对。三者**互相替代**必须一起比：冰改 `friction(0.98)`、蜂蜜块不改 friction 而改 `speedFactor(0.4)+jumpFactor(0.5)`、灵魂沙改 `speedFactor(0.4)` 但不改 jumpFactor。只比 friction 会漏掉蜂蜜块与灵魂沙的大半行为。**顺带补齐** `Block::speedFactor()` / `jumpFactor()` 访问器（此前只在 `BlockProperties` 上有）|
| `block_fire_info` | 原版 jar `BlockFireInfoOracle` | 1166 个方块 | **全部一致** | 可燃性参数（`FireBlock.setFlammable`）：`encouragement`（蔓延权重，木板 5、树叶 30）与 `flammability`（烧毁权重，木板 20、羊毛 60）。两者**独立**：树叶极易被点燃但烧尽速度和木板差不多。火焰传播是概率系统，抄错不崩溃、只会让火势「感觉不太对」。oracle 用反射读 `FireBlock` 的 private `igniteOdds`/`burnOdds`（只读），拿到的是**注册原始值**，不掺入原版 `waterlogged` 返回 0 的分支 |
| `block_solidity` | 原版 jar `BlockSolidityOracle` | 1166 个方块 | **全部一致** | 「可放置 / 可穿过」两态：`isSolid`（是否算实体方块，决定刷怪与视线）与 `canBeReplaced`（能否被直接覆盖）。**修复**：`Block::isSolid` 此前是注册期的一个独立布尔量，现对齐原版 `calculateSolid` 从碰撞形状推断（`forceSolidOn` → 真 / `forceSolidOff` → 假 / 否则看包围盒「三轴平均 ≥ 35/48 或整格高」）；`BlockState::isSolid` 改为实时委托（原先构造期缓存，门/台阶/栅栏门的形状还没设好就被判成非实体）；`notSolid()` 改为置 `forceSolidOff`；`replaceable` 不再从材质推导而是显式开关（原版全库仅 29 个方块可替换）；补齐 133 处方块缺失的 `forceSolidOn`（告示牌、旗帜、挂签、灯笼、紫水晶、蜘蛛网、竹笋等） |
| `block_instrument` | 原版 jar `BlockInstrumentOracle` | 1166 个方块 | **全部一致** | 音符盒放在方块上方时演奏的乐器（`BlockStateBase#instrument()`）。石头类给大鼓、沙类给小军鼓、玻璃给踩镲、木类给贝斯、泥土给长笛、金块给钟、羊毛给吉他、冰给风铃、骨块给木琴、铁块给铁琴、灵魂沙给牛铃、南瓜给迪吉里杜管、绿宝石块给电子音、干草块给班卓琴、萤石给电钢琴，头颅模仿生物叫声，其余竖琴。**修复**：644 处方块根本没设 `.instrument()`（全退回竖琴），以及 `SkullBlock` 在基类 `AbstractSkullBlock` 里设乐器导致**墙挂头颅**也演奏生物叫声（原版只有站立头颅带乐器） |
| `noteblock_pitch` | 原版 jar `NoteBlockPitchOracle` | 25 个音符值 | **全部一致** | 音符盒的调音：`getPitchFromNote(note)` = `2^((note-12)/12)` 按 **f32 位模式**比对（中间用 f32 还是 f64 会让结果差 1 ulp，人耳听不出来但会让包络计算逐帧漂移），以及 `NOTE_0_24` 属性的 `cycle(NOTE)` 回绕点（0..24，写错会让某些音根本弹不出来）。**注**：探针曾输出十进制 `pitchValue`，Gson 把 f32 展开成 double 会制造伪差异，已删除、只留位模式 |
| `noteblock_trigger` | 原版 jar `NoteBlockTriggerOracle` | 23 种乐器 × 3 个音符值 = 69 | **全部一致** | 音符盒被触发时播放的**音效三元组**：音效 id、音量（恒 3.0）、音高（可调音乐器走 `getPitchFromNote`，头颅类固定 1.0），以及音符粒子（只有可调音乐器发，颜色偏移 `note/24`）。绑定在 `Block::triggerEvent` 上：原版正是靠方块事件广播让附近**所有**玩家都听到，直接调播放函数只有发起方出声。**修复**：`NoteBlock` 根本没覆写 `triggerEvent`（播放走私有路径、绕过事件系统）；`custom_head` 取不到自定义音效时被兜底成 `ui.button.click`，而原版是**一声不出** |
| `bell_placement` | 原版 jar `BellPlacementOracle` | 6 种点击面 × 4 种朝向 × 16 种四邻坚固性 = 384 | **全部一致** | 钟被人字放置时推导出的**附着方式**（`BellBlock#getStateForPlacement`）：点顶面立在地上（floor）、点底面吊在天花板（ceiling）、点侧面且对向两侧都是墙就跨在两面墙之间（double_wall），否则单面挂（single_wall）。四种的碰撞形状与敲击范围都不同。**修复**：原版在四种挂法都不可存活时 `return null`（放置取消、**不消耗物品**），Cubium 退回 `defaultState()`，于是点底面而上方是空气时钟照挂不误 |
| `bell_ring` | 原版 jar `BellRingOracle` | 4 种附着方式 × 4 种朝向 × 6 种点击面 × 5 个高度 = 480 | **全部一致** | 敲钟的「有效敲击」判定（`BellBlock#isProperHit`）：点击面在 Y 轴永远不响、点击高度 > **0.8124** 永远不响（敲钟帽不响），floor 要求点击面与钟朝向**同轴**，墙钟要求**不同轴**，ceiling 任何水平面都响。判定错不崩溃、不掉日志，只会让某些角度敲不响。**顺带修复**：`attemptToRing` 发的是 `BLOCK_ACTIVATE` 而非原版的 `BLOCK_CHANGE`，幽匿感测体按事件 id 区分响应，发错会让钟声在幽匿电路里被当成别的事件 |
| `jukebox_insert` | 原版 jar `JukeboxInsertOracle` | 2 个固定样本 + 21 张唱片 × 2 态 = 46 | **全部一致** | 唱片机的交互判定（`JukeboxPlayable#tryInsertIntoJukebox` 的分支表）：只有 insert / eject / try_empty_hand 三种结果。陷阱是**顺序**——「机器里有唱片」优先于「手里拿什么」，拿着另一张唱片点也是先弹旧的，写反了旧唱片会凭空消失。**顺带修复**：`useItemOn` 塞入唱片后漏发 `BLOCK_CHANGE` 游戏事件，唱片机在幽匿电路里完全隐形 |
| `lever_game_event` | 原版 jar `LeverGameEventOracle` | 拨动前 POWERED 两态 = 2 | **全部一致** | 拨动拉杆时发出的**游戏事件**与音效（`LeverBlock#play`）：开启发 `block_activate`、关闭发 `block_deactivate`，音效恒为 `block.lever.click`（音量 0.3，音高开 0.6 / 关 0.5）。游戏事件是幽匿感测体、尖啸体、监守者、悦灵这些「听声音」机制的唯一输入——**漏发不会崩溃、不会掉日志**，方块状态和音效都正常，只有红石侦测端完全没反应。**修复**：拉杆原来用的是木质按钮音（注释还写着「拉杆使用木质按钮音效」），音高也不随状态变化 |
| `note_block_event` | 原版 jar `NoteBlockEventOracle` | 23 种乐器 × 上方是否被堵 = 46 | **全部一致** | 音符盒**响不响、发不发事件**（`NoteBlock#playNote`）。前置条件是「乐器 above-note-block 生效 **或** 上方是空气」：头颅类乐器不受上方方块影响，其余乐器上方必须为空，否则**既不出声也不发 `NOTE_BLOCK_PLAY` 事件**——事件必须发在判定之后。**修复**：音符盒完全没有这个前置条件（上方埋了方块照样响）、也完全不发 `NOTE_BLOCK_PLAY` 事件。另顺带补齐按钮与压力板的 `BLOCK_ACTIVATE`/`BLOCK_DEACTIVATE` |
| `piston_event` | 原版 jar `PistonEventOracle` | 2 种活塞 × 6 个朝向 = 12 | **全部一致** | 活塞**缩回**时的音效与游戏事件（`PistonBaseBlock#move`）：缩回放 `block.piston.contract`（音量 0.5）并发 `BLOCK_DEACTIVATE`，伸出是 `extend` + `BLOCK_ACTIVATE`。音高带随机（`random*0.15+0.6`）让连续伸缩的声音不完全一致。**修复**：Cubium 的活塞**完全无声也完全不发事件**——`BLOCK_PISTON_EXTEND`/`BLOCK_PISTON_CONTRACT` 常量存在但全库无一处调用，活塞在幽匿电路里完全隐形。仅覆盖缩回：伸出要走 `PistonStructureHelper` 计算推动链 |
| `cake_eat` | 原版 jar `CakeEatOracle` | 吃之前的 BITES 0..6 = 7 | **全部一致** | 吃一片蛋糕时发出的游戏事件（`CakeBlock#eat`）。事件是**叠加**的：每一片都发 `eat`，若是**最后一片**（BITES 已是 6）额外发 `block_destroy`。只写 `EAT` 会让最后一片消失时幽匿电路少一次 destroy，只写 `DESTROY` 则前六片完全静默。**顺带修复**：蛋糕原来一个事件都不发；同批还补了营火浸灭（`dowse`）与堆肥桶（加料/成熟/清空三处）的 `BLOCK_CHANGE` |
| `lectern_book` | 原版 jar `LecternBookOracle` | 初始 HAS_BOOK × POWERED × 目标 = 8 | **全部一致** | 讲台放书/取书时的状态复位与事件（`LecternBlock#resetBookState`）：`POWERED` **一律复位为 false**、`HAS_BOOK` 置为目标值、发 `BLOCK_CHANGE`。讲台翻页时会拉高一次 `POWERED` 当比较器脉冲，放书取书不清掉的话，取走书之后下游比较器会一直读到不该存在的信号。**顺带修复**：讲台原来是状态对但**不发事件**；同批还补了日光探测器切换昼夜模式（`toggleMode`）的 `BLOCK_CHANGE`——注意功率随光照变化更新时原版**不发**事件 |
| `tripwire_state` | 原版 jar `TripwireStateOracle` | 变化前 powered/attached × 变化后 = 16 | **全部一致** | 绊线钩状态变化时的音效与事件（`TripWireHookBlock#emitState`）：未通电→通电 `click_on`(0.6)+`block_activate`；通电→未通电 `click_off`(0.5)+`block_deactivate`；未挂→挂上 `attach`(0.7)+`block_attach`；挂上→未挂 `detach`(`1.2/(random*0.2+0.9)`)+`block_detach`；**都不变时完全静默**。四分支**互斥且按优先级**——通电/断电优先于挂上/脱开，写成并列四个 if 会让一次扯断连发两个事件两个音效。音量恒 0.4（f32 位模式比）。**修复**：绊线钩原来四个音效常量都定义了却**一处都没调用**，也完全不发事件 |
| `farmland_decay` | 原版 jar `FarmlandDecayOracle` | 耕地 MOISTURE 0..7 = 8 | **全部一致** | 耕地退化时的方块替换与事件（`FarmBlock#turnToDirt`）：被踩踏、缺水、上方被盖住三条路径都走这里——换方块 + 发 `block_change`。换方块与发事件是两件事，幽匿感测体只认 `gameEvent`，不发就完全感知不到耕地被踩坏。退化与湿度无关，干湿都退化。**顺带修复**：`turnToDirt` 原来只换方块不发事件；同批还补了蜡烛熄灭（`AbstractCandleBlock#extinguish`，注意点亮时原版**不发**）与南瓜剪刀雕刻（`SHEAR` 而非 `BLOCK_CHANGE`——幽匿感测体把「剪」单独归类） |
| `tripwire_disarm` | 原版 jar `TripwireDisarmOracle` | 主手物品 3 种 = 3 | **全部一致** | 用剪刀拆绊线时的**解除武装**（`TripWireBlock#playerWillDestroy`）：手里是剪刀时先把绊线置成 `DISARMED` 再移除，两端钩子看到「已解除」走**拆除**路径而不是**触发**路径，并发 `shear` 事件。不置 `DISARMED` 的话，用剪刀拆陷阱反而会把它**引爆一次**——正好是剪刀存在的意义（安全拆除）的反面。**修复**：绊线网原来根本没覆写 `playerWillDestroy`；同批还补了蜂巢采集的 `shear`（剪刀取蜜脾）与 `fluid_pickup`（玻璃瓶装蜂蜜） |
| `berry_growth` | 原版 jar `BerryGrowthOracle` | AGE 0..3 = 4 | **全部一致** | 甜浆果丛生长时的阶段推进与事件（`SweetBerryBushBlock#randomTick`）：随机刻生长与骨粉催熟都让 `AGE+1` 并发 `block_change`。**AGE 已是 3 时不再生长、也不发事件**——满阶段还发事件的话，一丛浆果能让附近幽匿感测体每 tick 醒一次。**顺带修复**：浆果丛的生长、催熟、采摘三条路径都不发事件；同批还补了海龟蛋的裂纹（`BLOCK_CHANGE`）与孵化（`BLOCK_DESTROY`）——注意裂纹还没破壳所以是 CHANGE 不是 DESTROY |
| `anchor_charge` | 原版 jar `AnchorChargeOracle` | CHARGES 0..4 × 动作 = 10 | **全部一致** | 重生锚充能/放电时的层数变化与事件（`RespawnAnchorBlock#charge`）：充能 `+1` 上限 4、放电 `-1` 下限 0，都发 `block_change`；**已满或已空时不变也不发事件**。已达上限还继续加会让层数溢出到 5，光照等级 `charges*3.75` 算出 18，超出亮度上限。**顺带修复**：重生锚的 `charge`/`discharge` 都不发事件；同批还补了发射器的**空响**事件——注意原版只在**没有物品可发**时发 `BLOCK_ACTIVATE`，真正发射物品时反而不发，这个反直觉的点正是最容易写错的地方 |
| `vibration_frequency` | 原版 jar `VibrationFrequencyOracle` | 全部 40 个有频率的事件 | **全部一致** | 游戏事件到振动频率的映射（`VibrationSystem.VIBRATION_FREQUENCY_FOR_EVENT`）：把事件归入 1..15 频段，频段决定红石比较器输出强度与紫水晶共振方块的 `RESONATE_N` 事件，表里没有的事件频率为 0 不产生振动。**分组整体偏移不会崩也不会报错**，只是所有振动装置读数同时「差不多」地错掉——最难靠手动测试发现的一类缺陷。**修复**：Cubium 原来把 `block_*` 事件集中到低频、把落地/溅水归到 12，**40 条里 35 条与原版不符**（如 `block_change` 原版 11 却给了 7、`shear` 原版 6 却给了 10、`hit_ground` 原版 2 却给了 12）；现按原版逐条重建全表 |
| `vibration_strength` | 原版 jar `VibrationStrengthOracle` | 监听半径 × 距离 = 120 | **全部一致** | 振动距离到红石强度的换算（`VibrationSystem#getRedstoneStrengthForDistance`）：`max(1, 15 - floor(15/radius * distance))`，是幽匿感测体给红石比较器的**唯一**输出值。三个易错点：**除号内外**（整数截断两次会差 1）、**漏掉 `max(1,...)` 下限**（远处给出 0 而非弱信号，比较器读成「无信号」）、**距离为 0 必须给 15**。三个错都不崩不报错，只是读数差 1，而依赖精确读数的装置会整体失效 |
| `vibration_info_nbt` | 原版 jar `VibrationInfoNbtOracle` | 字段组合 4 种 | **全部一致** | 振动信息的 NBT 结构（`VibrationInfo.CODEC`）：`game_event`(string) / `distance`(float) / `pos`(double[3]) 必填，`source` 与 `projectile_owner` 是可选 **UUID**。**UUID 按原版 `UUIDUtil.CODEC` 拆成 4 个有符号 int，不是单个 long**——写成 long 会让存档与网络包都和原版对不上。**修复**：Cubium 的 `source` 原来是 `long`、`projectile_owner` **整个缺失**（且 `VibrationInfo.hpp` 的注释把两者都写成 `long`，属于按错误规格实现）；现已改为 `std::optional<Uuid>` 并按 int 数组读写，源实体查找同步改为 `getEntityByUuid` |
| `vibration_selector` | 原版 jar `VibrationSelectorOracle` | 事件 × 距离 × 时刻 = 24 | **全部一致** | 幽匿感测体的振动选优（`VibrationSelector#shouldReplaceVibration`）：同一时刻收到多个振动时要挑一个——**距离近的优先**，距离相同再比**频率高的优先**，不同时刻**一律不替换**。三条规则顺序不可颠倒：先比频率再比距离，远处一次爆炸（频率 15）会盖掉近处的脚步声（频率 1），玩家走过去完全无反应；允许跨 tick 替换则高频事件不断插队，低频事件永远得不到处理 |
| `sculk_sensor_step` | 原版 jar `SculkSensorStepOracle` | 相位 × 踩踏者 = 9 | **全部一致** | 幽匿感测体被踩踏时是否发出 `STEP` 振动（`SculkSensorBlock#stepOn`）。三个前置条件缺一不可：**相位为 Inactive**（否则站上去不走也会一直触发，感测体变压力板）／**踩踏者不是监守者**（同源不该自激）／**踩踏者不在潜行**（潜行绕过感测体是核心玩法）。**修复**：Cubium 原来**根本没有覆写 `onEntityWalk`**，踩踏振动完全缺失。实现走「发游戏事件」路线而非直连振动系统，`common` 层不必下转 `ServerWorld`，与幽匿尖啸体的踩踏实现一致 |
| `brushable_fall` | 原版 jar `BrushableFallOracle` | 可疑沙 / 可疑沙砾 = 2 | **全部一致** | 可疑沙/沙砾坠落碎开时的效果与事件（`BrushableBlock#onBrokenAfterFall`）：放**方块破碎粒子**（`levelEvent 2001`）+ 发 **`block_destroy`** 事件，两个动作缺一不可。只发粒子不发事件，坠落破碎在幽匿电路里隐形；只发事件不放粒子，玩家看不到摔碎的视觉反馈。**修复**：Cubium 的 `BrushableBlock` 虽继承 `FallingBlock` 却**没有覆写 `onBroken`**，两者都没有。注：Cubium 把原版的 `onBroken` 与 `onBrokenAfterFall` 合并成一个 `onBroken`，按调用点（`_handleLanding`）判断等价于后者 |
| `vibration_selector_nbt` | 原版 jar `VibrationSelectorNbtOracle` | 无候选 / 有候选 = 2 | **全部一致** | 振动选择器的 NBT 结构与往返（`VibrationSelector.CODEC`）：只有 `event`（可选 `VibrationInfo` 嵌套）与 `tick`（long）两个字段。**无候选时 `tick` 必须写 -1 而不是省略字段**（原版 `orElse(-1L)`，字段存在但值是哨兵），否则「读取失败」与「没有候选」语义混淆；**`event` 存在但 `tick < 0` 时不能恢复候选**，两字段是一对，只信一个会把损坏存档当成有效候选 |
| `block_blast_resistance` | 原版 jar `BlockBlastResistanceOracle` | 全部 1166 个方块 | **全部一致** | 方块默认状态的爆炸抗性（`Block#getExplosionResistance`）：爆炸破坏时每个方块按抗性算是否被摧毁，阈值约等于 `威力/抗性`。写错不会崩也不会报错，只表现为「TNT 炸不动这堵墙」或「炸得比预期干净」。原版很多方块的抗性**不来自自身属性而是转交底层方块**（楼梯/台阶/活板门/栅栏/墙都 `return base.getExplosionResistance()`），这类继承关系最易出错；Cubium 虽有 `StairBlock` 等转交实现，实际值逐方块一致 |
| `block_movement_factor` | 原版 jar `BlockMovementFactorOracle` | 全部 1166 个方块 | **全部一致** | 方块的速度倍率与跳跃倍率（`Block#getSpeedFactor` / `getJumpFactor`）：速度倍率决定站上去走得多快，跳跃倍率决定起跳初速度。**灵魂沙与蜂蜜块都是 0.4，但只有蜂蜜块的跳跃倍率是 0.5**——灵魂沙只减速不限制跳跃。把蜂蜜块的跳跃倍数写成 1.0，玩家能轻松跳出蜂蜜陷阱；把灵魂沙的写成 0.5，沙漠神殿的灵魂沙跑酷就几乎不可能。与摩擦系数是**两套独立机制**（摩擦决定是否打滑，倍率决定走得多快） |
| `block_shade_brightness` | 原版 jar `BlockShadeBrightnessOracle` | 全部 1166 个方块 | **全部一致** | 方块遮光亮度与天空光透传（`BlockBehaviour#getShadeBrightness` / `propagatesSkylightDown`）：碰撞形状是完整方块返回 **0.2**（形成阴影），否则 **1.0**。**修复 3 类系统性缺陷**：① 基类判据误用 `canOcclude`，正确判据是「碰撞形状是否覆盖整格」，导致 163 个栅栏/台阶/楼梯/墙被误判成 0.2、37 个树叶/冰/荧石被误判成 1.0；② 灵魂沙、泥巴是 14/16 矮块却需 0.2，缺覆写；③ 玻璃/染色玻璃/铜格栅/遮光玻璃碰撞形状整格却不该投影（原版靠 `TransparentBlock` 统一处理），缺覆写；另补藤蔓 `propagatesSkylightDown` |
| `block_random_ticks` | 原版 jar `BlockRandomTicksOracle` | 全部 29671 个状态 | **全部一致** | 方块状态是否响应随机刻（`BlockBehaviour#isRandomlyTicking`）。随机刻是「自然发生」的引擎：每游戏刻随机挑一批位置，只对 `true` 的状态调 `randomTick`——作物生长、树叶腐烂、草方块蔓延、冰融化、铜氧化全靠它。设计上是方块级开关，但**有大量方块按状态覆写**：树叶只在 `distance=7 && !persistent`（已断联）时腐烂、作物在最大年龄后停止、紫颂花 `age<5`、可可果 `age<2`、下界疣 `age<3`、海带/藤蔓 `age<25`、竹子 `stage=0`、投手作物仅下半部分。**修复 314 处**：树叶与 12 类作物全部缺少覆写，导致成熟作物与所有树叶每刻都被无谓调度 |
| `block_signal_source` | 原版 jar `BlockSignalSourceOracle` | 全部 29671 个状态 | **全部一致** | 方块状态是否红石信号源（`BlockBehaviour#isSignalSource` / Cubium 的 `canProvidePower`）。为 true 的方块会被红石粉当作**强充能**来源（强度 15），参与比较器/中继器与相邻活塞、门、铁轨的判定。最容易写错的是那些**看起来该是信号源却不是**的方块：**铁轨**（只自己变形状/启动矿车，不向周围输出）、**命令方块**（只向比较器输出成功执行次数）、**合成器**、**信标**（「输出金字塔等级」是常见误解）、**幽匿催化体**——这些「不是」是原版刻意的设计 |
| `block_replaceable` | 原版 jar `BlockReplaceableOracle` | 全部 29671 个状态 | **全部一致** | 方块状态是否可被替换（`BlockStateBase#canBeReplaced`）。它描述「这一格现在算不算空」：**直接放置、流体流动、世界生成、掉落方块**都靠它。四处容易写反：草/蕨/海草必须 `true`（`false` 则玩家在草上放不了方块）；**树叶必须 `false`**（`true` 则被后来生成的树覆盖，森林一片片秃）；**铁轨必须 `true`**（`false` 则放不了方块覆盖轨道）；**耕地必须 `false`**（`true` 则农田被无意填平）。注意与 `canBeReplacedByFluid` **不是一回事**：后者对非固体方块放宽（门、告示牌虽不可替换但水能流进去），不能混用同一判定 |
| `block_redstone_conductor` | 原版 jar `BlockRedstoneConductorOracle` | 全部 29671 个状态 | **全部一致** | 方块状态是否红石导体（`BlockBehaviour#isRedstoneConductor`）。红石粉向四周传播时会穿过导体方块向下找信号源，非导体则不会，同时影响火把/拉杆/中继器/观察者/活塞的充能判定。原版默认判据是「碰撞形状完整覆盖整格」，再由 22 处 `isRedstoneConductor` 逐个覆盖（绝大多数是 `never`）。**修复 98 处（35 种方块）**：形状判据虽然覆盖了 1228 种方块中的绝大多数，但有 21 种「形状完整却被显式排除」（**红石块**、活塞、观察者、TNT、铜灯、铜格栅）与 14 种「形状不完整却是默认导体」（紫水晶、屏障、蓝冰、织布机、**灵魂沙**、幽匿、泥巴、苔藓），已改为显式覆盖表 |
| `block_pathfindable` | 原版 jar `BlockPathfindableOracle` | 全部 29671 个状态 × 3 种类型 = 89013 | **差分收敛中（48371 → 9647）** | 方块状态在三种寻路类型下是否可通行（`BlockBehaviour#isPathfindable`）：LAND=陆地行走、WATER=水中游动、AIR=空中飞行。原版默认判据：**LAND/AIR 为「碰撞形状不完整即可通行」**，**WATER 为「只认水」**。两处最容易写错：① 判据是「不完整」而不是「为空」—— 台阶、楼梯、栅栏门碰撞形状非空但不完整，用 `isEmpty()` 会让生物绕开所有台阶（表现为村民卡在台阶前不动）；② WATER 不看形状只看是不是水 —— 三种类型统一按形状判，陆行生物会走进水里。原版还有 **60 个类覆写**该方法（墙、楼梯、栅栏、头颅、箱子等返回 `false`）。**已修复基类默认实现与 9 个类族的覆写（StairsBlock/WallBlock/AbstractSkullBlock/FenceBlock/CampfireBlock/PaneBlock/ChorusPlantBlock/SculkSensorBlock/ShelfBlock），剩余 435 种方块的长尾待继续收敛** |
| `block_light_occlusion_shape` | 原版 jar `BlockLightOcclusionShapeOracle` | 全部 29671 个状态 | **全部一致** | 方块状态是否按形状决定光照遮挡（`BlockBehaviour#useShapeForLightOcclusion`）。光照引擎判断方块是否挡光有两种口径：`false`（默认）**只看 `canOcclude`**，与具体形状无关；`true` **按形状实际包围盒累加遮挡**，没占满就不挡光。台阶、楼梯、耕地、土径、雪、泥巴、灵魂沙这类**半格或略矮**的方块必须是 `true`——写成 `false` 会让**台阶下方一片漆黑**、室内光照蔓延范围偏小；反向写成 `true` 则让整格不透明但形状略小的方块在其上方**凭空透光**。此标志**不影响实体碰撞，只影响光照**，所以错了不会撞穿，只会「暗得没道理」 |
| `block_skylight_propagation` | 原版 jar `BlockSkylightPropagationOracle` | 全部 29671 个状态 | **全部一致** | 方块状态是否向下透传天空光（`BlockBehaviour#propagatesSkylightDown`）。天空光从世界顶部往下传播时，需要判断光能否直接穿过这一格；该值同时决定 `getLightBlock` 的返回值（透光则为 0，否则为 1）。原版默认：`!isShapeFullBlock(shape) && fluidState.isEmpty()`。两个容易写错的地方：① **必须同时检查形状与流体**——只判形状会让水透光（海底一片明亮），只判流体会让台阶、栅栏、楼梯全部挡光（白天进屋一片漆黑）；② **判据是「形状非整格」而不是「碰撞形状为空」**——树叶、草丛、栅栏、告示牌的碰撞形状都不是整格，它们都应当透光，若用「碰撞为空」会让树叶下方凭空变暗（森林地面收不到天空光）。**修复 1 处**：`pale_moss_carpet[bottom=false]` 缺 `propagatesSkylightDown` 覆写（原版恒返回 `true`），基类把「没有底座的空状态」判为挡光 |
| `block_pathfindable` | 原版 jar `BlockPathfindableOracle` | 全部 29671 个状态 × 3 种类型 = 89013 | **88967/89013 一致；46 处已登记缺口** | 方块状态在三种寻路类型下是否可通行（`BlockBehaviour#isPathfindable`）：LAND=陆地行走、WATER=水中游动、AIR=空中飞行。原版默认判据：**LAND/AIR 为「碰撞形状不完整即可通行」**，**WATER 为「只认水」**。两处最容易写错：① 判据是「不完整」而不是「为空」—— 台阶、楼梯、栅栏门碰撞形状非空但不完整，用 `isEmpty()` 会让生物绕开所有台阶（表现为村民卡在台阶前不动）；② WATER 不看形状只看是不是水 —— 三种类型统一按形状判，陆行生物会走进水里。**修复 48000+ 处**：基类默认实现两处语义错误 + 缺失 **60 个类**的覆写（楼梯、墙、栅栏、栅栏门、门、活板门、钟、箱子、书架、头颅、玻璃板、营火、紫颂、幽匿传感器、铜制锁链/灯笼/栏杆、台阶、花盆、炼药锅、蜡烛蛋糕、铁砧、竹子、可可、砂轮、末影箱、漏斗、海泡菜、堆肥桶、酿造台、末地传送门框、耕地、土径、龙蛋、附魔台、重型核心、切石机、潮涌核心、讲台、蛋糕、活塞、灵魂沙、泥巴、细雪、避雷针、床、仙人掌、杜鹃等）。另修复 `SlabBlock::getFluidState` 错误地让 `[type=double,waterlogged=true]` 不返回水。已知缺口：`decorated_pot`（40）、`sniffer_egg`（6）—— Cubium 无对应方块类 |
| `block_shape` | 原版 jar `BlockShapeOracle` | 1166 个方块 × 三种形状 | **全部一致** | 体素化 16³ 输出三种形状；逐盒输出会误报盒子拆分差异，必须体素化 |
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
| `density_nested` | 原版 jar `DensityNestedOracle` | 17 棵树 × 10 个采样点 | 全部一致 | 嵌套密度函数树，区间传播与短路语义 |
| `carving_mask` | 原版 jar `CarvingMaskOracle` | 7 个场景 | 全部一致 | 雕刻掩码的索引公式与三种维度几何 |
| `shape_collide` | 原版 jar `ShapeCollideOracle` | 19 个场景 | 全部一致 | 实体与方块形状的碰撞求解（三轴） |
| `edge_backoff` | 原版 jar `EdgeBackoffOracle` | 12 个场景 | 全部一致 | 潜行边缘保护的分轴回退 |
| `item_cooldowns` | 原版 jar `ItemCooldownsOracle` | 14 个操作序列 | 全部一致 | 物品冷却进度与到期移除 |
| `inventory_slot_mapping` | 原版 jar `InventorySlotMappingOracle` | 94 个槽位互转 | 全部一致 | 背包内部索引 ↔ 菜单槽位 |
| `inventory_add` | 原版 jar `InventoryAddOracle` | 14 个装填场景 | 全部一致 | 物品进包的合并与铺开顺序 |
| `fall_damage` | 原版 jar `FallDamageOracle` | 109 个距离×倍率 | 全部一致 | 摔落伤害公式 |
| `surface_rule` | 原版 jar `SurfaceRuleOracle` | 684 个条件判定 | 全部一致 | 地表规则的水面/石头深度/Y 条件 |
| `block_spawn_query` | 原版 jar `BlockSpawnQueryOracle` | 全部 29671 个方块状态 | 待重跑确认 | 四条生成判据：碰撞满格 / 红石导体 / 信号源 / 支撑面；后三条不能当成「碰撞形状满格」 |
| `block_view_blocking` | 原版 jar `BlockViewBlockingOracle` | 全部 29671 个方块状态 | 待重跑确认 | 挡视线（第三人称相机拉近）与窒息（持续掉血）；树叶、玻璃等数十处按注册覆写 |
| `dig_speed` | 原版 jar `DigSpeedOracle` | 32 个输入组合 | 待重跑确认 | 挖掘速度公式，f32 位模式比对；出错的是「多个因子同时在场」的组合 |
| `entity_physics` | 原版 jar `EntityPhysicsOracle` | 48 个移动 tick 组合 | 待重跑确认 | 一次移动 tick 的速度更新，f32 位模式比对；摩擦/重力/滑度/缓降的组合 |
| `timeline_sample` | 原版 jar `TimelineSampleOracle` | 10 条轨道 × 采样时刻 | 待重跑确认 | 关键帧轨道采样；`loopTicks` 用 `floorMod`，负数必须回绕到周期末尾 |
| `ray_aabb` | 原版 jar `RayAabbOracle` | 36 个盒 × 起点 × 方向 | 待重跑确认 | `Mth.rayIntersectsAABB`；前两次叉积判定用 `>`、最后一次用 `<`，不对称是关键 |
| `jigsaw_joint_order` | 原版 jar `JigsawJointOrderOracle` | 6 个模板 × 旋转 × 种子 | 待重跑确认 | 拼图连接点洗牌顺序；随机数消耗差一就让整棵结构分叉 |
| `cubic_spline` | 原版 jar `CubicSplineOracle` | 11 条样条 × 采样值 | 待重跑确认 | 三次 Hermite 样条；全零导数是线性插值，非零导数走真样条 |
| `density_composite` | 原版 jar `DensityCompositeOracle` | 3 类组合（Add/Mul/Min/Max/Lerp/RangeChoice） | 待重跑确认 | 子函数全用 `Constant`，整棵树由单元名参数量化重建 |
| `density_function` | 原版 jar `DensityFunctionOracle` | 3 类纯函数 × 采样点 | 待重跑确认 | 只覆盖不依赖噪声的密度函数；噪声类见 `noise_density` 等 |
| `noise_density` | 原版 jar `NoiseDensityOracle` | 11 条配置 × 采样点 | 待重跑确认 | `NoiseDensity` 与 `MappedNoise`；缩放取非 2 的幂以暴露舍入 |
| `perlin_noise` | 原版 jar `PerlinNoiseOracle` | 9 条配置 × 12 个采样点 | 待重跑确认 | 单/多倍频与真实地形的振幅链；整数点验端点、小数点压三线性插值内部 |
| `normal_noise` | 原版 jar `NormalNoiseOracle` | 8 条配置 × 采样点 | 待重跑确认 | 双 fork 叠加与 `expectedDeviation` 归一化；`octaveRange` 只统计非零振幅 |
| `shift_noise` | 原版 jar `ShiftNoiseOracle` | 12 种 ShiftA/ShiftB/Shift 形态 × 采样点 | 待重跑确认 | 顶点位移噪声，坐标乘 0.25 后取值再乘 4 |
| `shifted_noise` | 原版 jar `ShiftedNoiseOracle` | 8 种位移形态 × 采样点 | 待重跑确认 | 坐标缩放后叠加位移密度函数再采样噪声 |
| `weird_scaled_sampler` | 原版 jar `WeirdScaledSamplerOracle` | 3 类 rarity 映射 × 采样点 | 待重跑确认 | `value < 阈值` 是严格小于；输入刻意取阈值与其两侧 |
| `end_islands` | 原版 jar `EndIslandsOracle` | 3 个种子 × 采样点 | 待重跑确认 | 末地主密度函数；采样点覆盖中心区与 25×25 邻域的外岛分支 |
| `lerp3_interpolate` | 原版 jar `Lerp3InterpolateOracle` | 7 组八角色值 × 插值因子 | 待重跑确认 | 区块噪声三线性插值，顺序 X→Y→Z；任何一步被融合成 FMA 都会偏若干 ulp |
| `mth_atan2` | 原版 jar `MthAtan2Oracle` | 33 个角度用例 | 32 一致，2 处已登记 | 查表近似；`1|1` 与 `-1|1` 为平台 `std::cos` 差 1 ULP |
| `mth_map_lerp` | 原版 jar `MthMapLerpOracle` | 35 个整数插值/映射用例 | 待重跑确认 | `lerpInt` 乘 span、`lerpDiscrete` 乘 span-1 再对正值补一格 |
| `mth_misc` | 原版 jar `MthMiscOracle` | 7 类函数的取值范围组合 | 待重跑确认 | `murmur` 无符号右移、`invCube` 向零取整、`sign` 对负零/NaN、`quantize` 向下取整、`square` 保留符号 |

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

[`aabb_ops`](../../docs/test/diff/aabb_ops.md)、[`aabb_contains`](../../docs/test/diff/aabb_contains.md)、[`fluid_height`](../../docs/test/diff/fluid_height.md)、[`mth_inverse_lerp`](../../docs/test/diff/mth_inverse_lerp.md)、[`mth_angle_lerp`](../../docs/test/diff/mth_angle_lerp.md)、[`mth_waveform`](../../docs/test/diff/mth_waveform.md)、[`mth_hsv`](../../docs/test/diff/mth_hsv.md)、[`argb_ops`](../../docs/test/diff/argb_ops.md)、[`direction_nearest`](../../docs/test/diff/direction_nearest.md)、[`mth_seed`](../../docs/test/diff/mth_seed.md)、[`mth_pack_degrees`](../../docs/test/diff/mth_pack_degrees.md)、[`mth_degrees`](../../docs/test/diff/mth_degrees.md)、[`mth_frac`](../../docs/test/diff/mth_frac.md)、[`mth_sqrt`](../../docs/test/diff/mth_sqrt.md)、[`string_util`](../../docs/test/diff/string_util.md)、[`identifier_parse`](../../docs/test/diff/identifier_parse.md)、[`identifier_path`](../../docs/test/diff/identifier_path.md)、[`blockpos_iterate`](../../docs/test/diff/blockpos_iterate.md)、[`blockpos_spiral`](../../docs/test/diff/blockpos_spiral.md)、[`aabb_factory`](../../docs/test/diff/aabb_factory.md)、[`front_and_top`](../../docs/test/diff/front_and_top.md)、[`structure_orient`](../../docs/test/diff/structure_orient.md)、[`bounding_box_encap`](../../docs/test/diff/bounding_box_encap.md)、[`chunk_region`](../../docs/test/diff/chunk_region.md)、[`portal_relative`](../../docs/test/diff/portal_relative.md)、[`relative_mask`](../../docs/test/diff/relative_mask.md)、[`position_move_rotation`](../../docs/test/diff/position_move_rotation.md)、[`style_apply`](../../docs/test/diff/style_apply.md)、[`text_color`](../../docs/test/diff/text_color.md)、[`click_event`](../../docs/test/diff/click_event.md)、[`hover_event`](../../docs/test/diff/hover_event.md)、[`identifier_resolve`](../../docs/test/diff/identifier_resolve.md)、[`plain_text_contents`](../../docs/test/diff/plain_text_contents.md)、[`mth_absmax`](../../docs/test/diff/mth_absmax.md)、[`display_slot`](../../docs/test/diff/display_slot.md)、[`objective_criteria`](../../docs/test/diff/objective_criteria.md)、[`team_rule`](../../docs/test/diff/team_rule.md)、[`boss_bar`](../../docs/test/diff/boss_bar.md)、[`sound_source`](../../docs/test/diff/sound_source.md)、[`difficulty`](../../docs/test/diff/difficulty.md)、[`anvil_durability`](../../docs/test/diff/anvil_durability.md)、[`banner_pattern`](../../docs/test/diff/banner_pattern.md)、[`beacon_effects`](../../docs/test/diff/beacon_effects.md)、[`brewing_tick`](../../docs/test/diff/brewing_tick.md)、[`dimension_type`](../../docs/test/diff/dimension_type.md)、[`direction_rotate`](../../docs/test/diff/direction_rotate.md)、[`enchantment_provider`](../../docs/test/diff/enchantment_provider.md)、[`enchantment_selection`](../../docs/test/diff/enchantment_selection.md)、[`entity_attributes`](../../docs/test/diff/entity_attributes.md)、[`entity_dimensions`](../../docs/test/diff/entity_dimensions.md)、[`explosion_damage`](../../docs/test/diff/explosion_damage.md)、[`explosion_ray`](../../docs/test/diff/explosion_ray.md)、[`explosion_seen_grid`](../../docs/test/diff/explosion_seen_grid.md)、[`food_stats`](../../docs/test/diff/food_stats.md)、[`food_tick`](../../docs/test/diff/food_tick.md)、[`furnace_tick`](../../docs/test/diff/furnace_tick.md)、[`hopper_cooldown`](../../docs/test/diff/hopper_cooldown.md)、[`item_same_components`](../../docs/test/diff/item_same_components.md)、[`item_stackable`](../../docs/test/diff/item_stackable.md)、[`mob_effect_merge`](../../docs/test/diff/mob_effect_merge.md)、[`mth_modulo`](../../docs/test/diff/mth_modulo.md)、[`mth_rounding`](../../docs/test/diff/mth_rounding.md)、[`repair_cost`](../../docs/test/diff/repair_cost.md)、[`spawner_delay`](../../docs/test/diff/spawner_delay.md)、[	rial_spawner_config](../../docs/test/diff/trial_spawner_config.md)、[`vec3_math`](../../docs/test/diff/vec3_math.md)、[`blockpos_codec`](../../docs/test/diff/blockpos_codec.md)、[`axis_plane`](../../docs/test/diff/axis_plane.md)、[`mth_lerp`](../../docs/test/diff/mth_lerp.md)、[`mth_bit`](../../docs/test/diff/mth_bit.md)、[`mth_rot`](../../docs/test/diff/mth_rot.md)、[`mth_sin`](../../docs/test/diff/mth_sin.md)、[`mth_approach`](../../docs/test/diff/mth_approach.md)、[`region_sector`](../../docs/test/diff/region_sector.md)、[`blockpos_dist`](../../docs/test/diff/blockpos_dist.md)、[`enchantment_storage`](../../docs/test/diff/enchantment_storage.md)、[`collide_sweep`](../../docs/test/diff/collide_sweep.md)、[`shapes_boolean`](../../docs/test/diff/shapes_boolean.md)、[`fluid_spread`](../../docs/test/diff/fluid_spread.md)、[`fluid_spread_dir`](../../docs/test/diff/fluid_spread_dir.md)、[`blockpos_bfs`](../../docs/test/diff/blockpos_bfs.md)、[`scheduled_tick`](../../docs/test/diff/scheduled_tick.md)、[`random_tick_pos`](../../docs/test/diff/random_tick_pos.md)、[`mth_color`](../../docs/test/diff/mth_color.md)、[`block_transform`](../../docs/test/diff/block_transform.md)、[`bounding_box`](../../docs/test/diff/bounding_box.md)、[`enchantment_compat`](../../docs/test/diff/enchantment_compat.md)、[`section_pos`](../../docs/test/diff/section_pos.md)、[`level_height`](../../docs/test/diff/level_height.md)、[`chunk_pos`](../../docs/test/diff/chunk_pos.md)、[`mth_clamp`](../../docs/test/diff/mth_clamp.md)、[`chat_formatting`](../../docs/test/diff/chat_formatting.md)、[`dye_color`](../../docs/test/diff/dye_color.md)、[`equipment_slot`](../../docs/test/diff/equipment_slot.md)、[`heightmap_types`](../../docs/test/diff/heightmap_types.md)、[`mth_int_div`](../../docs/test/diff/mth_int_div.md)、[`direction_plane`](../../docs/test/diff/direction_plane.md)、[`rotation_info`](../../docs/test/diff/rotation_info.md)、[`blockpos_geom`](../../docs/test/diff/blockpos_geom.md)、[`vec3i_arith`](../../docs/test/diff/vec3i_arith.md)、[`fuel_values`](../../docs/test/diff/fuel_values.md)、[`recipe_book_category`](../../docs/test/diff/recipe_book_category.md)、[`text_formatting_color`](../../docs/test/diff/text_formatting_color.md)、[`text_formatting_name`](../../docs/test/diff/text_formatting_name.md)、[`style_tristate`](../../docs/test/diff/style_tristate.md)、[`team_rule_strings`](../../docs/test/diff/team_rule_strings.md)、[`render_type_strings`](../../docs/test/diff/render_type_strings.md)、[`uuid_strings`](../../docs/test/diff/uuid_strings.md)、[`game_type`](../../docs/test/diff/game_type.md)、[`map_decoration_type`](../../docs/test/diff/map_decoration_type.md)、[`block_state_enum_strings`](../../docs/test/diff/block_state_enum_strings.md)、[`block_state_enum_strings_2`](../../docs/test/diff/block_state_enum_strings_2.md)、[`block_state_enum_strings_3`](../../docs/test/diff/block_state_enum_strings_3.md)、[`boolean_property_index`](../../docs/test/diff/boolean_property_index.md)、[`enum_property_order`](../../docs/test/diff/enum_property_order.md)、[`enum_property_order_all`](../../docs/test/diff/enum_property_order_all.md)、[`face_sturdy`](../../docs/test/diff/face_sturdy.md)、[`world_border`](../../docs/test/diff/world_border.md)、[`easing_type`](../../docs/test/diff/easing_type.md)、[`mth_interpolation`](../../docs/test/diff/mth_interpolation.md)、[`legacy_random`](../../docs/test/diff/legacy_random.md)、[`custom_stats`](../../docs/test/diff/custom_stats.md)、[`sound_events`](../../docs/test/diff/sound_events.md)、[`particle_type`](../../docs/test/diff/particle_type.md)、[`shape_face`](../../docs/test/diff/shape_face.md)、[`block_shape`](../../docs/test/diff/block_shape.md)
[`aabb_ops`](../../docs/test/diff/aabb_ops.md)、[`aabb_contains`](../../docs/test/diff/aabb_contains.md)、[`fluid_height`](../../docs/test/diff/fluid_height.md)、[`mth_inverse_lerp`](../../docs/test/diff/mth_inverse_lerp.md)、[`mth_angle_lerp`](../../docs/test/diff/mth_angle_lerp.md)、[`mth_waveform`](../../docs/test/diff/mth_waveform.md)、[`mth_hsv`](../../docs/test/diff/mth_hsv.md)、[`argb_ops`](../../docs/test/diff/argb_ops.md)、[`direction_nearest`](../../docs/test/diff/direction_nearest.md)、[`mth_seed`](../../docs/test/diff/mth_seed.md)、[`mth_pack_degrees`](../../docs/test/diff/mth_pack_degrees.md)、[`mth_degrees`](../../docs/test/diff/mth_degrees.md)、[`mth_frac`](../../docs/test/diff/mth_frac.md)、[`mth_sqrt`](../../docs/test/diff/mth_sqrt.md)、[`string_util`](../../docs/test/diff/string_util.md)、[`identifier_parse`](../../docs/test/diff/identifier_parse.md)、[`identifier_path`](../../docs/test/diff/identifier_path.md)、[`blockpos_iterate`](../../docs/test/diff/blockpos_iterate.md)、[`blockpos_spiral`](../../docs/test/diff/blockpos_spiral.md)、[`aabb_factory`](../../docs/test/diff/aabb_factory.md)、[`front_and_top`](../../docs/test/diff/front_and_top.md)、[`structure_orient`](../../docs/test/diff/structure_orient.md)、[`bounding_box_encap`](../../docs/test/diff/bounding_box_encap.md)、[`chunk_region`](../../docs/test/diff/chunk_region.md)、[`portal_relative`](../../docs/test/diff/portal_relative.md)、[`relative_mask`](../../docs/test/diff/relative_mask.md)、[`position_move_rotation`](../../docs/test/diff/position_move_rotation.md)、[`style_apply`](../../docs/test/diff/style_apply.md)、[`text_color`](../../docs/test/diff/text_color.md)、[`click_event`](../../docs/test/diff/click_event.md)、[`hover_event`](../../docs/test/diff/hover_event.md)、[`identifier_resolve`](../../docs/test/diff/identifier_resolve.md)、[`plain_text_contents`](../../docs/test/diff/plain_text_contents.md)、[`mth_absmax`](../../docs/test/diff/mth_absmax.md)、[`display_slot`](../../docs/test/diff/display_slot.md)、[`objective_criteria`](../../docs/test/diff/objective_criteria.md)、[`team_rule`](../../docs/test/diff/team_rule.md)、[`boss_bar`](../../docs/test/diff/boss_bar.md)、[`sound_source`](../../docs/test/diff/sound_source.md)、[`difficulty`](../../docs/test/diff/difficulty.md)、[`permission_level`](../../docs/test/diff/permission_level.md)、[`anvil_durability`](../../docs/test/diff/anvil_durability.md)、[`banner_pattern`](../../docs/test/diff/banner_pattern.md)、[`beacon_effects`](../../docs/test/diff/beacon_effects.md)、[`brewing_tick`](../../docs/test/diff/brewing_tick.md)、[`dimension_type`](../../docs/test/diff/dimension_type.md)、[`direction_rotate`](../../docs/test/diff/direction_rotate.md)、[`enchantment_provider`](../../docs/test/diff/enchantment_provider.md)、[`enchantment_selection`](../../docs/test/diff/enchantment_selection.md)、[`entity_attributes`](../../docs/test/diff/entity_attributes.md)、[`entity_dimensions`](../../docs/test/diff/entity_dimensions.md)、[`explosion_damage`](../../docs/test/diff/explosion_damage.md)、[`explosion_ray`](../../docs/test/diff/explosion_ray.md)、[`explosion_seen_grid`](../../docs/test/diff/explosion_seen_grid.md)、[`food_stats`](../../docs/test/diff/food_stats.md)、[`food_tick`](../../docs/test/diff/food_tick.md)、[`furnace_tick`](../../docs/test/diff/furnace_tick.md)、[`hopper_cooldown`](../../docs/test/diff/hopper_cooldown.md)、[`item_same_components`](../../docs/test/diff/item_same_components.md)、[`item_stackable`](../../docs/test/diff/item_stackable.md)、[`mob_effect_merge`](../../docs/test/diff/mob_effect_merge.md)、[`mth_modulo`](../../docs/test/diff/mth_modulo.md)、[`mth_rounding`](../../docs/test/diff/mth_rounding.md)、[`repair_cost`](../../docs/test/diff/repair_cost.md)、[`spawner_delay`](../../docs/test/diff/spawner_delay.md)、[	rial_spawner_config](../../docs/test/diff/trial_spawner_config.md)、[`vec3_math`](../../docs/test/diff/vec3_math.md)、[`blockpos_codec`](../../docs/test/diff/blockpos_codec.md)、[`axis_plane`](../../docs/test/diff/axis_plane.md)、[`mth_lerp`](../../docs/test/diff/mth_lerp.md)、[`mth_bit`](../../docs/test/diff/mth_bit.md)、[`mth_rot`](../../docs/test/diff/mth_rot.md)、[`mth_sin`](../../docs/test/diff/mth_sin.md)、[`mth_approach`](../../docs/test/diff/mth_approach.md)、[`region_sector`](../../docs/test/diff/region_sector.md)、[`blockpos_dist`](../../docs/test/diff/blockpos_dist.md)、[`enchantment_storage`](../../docs/test/diff/enchantment_storage.md)、[`collide_sweep`](../../docs/test/diff/collide_sweep.md)、[`shapes_boolean`](../../docs/test/diff/shapes_boolean.md)、[`fluid_spread`](../../docs/test/diff/fluid_spread.md)、[`fluid_spread_dir`](../../docs/test/diff/fluid_spread_dir.md)、[`blockpos_bfs`](../../docs/test/diff/blockpos_bfs.md)、[`scheduled_tick`](../../docs/test/diff/scheduled_tick.md)、[`random_tick_pos`](../../docs/test/diff/random_tick_pos.md)、[`mth_color`](../../docs/test/diff/mth_color.md)、[`block_transform`](../../docs/test/diff/block_transform.md)、[`bounding_box`](../../docs/test/diff/bounding_box.md)、[`enchantment_compat`](../../docs/test/diff/enchantment_compat.md)、[`section_pos`](../../docs/test/diff/section_pos.md)、[`level_height`](../../docs/test/diff/level_height.md)、[`chunk_pos`](../../docs/test/diff/chunk_pos.md)、[`mth_clamp`](../../docs/test/diff/mth_clamp.md)、[`chat_formatting`](../../docs/test/diff/chat_formatting.md)、[`dye_color`](../../docs/test/diff/dye_color.md)、[`equipment_slot`](../../docs/test/diff/equipment_slot.md)、[`heightmap_types`](../../docs/test/diff/heightmap_types.md)、[`mth_int_div`](../../docs/test/diff/mth_int_div.md)、[`direction_plane`](../../docs/test/diff/direction_plane.md)、[`rotation_info`](../../docs/test/diff/rotation_info.md)、[`blockpos_geom`](../../docs/test/diff/blockpos_geom.md)、[`vec3i_arith`](../../docs/test/diff/vec3i_arith.md)、[`fuel_values`](../../docs/test/diff/fuel_values.md)、[`recipe_book_category`](../../docs/test/diff/recipe_book_category.md)、[`text_formatting_color`](../../docs/test/diff/text_formatting_color.md)、[`text_formatting_name`](../../docs/test/diff/text_formatting_name.md)、[`style_tristate`](../../docs/test/diff/style_tristate.md)、[`team_rule_strings`](../../docs/test/diff/team_rule_strings.md)、[`render_type_strings`](../../docs/test/diff/render_type_strings.md)、[`uuid_strings`](../../docs/test/diff/uuid_strings.md)、[`game_type`](../../docs/test/diff/game_type.md)、[`map_decoration_type`](../../docs/test/diff/map_decoration_type.md)、[`block_state_enum_strings`](../../docs/test/diff/block_state_enum_strings.md)、[`block_state_enum_strings_2`](../../docs/test/diff/block_state_enum_strings_2.md)、[`block_state_enum_strings_3`](../../docs/test/diff/block_state_enum_strings_3.md)、[`boolean_property_index`](../../docs/test/diff/boolean_property_index.md)、[`enum_property_order`](../../docs/test/diff/enum_property_order.md)、[`enum_property_order_all`](../../docs/test/diff/enum_property_order_all.md)、[`face_sturdy`](../../docs/test/diff/face_sturdy.md)、[`world_border`](../../docs/test/diff/world_border.md)、[`easing_type`](../../docs/test/diff/easing_type.md)、[`mth_interpolation`](../../docs/test/diff/mth_interpolation.md)、[`legacy_random`](../../docs/test/diff/legacy_random.md)、[`custom_stats`](../../docs/test/diff/custom_stats.md)、[`sound_events`](../../docs/test/diff/sound_events.md)、[`particle_type`](../../docs/test/diff/particle_type.md)、[`shape_face`](../../docs/test/diff/shape_face.md)、[`block_shape`](../../docs/test/diff/block_shape.md)

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

### 数据来源：探针与 oracle 必须读同一份数据包

标签类能力（`block_tags`、`item_tags`、`can_harvest`……）两侧都读数据包。oracle 走原版
`ServerPacksSource.createVanillaTrustedRepository()`，读的是 **merged jar 内嵌的 data/**。

探针也必须读这个 jar（`vanilla_datapack.hpp` 的 `scanDataPack()` 把 jar 内的 `data/` 解到
临时目录再当数据包加载）。曾经让探针读磁盘上的数据包目录，结果是两边数据版本不同：
jar 里的 `small_flowers` 不含 `cactus_flower` / `wildflowers`，磁盘副本却含有，差分随即报出
几十处「Cubium 多出成员」的假差异。**判据**：差异全部呈「被测多出、参照不少」时，先核对
两侧数据源，再怀疑实现。

配套的坑：
- `DataPackRepository::scanDirectory` 的语义是「扫描一个**包含多个包**的目录」，传数据包
  根会得到 0 个包且不报错；单个包要么 `addPack`，要么把包装进容器目录。
- `ZipResourcePack::listResources` 传空目录名表示类型目录根；返回的路径已剥离 `data/`
  前缀，落盘时要补回。
- 两个探测位置都别复用过期结果：环境变量改变后 oracle 缓存键可能未覆盖，用 `--fresh` 复核。

### `memberGaps` 必须两侧同时剔除

`memberGaps.missing` 是「参照有、本项目尚未实现」的成员。比较时**两侧都要删掉**：只删参照侧
会让这些成员在被测侧因「本就不可能有」而被记成**多出来**，同一个实现缺口被计两次差异，而且
方向完全误导（看起来像本项目多实现了东西）。`projectOnly` 是互补的另一类豁免（本项目独有），
两者都只在声明时生效，并在摘要里输出剔除条数。

### 实现缺口：`KNOWN_MISSING` 清单

`known_missing_items.ts`、`entity_shape.ts`、`enchantment_table.ts`、
`known_missing_entity_attributes.ts` 里的 `KNOWN_MISSING` 列的是「原版有、本项目尚未实现」
的条目——**实现缺口，不是参照数据缺失**。清单显式列出、每次运行输出条数，实现补齐后必须
删掉对应条目，否则会出现「豁免了原版也有的项」的假一致。

### 已定位但未修：`VoxelShape` 坐标压缩

`block_support` 有 61 处差异，根因是 Cubium 的 `VoxelShape` 坐标压缩方式与原版不同。
详见 [`_investigations.md`](../../docs/test/diff/_investigations.md)。

### 已收敛：`structure_starts`

曾经长期未命中的要塞生成已收敛：`structure_starts` 845/845、`structure_start_single` 73/73、
`structure_blocks_single` 170/170 全部一致。两处关键修复：

1. 同心环放置的搜索起点必须用**关卡种子**（`concentricRingsSeed == levelSeed`）而不是常量 0；
   fork 必须在同一条随机流上取（见 `StructurePlacementState::ringPositions` 的注释）。
2. 群系校验必须发生在完整装配**之前**（原版 `GenerationStub` 的构件构造器是惰性 lambda）。
   此前先装配再校验，多条目结构集会为每个被拒绝的变体白做整族装配，并让后续重抽的随机数
   与原版错位（详见 `src/server/world/gen/structure/README.md` 的「容易踩的坑」第 17 条）。

注意 `structure_starts` 的**首次**运行会触发同心环的 128 次螺旋生物群系搜索（约 3.2M 次
群系采样，本机实测 2.7s），这是原版同款的一次性开销，缓存进 `ConcentricRingsStructurePlacement`
后不再重复；它不是差异，也不应据此判定能力不稳定。

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
