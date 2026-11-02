# 差分能力：`block_state_enum_strings`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 179 个能力，34 个用例全部一致。**探针调用 Cubium 的
`EnumProperty<T>::Traits::toString` / `fromName`**，一次覆盖 5 个方块状态枚举。

**本能力未发现 Cubium 缺陷**，但钉住了最容易混淆的一对枚举。

## 核心发现：`Half` 与 `DoubleBlockHalf` 完全不通用

```java
public enum Half implements StringRepresentable {
    TOP("top"), BOTTOM("bottom");
}
public enum DoubleBlockHalf implements StringRepresentable {
    UPPER(Direction.DOWN), LOWER(Direction.UP);      // 没有自定义 id 字段
}
```

| 枚举 | 项 | 序列化名 |
|---|---|---|
| `Half` | `TOP` | **`top`** |
| `Half` | `BOTTOM` | **`bottom`** |
| `DoubleBlockHalf` | `UPPER` | **`upper`** |
| `DoubleBlockHalf` | `LOWER` | **`lower`** |

### 交叉验证：两个名字集互不重叠

| 单元 | 结果 |
|---|---|
| **`DoubleBlockHalf\|fromName\|top`** | **`null`** |
| **`Half\|fromName\|upper`** | **`null`** |
| `DoubleBlockHalf\|fromName\|upper` | `UPPER` |
| `Half\|fromName\|top` | `TOP` |

**`top` 只属于 `Half`，`upper` 只属于 `DoubleBlockHalf`。**

这不是理论问题：

- `Half` 用于**楼梯**（`minecraft:oak_stairs[half=top]`）、**台阶**（`slab`）
- `DoubleBlockHalf` 用于**高植物**（`tall_grass`）、**门**（`door`）、**双高方块**

**若把两者混用，所有楼梯与高植物的方块状态都会对不上**，`block_state_ids` 的调色板索引也会错位——
玩家放下的楼梯会变成另一状态，高植物的上半/下半会颠倒。

## 核心发现：`DoubleBlockHalf` 走 `StringRepresentable` 的默认实现

```java
public enum DoubleBlockHalf implements StringRepresentable {
    UPPER(Direction.DOWN),
    LOWER(Direction.UP);

    private final Direction directionToOther;      // 不是名字字段

    private DoubleBlockHalf(Direction direction) { this.directionToOther = direction; }

    @Override
    public String toString() { return this.getSerializedName(); }
}
```

**它没有 `name` 字段**，序列化名来自 `StringRepresentable` 的默认实现
（`name().toLowerCase(Locale.ROOT)`）。

字节码确认：

```
public java.lang.String getSerializedName();
  4: ldc           #65                 // String UPPER
  20: ldc           #73                 // String LOWER
```

它把**大写枚举名**传给 `toLowerCase`，结果是 `upper` / `lower`。

**它还重写了 `toString()`**（返回 `getSerializedName()`）——大多数枚举不重写。

## 核心发现：5 个枚举的全部名字

| 枚举 | 项数 | 序列化名 |
|---|---|---|
| `AttachFace` | 3 | `floor` / `wall` / `ceiling` |
| `BedPart` | 2 | `head` / `foot` |
| `ChestType` | 3 | `single` / `left` / `right` |
| `DoubleBlockHalf` | 2 | `upper` / `lower` |
| `Half` | 2 | `top` / `bottom` |

**全部是单个小写单词**，无下划线、无驼峰、无数字。

### `fromName` 的严格性

`EnumProperty<T>::Traits::fromName` 返回 `std::optional`，未命中返回 `nullopt`——**与原版
`CODEC.byName` 的单参数重载一致**（未知返回 `null`）。

| 单元 | 结果 |
|---|---|
| `AttachFace\|fromName\|dunno` | `null` |
| `AttachFace\|fromName\|FLOOR` | `null`（大小写敏感）|
| `BedPart\|fromName\|dunno` | `null` |
| `ChestType\|fromName\|dunno` | `null` |

## 踩坑：`BlockStateProperties` 是类，不是命名空间

探针最初写成：

```cpp
using namespace mc::BlockStateProperties;      // ← 错误：它是类
```

编译报 `unknown type name 'Half'`。查头文件才确认：

```cpp
namespace mc {
class BlockStateProperties {        // ← 类
public:
    enum class ChestType : u8 { ... };
    enum class DoubleBlockHalf : u8 { ... };
    // ...
};
} // namespace mc
```

正确写法：

```cpp
using mc::BlockStateProperties;
// 使用 BlockStateProperties::Half、BlockStateProperties::DoubleBlockHalf
```

**这是本能力唯一的编译障碍，一共改了 3 轮才收敛**（先猜 `mc::X`，再猜 `using namespace`，
最后查类声明）。

另外 `EnumProperty` 定义在**独立头文件** `EnumProperty.hpp` 中，`Properties.hpp` 不包含它：

```cpp
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
```

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`block_state_names`](./block_state_names.md) | 方块状态名（已覆盖）|
| [`block_state_ids`](./block_state_ids.md) | 状态调色板索引（已覆盖）|
| [`block_properties`](./block_properties.md) | 方块属性表（已覆盖）|
| [`game_type`](./game_type.md) | 同为「小枚举 + 名字映射」|

**本能力补上了这批枚举的字符串层**——`block_state_names` 验证的是状态名组合，
本能力验证的是**单个属性值的字符串形式**，粒度更细。

## 未覆盖范围

- 其余方块状态枚举（`Direction` / `PistonType` / `SlabType` / `StairsShape` / `RailShape` 等）
- `EnumProperty` 的 `getAllValues()` 顺序（影响调色板索引）
- 属性值到 `block_state_ids` 索引的映射
- 方块状态字符串的完整拼装（`minecraft:oak_stairs[facing=north,half=top,shape=straight,waterlogged=false]`）