# 差分能力：`dimension_type`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

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
