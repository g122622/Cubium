# 差分能力：`vec3i_arith`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 129 个能力，37 个用例全部一致。`Vec3i`（`BlockPos` 的父类）的整数算术与排序此前未被覆盖
——[`vec3_math`](./vec3_math.md) 覆盖的是**浮点 `Vec3`** 的 `cross`/`dot`/`normalize`/`scale`。

## 核心发现一：`compareTo` 的优先级是 Y → Z → X

```java
public int compareTo(Vec3i other) {
    if (this.getY() == other.getY()) {
        return this.getZ() == other.getZ()
            ? this.getX() - other.getX()
            : this.getZ() - other.getZ();
    }
    return this.getY() - other.getY();
}
```

| 单元 | 结果 | 说明 |
|---|---|---|
| **`compare\|5,0,0\|0,1,0`** | **`-1`** | **Y 更大者排在后面，X 的优势被忽略** |
| **`compare\|0,1,0\|5,0,0`** | **`1`** | |
| **`compare\|0,0,5\|0,1,0`** | **`-1`** | Y 优先于 Z |
| **`compare\|1,0,0\|0,0,0`** | **`1`** | 只有 Y、Z 都相同时才比 X |
| `compare\|0,0,0\|0,0,1` | `-1` | Z |
| `compare\|1,2,3\|1,2,3` | `0` | |

**Y 是第一关键字，X 反而最后。**

这是方块排序的关键：若 Cubium 用常见的 `(x, y, z)` 字典序，**所有依赖坐标排序的迭代顺序都会与
原版不同**——包括结构生成、方块实体更新、随机刻遍历。这类差异不会立即崩溃，但会让「按顺序取
第一个」的逻辑选到不同的方块。

**注意返回值是差值，不是 `-1/0/1`。** 差值可能很大，也可能因 `int` 溢出而为负（例如
`compare(2147483647,... , -1,...)`）。

## 核心发现二：`distManhattan` 内部用 `float`，会丢精度

```java
public int distManhattan(Vec3i other) {
    float f  = Math.abs(other.getX() - this.getX());   // int → float
    float f1 = Math.abs(other.getY() - this.getY());
    float f2 = Math.abs(other.getZ() - this.getZ());
    return (int) (f + f1 + f2);
}
```

| 单元 | 结果 | 纯整数应为 |
|---|---|---|
| `manhattan\|0,0,0\|1,2,3` | `6` | 6 |
| `manhattan\|0,0,0\|8388608,0,0` | `8388608` | 8388608 |
| **`manhattan\|0,0,0\|16777216,0,0`** | `16777216` | 16777216（恰好是 `2^24`，可精确表示）|
| **`manhattan\|0,0,0\|16777217,0,0`** | **`16777216`** | **16777217（少 1）** |
| `manhattan\|0,0,0\|9000000,9000000,9000000` | `27000000` | 27000000 |

**`float` 只有 24 位尾数，大于 `2^24 = 16777216` 的整数无法精确表示。** `16777217` 被舍入成
`16777216`，结果**少 1**。

**这是原版的真实行为，不是 bug 也不该「修正」**——Cubium 若要逐字节对齐，`distManhattan` 也必须
用 `float` 中间量。若改用 `int` 计算，在超过 1677 万格的距离上会与原版差 1。

`distChessboard` 则**全程用 `int`**：

```java
int i = Math.abs(this.getX() - other.getX());
int j = Math.abs(this.getY() - other.getY());
int k = Math.abs(this.getZ() - other.getZ());
return Math.max(i, Math.max(j, k));
```

| 单元 | 结果 |
|---|---|
| `chessboard\|0,0,0\|1,2,3` | `3` |
| `chessboard\|0,0,0\|9000000,9000000,9000000` | **`9000000`**（无精度损失）|

**同一个类里两个距离方法的数值类型不同**，这是本能力最需要留意的对比。

## 核心发现三：`distSqr` 返回 `double`，不是 `int`

```java
public double distSqr(Vec3i other) {
    return this.distToLowCornerSqr(other.getX(), other.getY(), other.getZ());
}
```

本能力**不覆盖**它（涉及浮点输出格式）。调用时务必看清返回类型——它与
[`blockpos_dist`](./blockpos_dist.md) 的整数版本可能不同。

## 其余实测

| 单元 | 结果 |
|---|---|
| `relative\|0,0,0\|UP\|1` | `0,1,0` |
| `relative\|0,0,0\|UP\|0` | `0,0,0`（返回同一实例）|
| **`relative\|0,0,0\|UP\|-1`** | **`0,-1,0`**（负步数反向）|
| `relative\|5,3,7\|NORTH\|2` | `5,3,5` |
| **`relative\|5,3,7\|EAST\|-3`** | **`2,3,7`**（`5 + 1×(-3)`）|
| `multiply\|5,3,7\|0` | `0,0,0` |
| `multiply\|5,3,7\|1` | `5,3,7`（返回同一实例）|
| `multiply\|5,3,7\|-1` | `-5,-3,-7` |
| `multiply3\|5,3,7\|2,-1,0` | `10,-3,0` |
| `offset\|5,3,7\|1,2,3` | `6,5,10` |
| `offset\|5,3,7\|-1,-2,-3` | `4,1,4` |
| `subtract\|5,3,7\|2,8,4` | `3,-5,3` |

`relative(dir, 0)` 与 `multiply(1)` 都**返回同一实例**（短路），但坐标值不变，只影响对象身份。

## 本轮情况：一次通过

与 `rotation_info`、`blockpos_geom` 相同，写代码前完整读取了 `Vec3i.java` 的
`compareTo` / `distManhattan` / `distChessboard` / `relative` / `multiply` / `offset` /
`subtract` 实现。

**`distManhattan` 的 `float` 中间量很容看漏**——方法签名返回 `int`，参数也是 `int`，只有读进
方法体才能看到三个 `float` 局部变量。这也是为什么必须读到方法体而不能只看签名。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`vec3_math`](./vec3_math.md) | **浮点 `Vec3`** 的 `cross` / `dot` / `normalize` / `scale`；本能力是整数 `Vec3i` |
| [`blockpos_geom`](./blockpos_geom.md) | `BlockPos` 的重写版本（`multiply` 等行为略有不同）|
| [`blockpos_dist`](./blockpos_dist.md) | `BlockPos` 的距离方法 |
| [`blockpos_codec`](./blockpos_codec.md) | `offset` / `pack` |

**注意 `Vec3i.multiply(int)` 与 `BlockPos.multiply(int)` 行为不同**：`BlockPos` 版本对 `0` 返回
`BlockPos.ZERO` 常量，`Vec3i` 版本没有这个特判（但坐标值相同）。

## 未覆盖范围

- `distSqr` / `distToCenterSqr` / `distToLowCornerSqr`（返回 `double`）
- `closerThan` / `closerToCenterThan`（浮点比较）
- `closestToCenter`（`BlockPos` 版本，需 `List`）
- `relative(Axis, int)`
- `above`/`below`/`north`/`south`/`west`/`east` 系列（等价于 `relative`，已被间接覆盖）
- `offsetCodec`（`blockpos_codec` 已覆盖 `offset` 的编解码）