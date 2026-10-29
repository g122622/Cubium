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
│   ├── entity_shape.cpp         # 实体尺寸 / 分类
│   ├── enchantment_table.cpp    # 附魔等级与附魔台代价
│   ├── loot_table.cpp           # 战利品表掷骰（最小上下文：origin / 中性方块 / 空工具）
│   ├── loot_table_block.cpp     # 战利品表掷骰（挖掘上下文：方块状态 + 时运工具）
│   ├── loot_table_entity.cpp    # 战利品表掷骰（击杀上下文：this_entity + 攻击者 + 掠夺）
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
│   ├── loot_world.hpp           # 战利品探针共用的空世界 / 掷骰入口
│   └── loot_probe_common.hpp    # 战利品探针共用的样本循环与摘要
├── java/dev/horizonlab/diff/    # 原版 Java oracle（直接调 1.21.11 服务端 jar）
│   ├── DataPackRegistries.java  # 像服务端一样加载原版数据包（数据驱动注册表）
│   ├── *Oracle.java / *Export.java
└── harness/                     # TypeScript 编排层（Node 24 原生 type stripping）
    ├── vanilla.ts               # minecraft-data 读取（回退数据源）
    ├── oracle-env.ts            # 定位原版 jar + 依赖库，拼 classpath
    ├── java-oracle.ts           # 编译 oracle、两侧各跑一次、按 key 比对
    ├── comparison.ts            # 共享字段投影：严格检查数组长度，仅显式声明为无序的数组才忽略顺序
    ├── runner.ts                # 构建 / 运行 / 比对 / 写报告；CLI 入口
    ├── test_text_encode_parity.ts   # 手写 harness（方块状态全集 + 摘要）
    └── harnesses/               # 声明式 harness 与注册表
        ├── index.ts             # 注册表 + run all / run <name>
        ├── loot_context.ts      # 战利品三个探针的上下文模板与已知缺口
        ├── recipe.ts            # 导出配方夹具，生成两侧共用的锻造三槽输入
        ├── status_effect.ts     # 效果实例的共同边界语料和固定操作序列
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

3. 在 `java/dev/horizonlab/diff/` 写 `<Domain>Oracle.java`（`list` / `run` 两个子命令），
   在 `harness/harnesses/index.ts` 的 `JAVA_HARNESSES` 里登记 oracle 类名、输入模板与
   比对键。`.java` 由 `java-oracle.ts` 在运行前编译，不需要额外构建步骤。

   读数据包的能力（战利品表、进度、群系生成、配方）：把域名加进 `runner.ts` 的
   `DATAPACK_DOMAINS`，探针就能从 `MC_DIFF_VANILLA_DATAPACK` 拿到与 oracle 同一个
   数据包目录；探针共用的注册表初始化与数据包标签加载在 `probes/vanilla_datapack.hpp`。

   原版有、本项目尚未实现的单元登记进 `oracleMissing`（整条单元）或
   `memberGaps`（只从原版一侧剔除某些成员），报告里会单独说明条数。

**确定性是硬要求**：同一输入必须永远产出同一输出。涉及无序容器、随机数、时间、
地址的，probe 必须在写出前自行收敛（排序、固定种子、去掉地址），否则差分没有意义。

---

## 当前状态

以下能力已接入。结果描述最近一次验证；`spawn_placement` 单独比较实际位置判定，
不把静态群系配置通过视为运行时生成规则通过。

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
| `damage_type` | 原版 jar `DamageTypeOracle` | 50 | 全部一致 | — |
| `block_tags` | 原版 jar `BlockTagOracle` | 204 | 全部一致 | — |
| `item_tags` | 原版 jar `ItemTagOracle` | 198 | 全部一致 | 300 条成员引用了未注册物品 |
| `random_sequence` | 原版 jar `RandomSequenceOracle` | 24 | 全部一致 | — |
| `status_effect` | 原版 jar `StatusEffectOracle` | 2430 个场景 / 48,822 次操作 | 全部一致 | — |
| `command_argument` | 原版 jar `CommandArgumentOracle` | 162 | 全部一致 | — |
| `command_tree` | 原版 jar `CommandTreeOracle` | 1（全部根命令）| 全部一致 | 9 条命令未实现（见下） |
| `loot_table` | 原版 jar `LootTableOracle` | 1324 | 全部一致 | — |
| `loot_table_block` | 原版 jar `LootTableBlockOracle` | 1324 | 全部一致 | — |
| `loot_table_entity` | 原版 jar `LootTableEntityOracle` | 5288（1324 张表 × 4 个场景） | 14 处差异（13 张表的 `#player` 场景 + `magma_cube`/`pillager`） | 2 张表的实体类型建不出来（`creaking`、`mannequin`） |
| `advancement` | 原版 jar `AdvancementOracle` | 1584 | 全部一致 | — |
| `biome_spawns` | 原版 jar `BiomeSpawnsOracle` | 65 | 全部一致 | — |
| `spawn_placement` | 原版 jar `SpawnPlacementOracle` | 157 种实体 × 332 场景 | 全部一致 | — |
| `spawn_rules` | 原版 jar `SpawnRulesOracle` | 157 种实体 / 168,876 个场景 | 全部一致 | — |
| `recipe` | 原版 jar `RecipeOracle` | 1470 | 全部一致 | — |
| `block_experience` | 原版 jar `BlockExperienceOracle` | 1166 | 全部一致 | — |
| `item_attribute_modifiers` | 原版 jar `ItemAttributeModifiersOracle` | 1124 | 全部一致 | 381 个物品未注册 |
| `damage_reduction` | 原版 jar `DamageReductionOracle` | 2415 种伤害×护甲×韧性组合 | 全部一致（4 位小数） | — |
| `gamerule` | 原版 jar `GameRuleOracle` | 59 | 全部一致 | — |
| `armor_material` | 原版 jar `ArmorMaterialOracle` | 9 种材质 × 5 槽位 | 全部一致 | — |
| `tool_material` | 原版 jar `ToolMaterialOracle` | 7 | 全部一致 | 采集等级不比（原版用方块标签表达） |
| `enchantment_effects` | 原版 jar `EnchantmentEffectsOracle` | 43 | 全部一致 | 宝藏枚举与附魔台代价由 `enchantment_table` 覆盖 |
| `attribute_range` | 原版 jar `AttributeRangeOracle` | 35 | 全部一致 | 项目自研属性 `horse.jump_strength` 原版没有 |

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
也读不出重点；`java-oracle.ts` 因此新增 `memberGaps` 选项，按成员名从**原版一侧**剔除并报告
条数（probe 多出来的成员永远算差异）。

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

- **`loot_table_entity` 的 14 处 `#player` 差异**：13 张表在两套上下文下只差玩家相关条件
  （`killed_by_player`、`direct_attacker` 的装备等）。两侧单独跑都自洽，只在 harness 环境里
  分叉；根因疑似 oracle 侧未登录 `ServerPlayer` 的 `LAST_DAMAGE_PLAYER` 参数缺失，尚未定论。
  代码里以 `TODO(known-failure)` 标注。
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
