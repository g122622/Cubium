# 差分能力：`item_stackable`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 84 个能力，23 个「物品 × 损伤」组合。22 个一致，1 个记为已知差异。

#### 原版三段逻辑

```java
public int getMaxStackSize() {
    return this.getOrDefault(DataComponents.MAX_STACK_SIZE, 1);   // 默认 1！
}

public boolean isDamageableItem() {
    return this.has(DataComponents.MAX_DAMAGE)
        && !this.has(DataComponents.UNBREAKABLE)
        && this.has(DataComponents.DAMAGE);
}

public boolean isStackable() {
    return this.getMaxStackSize() > 1 && (!this.isDamageableItem() || !this.isDamaged());
}
```

#### 两个反直觉的结论

**1. `MAX_STACK_SIZE` 的默认值是 1，不是 64。** 没有这个组件的物品（工具、盔甲）一律不可
堆叠。默认值写成 64 会让所有工具都被判为可堆叠。

**2. 可损坏 ≠ 不可堆叠。** 实测对照：

| 物品 | 上限 | 可损坏 | 已损伤 | `stackable` |
|---|---|---|---|---|
| 泥土 | 64 | false | false | **true** |
| 雪球 | 16 | false | false | **true** |
| 钻石剑（未损伤） | 1 | true | false | **false** |
| 钻石剑（已损伤） | 1 | true | true | **false** |

钻石剑在「未损伤」时仍不可堆叠——因为 `maxStack > 1` 这一项就已经为假，`isDamageableItem`
那半边根本没机会生效。**真正决定工具不可堆叠的是上限 1，不是「可损坏」这个属性。**

**3.（未覆盖）`UNBREAKABLE` 会让物品重新变得可堆叠。** `isDamageableItem()` 里有
`!has(UNBREAKABLE)`，不可破坏的钻石剑在组件层面不再是「可损坏物品」，于是
`isStackable()` 直接放行。Cubium 的 `isDamageable()` 已经排除了 unbreakable，逻辑等价；
但 `ItemStack` 目前没有设置该组件的公开入口，因此单元表无法覆盖这一支，已在探针头
记 TODO。

#### 一处表示法差异

`minecraft:air` 单元暴露了空栈表示法的差异：

| 字段 | 原版 | Cubium |
|---|---|---|
| `empty` | true | false |
| `maxStackSize` | 1 | 64 |
| `stackable` | false | true |

根因是两边用**不同的东西**表示空栈：原版 `isEmpty()` 是
`this == EMPTY || item == Items.AIR || count <= 0`，把 AIR 物品本身当作空；Cubium 用
`nullptr` 表示空，`ItemStack(Items.AIR)` 是一个**非空的、装着空气的栈**。

这是既有的设计选择（不是本能力引入的），因此登记为已知差异而非缺陷。但它提醒了一件事：
**下游任何 `if (stack.isEmpty())` 的分支，在两边的边界行为并不相同**。
