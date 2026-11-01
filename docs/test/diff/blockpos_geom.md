# 差分能力：`blockpos_geom`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 128 个能力，34 个用例全部一致。`BlockPos` 的几何变换（旋转、叉积、极值、缩放）此前未被
覆盖——[`blockpos_codec`](./blockpos_codec.md) 覆盖 `offset` 与 `pack`，
[`blockpos_dist`](./blockpos_dist.md) 覆盖距离，都不涉及变换。

## 核心发现一：`containing` 用 `Mth.floor`，向负无穷取整

```java
public static BlockPos containing(double x, double y, double z) {
    return new BlockPos(Mth.floor(x), Mth.floor(y), Mth.floor(z));
}
```

| 单元 | 结果 |
|---|---|
| `containing\|1.9,2.1,3.5` | `1,2,3` |
| **`containing\|-0.5,-1.5,-0.1`** | **`-1,-2,-1`** |
| **`containing\|5.999,6.001,-3.5`** | **`5,6,-4`** |
| `containing\|-1.0,-2.0,-3.0` | `-1,-2,-3` |
| `containing\|-0.0,-0.0,-0.0` | `0,0,0` |

**`(int)` 强转（向零截断）会得 `0,-1,0`，与原版不符。**

**这是负坐标放置方块时 1 格偏差的经典来源**：玩家站在 `x = -0.5`（跨在 `x=0` 与 `x=-1` 两格的
边界偏负侧），原版认为他们处在 `x = -1` 格，而向零截断会认为处在 `x = 0` 格。

`-0.0` 的 floor 是 `0`（不是 `-1`），因为 IEEE 754 的负零在数值上等于零。

## 核心发现二：`rotate` 的 Y 坐标恒不变，但 X/Z 的变换不对称

```java
case CLOCKWISE_90        -> new BlockPos(-this.getZ(),  this.getY(),  this.getX());
case CLOCKWISE_180       -> new BlockPos(-this.getX(),  this.getY(), -this.getZ());
case COUNTERCLOCKWISE_90 -> new BlockPos( this.getZ(),  this.getY(), -this.getX());
case NONE                -> this;
```

| 单元 | 结果 |
|---|---|
| `rotate\|NONE\|5,3,7` | `5,3,7` |
| **`rotate\|CLOCKWISE_90\|5,3,7`** | **`-7,3,5`** |
| **`rotate\|CLOCKWISE_180\|5,3,7`** | **`-5,3,-7`** |
| **`rotate\|COUNTERCLOCKWISE_90\|5,3,7`** | **`7,3,-5`** |
| **`rotate\|CLOCKWISE_90\|0,99,0`** | **`0,99,0`** |
| `rotate\|CLOCKWISE_90\|0,0,0` | `0,0,0` |

**Y 坐标在任何旋转下都不变**——这是水平旋转的定义。X/Z 的符号在 `CLOCKWISE_90` 与
`COUNTERCLOCKWISE_90` 之间**互换**，但取负的位置不同：`cw` 是 `x → -z, z → x`，`ccw` 是
`x → z, z → -x`。

**注意 `CLOCKWISE_90` 不是 `COUNTERCLOCKWISE_90` 的简单取反**：对 `(5,3,7)`，`cw` 得
`(-7,3,5)`，其逐分量取反是 `(7,-3,-5)`，而 `ccw` 得 `(7,3,-5)`——**Y 不变**，所以不是相反数。

## 核心发现三：`cross` 是标准右手叉积，但参数类型是 `Vec3i`

```java
cross(v) = (y*v.z - z*v.y,  z*v.x - x*v.z,  x*v.y - y*v.x)
```

| 单元 | 结果 | 说明 |
|---|---|---|
| `cross\|1,0,0\|0,1,0` | `0,0,1` | X × Y = Z |
| **`cross\|1,0,0\|0,0,1`** | **`0,-1,0`** | X × Z = **-Y**（右手系）|
| `cross\|0,1,0\|0,0,1` | `1,0,0` | Y × Z = X |
| `cross\|1,2,3\|1,2,3` | `0,0,0` | 自叉积为零 |
| `cross\|2,3,4\|5,6,7` | `-3,6,-3` | |
| `cross\|0,0,0\|1,2,3` | `0,0,0` | 零向量 |

**`X × Z = -Y` 而非 `+Y`**，这是右手系的必然结果（`X × Y = Z`、`Y × Z = X`、`Z × X = Y`）。

参数类型是 `Vec3i` 而非 `BlockPos`，因此任何 `Vec3i` 子类都能参与运算。`int` 乘法**可能溢出**，
原版不做保护。

## 核心发现四：`multiply` 的短路只影响对象身份，不影响坐标值

```java
public BlockPos multiply(int factor) {
    if (factor == 1) return this;
    return factor == 0 ? ZERO : new BlockPos(x * factor, y * factor, z * factor);
}
```

| 单元 | 结果 |
|---|---|
| `multiply\|5,3,7\|0` | `0,0,0` |
| `multiply\|5,3,7\|1` | `5,3,7` |
| `multiply\|5,3,7\|2` | `10,6,14` |
| **`multiply\|5,3,7\|-1`** | **`-5,-3,-7`** |
| `multiply\|-5,3,-7\|-3` | `15,-9,21` |

**`multiply(0)` 返回共享的 `BlockPos.ZERO` 常量**，但坐标值就是 `(0,0,0)`，与直接计算一致。
短路只影响**是否返回同一实例**（内存层面），不影响可观测的坐标。

**对负数因子没有保护**：`multiply(-1)` 正常返回 `(-x,-y,-z)`。

## 其余实测

| 单元 | 结果 |
|---|---|
| `min\|5,3,7\|2,8,4` | `2,3,4` |
| `max\|5,3,7\|2,8,4` | `5,8,7` |
| `max\|-5,-3,-7\|-2,-8,-4` | `-2,-3,-4` |
| `subtract\|5,3,7\|2,8,4` | `3,-5,3` |
| `subtract\|0,0,0\|1,2,3` | `-1,-2,-3` |

`min`/`max` 是**逐分量**取极值，不是取「字典序更小/更大的那个点」。

## 本轮情况：一次通过

与 [`rotation_info`](./rotation_info.md) 相同，本轮在写代码前完整读取了 `BlockPos.java` 的
`rotate` / `cross` / `containing` / `min` / `max` / `multiply` 实现，没有留下猜测空间。

**对比**：`direction_plane` 因为按参数位置猜构造器含义而错了 9 处；本轮 `BlockPos` 的方法都是
直接的算术表达式，读一遍即可。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`blockpos_codec`](./blockpos_codec.md) | `offset` / `pack` / `asLong` |
| [`blockpos_dist`](./blockpos_dist.md) | `distSqr` / `distManhattan` / `betweenClosed` |
| [`blockpos_bfs`](./blockpos_bfs.md) | 邻域遍历 |
| [`rotation_info`](./rotation_info.md) | `Rotation` 枚举本身；本能力用它旋转坐标 |
| [`block_transform`](./block_transform.md) | `Rotation.rotate(Direction)` 方向旋转，与本能力的坐标旋转配套 |
| [`mth_int_div`](./mth_int_div.md) | `Mth.floor` 的整数版本 |

## 未覆盖范围

- `offset` 系列（`blockpos_codec` 已覆盖 `offset`）
- `asLong` / `getFlatIndex`（`blockpos_codec` 已覆盖 `pack`）
- `getCenter` / `getBottomCenter` / `getCenter(Alignment)`（返回 `Vec3`，涉及浮点表示）
- `betweenClosed` / `betweenClosedStream`（`blockpos_dist` 已覆盖 `betweenClosed`）
- `BlockPos.breadthFirstTraversal`（`blockpos_bfs` 已覆盖）
- `Mth.floor` 的浮点边界（`NaN`、`±Infinity`）