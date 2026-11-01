# 差分能力：`beacon_effects`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

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
