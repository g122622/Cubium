# 差分能力：`enchantment_provider`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

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
