# 差分能力：`enum_property_order`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 183 个能力，**修复前 28 处差异，修复后 36/36 一致**。**探针调用 Cubium 的
`EnumProperty::allowedValues()` / `Property::indexOf`**。

**本能力发现并修复了 2 处真实缺陷。**

## 缺陷一：`FACING` 用了枚举声明顺序，原版是显式列表

### 原版

```java
// BlockStateProperties.java:53
public static final EnumProperty<Direction> FACING = EnumProperty.create(
    "facing", Direction.class,
    Direction.NORTH, Direction.EAST, Direction.SOUTH, Direction.WEST,
    Direction.UP, Direction.DOWN);
```

**这是一个显式列表，不是 `Direction.values()` 的顺序。**

### Cubium（修复前）

```cpp
static const DirectionProperty& FACING()
{
    static auto prop = DirectionProperty::create("facing");   // ← 走 Directions::all()
    return *prop;
}
```

`DirectionProperty::create(name)` 内部使用：

```cpp
// src/common/util/Direction.hpp:162
inline std::array<Direction, 6> all()
{
    return {Direction::Down, Direction::Up, Direction::North,
            Direction::South, Direction::West, Direction::East};
}
```

### 差异

| 位置 | 原版 `FACING` | Cubium（修复前） |
|---|---|---|
| 0 | `NORTH` | **`DOWN`** |
| 1 | `EAST` | **`UP`** |
| 2 | `SOUTH` | **`NORTH`** |
| 3 | `WEST` | **`SOUTH`** |
| 4 | `UP` | **`WEST`** |
| 5 | `DOWN` | **`EAST`** |

**两者完全不同，连首项都不同。**

**注意**：底层 `Direction` 枚举本身的顺序两边**完全一致**
（`DOWN=0, UP=1, NORTH=2, SOUTH=3, WEST=4, EAST=5`）——差异只在**属性**这一层。

### 为什么这是真缺陷：调试棒行为

原版：

```java
// StateHolder.java:46
public <T extends Comparable<T>> S cycle(Property<T> prop) {
    return this.setValue(prop, findNextInCollection(prop.getPossibleValues(), this.getValue(prop)));
}

// DebugStickItem.java
private static <T extends Comparable<T>> BlockState cycleState(BlockState state, Property<T> prop,
                                                               boolean backwards) {
    return state.setValue(prop, getRelative(prop.getPossibleValues(), state.getValue(prop), backwards));
}
private static <T> T getRelative(Iterable<T> values, @Nullable T value, boolean backwards) {
    return backwards ? Util.findPreviousInIterable(values, value) : Util.findNextInIterable(values, value);
}
```

Cubium（`DebugStickItem.cpp`）：

```cpp
const size_t nextValueIndex = _relativeIndex(selectedProp->valueCount(), currentValueIndex.value(), backwards);
const BlockState& newState = state.withValueIndex(*selectedProp, nextValueIndex);
// _relativeIndex: (currentIndex + 1) % count
```

**两边逻辑完全等价，差异全部来自 `getPossibleValues()` 的顺序。**

玩家用调试棒右键一个六向方块（如墙上的按钮、发射器、活塞）时：

| 当前朝向 | 原版点击后 | Cubium（修复前）点击后 |
|---|---|---|
| `NORTH` | **`EAST`** | `SOUTH` |
| `EAST` | **`SOUTH`** | `DOWN` |
| `SOUTH` | **`WEST`** | `WEST`（巧合相同）|
| `WEST` | **`UP`** | `EAST` |
| `UP` | **`DOWN`** | `NORTH` |
| `DOWN` | **`NORTH`** | `UP` |

**6 个方向里有 5 个的演化路径不同。** 玩家按原版经验操作会得到意料之外的结果。

### 影响面

473 处代码引用 `FACING()`，覆盖**全部六向方块**：按钮、拉杆、发射器、投掷器、
活塞、观察者、末地传送门框架、南瓜灯、雕刻南瓜、避雷针等。

调试棒本身已实现（`src/common/item/items/special/DebugStickItem.cpp`），
并非未完成功能。

### 修复

```cpp
static const DirectionProperty& FACING()
{
    static auto prop = DirectionProperty::create("facing",
        {Direction::North,
            Direction::East,
            Direction::South,
            Direction::West,
            Direction::Up,
            Direction::Down});
    return *prop;
}
```

## 缺陷二：`HORIZONTAL_FACING` 顺序不正确

### 原版

```java
public static final EnumProperty<Direction> HORIZONTAL_FACING =
    EnumProperty.create("facing", Direction.class, Direction.Plane.HORIZONTAL);
```

`Plane.HORIZONTAL` 的定义：

```java
// Direction.java:64
.filter(d -> d.getAxis().isHorizontal())
.sorted(Comparator.comparingInt(d -> d.data2d))
.toArray(Direction[]::new);
```

**按 `data2d` 字段升序排序**：

| 方向 | `data2d` |
|---|---|
| `SOUTH` | **0** |
| `WEST` | **1** |
| `NORTH` | **2** |
| `EAST` | **3** |

所以原版 `HORIZONTAL_FACING` = **`NORTH, SOUTH, WEST, EAST`**。

### Cubium（修复前）

```cpp
static auto prop = DirectionProperty::createHorizontal("facing");
```

`createHorizontal` 内部使用：

```cpp
// src/common/util/Direction.hpp
inline std::array<Direction, 4> horizontal()
{
    return {Direction::North, Direction::East, Direction::South, Direction::West};
}
```

即 **`NORTH, EAST, SOUTH, WEST`**——按顺时针排列，而不是 `data2d` 顺序。

### 差异

| 位置 | 原版 | Cubium（修复前） |
|---|---|---|
| 0 | `NORTH` | `NORTH`（相同）|
| 1 | **`SOUTH`** | **`EAST`** |
| 2 | **`WEST`** | **`SOUTH`** |
| 3 | **`EAST`** | **`WEST`** |

**首项相同，后三项全部错位。** 这是最容易漏掉的情况——只抽查首项会误判为一致。

调试棒行为：

| 当前朝向 | 原版 | Cubium（修复前） |
|---|---|---|
| `NORTH` | **`SOUTH`** | `EAST` |
| `EAST` | **`NORTH`** | `SOUTH` |
| `SOUTH` | **`WEST`** | `WEST`（巧合相同）|
| `WEST` | **`EAST`** | `NORTH` |

### 影响面

`HORIZONTAL_FACING()` 覆盖**全部四向方块**：楼梯、台阶、箱子、熔炉、门、告示牌、
床、藤蔓、梯子、漏斗、工作台等——这是方块状态里最常见的属性。

### 修复

```cpp
static const DirectionProperty& HORIZONTAL_FACING()
{
    static auto prop = DirectionProperty::create("facing",
        {Direction::North,
            Direction::South,
            Direction::West,
            Direction::East});
    return *prop;
}
```

## 修复验证

```
修复前：[1/1] enum_property_order   差异    28 处差异（36 个单元）
修复后：[1/1] enum_property_order   通过    36/36 一致
```

`mc_tests` 全量构建通过（退出码 0）。

## 核心教训：`data2d` 是一个隐藏的排序键

原版 `Direction` 的构造函数有**三个整数字段**：

```java
NORTH(2, 3, 2, "north", ...),
//     ^  ^  ^
//     |  |  data2d   ← 这个
//     |  data3d
//     ordinal
```

| 方向 | `data3d` | `data2d` | `ordinal` |
|---|---|---|---|
| `DOWN` | 0 | **-1** | 0 |
| `UP` | 1 | **-1** | 1 |
| `NORTH` | 2 | **2** | 2 |
| `SOUTH` | 3 | **0** | 3 |
| `WEST` | 4 | **1** | 4 |
| `EAST` | 5 | **3** | 5 |

`Direction.Plane.HORIZONTAL` 用 **`data2d`** 排序，而不是 `ordinal`：

```
按 ordinal:        DOWN, UP, NORTH, SOUTH, WEST, EAST
按 data2d 过滤:    SOUTH(0), WEST(1), NORTH(2), EAST(3)
```

**`data2d` 的顺序（`SOUTH, WEST, NORTH, EAST`）与 `ordinal` 顺序完全不同**，
这在读源码时极易忽略——构造函数里三个数字挨在一起，看不出哪个是排序键。

## 与上一轮 `boolean_property_index` 的对照

| 维度 | `boolean_property_index` | `enum_property_order` |
|---|---|---|
| 索引顺序差异 | 有（`{false,true}` vs `[true,false]`）|
| 影响状态调色板 | 是 | 是 |
| 是否通过网络泄漏 | **否**（按状态名映射）|
| **是否影响可见行为** | **否** | **是（调试棒）** |
| 判定 | 无害差异 | **真实缺陷** |

**关键区别**：布尔属性只有 2 个值，`cycle()` 的效果是 `true ↔ false`，与顺序无关；
而枚举属性有 3~6 个值，顺序直接改变循环路径。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`boolean_property_index`](./boolean_property_index.md) | 布尔属性的索引顺序（无害）|
| [`direction_rotate`](./direction_rotate.md) | `Direction` 的旋转辅助方法 |
| [`direction_nearest`](./direction_nearest.md) | 由向量求最近方向 |
| [`front_and_top`](./front_and_top.md) | `FrontAndTop` 枚举 |
| [`block_state_ids`](./block_state_ids.md) | 状态调色板索引 |

## 未覆盖范围

- `FACING_HOPPER`（原版用谓词排除 `UP`）——Cubium 是否有对应属性待确认
- 其余 `EnumProperty`（`Axis` / `SlabType` / `StairsShape` 等）的值顺序
- `IntegerProperty` 的值顺序（由 `create(name, min, max)` 单调生成，无歧义）
- `cycle()` 在**多属性状态**下的完整行为（本能力只验证单属性等价）