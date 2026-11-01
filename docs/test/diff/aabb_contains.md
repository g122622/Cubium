# 差分能力：`aabb_contains`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 130 个能力，30 个用例全部一致。这是 [`aabb_ops`](./aabb_ops.md)（第 94 个能力）的**补充**
——后者覆盖 `inflate`/`expand`/`move`/`intersect`/`minmax`/`intersects` 六个方法，本能力覆盖
`contains`/`getSize`/`deflate`/`contract`/`hasNaN`/`distanceToSqr`。

## 核心发现一：`contains` 是半开区间 `[min, max)`

```java
public boolean contains(double x, double y, double z) {
    return x >= this.minX && x < this.maxX
        && y >= this.minY && y < this.maxY
        && z >= this.minZ && z < this.maxZ;
}
```

对 `(0,0,0) → (1,1,1)`：

| 单元 | 结果 | 说明 |
|---|---|---|
| **`contains\|0\|0\|0`** | **`true`** | **`min` 边界算在盒内** |
| **`contains\|1\|1\|1`** | **`false`** | **`max` 边界不算** |
| `contains\|0.999\|0.999\|0.999` | `true` | |
| `contains\|1\|0.5\|0.5` | `false` | 单轴越界即整体不在 |
| `contains\|-0.001\|0.5\|0.5` | `false` | |

**这是方块判定的标准约定**：一个方块占据 `[x, x+1)`，相邻方块的 `min` 正好是前一个的 `max`，
用半开区间可以保证**每一点恰好属于一个方块**，不会出现「边界点同时属于两格」或「不属于任何格」。

**若把 `contains` 写成 `<= max`，站在方块边界上的实体会被多算一格**；结合 `intersects`（闭区间
重叠）使用时会产生重复计入。

`max` 恰好相等时想判定在盒内，必须用 `intersects` 或显式比较，不能用 `contains`。

## 核心发现二：`contract` 只改单侧，`deflate` 改两侧

```java
public AABB contract(double dx, double dy, double dz) {
    double d0 = this.minX, d1 = this.minY, d2 = this.minZ;
    double d3 = this.maxX, d4 = this.maxY, d5 = this.maxZ;
    if (dx < 0.0) d0 -= dx; else if (dx > 0.0) d3 -= dx;
    // y、z 同理
    return new AABB(d0, d1, d2, d3, d4, d5);
}

public AABB deflate(double dx, double dy, double dz) {
    return this.inflate(-dx, -dy, -dz);
}
```

| 方法 | 参数为正 | 参数为负 | 参数为 0 |
|---|---|---|---|
| `deflate(d)` | 收缩（min 增、max 减）| 膨胀 | 不变 |
| **`contract(d)`** | **只减 `max`** | **只增 `min`** | **不变** |

实测（盒为 `(0,0,0) → (1,1,1)`）：

| 单元 | 结果 | 说明 |
|---|---|---|
| **`contract\|0.5\|0\|0`** | **`0,0,0,0.5,1,1`** | **只 `maxX` 变小** |
| **`contract\|-0.5\|0\|0`** | **`0.5,0,0,1,1,1`** | **只 `minX` 变大** |
| `contract\|0\|0.25\|0` | `0,0,0,1,0.75,1` | 只 `maxY` 变小 |
| `contract\|0\|0\|0` | `0,0,0,1,1,1` | 完全不变 |
| `deflate\|0.25\|0.25\|0.25` | `0.25,0.25,0.25,0.75,0.75,0.75` | **两侧都动** |
| `deflate\|-0.5\|-0.5\|-0.5` | `-0.5,-0.5,-0.5,1.5,1.5,1.5` | 膨胀 |

**`contract` 用于「从某一侧收缩包围盒」**——例如玩家贴墙时判定框只往墙的一侧收。若实现成
`min += d; max -= d`（即 `deflate` 的行为），参数为正时两侧都会缩，与原来不符。

**参数为 0 时 `contract` 完全不动**（`if/else if` 两个分支都不进），这也需要显式复刻。

## 核心发现三：`getSize` 是三边算术平均

```java
public double getSize() {
    return (this.getXsize() + this.getYsize() + this.getZsize()) / 3.0;
}
```

| 单元 | 结果 |
|---|---|
| `getSize\|0,0,0,1,1,1` | `1` |
| **`getSize\|0,0,0,1,2,3`** | **`2`**（`(1+2+3)/3`）|
| `getXsize\|0,0,0,1,2,3` | `1` |
| `getYsize\|0,0,0,1,2,3` | `2` |
| `getZsize\|0,0,0,1,2,3` | `3` |

**不是体积 `1×2×3 = 6`，也不是对角线 `√14 ≈ 3.74`，而是 `2`。** 这个「尺寸」用于实体判定框的
粗粒度比较（例如「这个盒子够不够大」）。

## 核心发现四：`distanceToSqr(AABB)` 只算两盒之间的间隙

```java
double d0 = Math.max(Math.max(this.minX - other.maxX, other.minX - this.maxX), 0.0);
double d1 = ...;
double d2 = ...;
return Mth.lengthSquared(d0, d1, d2);
```

| 单元 | 结果 | 说明 |
|---|---|---|
| **`distAabb` 与重叠盒** | **`0`** | 相交时距离为 0，不是负数 |
| `distAabb\|2,0,0,3,1,1` | `1` | X 方向间隙 1 |
| `distAabb\|2,2,2,3,3,3` | `3` | 三轴各差 1，`1+1+1=3` |
| `distAabb\|5,5,5,6,6,6` | `48` | 三轴各差 4，`16×3=48` |

**`Math.max(..., 0.0)` 保证重叠时返回 0 而不是负数。**

**注意：`AABB` 没有 `distanceToSqr(double, double, double)` 单点重载**——源码里只有
`distanceToSqr(Vec3)` 和 `distanceToSqr(AABB)`。本能力验证的是后者。

## `hasNaN` 检查全部六个分量

```java
return Double.isNaN(this.minX) || Double.isNaN(this.minY) || Double.isNaN(this.minZ)
    || Double.isNaN(this.maxX) || Double.isNaN(this.maxY) || Double.isNaN(this.maxZ);
```

`0,0,0,1,1,1|hasNaN` → `false`。

`NaN` 参与任何比较都返回 `false`，会让 `contains` / `intersects` **静默失效**（永远返回
`false`），所以这个防御性检查必须与原版一致。

## 本轮踩坑：方法重载查错

写探针时我按直觉实现了 `distanceToSqr(double x, double y, double z)`，但**原版没有这个重载**。
用 `grep` 源码和 `javap` 双重确认后，改为 `distanceToSqr(AABB)`。

**教训**：浮点几何类的方法重载很多（`Vec3` / `AABB` / 三坐标），写探针前必须确认**重载的确切
签名**，不能凭「应该有这样的重载」推断。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`aabb_ops`](./aabb_ops.md) | 覆盖 `inflate`/`expand`/`move`/`intersect`/`minmax`/`intersects`；本能力是其补集 |
| [`vec3_math`](./vec3_math.md) | `Vec3` 的浮点运算 |
| [`blockpos_geom`](./blockpos_geom.md) | `BlockPos` 的整数几何变换 |

**注意 `aabb_ops` 的文档记录「Cubium 目前没有 `AABB` 类」**，本能力在同一前提下验证原版语义。

## 未覆盖范围

- `clip` 系列（射线与盒求交，返回 `Optional<Vec3>`）
- `collidedAlongVector`
- `encapsulatingFullBlocks` / `unitCubeFromLowerCorner` / `of(BoundingBox)`
- `setMinX` 等六个 setter
- `min(Axis)` / `max(Axis)`
- `toString`
- `NaN` 的实际注入（探针侧无法从 JSON 输入构造 `NaN`，因此 `hasNaN` 只验证了 `false` 分支）