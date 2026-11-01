# 差分能力：`enchantment_storage`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 105 个能力，21 个操作序列全部一致。`ItemEnchantments.Mutable` 的增删改语义此前没有被
比对过。Cubium 只有 `ItemEnchantmentsCodec`（解码用），**没有 `Mutable` 的 set/upgrade
语义**——本能力提供的是待实现的行为基准。

## 核心发现：同一个 `0` 在两个方法里语义相反

```java
public void set(Holder<Enchantment> ench, int level) {
    if (level <= 0) {
        this.enchantments.removeInt(ench);          // <= 0 就删除
    } else {
        this.enchantments.put(ench, Math.min(level, 255));
    }
}

public void upgrade(Holder<Enchantment> ench, int level) {
    if (level > 0) {
        this.enchantments.merge(ench, Math.min(level, 255), Integer::max);
    }
    // level <= 0 时什么都不做
}
```

| 单元 | 结果 |
|---|---|
| `set:a:3,set:a:0` | **`-`（空）** |
| `set:a:3,set:a:-1` | **`-`（空）** |
| `upgrade:a:3,upgrade:a:0` | **`a=3`（保留）** |
| `upgrade:a:3,upgrade:a:-1` | **`a=3`（保留）** |

- `set(e, 0)` 是「**赋值为 0 等于没有这个附魔**」→ 删除
- `upgrade(e, 0)` 是「**加 0 级等于没提出要求**」→ 什么都不做

两者互换会让「删除附魔」或「升级附魔」的某一条路径静默失效，且两种错误都**不会报错**。

## 其他三个关键点

**1. `set` 会覆盖，`upgrade` 只取 max。**

| 单元 | 结果 |
|---|---|
| `set:a:3,upgrade:a:2` | `a=3`（不降级） |
| `upgrade:a:5,set:a:2` | `a=2`（set 直接覆盖） |

`upgrade` 的语义是 `merge(ench, level, Integer::max)`——只升不降。但 `set` 是无条件赋值，
可以降级。

**2. 上限 255 是静默截断。**

| 单元 | 结果 |
|---|---|
| `set:a:255` | `a=255` |
| `set:a:256` | `a=255` |
| `set:a:1000` | `a=255` |
| `upgrade:a:300` | `a=255` |

`Math.min(level, 255)`，不报错、不告警。

**3. 删除后可以重新升级恢复。**

`set:a:3,set:a:0,upgrade:a:1` → `a=1`。删除是彻底的，之后 `upgrade` 从不存在开始。

## 三层等级范围不一致

| 层 | 允许范围 | 对 0 的处理 |
|---|---|---|
| `Mutable.set` / `upgrade` | 任意 `int` | `<= 0` 特殊处理，`> 255` 截断 |
| `ItemEnchantments` 构造器 | `[0, 255]` | **允许 0**，会保留「0 级」条目 |
| `LEVEL_CODEC`（序列化） | `[1, 255]` | **拒绝 0** |

所以「0 级附魔」在内存表示里是**可能的**（通过构造器直传），但**无法被序列化**。正常路径下
`Mutable.set` 会把 0 删掉，因此不会产生这种状态——这层不一致只在构造器被直接调用时暴露。

## 实现说明

单元里的 `a`~`e` 代表注册表中按标识符排序的前 5 个附魔。探针侧用字母占位，oracle 侧把
字母映射到真实 `Holder<Enchantment>`，输出统一的 `字母=等级` 摘要。

**踩坑**：1.21.11 的 `ResourceKey` 用 `identifier()` 而不是 `location()`，写错会导致编译失败。

## 未覆盖范围

- `removeIf`（在 `Mutable` 上只是转发到 keySet 的 removeIf）
- `ItemEnchantments.CODEC` 对 0 级拒绝的序列化行为（属序列化层，需单独能力）
- `ItemEnchantments.equals` / `hashCode`（`Object2IntOpenHashMap` 的语义）