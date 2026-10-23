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
├── CMakeLists.txt               # probe 目标定义（EXCLUDE_FROM_ALL）
├── probes/                      # C++ 侧小程序，只链 mc_common
│   ├── probe.hpp                # Probe 接口 + MC_DIFF_DEFINE_PROBE 宏
│   ├── runner_main.cpp          # 所有 probe 共用的 main（命令行、IO、错误处理）
│   ├── text_encode.cpp          # 方块状态的文本表示
│   ├── item_summary.cpp         # 堆叠上限 / 食物属性
│   ├── item_components.cpp      # 耐久 / 稀有度 / 附魔能力
│   ├── entity_shape.cpp         # 实体尺寸 / 分类
│   └── enchantment_table.cpp    # 附魔等级与附魔台代价
├── java/dev/horizonlab/diff/    # 原版 Java oracle（直接调 1.21.11 服务端 jar）
│   ├── DataPackRegistries.java  # 像服务端一样加载原版数据包（数据驱动注册表）
│   ├── *Oracle.java / *Export.java
└── harness/                     # TypeScript 编排层（Node 24 原生 type stripping）
    ├── vanilla.ts               # minecraft-data 读取（回退数据源）
    ├── oracle-env.ts            # 定位原版 jar + 依赖库，拼 classpath
    ├── java-oracle.ts           # 编译 oracle、两侧各跑一次、按 key 比对
    ├── runner.ts                # 构建 / 运行 / 比对 / 写报告；CLI 入口
    ├── test_text_encode_parity.ts   # 手写 harness（方块状态全集 + 摘要）
    └── harnesses/               # 声明式 harness 与注册表
        ├── index.ts             # 注册表 + run all / run <name>
        └── known_missing_items.ts   # 物品侧已知缺口清单
```

产物全部落在 `build/diff/<harness>/`：`inputs.jsonl`、`probe_outputs.jsonl`、`report.json`。

---

## 运行

```bash
# 全部 harness（Java oracle 部分需要 JDK，见下方「前置」）
node tests/diff/harness/runner.ts run all

# 单个
node tests/diff/harness/runner.ts run item_components
node tests/diff/harness/runner.ts run text_encode

# 只构建全部 probe / 单个 probe
node tests/diff/harness/runner.ts build
node tests/diff/harness/runner.ts build item_components

# 手写 harness（方块状态全集 + 摘要，比 runner 多一层全集核对）
node tests/diff/harness/test_text_encode_parity.ts --no-build

# 指定其它构建目录（默认 <仓库>/build）
MC_DIFF_BUILD_DIR=build-noprof node tests/diff/harness/runner.ts run all
```

退出码：`0` 全部一致；`1` 存在差异；`2` 环境问题（缺 probe、构建失败、缺原版数据）。

**前置**：

1. `MC_BUILD_DIFF_PROBES` 默认为 `OFF`，构建脚本会自动打开它；Windows 上构建必须经过
   `scripts/configure.sh`（注入 VS 环境），`runner.ts` 已经这样调用。
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
4. `text_encode` 用 minecraft-data 作参照：`tests/e2e/bot/node_modules/minecraft-data`，
   未安装时 `cd tests/e2e/bot && npm install`。

---

## 一个 harness 的协议

C++ probe 只有两个子命令：

```
diff_<domain> list                          # 每行一个单元（方块状态文本）
diff_<domain> run <input.jsonl> <output.jsonl>   # 每行输入 → 一行 JSON 输出
```

`list` 枚举「可被枚举的全集」，`run` 按方块出摘要，两者不必是同一个集合——比对什么由
harness 决定。`runner.ts` 负责按 `key` 对齐两侧输出、规范化、比对、写报告。

**规范化必须在两侧都做**（`compareTransform`）。当前做了三件事：

1. 属性按名字排序 —— vanilla 的 `StateHolder#toString` 按名字字典序输出，注册顺序不是语义；
2. 取值域排序 —— minecraft-data 给字符串序（`"1","10","2"`），Cubium 给枚举声明序；
3. `DirectionProperty` 归一到 `EnumProperty` —— 前者是后者的子类，vanilla 的 `facing` 就是
   `EnumProperty<Direction>`。**只归一类型名，不放开取值比较**。

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

3. 在 `harness/` 下写 `test_<domain>_parity.ts`，参照实现放在 `<domain>.ts` 或
   `vanilla.ts` 里，调用 `runHarness()` 并自行决定退出码。

**确定性是硬要求**：同一输入必须永远产出同一输出。涉及无序容器、随机数、时间、
地址的，probe 必须在写出前自行收敛（排序、固定种子、去掉地址），否则差分没有意义。

---

## 当前状态

九个能力全部对齐（`node tests/diff/harness/runner.ts run all` 退出码 0）：

| harness | 参照实现 | 单元 | 结果 | 已知缺口 |
|---|---|---|---|---|
| `text_encode` | minecraft-data | 1166 个方块 / 29671 条状态文本 | 全部一致 | — |
| `item_summary` | 原版 jar `ItemSummaryOracle` | 1124 | 全部一致 | 381 个物品未注册 |
| `item_components` | 原版 jar `ItemComponentsOracle` | 1124 | 全部一致 | 381 个物品未注册 |
| `entity_shape` | 原版 jar `EntityShapeOracle` | 125 | 全部一致 | 32 个实体类型未注册 |
| `enchantment_table` | 原版 jar `EnchantmentExport` | 40 | 全部一致 | 3 个附魔未实现 |
| `block_properties` | 原版 jar `BlockPropertiesOracle` | 1166 | 全部一致 | — |
| `mob_category` | 原版 jar `MobCategoryOracle` | 8 | 全部一致 | — |
| `entity_attributes` | 参考表 + 原版 jar `EntityAttributesOracle` | 73 | 全部一致 | 2 个实体类型未对齐（盔甲架、凋灵） |
| `block_state_names` | 原版 jar `BlockStateNamesOracle` | 1166 | 全部一致 | — |
| `random_sequence` | 原版 jar `RandomSequenceOracle` | 24 | 全部一致 | — |

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

### 已知缺口清单

`known_missing_items.ts`、`entity_shape.ts`、`enchantment_table.ts`、
`known_missing_entity_attributes.ts` 里的 `KNOWN_MISSING`
列的是「原版有、本项目尚未实现」的条目——**实现缺口，不是参照数据缺失**。清单显式列出、
每次运行输出条数，实现补齐后必须删掉对应条目，否则会出现「豁免了原版也有的项」的假一致。

---

## 写一个 oracle 时要注意

- **协议通道不能用 stdout/stderr**：原版 log4j 的配置会把 stdout 拿去记日志，`System.err`
  也会被套成 `[time] [main/INFO]: [STDERR]: ` 前缀行。所以 `list` 子命令把单元写到
  文件（`units.txt`），`run` 则写输出文件。
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
- 表面数据可用 `node tests/diff/harness/scripts/export_entity_attributes_reference.ts`
  用 oracle 重新导出（不带 `--write` 只打印，便于 diff）。

## 下一步候选能力

按「只读、无状态、易对齐」排序：

1. **方块状态 → NBT / 数字 ID 编解码** —— 直接落在存档与网络协议的 1:1 对齐上；
2. **命令解析树** —— 从原版导出 command tree，对同一批输入比解析结果；
3. **随机数序列** —— 先锁死 `RandomSource` 的位级等价，后面世界生成的差分才可能成立；
4. **实体 / 附魔缺失项** —— 补齐 `KNOWN_MISSING` 里那 3 个附魔与 32 个实体类型，
   清单随之删条目。
