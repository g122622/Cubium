# 差分能力：`enchantment_compat`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 116 个能力，155 个配对用例全部一致。附魔互斥性判定此前完全未覆盖。

## 被验证的原版逻辑

```java
public static boolean areCompatible(Holder<Enchantment> a, Holder<Enchantment> b) {
    return !a.equals(b)
        && !a.value().exclusiveSet().contains(b)
        && !b.value().exclusiveSet().contains(a);
}
```

## 四个关键点

**1. 同一附魔永远不兼容自己。**

第一个条件 `!a.equals(b)` 让 `areCompatible(x, x)` 恒为 `false`。这不是「互斥」，而是「不能
重复附魔同一个」，但走同一条返回路径。实测 43 个附魔的自反配对全部为 `false`。

**2. 检查是双向的，且是「或」的关系。**

只要 `a.exclusiveSet` 含 `b` **或** `b.exclusiveSet` 含 `a`，就不兼容。**单向声明足以阻断
双向组合**，所以数据包只需在一侧写入即可。

**3. `exclusiveSet` 常是 `#tag` 引用，必须展开。**

数据包里写的是：

```json
"exclusive_set": "#minecraft:exclusive_set/damage"
```

实测 `minecraft:sharpness` 展开后得到 6 个附魔：

```
bane_of_arthropods, breach, density, impaling, sharpness, smite
```

**注意 `sharpness` 自己也在列表里**——数据包把自身写进了互斥标签。这不影响结果（自反由第一个
条件先挡住），但说明「互斥表含自己」在真实数据里是常态，不能拿它当异常。

**4. 兼容性不涉及物品。**

`areCompatible` 只看两个附魔之间的关系，与它们分别能否附到同一物品上无关——那是
`canEnchant` / `supportedItems` 的职责（见 `enchantment_applicability`）。**两个都适用于某物品
的附魔，仍可能因互斥而不能共存。**

## 实测覆盖

| 类别 | 数量 | 例 |
|---|---|---|
| 不兼容 | 93 | `sharpness` × `smite` |
| 兼容 | 62 | `aqua_affinity` × `binding_curse` |
| 合计 | **155** | 全部一致 |

单元的构造覆盖三类配对：

1. **自反**：每个附魔与自身
2. **真实互斥对**：数据包里确有互斥关系的组合，**双向各出一条**（确认判定对称）
3. **无互斥搭配**：前 12 个附魔两两配对中不互斥的那些

## 本轮走过的弯路：`Holder` 身份与构造循环

这是本能力最耗时的部分，记录下来避免重犯。

`HolderSet.contains` 按 **Holder 的身份**比较，不是按附魔内容比较。最初的 oracle 试图用合成
`Enchantment` 验证任意互斥表，于是：

1. 先造 `Enchantment`，再用 `Holder.direct(...)` 包起来 → **同一附魔在不同位置是不同 Holder
   实例**，`contains` 恒为 `false`，双向互斥被误判成兼容。第一次跑就得到 1 处差异：
   `sharpness|smite|smite|sharpness` 期望 `false`，实际 `true`。

2. 想改成「先造空壳 Holder、再 `bindValue` 填实例」，以解开
   `Enchantment` 要 `exclusiveSet`、`Holder` 又要包 `Enchantment` 的循环。结果
   **`Holder.Reference.bindValue` 是 `protected`**，外部无法调用。

3. 最终放弃合成实例，**改用注册表里的真实附魔**。它们的 Holder 天然唯一，互斥关系直接来自
   数据包。这不是妥协，而是更好的形态——验证的是真实数据，不是人造数据。

**教训**：当一个变通方案需要反复绕过框架的可见性/身份约束时，应当尽早退回到「用真实数据」
这条最简单也最可信的路径。这与 `_patterns.md` 第十三条（oracle 无法直接调用原版时的处理）
互补：那条讲「不能调用怎么办」，这条讲「能调用但构造代价过高时怎么办」。

## 两侧分工

| 侧 | 职责 |
|---|---|
| harness（`listEnchantmentCompatUnits`） | 读数据包，展开 `#tag`，生成全部配对单元 |
| 探针 | 复刻三行布尔判定，输入是已展开的扁平列表 |
| oracle | 从注册表取真实 Holder，调用原版 `Enchantment.areCompatible` |

**`exclusiveSet` 的展开由 harness 完成**，两侧收到同一份扁平列表。若探针或 oracle 对数据包的
理解与 harness 不一致，结果就会不同——这是本能力能捕获的真实错误。

单元使用 `kind: "computed"`，因此 `vanillaDataPackDir()` 取的是真实数据包路径（computed units
在 runner 注入 `MC_DIFF_VAULT_DATAPACK` 之前求值）。

## 未覆盖范围

- `exclusive_set` 的 `HolderSet.direct` 形态（直接列 id 而非标签）
- 附魔不存在时的行为（`absent` 分支已实现但未触发）
- 互斥对实际能否同时出现在物品上（依赖 `canEnchant`，见 `enchantment_applicability`）