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

`(int)` 是**截断而非四舍五入**，且下界不随难度变化。

以 `mob_spawn_equipment`（`min_cost=5`、`max_cost_span=17`）为例：

| 乘数 | `f × 17` | 截断 | 上界 | 四舍五入 | 结果 |
|---|---|---|---|---|---|
| `0` | `0` | `0` | `5` | `0` | 相同 |
| `0.25` | `4.25` | `4` | `9` | `4` | **相同** |
| `0.5` | `8.5` | `8` | `13` | `8` | **相同** |
| **`0.75`** | `12.75` | `12` | **`17`** | `13` | **不同（18）** |
| `1` | `17` | `17` | `22` | `17` | 相同 |
| `1.25` | `21.25` | `21` | `26` | `21` | **相同** |
| **`1.5`** | `25.5` | `25` | **`30`** | `26` | **不同（31）** |

**注意上表加粗的两行**：乘数 `0.75` 与 `1.5` 才会让「截断」与「四舍五入」得到不同上界
（`17` vs `18`、`30` vs `31`）。

这与直觉相反——`0.5` 的 `8.5` 看起来最容易暴露差异，但 `.5` 正是 `Math.round` 向上取整的
临界点，两种算法恰好都得到 8。**真正能区分两者的是小数部分为 `.25` 或 `.75` 的值，
且只有乘积的小数部分 ≥ 0.5 时才有差别。**

`0.25 × 17 = 4.25` 小数部分 0.25 < 0.5，四舍五入仍得 4，所以也区分不了。

**这解释了为什么 harness 要跑 7 个乘数而不是 2–3 个**：只有覆盖面足够广，
才能命中 `0.75` 和 `1.5` 这两个真正有判别力的值。

#### 乘数的真实值域是 `[0, 1]`，不是 `[0, 1.5]`

```java
// DifficultyInstance.java:35
public float getSpecialMultiplier() {
    if (this.effectiveDifficulty < 2.0F) {
        return 0.0F;
    } else {
        return this.effectiveDifficulty > 4.0F ? 1.0F : (this.effectiveDifficulty - 2.0F) / 2.0F;
    }
}
```

`effectiveDifficulty` 被钳到 `[0, 4]`，所以返回值域是 **`[0, 1]`**：
难度为和平时恒为 `0`，到达「困难 + 满驻留」时为 `1`。

harness 里的 `1.25` 与 `1.5` **超出真实值域**，是为了探测截断边界而额外加入的
（见上表，`1.5` 是仅有的两个有判别力的乘数之一）。这类「超出定义域但语义合法」的
用例在本套 harness 里是常见手法：外部输入可以来自命令、数据包或旧存档，
不保证落在原版运行时会产生的范围内。

#### 单元数不等于 `list` 行数

本能力用「提供器 × 难度乘数」的笛卡尔积，7 个提供器 × 7 个乘数 = 49 个单元。
`computed` 单元拿不到 `list` 的输出，所以单元生成函数直接扫数据包目录；
它用的是 oracle 侧的 `vanillaDataPackDir()` 而不是直接读环境变量——**`computed`
单元在 runner 注入 `MC_DIFF_VANILLA_DATAPACK` 之前就会求值**，这里踩过一次。

注意报告里的 `units: 7` 是按比较键（`provider`）去重后的数，**49 行输入实际逐行比过**。
实测确认：

```
[1/1] enchantment_provider         通过      7.3s  7/7 一致
```

同时 `build/diff/enchantment_provider/run/inputs.jsonl` 确实是 **49 行**。

这里容易误读成「只测了 7 个用例」。报告的分母用的是能力声明的 `key`（本能力是
`provider`），而同一提供器在 7 个乘数下的 7 行共享同一个 key，因此被折叠成 1 个单元。
**只要任何一行的分量不一致，该 key 就会整体判为差异**——所以覆盖面并没有损失，
只是显示上被压缩了。

#### 浮点参数以字符串传参

单元里的乘数写作 `"0.75"` 这样的**字符串**而非 JSON 数字：

```ts
const SPECIAL_MULTIPLIERS = ["0", "0.25", "0.5", "0.75", "1", "1.25", "1.5"] as const;
```

两侧都先用文本转成 `float`，避免 JSON 解析器对数字的不同精度处理引入假差异。
C++ 侧用 `std::stod`，Java 侧用 `Float.parseFloat`，两者对十进制字面量的舍入都是
「就近取偶」，结果一致。

#### 结果

49 行全部一致。TODO 与 `banner_pattern` 相同：待 Cubium 实现该注册表后，
探针应改为读注册表而不是直接解析 JSON。
