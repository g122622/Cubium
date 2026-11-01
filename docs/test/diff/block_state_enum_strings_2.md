# 差分能力：`block_state_enum_strings_2`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 180 个能力，78 个用例全部一致。**探针调用 Cubium 的
`EnumProperty<T>::Traits::toString` / `fromName`**，一次覆盖 11 个方块状态枚举。

**本能力未发现 Cubium 缺陷**，但钉住了 `PistonType.DEFAULT` 这个「常量名 ≠ 序列化名」的陷阱。

## 核心发现一：`PistonType.DEFAULT` 的序列化名是 `normal`

```java
public enum PistonType implements StringRepresentable {
    DEFAULT("normal"),
    STICKY("sticky");
}
```

| 常量 | 序列化名 |
|---|---|
| **`DEFAULT`** | **`normal`** |
| `STICKY` | `sticky` |

**常量名与序列化名完全不同**——`DEFAULT` → `normal`，没有共同前缀。

实测：

```
PistonType|toString|DEFAULT      -> normal
PistonType|fromName|normal       -> DEFAULT
PistonType|fromName|default      -> null      ← default 不是合法序列化名
```

**Cubium 的 `PistonType::Default` 正确返回 `normal`。**

若按「枚举名小写」推导会得到 `default`，活塞头方块状态会整体失配。

## 核心发现二：`SlabType` 与 `Half` 的顺序相反

| 枚举 | 枚举顺序 |
|---|---|
| [`Half`](./block_state_enum_strings.md) | **`TOP`(0), `BOTTOM`(1)** |
| **`SlabType`** | **`BOTTOM`(0), `TOP`(1), `DOUBLE`(2)** |

两个都表示「半部分」，但**索引顺序相反**。

`SlabType` 还多一个 `DOUBLE`（双台阶占满整个方块）。

实测：

```
SlabType|toString|BOTTOM -> bottom
SlabType|toString|TOP    -> top
SlabType|toString|DOUBLE -> double
```

## 核心发现三：`RedstoneSide` 与 `WallSide` 的 `none` 位置相反

| 枚举 | 顺序 |
|---|---|
| **`RedstoneSide`** | **`UP`(0), `SIDE`(1), `NONE`(2)** |
| `WallSide` | **`NONE`(0), `LOW`(1), `TALL`(2)** |

「无连接」在前者排**最后**，在后者排**最前**。

这是调色板索引的隐患：若两者都用 `0` 表示「无」，红石线的 `none` 会被错当成 `up`。

## 核心发现四：`RailShape` 的弯道顺序是顺时针

```
NORTH_SOUTH, EAST_WEST,
ASCENDING_EAST, ASCENDING_WEST, ASCENDING_NORTH, ASCENDING_SOUTH,
SOUTH_EAST, SOUTH_WEST, NORTH_WEST, NORTH_EAST
```

前 6 项是**直线/斜线**，后 4 项是**弯道**。

弯道顺序 **SE → SW → NW → NE，顺时针绕一圈**——既不是字母序，也不是 NSEW 顺序。

**共 10 项**，本能力全部覆盖。

## 核心发现五：原版类名与 Cubium 枚举名不一致

| 原版类 | Cubium 枚举 | 说明 |
|---|---|---|
| `BellAttachType` | `BellAttachment` | **Type vs ment** |
| `DoorHingeSide` | `DoorHinge` | **少了 Side** |

**这两个名字差异不是缺陷**（Cubium 命名更简洁），但写 oracle 时若按 Cubium 的名字去找原版类
会找不到。

## 11 个枚举的完整清单

| 枚举 | 项数 | 序列化名 |
|---|---|---|
| `PistonType` | 2 | `normal` / `sticky` |
| `RedstoneSide` | 3 | `up` / `side` / `none` |
| `DoorHinge` | 2 | `left` / `right` |
| `SlabType` | 3 | `bottom` / `top` / `double` |
| `StairsShape` | 5 | `straight` / `inner_left` / `inner_right` / `outer_left` / `outer_right` |
| `RailShape` | 10 | `north_south` / `east_west` / `ascending_{east,west,north,south}` / `{south,north}_{east,west}` |
| `BellAttachment` | 4 | `floor` / `ceiling` / `single_wall` / `double_wall` |
| `BambooLeaves` | 3 | `none` / `small` / `large` |
| `Tilt` | 4 | `none` / `unstable` / `partial` / `full` |
| `DripstoneThickness` | 5 | `tip_merge` / `tip` / `frustum` / `middle` / `base` |
| `SculkSensorPhase` | 3 | `inactive` / `active` / `cooldown` |

**下划线命名出现于**：`inner_left`、`outer_right`、`north_south`、`ascending_east`、
`single_wall`、`tip_merge` 等——**多词项全部用下划线**，没有驼峰。

这与 [`team_rule_strings`](./team_rule_strings.md) 的驼峰（`hideForOtherTeams`）、
[`render_type_strings`](./render_type_strings.md) 的单单词形成对照。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`block_state_enum_strings`](./block_state_enum_strings.md) | 第一组（5 个枚举），含 `Half` |
| [`block_state_names`](./block_state_names.md) | 方块状态名组合 |
| [`block_state_ids`](./block_state_ids.md) | 状态调色板索引 |
| [`redstone_signal`](./redstone_signal.md) | 红石连接计算（`RedstoneSide` 的消费者）|

## 未覆盖范围

- 剩余方块状态枚举：`NoteBlockInstrument` / `ComparatorMode` / `StructureMode` /
  `TestBlockMode` / `OxidationLevel` / `TrialSpawnerState` / `VaultState` /
  `CreakingHeartState` / `SideChainPart` / `CopperGolemPose` / `WallHeight`
- `Direction` / `Axis`（已有 `direction_rotate` 等能力覆盖部分）
- 各枚举在 `block_state_ids` 调色板中的实际索引