# 差分能力：`equipment_slot`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 123 个能力，47 个用例全部一致。装备槽位与槽位组此前完全未被覆盖。

## 四个关键点

**1. `isArmor()` 包含 `BODY`（动物护甲），但不包含 `SADDLE`。**

```java
public boolean isArmor() {
    return type == Type.HUMANOID_ARMOR || type == Type.ANIMAL_ARMOR;
}
```

| 单元 | 结果 |
|---|---|
| **`groupMembers\|ARMOR`** | **`FEET,LEGS,CHEST,HEAD,BODY`** |
| `groupTest\|ARMOR\|BODY` | `true` |
| **`groupTest\|ARMOR\|SADDLE`** | **`false`** |

**`ARMOR` 组覆盖五个槽位，不是四个。** 经典的「四件套」说法忽略了 `BODY`——那是给狼、马等
动物穿的护甲。

`SADDLE` 是独立的 `Type.SADDLE`，**不算护甲**，尽管它在装备菜单里也占一格。

**2. 4 参数构造器的第 3 个参数是 `id`，不是 `countLimit`。**

这是本能力最容易抄错的地方。

```java
private EquipmentSlot(Type type, int index, int countLimit, int id, String name) { ... }

private EquipmentSlot(Type type, int index, int id, String name) {
    this(type, index, 0, id, name);   // countLimit 固定为 0（NO_COUNT_LIMIT）
}
```

`MAINHAND`/`OFFHAND` 用的是 **4 参数**版本，`FEET`/`LEGS`/`CHEST`/`HEAD`/`BODY` 用的是
**5 参数**版本，`SADDLE` 又是 4 参数。

| 槽位 | 构造器 | type | index | countLimit | id |
|---|---|---|---|---|---|
| `MAINHAND` | 4 参数 | `HAND` | 0 | **0** | 0 |
| `OFFHAND` | 4 参数 | `HAND` | 1 | **0** | **5** |
| `FEET` | 5 参数 | `HUMANOID_ARMOR` | 0 | 1 | 1 |
| `LEGS` | 5 参数 | `HUMANOID_ARMOR` | 1 | 1 | 2 |
| `CHEST` | 5 参数 | `HUMANOID_ARMOR` | 2 | 1 | 3 |
| `HEAD` | 5 参数 | `HUMANOID_ARMOR` | 3 | 1 | 4 |
| `BODY` | 5 参数 | `ANIMAL_ARMOR` | 0 | 1 | **6** |
| `SADDLE` | 4 参数 | `SADDLE` | 0 | **0** | **7** |

实测：

```text
slotInfo|MAINHAND -> HAND,0,0,0,mainhand,false
slotInfo|OFFHAND  -> HAND,1,0,5,offhand,false
slotInfo|FEET     -> HUMANOID_ARMOR,0,1,1,feet,true
slotInfo|CHEST    -> HUMANOID_ARMOR,2,1,3,chest,true
slotInfo|BODY     -> ANIMAL_ARMOR,0,1,6,body,true
slotInfo|SADDLE   -> SADDLE,0,1,7,saddle,false
```

**注意 `SADDLE` 的 `countLimit` 是 1**——虽然它用 4 参数构造器，但那个 4 参数版本是另一条
重载 `(type, index, countLimit, name)`（5 参数的直接写法）。源码里 `SADDLE` 声明为
`SADDLE(EquipmentSlot.Type.SADDLE, 0, 1, 7, "saddle")`，是 **5 参数**版本。

**3. 数字 `id` 有空洞——没有 5。**

`id` 序列是 `0, 5, 1, 2, 3, 4, 6, 7`（按枚举顺序）。**`OFFHAND` 的 id 是 5，跳过了 5 之前的
连续段**，而 `BODY` 直接跳到 6。

`getFilterBit(offset) = id + offset`，所以筛选位也继承了这个空洞：

| 单元 | 结果 |
|---|---|
| `filterBit\|MAINHAND\|0` | `0` |
| `filterBit\|CHEST\|0` | `3` |
| **`filterBit\|BODY\|0`** | **`6`** |
| `filterBit\|SADDLE\|0` | `7` |
| **`filterBit\|HEAD\|10`** | **`14`**（`4 + 10`） |

**4. `canIncreaseExperience()` 只排除 `SADDLE`。**

```java
public boolean canIncreaseExperience() {
    return this.type != EquipmentSlot.Type.SADDLE;
}
```

| 单元 | 结果 |
|---|---|
| `canXp\|MAINHAND` | `true` |
| `canXp\|CHEST` | `true` |
| `canXp\|BODY` | `true` |
| **`canXp\|SADDLE`** | **`false`** |

**动物护甲（`BODY`）能加经验**，只有鞍具不能。

## 槽位组

`EquipmentSlotGroup` 共 11 个，`BY_ID` 越界返回 `ANY`（`OutOfBoundsStrategy.ZERO`，与
[`dye_color`](./dye_color.md) 同一策略）：

| 单元 | 结果 |
|---|---|
| `groupById\|0` | `ANY` |
| `groupById\|3` | `HAND` |
| `groupById\|8` | `ARMOR` |
| `groupById\|10` | `SADDLE` |
| **`groupById\|11`** | **`ANY`**（越界） |
| **`groupById\|-1`** | **`ANY`** |
| **`groupById\|999`** | **`ANY`** |

各组匹配的槽位：

```text
ANY      -> MAINHAND,OFFHAND,FEET,LEGS,CHEST,HEAD,BODY,SADDLE    （全部 8 个）
HAND     -> MAINHAND,OFFHAND
ARMOR    -> FEET,LEGS,CHEST,HEAD,BODY                             （5 个）
```

`HAND` 组按 `Type.HAND` 判定，`ARMOR` 组按 `isArmor()` 判定，**语义层次不同**。

## `getName()` 返回小写

`slotInfo` 输出里的名字段是小写（`mainhand`、`offhand`、`body`），来自
`EquipmentSlot.getName()`，而不是枚举名 `name()`。这两者不同，容易混用。

## 本轮踩坑：三处错误，逐次被测试抓出

1. **`countLimit` 与 `id` 写反**：`slotInfo|OFFHAND` 期望 `...,0,5,...`，我写成 `...,5,1,...`。
   根因是把 4 参数构造器的第 3 个参数当成了 `countLimit`。
2. **`SADDLE` 的 `countLimit` 写成 0**：实际是 1。
3. **`getName()` 大小写**：我用了枚举名（`BODY`），实际是小写（`body`）。

**三处都是抄写错误，全部被差分测试捕获。** 这再次说明逐单元核对输出的必要性。

## 未覆盖范围

- `EquipmentSlot.byName(String)`（按序列化名查找）
- `EquipmentSlot.BY_ID` 的独立验证（已通过 `filterBit` 间接覆盖 id）
- `limit(ItemStack)`（依赖 `ItemStack` 与组件系统）
- `EquipmentSlotGroup.slots()` 与 `iterator()`（语义等价于 `groupMembers`）
- 自定义槽位组（`createTestGroup` 等，不在原版枚举里）