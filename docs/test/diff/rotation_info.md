# 差分能力：`rotation_info`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 127 个能力，45 个用例全部一致。`Rotation` 的枚举语义此前未被覆盖
（[`block_transform`](./block_transform.md) 只覆盖 `Rotation.rotate(Direction)` 一个方法）。

## 核心发现一：`CLOCKWISE_180` 的序列化名是 `"180"`

| 枚举 | index | `getSerializedName()` |
|---|---|---|
| `NONE` | 0 | `none` |
| `CLOCKWISE_90` | 1 | `clockwise_90` |
| **`CLOCKWISE_180`** | 2 | **`180`** |
| `COUNTERCLOCKWISE_90` | 3 | `counterclockwise_90` |

实测：

```text
info|NONE                -> 0,none
info|CLOCKWISE_90        -> 1,clockwise_90
info|CLOCKWISE_180       -> 2,180
info|COUNTERCLOCKWISE_90 -> 3,counterclockwise_90
```

**`CLOCKWISE_180` 是四个里唯一不符合「小写下划线」惯例的。** 若按枚举名机械转小写会得到
`clockwise_180`，与原版不符，会让使用 `Rotation.CODEC` 的方块状态解析失败。

## 核心发现二：`BY_ID` 用 `WRAP`，是第四种越界策略

```java
public static final IntFunction<Rotation> BY_ID =
    ByIdMap.continuous(Rotation::getIndex, values(), ByIdMap.OutOfBoundsStrategy.WRAP);

// ByIdMap
case WRAP -> id -> at[Mth.positiveModulo(id, length)];
```

| 单元 | 结果 | 推导 |
|---|---|---|
| `byId\|0` | `NONE` | |
| `byId\|3` | `COUNTERCLOCKWISE_90` | |
| **`byId\|4`** | **`NONE`** | `4 % 4 = 0`（正数越界回绕）|
| `byId\|5` | `CLOCKWISE_90` | |
| `byId\|9` | `CLOCKWISE_90` | `9 % 4 = 1` |
| **`byId\|-1`** | **`COUNTERCLOCKWISE_90`** | `positiveModulo(-1,4) = 3` |
| `byId\|-2` | `CLOCKWISE_180` | `positiveModulo(-2,4) = 2` |
| `byId\|-4` | `NONE` | `positiveModulo(-4,4) = 0` |

### 已确认的四种越界策略

| 能力 | 策略 | `id = -1` 的结果 |
|---|---|---|
| [`dye_color`](./dye_color.md) / [`equipment_slot`](./equipment_slot.md) / [`heightmap_types`](./heightmap_types.md) | `ByIdMap.ZERO` | 第 0 项 |
| [`direction_plane`](./direction_plane.md) | `abs(id % len)` | `abs(-1 % 6) = 1` → 第 1 项 |
| **`rotation_info`** | **`ByIdMap.WRAP`** | **`positiveModulo(-1, 4) = 3`** → 第 3 项 |
| （`ByIdMap.CLAMP`） | 尚未在已覆盖能力中出现 | 第 0 项 |

**`positiveModulo` 与 `abs(a % b)` 在负数上的差别**：

| 表达式 | `a = -1, b = 4` | 结果项 |
|---|---|---|
| `positiveModulo(-1, 4)` | `3` | 第 3 项 |
| `abs(-1 % 4)` | `1` | 第 1 项 |

**两者都回绕，但落在不同的项上。** 这是 `direction_plane` 与 `rotation_info` 结果不同的根本原因。

## 核心发现三：`rotate(Direction)` 对 Y 轴恒等

```java
public Direction rotate(Direction dir) {
    if (dir.getAxis() == Axis.Y) return dir;
    ...
}
```

| 单元 | 结果 |
|---|---|
| **`rotateDir\|CLOCKWISE_90\|UP`** | **`UP`** |
| **`rotateDir\|CLOCKWISE_90\|DOWN`** | **`DOWN`** |
| **`rotateDir\|CLOCKWISE_180\|UP`** | **`UP`** |
| **`rotateDir\|COUNTERCLOCKWISE_90\|DOWN`** | **`DOWN`** |

**水平旋转永远不改变上下方向。** 水平面的旋转结果：

| 旋转 | `NORTH` → | `EAST` → | `SOUTH` → | `WEST` → |
|---|---|---|---|---|
| `NONE` | `NORTH` | | | |
| `CLOCKWISE_90` | `EAST` | `SOUTH` | `WEST` | `NORTH` |
| `CLOCKWISE_180` | `SOUTH` | `WEST` | | |
| `COUNTERCLOCKWISE_90` | `WEST` | `NORTH` | | |

## 核心发现四：`rotate(int, int)` 的 `modulus * 3 / 4` 是先乘后除

```java
case CLOCKWISE_90        -> (value + modulus / 4) % modulus;
case CLOCKWISE_180       -> (value + modulus / 2) % modulus;
case COUNTERCLOCKWISE_90 -> (value + modulus * 3 / 4) % modulus;
```

**注意 `modulus * 3 / 4`（先乘后除），不是 `modulus / 4 * 3`。**

对能被 4 整除的模数两者相同，但模 6 时不同：

| 表达式 | `modulus = 6` |
|---|---|
| `6 * 3 / 4` | **`4`**（正确）|
| `6 / 4 * 3` | `3`（错误）|

实测：

| 单元 | 结果 |
|---|---|
| `rotateInt\|CLOCKWISE_90\|0\|4` | `1` |
| `rotateInt\|CLOCKWISE_180\|0\|4` | `2` |
| `rotateInt\|COUNTERCLOCKWISE_90\|0\|4` | `3` |
| `rotateInt\|COUNTERCLOCKWISE_90\|1\|4` | `0`（回绕）|
| **`rotateInt\|COUNTERCLOCKWISE_90\|0\|6`** | **`4`**（若写成 `/4*3` 会得 3）|
| `rotateInt\|CLOCKWISE_90\|0\|6` | `1` |
| `rotateInt\|CLOCKWISE_180\|0\|6` | `3` |

**全部用 Java 的 `%`（向零截断），`value` 为负时结果可能为负**——与 `positiveModulo` 不同：

| 单元 | 结果 | 推导 |
|---|---|---|
| `rotateInt\|CLOCKWISE_90\|-1\|4` | `0` | `(-1 + 1) % 4 = 0` |
| **`rotateInt\|COUNTERCLOCKWISE_90\|-1\|6`** | **`3`** | `(-1 + 4) % 6 = 3` |

## `getRotated` 是索引相加取模

```text
getRotated|NONE|NONE                                  -> NONE
getRotated|NONE|CLOCKWISE_90                          -> CLOCKWISE_90
getRotated|CLOCKWISE_90|CLOCKWISE_90                  -> CLOCKWISE_180
getRotated|CLOCKWISE_90|COUNTERCLOCKWISE_90           -> NONE
getRotated|CLOCKWISE_180|CLOCKWISE_180                -> NONE
getRotated|COUNTERCLOCKWISE_90|CLOCKWISE_90           -> NONE
```

`getRotated(other)` 等价于 `(this.index + other.index) % 4`，但原版实现是**两层嵌套 switch**
（4 × 4 = 16 个分支），不是算术。行为等价，实现不同。

## 本轮情况：一次通过

这是近期少见的**一次运行即 0 差异**的能力。原因是在写代码前先完整读取了
`Rotation.java` 的枚举声明、构造器、`getRotated` 的两层 switch、`rotate(Direction)` 与
`rotate(int, int)`，以及 `ByIdMap.WRAP` 的实现，没有留下猜测空间。

**对比前几轮**：`dye_color`（10 处差异）、`equipment_slot`（3 处）、`heightmap_types`（属性校验
拦截）、`direction_plane`（9 处）——那些都是「读了但读得不够细」或「按直觉推断参数含义」导致的。

**结论**：对**枚举声明 + 构造器签名**这类「参数含义不直观」的代码，必须读到字段赋值那一行
（`this.data2d = p_122358_;`）才能确定映射关系，不能靠参数位置猜。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`direction_plane`](./direction_plane.md) | `Direction` 的数据值；本能力的 `getClockWise` 等建立在它的坐标之上 |
| [`block_transform`](./block_transform.md) | 已覆盖 `Rotation.rotate(Direction)`；本能力补充枚举语义与 `rotate(int,int)` |
| [`mth_int_div`](./mth_int_div.md) | `positiveModulo` 与 `abs` 的差别 |

## 未覆盖范围

- `Rotation.getRandom` / `getShuffled`（需要随机源）
- `OctahedralGroup` 的底层群运算（`ROT_90_Y_NEG` 等常量）
- `Rotation.CODEC` / `LEGACY_CODEC` 的编解码形式
- `Mirror` 与本能力的复合变换