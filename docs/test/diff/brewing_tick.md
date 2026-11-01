# 差分能力：`brewing_tick`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 87 个能力，12 种状态组合全部一致。酿造台每 tick 的状态机此前没有被比对过。

#### 原版状态机

```java
if (fuel <= 0 && fuelSlot.is(ItemTags.BREWING_FUEL)) {
    fuel = 20;                       // 设为 20，不是加 20
    消耗一个燃料;
}

boolean brewable = isBrewable(...);
boolean brewing = brewTime > 0;
if (brewing) {
    brewTime--;
    if (brewTime == 0 && brewable) {
        doBrew(...);
    } else if (!brewable || !ingredientSlot.is(this.ingredient)) {
        brewTime = 0;                // 中途条件破坏 -> 取消
    }
} else if (brewable && fuel > 0) {
    fuel--;                          // 开始酿造时就扣燃料
    brewTime = 400;
    this.ingredient = ingredientSlot.getItem();
}
```

#### 四处差异

Cubium 已有 `BrewingStandEntity::tick`，但结构与原版不同：

| 项 | 原版 | Cubium |
|---|---|---|
| 燃料种类 | `ItemTags.BREWING_FUEL` 标签 | 硬编码 `Items::BLAZE_POWDER` |
| 燃料补充方式 | `fuel = 20`（覆盖写） | `m_fuel += FUEL_PER_BREW`（累加） |
| **燃料扣除时机** | **开始酿造时** `fuel--` | **酿造完成时** `_consumeFuel()` |
| 不可酿造时 | 取消只发生在酿造中；未开始时什么都不做 | 开头直接 `m_brewTime = 0; return;` |

第三条影响最实质：**若在完成时才扣燃料，中断的酿造不会消耗燃料**——玩家可以反复启动、取消来白嫖。原版在 `brewTime = 400` 的同时就扣掉了。

第四条也有行为差异：原版在「未在酿造且不可酿造」时**不触碰** `brewTime`；Cubium 主动清零。

#### 本轮的验证方法调整

前两轮连续出现「用例看起来合理、实际没打到目标分支」（`99|50|99|50` 被钳到 0、`hasFuel` 字段位写错）。这轮改进了做法：**跑完后把每一条单元的实际输出逐条打出来，对照单元表注释核验它走了哪个分支**。

12 条全部确认命中目标：

| 单元 | 预期分支 | 实际输出 |
|---|---|---|
| `0 / 0 / true / true / true` | 补燃料后开始 | `fuelAdded=true, fuel=19` |
| `0 / 10 / true / false / true` | 开始酿造 | `fuelConsumed=true, fuel=9, brewTime=400` |
| `1 / 10 / true / false / true` | 本 tick 完成 | `brewed=true` |
| `100 / 10 / true / false / false` | 换试剂取消 | `brewTime=0` |
| `200 / ...` / `2 / ...` | 正常递减 | `199` / `1` |

这一步成本很低（一条命令），但能直接暴露「单元没生效」的问题。后续新能力会固定带上这一步。

#### 未覆盖范围

- `doBrew` 的实际产物转换（属 `potion_brewing` 的职责）
- 药水瓶位的 `HAS_BOTTLE` 方块状态同步（需要世界）
