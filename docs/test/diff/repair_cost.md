# 差分能力：`repair_cost`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

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
