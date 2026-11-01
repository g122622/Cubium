# 差分能力：`item_same_components`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 85 个能力，12 组配对全部一致。Cubium **没有** `isSameItemSameComponents`，最接近的是
`ItemStack::canMergeWith`——后者语义不同，会影响容器合并、配方匹配、掉落物堆叠等多处。

#### 原版逻辑

```java
public static boolean isSameItemSameComponents(ItemStack a, ItemStack b) {
    if (!a.is(b.getItem())) {
        return false;
    }
    return a.isEmpty() && b.isEmpty() ? true : Objects.equals(a.components, b.components);
}

public static boolean matches(ItemStack a, ItemStack b) {
    if (a == b) {
        return true;
    }
    return a.getCount() != b.getCount() ? false : isSameItemSameComponents(a, b);
}
```

#### 三个关键点

**1. 两个空栈被视为「相同」。** `a.isEmpty() && b.isEmpty()` 这一支返回 `true`——空栈之间
没有组件可比，语义上就是相等。

**2. `matches` 比 `isSameItemSameComponents` 多一道数量判定。** 实测：

| 输入 | `sameItemSameComponents` | `matches` |
|---|---|---|
| 泥土 ×1 vs 泥土 ×5（同损伤） | **true** | **false** |
| 钻石剑（损伤 0）vs 钻石剑（损伤 1） | false | false |
| 空 vs 空 | **true** | **true** |

两者用途不同：前者回答「能否堆叠到一起」，后者回答「是否完整相等」（如配方匹配）。

**3. 组件相等是深比较。** 损伤值、附魔、自定义名称、lore 都属于组件；任意一项不同就不相等。
「同物品、不同损伤」的两件钻石剑**不能**堆叠。

#### 本轮自己踩的坑：判空顺序写反了

我第一版探针是这么写的：

```cpp
const bool sameItem = !left.isEmpty() && !right.isEmpty() && left.getItem() == right.getItem();
```

它要求**两边都非空**才算「同物品」，于是「两空」这一支永远走不到，空栈配对得到 `false`。
差分比对立刻把这个单元指出来了（原版 `true` / 探针 `false`）。

正确写法是**先判两空**，再比物品：

```cpp
if (left.isEmpty() && right.isEmpty()) {
    sameItemSameComponents = true;
} else if (!left.isEmpty() && !right.isEmpty() && left.getItem() == right.getItem()) {
    sameItemSameComponents = left.getDamage() == right.getDamage();
}
```

原版之所以能写成那样，是因为在它那里「空栈的物品就是 AIR」，`a.is(b.getItem())` 对两个空栈
天然为真，判空分支只是在短路组件比较。**Cubium 用 `nullptr` 表示空，这个前提不成立，直接
照抄表达式就会错。** 这是表示法差异第一次真正影响到逻辑复刻——`item_stackable` 那轮只是
输出值不同，这次是**控制流走错**。

#### 与 Cubium 的口径差异

`canMergeWith` 在任一为空时返回 `false`（原版两空为 `true`），且只比较了损伤、修复成本、
名称、lore 等若干字段，不是完整的组件深比较。已在探针头记 TODO：待 Cubium 实现完整深比较后
改为直接调用。
