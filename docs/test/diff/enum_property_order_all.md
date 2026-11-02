# 差分能力：`enum_property_order_all`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 184 个能力，**修复前 3 处差异，修复后 120/120 一致**。**探针调用 Cubium 的
`EnumProperty::allowedValues()` / `IProperty::valueToString`**，覆盖**全部 40 个枚举属性**。

**本能力发现并修复了 3 处真实缺陷。**

## 设计要点：只用 `values` 一行概括

每个属性只产出 3 个单元：

| 单元 | 含义 |
|---|---|
| `values` | 全部值按索引顺序，逗号分隔（**序列化名**，如 `north`）|
| `count` | 值个数 |
| `name` | 属性字符串名 |

**为什么不测 `index` / `at` / `cycle`**：[`enum_property_order`](./enum_property_order.md)
已经证明，`cycle()` 与 `getInternalIndex` 的差异**完全来自** `getPossibleValues()` 的顺序，
逻辑本身两边等价。因此 `values` 序列一致 ⇒ 其余全部一致。

这把 40 个属性的验证压缩到 120 个单元，一次跑完只需 10 秒。

## 缺陷一：`SLAB_TYPE` 顺序是 `top, bottom, double`

### 原版

```java
// SlabType.java
public enum SlabType implements StringRepresentable {
    TOP("top"),
    BOTTOM("bottom"),
    DOUBLE("double");
}
```

**注意 `TOP` 在前。**

### Cubium（修复前）

```cpp
static auto prop = EnumProperty<SlabType>::create("type",
    {SlabType::Bottom, SlabType::Top, SlabType::Double});
//  ^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^
//  BOTTOM 在前                    TOP 在后
```

### 差异

```
原版  = [top,bottom,double]
Cubium= [bottom,top,double]
```

### 影响

`SLAB_TYPE` 用于**全部台阶方块**（石台阶、木台阶、石英台阶等）。

调试棒点击台阶时：

| 当前 | 原版 | Cubium（修复前） |
|---|---|---|
| `top` | **`bottom`** | `double` |
| `bottom` | **`double`** | `top` |
| `double` | **`top`** | `bottom` |

**三步循环的方向完全相反。** 玩家想从双层台阶变回单层，按原版经验点一次会得到 `top`，
而在 Cubium 会得到 `bottom`。

### 本节修正了上一轮的文档错误

[`block_state_enum_strings_2`](./block_state_enum_strings_2.md) 曾写：

> `SlabType` 的枚举顺序是 `BOTTOM`(0), `TOP`(1), `DOUBLE`(2)

**这是错的。** 那一节依据的是 Cubium 侧的 `create()` 调用列表，而不是原版枚举声明。
本能力通过对原版 `.java` 源码的核对纠正了它。

## 缺陷二：`VERTICAL_DIRECTION` 顺序反了

### 原版

```java
// BlockStateProperties.java
public static final EnumProperty<Direction> VERTICAL_DIRECTION =
    EnumProperty.create("vertical_direction", Direction.class, Direction.UP, Direction.DOWN);
```

**显式列表，`UP` 在前。**

### Cubium（修复前）

```cpp
static auto prop = DirectionProperty::create("vertical_direction", [](Direction d) {
    return d == Direction::Up || d == Direction::Down;
});
```

谓词过滤遍历 `Directions::all()` = `DOWN, UP, NORTH, SOUTH, WEST, EAST`，
过滤后得到 **`DOWN, UP`**。

### 差异

```
原版  = [up,down]
Cubium= [down,up]
```

### 影响

`VERTICAL_DIRECTION` 用于**滴水石锥**（`pointed_dripstone`）与**避雷针**等方块。

调试棒点击时 `up ↔ down` 会互换方向——但因为是两个值的循环，
**单次点击结果相同，回退操作次数不同**，且内部调色板偏移不同。

## 缺陷三：`SIDE_CHAIN_PART` 的 `RIGHT`/`LEFT` 位置颠倒

### 原版

```java
public enum SideChainPart implements StringRepresentable {
    UNCONNECTED("unconnected"),
    RIGHT("right"),
    CENTER("center"),
    LEFT("left");
}
```

**`RIGHT` 在 `CENTER` 之前。**

### Cubium（修复前）

```cpp
static auto prop = EnumProperty<SideChainPart>::create("side_chain",
    {SideChainPart::Unconnected, SideChainPart::Left, SideChainPart::Center, SideChainPart::Right});
//  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ LEFT 在前
```

### 差异

```
原版  = [unconnected,right,center,left]
Cubium= [unconnected,left,center,right]
```

### 影响

`SIDE_CHAIN_PART` 用于**1.21.11 新增的书架（shelf）方块**。

调试棒点击时的循环路径：

| 当前 | 原版 | Cubium（修复前） |
|---|---|---|
| `unconnected` | **`right`** | `left` |
| `right` | **`center`** | `unconnected` |
| `center` | **`left`** | `right` |
| `left` | **`unconnected`** | `center` |

**4 个状态循环的方向完全相反**（原版是顺时针 unconnected→right→center→left，
Cubium 是逆时针）。

## 其余 37 个属性全部正确

| 属性 | 值 |
|---|---|
| `FACING` | `north,east,south,west,up,down`（上一轮已修复）|
| `HORIZONTAL_FACING` | `north,south,west,east`（上一轮已修复）|
| `FACING_EXCEPT_UP` | `down,north,south,west,east` |
| `AXIS` | `x,y,z` |
| `HORIZONTAL_AXIS` | `x,z` |
| `CHEST_TYPE` | `single,left,right` |
| `DOUBLE_BLOCK_HALF` | `upper,lower` |
| `HINGE` | `left,right` |
| `BED_PART` | `head,foot` |
| `BELL_ATTACHMENT` | `floor,ceiling,single_wall,double_wall` |
| `STAIRS_SHAPE` | `straight,inner_left,inner_right,outer_left,outer_right` |
| `WALL_HEIGHT_*` | `none,low,tall` |
| `ATTACH_FACE` | `floor,wall,ceiling` |
| `BAMBOO_LEAVES_PROP` | `none,small,large` |
| `HALF` | `top,bottom` |
| `RAIL_SHAPE` | `north_south,east_west,ascending_*,south_east,...`（10 项）|
| `RAIL_SHAPE_STRAIGHT` | `north_south,east_west,ascending_east,...,ascending_south`（6 项）|
| `REDSTONE_*` | `up,side,none` |
| `PISTON_TYPE` | `normal,sticky` |
| `COMPARATOR_MODE` | `compare,subtract` |
| `NOTE_BLOCK_INSTRUMENT` | 23 项 |
| `STRUCTURE_MODE` | `save,load,corner,data` |
| `TEST_BLOCK_MODE` | `start,log,fail,accept` |
| `DRIPSTONE_THICKNESS` | `tip_merge,tip,frustum,middle,base` |
| `TILT` | `none,unstable,partial,full` |
| `SCULK_SENSOR_PHASE` | `inactive,active,cooldown` |
| `TRIAL_SPAWNER_STATE` | 6 项 |
| `VAULT_STATE` | `inactive,active,unlocking,ejecting` |
| `CREAKING_HEART_STATE` | `uprooted,dormant,awake` |
| `COPPER_GOLEM_POSE` | — |
| `ORIENTATION` | 12 项（`down_east,...,south_up`）|

## 核心教训：`EnumProperty.create(name, Class)` 不是「取枚举声明顺序」那么简单

原版 `EnumProperty` 有 4 个 `create` 重载：

```java
create(String, Class)                          // 全部枚举常量，声明顺序
create(String, Class, Predicate<T>)            // 过滤，仍按声明顺序
create(String, Class, T...)                    // ← 显式列表，可以任意顺序
create(String, Class, List<T>)                 // ← 同上
```

**第三个重载可以给出与声明顺序完全不同的排列。** 读源码时若只看到 `EnumProperty.create(...)`
就假设是声明顺序，会漏掉：

- `FACING`：`NORTH, EAST, SOUTH, WEST, UP, DOWN`（不是 `DOWN, UP, NORTH, ...`）
- `HORIZONTAL_FACING`：`Plane.HORIZONTAL`（按 `data2d` 排序，不是 `ordinal`）
- `VERTICAL_DIRECTION`：`UP, DOWN`（不是 `DOWN, UP`）
- `HORIZONTAL_AXIS`：`X, Z`（不是 `X, Y, Z`）
- `RAIL_SHAPE_STRAIGHT`：仅 6 项（不是全部 10 项）

**必须逐个读 `create(...)` 的实参列表，不能凭属性名推断。**

## 踩坑：`IProperty` 的 `valueToString` 重载歧义

```cpp
const auto& prop = PropRef::get();   // 返回派生类型 EnumProperty<T>
prop.valueToString(0);               // ← 重载解析选了 valueToString(const T&)，0 被转成枚举
```

编译报：

```
error: reference to type 'const mc::BlockStateProperties::WallHeight' could not bind
       to an lvalue of type 'size_t'
```

修复：显式转成基类引用。

```cpp
const ::mc::IProperty& prop = PropRef::get();
prop.valueToString(i);       // 现在只匹配 valueToString(size_t)
```

**教训**：`Property<T>` 同时有 `valueToString(const T&)` 和 `valueToString(size_t index)`
两个重载，整数实参会优先匹配 `size_t` 版本——但如果是 `0` 且 `T` 可隐式转换，就可能有歧义。
显式限定基类引用最稳妥。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`enum_property_order`](./enum_property_order.md) | 仅 `FACING` × 2（本能力是其扩展）|
| [`boolean_property_index`](./boolean_property_index.md) | 布尔属性的索引顺序（无害）|
| [`block_state_enum_strings_2`](./block_state_enum_strings_2.md) | 枚举的**字符串层**（本能力是**顺序层**）|
| [`block_state_ids`](./block_state_ids.md) | 状态调色板索引 |

## 未覆盖范围

- `Property::getName(value)` 的输出（已在 `block_state_enum_strings*` 覆盖）
- 各属性在**具体方块**上的组合顺序（`BlockStateDefinition` 的属性排列）
- `IntegerProperty` 的值顺序（`create(name, min, max)` 单调生成，无歧义）
- `FACING_HOPPER` 与原版谓词的**边界**（已验证结果一致，但未测 `create` 重载的其他用法）