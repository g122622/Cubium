# 差分能力：`aabb_factory`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 148 个能力，25 个用例全部一致。补全了 [`aabb_ops`](./aabb_ops.md) 文档中列出的
`AABB.of` / `encapsulatingFullBlocks` / `getCenter` / `getSize` 缺口。

## 核心发现一：`AABB` 构造器无条件归一化 min/max

```java
public AABB(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) {
    this.minX = Math.min(minX, maxX);
    this.minY = Math.min(minY, maxY);
    this.minZ = Math.min(minZ, maxZ);
    this.maxX = Math.max(minX, maxX);
    this.maxY = Math.max(minY, maxY);
    this.maxZ = Math.max(minZ, maxZ);
}
```

**构造器不信任调用方传入的顺序**——它总是取 `min` 一侧放小值、`max` 一侧放大值。

这有一个**重要后果**：任何「算出反的盒子」的运算都会被静默归一化成正尺寸盒。

## 核心发现二：`intersect` 不相交时得到的是「跨越两者」的正尺寸盒，不是负尺寸盒

```java
public AABB intersect(AABB other) {
    return new AABB(
        Math.max(this.minX, other.minX), ...,   // 得到 5
        Math.min(this.maxX, other.maxX), ...);  // 得到 1
    // 但构造器把 (5, 1) 归一化成 (1, 5)
}
```

| 单元 | 中间值 | 最终结果 |
|---|---|---|
| **`intersect\|0\|0\|0\|1\|1\|1\|5\|5\|5\|6\|6\|6`** | `(5,5,5)..(1,1,1)` | **`1,1,1,5,5,5`** |
| `intersect\|0\|0\|0\|2\|2\|2\|1\|1\|1\|3\|3\|3` | `(1,1,1)..(2,2,2)` | `1,1,1,2,2,2` |
| `intersect\|0\|0\|0\|10\|10\|10\|2\|2\|2\|3\|3\|3` | `(2,2,2)..(3,3,3)` | `2,2,2,3,3,3` |

**两个相距很远的盒子求交，得到一个横跨它们之间全部空间的盒子。**

这是**极易踩的坑**：`intersect` 的结果必须配合 `intersects` 判断才有意义，否则会把
「完全不相交」误判成「有一个大盒子重叠」。

**这与很多几何库的约定不同**——那些库会返回空盒或 `null`。Cubium 若实现成返回负尺寸盒，
就必须在每次使用时判断 `minX < maxX`；而原版的语义是「结果永远有效，但可能不是你想要的」。

## 核心发现三：`getSize()` 返回三个轴尺寸的**算术平均**，不是 `Vec3`

```java
public double getSize() {
    double d0 = this.getXsize();
    double d1 = this.getYsize();
    double d2 = this.getZsize();
    return (d0 + d1 + d2) / 3.0;
}
```

**本能力开发时踩到了这个坑**：我最初按旧版记忆写成「返回 `Vec3`（三轴尺寸）」，编译报错：

```
error: incompatible types: double cannot be converted to Vec3
```

| 单元 | 结果 |
|---|---|
| **`size\|0\|0\|0\|1\|1\|1`** | **`1`** |
| **`size\|0\|0\|0\|3\|1\|1`** | **`1.66666667`**（`(3+1+1)/3`）|
| `size\|-1\|-2\|-3\|1\|2\|3` | `4`（`(2+4+6)/3`）|

要拿单轴尺寸必须用 `getXsize()` / `getYsize()` / `getZsize()`：

| 单元 | 结果 |
|---|---|
| `axisSizes\|0\|0\|0\|1\|1\|1` | `1,1,1` |
| `axisSizes\|-1\|-2\|-3\|1\|2\|3` | `2,4,6` |

## 核心发现四：`encapsulatingFullBlocks` 的 `+1` 只加在 `max` 一侧

```java
public static AABB encapsulatingFullBlocks(BlockPos a, BlockPos b) {
    return new AABB(
        Math.min(a.x, b.x), ...,                              // min 不加
        Math.max(a.x, b.x) + 1, ...);                         // max 加 1
}
```

| 单元 | 结果 |
|---|---|
| **`encap\|0\|0\|0\|0\|0\|0`** | **`0,0,0,1,1,1`**（单方块 → 边长 1 的完整方块）|
| `encap\|0\|0\|0\|2\|3\|1` | `0,0,0,3,4,2` |
| **`encap\|2\|3\|1\|0\|0\|0`** | **`0,0,0,3,4,2`**（端点顺序无关）|
| `encap\|-1\|-2\|-3\|1\|2\|3` | `-1,-2,-3,2,3,4` |

语义是「包含整格」：方块 `(0,0,0)` 占据 `[0,1)³`，所以包围盒是 `(0,0,0)..(1,1,1)`。

## 其余实测

### `getCenter()` 用 `Mth.lerp(0.5, min, max)`

```java
return new Vec3(Mth.lerp(0.5, minX, maxX), ...);
// Mth.lerp(delta, start, end) = start + delta * (end - start)
```

| 单元 | 结果 |
|---|---|
| `center\|0\|0\|0\|1\|1\|1` | `0.5,0.5,0.5` |
| `center\|10\|64\|-20\|12\|68\|-16` | `11,66,-18` |
| `center\|-1\|-1\|-1\|1\|1\|1` | `0,0,0` |

**`lerp(0.5, a, b) = a + 0.5*(b-a)` 与 `(a+b)/2` 在浮点上不等价**——前者不会因 `a+b` 溢出。
对 `a = b = 1e308`，`(a+b)/2` 得 `Infinity`，而 `lerp` 得 `1e308`。

### `inflate` 支持负值（收缩）

| 单元 | 结果 |
|---|---|
| `inflate\|0\|0\|0\|1\|1\|1\|0.5\|0.5\|0.5` | `-0.5,-0.5,-0.5,1.5,1.5,1.5` |
| **`inflate\|0\|0\|0\|1\|1\|1\|-0.1\|-0.1\|-0.1`** | **`0.1,0.1,0.1,0.9,0.9,0.9`**（收缩）|

### `ofSize`

```java
return new AABB(cx - dx/2, ..., cx + dx/2, ...);   // 构造器再归一化
```

| 单元 | 结果 |
|---|---|
| `ofSize\|0\|0\|0\|1\|1\|1` | `-0.5,-0.5,-0.5,0.5,0.5,0.5` |
| `ofSize\|10\|64\|-20\|2\|4\|6` | `9,62,-23,11,66,-17` |
| `ofSize\|0\|0\|0\|0\|0\|0` | `0,0,0,0,0,0`（零尺寸退化为点）|

### `hasNaN`

| 单元 | 结果 |
|---|---|
| `hasNaN\|0\|0\|0\|1\|1\|1` | `False` |
| **`hasNaN\|nan\|0\|0\|1\|1\|1`** | **`True`** |
| **`hasNaN\|0\|0\|0\|inf\|1\|1`** | **`False`**（`inf` 不是 NaN）|

## 本轮踩坑：三个错误假设都被测试抓出

1. **`getSize()` 返回 `Vec3`** → 编译错误（实际返回 `double` 平均值）
2. **`intersect` 不相交时返回负尺寸盒** → 实际被构造器归一化成正尺寸盒
3. **`%g` 格式化两侧一致** → Java 的 `%.9g` 保留尾零，C 的 `%.9g` 去掉尾零

第 3 条对应 `_patterns.md` 的「浮点格式化必须两侧同步规范化」——本能力在 oracle 侧手工去掉了
尾随零（尾数与指数分开处理）。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`aabb_ops`](./aabb_ops.md) | `intersects` / `contains` 等判定（本能力是构造与方法层）|
| [`vec3_math`](./vec3_math.md) | `Vec3` 运算（`getCenter` 的返回类型）|
| [`mth_rounding`](./mth_rounding.md) | `Mth.lerp` 所属的取整族 |
| [`blockpos_iterate`](./blockpos_iterate.md) | `BlockPos` 区间遍历（与包围盒搭配使用）|

## 未覆盖范围

- `AABB.clip`（射线求交，返回 `Optional<Vec3>`，`explosion_ray` 已覆盖一个侧面）
- `AABB.clip(Iterable, Vec3, Vec3, BlockPos)`（返回 `BlockHitResult`）
- `AABB.of(BoundingBox)`（依赖 `BoundingBox`）
- `AABB.unitCubeFromLowerCorner`
- `AABB.minmax` / `contract` / `expandTowards`
- `AABB.intersects` / `contains` / `move(BlockPos)`（已由 [`aabb_ops`](./aabb_ops.md) 覆盖一部分）
- `AABB` 的 `offsetCodec` / `CODEC`（序列化，属 codec 型）