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
│   ├── block_state_ids.cpp      # 方块状态的全局 ID（协议 / 存档编号）
│   ├── biome_climate.cpp        # 生物群系温度 / 降水 / 颜色
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

十三个能力已接入，`node tests/diff/harness/runner.ts run all` 退出码 0（全部一致）：

| harness | 参照实现 | 单元 | 结果 | 已知缺口 |
|---|---|---|---|---|
| `text_encode` | minecraft-data | 1166 个方块 / 29671 条状态文本 | 全部一致 | — |
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
| `random_sequence` | 原版 jar `RandomSequenceOracle` | 24 | 全部一致 | — |
| `command_argument` | 原版 jar `CommandArgumentOracle` | 162 | 全部一致 | — |
| `command_tree` | 原版 jar `CommandTreeOracle` | 1（全部根命令）| 全部一致 | 9 条命令未实现（见下） |

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

### 命令树：已对齐

`command_tree` 注册全部内置命令后遍历分发器，与原版 `new Commands(DEDICATED, ...)` 的树
逐节点比对（根命令集合 + 每个根命令的二级签名）。当前**全部一致**：原版 80 条根命令与
Cubium 80 条逐一对应，形状与参数名/类型名都不再有差异。

对齐过程中修掉的真问题：

- **`StringReader::canRead` 的边界**：`canRead(length)` 写成 `<=`，游标停在末尾时仍可读，
  「输入已读完」永远走不到——`list extra` 这类残留文本会被当成成功执行。新增语义正确的
  `atEnd()`，把 dispatcher 与各参数的「末尾判断」改用它；
- **dispatcher 的末端可执行判定**：只剩文本、末端节点本身没有命令时不能算成功候选；
  带命令的节点必须读到输入末尾（或可重定向）才算成功。修好 `list extra`、
  `enchant @p sharpness -1`，同时不让 `say`/`tellraw` 的组件参数被误判；
- **组件/消息参数**：命令链末端读整段原文（原版 MessageArgument 语义），开头是引号时
  按引号串读取（`kick @a "some reason"`）；
- **实现脚手架不进命令树**：/execute 链上的 `run` 链接、run 下的 `command` 占位参数、
  `/return run command` 都不属于协议命令树，用 `CommandNode::setTreeHidden` 在快照与
  probe 里跳过；
- **兼容层命令不再默认注册**：`debugconfig`/`publish`/`replaceitem` 原版已移除，
  需要时调用 `CommandRegistry::setRegisterCompatCommands(true)` 开启；
- **`gamerule` 迁移到原版 1.21.11 规则集**：规则名改为注册表键（`keep_inventory` /
  `advance_time` / `spawn_mobs` …，共 59 条），补上缺失的 12 条，短名与
  `minecraft:` 全名都注册；`max_minecart_speed` 受原版 FeatureFlag 门控，不注册命令节点；
- 形状修正：`clone` 用 `block_pos`、`execute rotated` 参数名 `rot`、`execute summon`、
  `loot` 改成「目标 → 来源字段」、`data modify … set`、`locate poi`、`schedule clear`
  用 `function:string`。

**仍未实现（9 条，见 `known_missing_commands.ts`）**：`fetchprofile`、`fillbiome`、
`item`、`place`、`random`、`test`、`tick`、`transfer`、`waypoint`。

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

### 随机数：已对齐到哪一层

`random_sequence` 直接比对两个**内层**生成器（`Xoroshiro128ppRandom`、
`JavaLegacyRandom`）的接口序列，全部逐位一致。上层的 `WorldgenRandom`（原版把它包在
生成器外面、用自己的 `next(bits)` 重写所有抽取）目前只保证：

| 路径 | 状态 |
|---|---|
| `nextInt(bound)` / `nextFloat` / `nextDouble` | 与原版逐位一致（golden 测试通过） |
| 裸 `nextLong()` / `nextInt()` | **未对齐**：与原版存在逐位差异，导致 `setDecorationSeed` 偏移 |

后者只影响世界生成的放置种子，已由 `WorldgenRandomVanillaGoldenTest` /
`BlendedNoiseVanillaGoldenTest` 的红灯标出（这两个测试在本轮 harness 之前就已失败）。

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
2. **命令解析树** —— 从原版导出 command tree（字面量/参数/重定向），与 `/help` 看到的
   树逐节点比对；`command_argument` 已经把参数层对齐了；
3. **实体 / 附魔缺失项** —— 补齐 `KNOWN_MISSING` 里那 3 个附魔与 32 个实体类型，
   清单随之删条目。
