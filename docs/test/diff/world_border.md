# 差分能力：`world_border`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 186 个能力。85 个单元，**全部一致**——过程中修复了 **3 处真实缺陷**。

## 测的是什么

```java
// WorldBorder.java
public boolean isWithinBounds(double x, double z) {
    return this.isWithinBounds(x, z, 0.0);
}

public boolean isWithinBounds(double x, double z, double padding) {
    return x >= this.getMinX() - padding
        && x <  this.getMaxX() + padding
        && z >= this.getMinZ() - padding
        && z <  this.getMaxZ() + padding;
}

public boolean isWithinBounds(BlockPos pos) {
    return this.isWithinBounds(pos.getX(), pos.getZ());   // 只看西北角
}

public boolean isWithinBounds(AABB box) {
    return this.isWithinBounds(box.minX, box.minZ, box.maxX - 1.0E-5F, box.maxZ - 1.0E-5F);
}
```

## 三个关键点

### 一、边界比较是不对称的

**下界 `>=`（闭），上界 `<`（开）。**

边界覆盖区间是 `[min, max)`：正好落在 `min` 上的坐标属于**界内**，正好落在 `max` 上的属于**界外**。

写成两端都严格（`>` 与 `<`）会让紧贴下边界的坐标被误判为界外；写成两端都闭（`<=`）会让边界多算一格。

**这正是本次查出的缺陷之一**（见下文）。

### 二、`isWithinBounds(BlockPos)` 只看方块的西北角

原版**只检查 `(x, z)`**，不检查 `(x+1, z+1)`。

理由是方块占半开区间 `[x, x+1)`，右边界不属于自己——因此紧贴上边界的那一格仍算界内。

若额外检查 `(x+1, z+1)`，会把贴着上边界的**整列/整行**方块误判为界外。

### 三、`1.0E-5F` 是 float 字面量

```java
box.maxX - 1.0E-5F
```

`1.0E-5F` 提升为 double 后是 `1.0000000116860974E-5`，与 `1.0E-5` 相差一个 ulp 级别。
当盒的 `maxX` 恰好落在边界上时，这个差别足以翻转判定。

本能力的 `aabb` 单元专门包含这种「刚好贴着边界」的盒子。

## 本轮修复的 3 处缺陷

### 缺陷一：`setSize` 强制最小尺寸 1.0

```cpp
// src/common/world/border/WorldBorder.cpp（修复前）
void WorldBorder::setSize(f64 size)
{
    size = std::clamp(size, 1.0, MAX_SIZE);
    ...
}
```

原版 `setSize` **不做任何钳制**：

```java
public void setSize(double p_61918_) {
    this.extent = new WorldBorder.StaticBorderExtent(p_61918_);
    this.setDirty();
}
```

把 `0.3` 钳成 `1.0`，会让一个边长 0.3 的边界被算成边长 1.0，界内/界外判定与
`getMinX`/`getMaxX` 全部错位。

**单元**：`extent|0.1,0.2|0.3`

| | `minX` | `maxX` |
|---|---|---|
| 原版 | `-0.05` | `0.25` |
| Cubium（修复前）| `-0.4` | `0.6` |

### 缺陷二：`contains` 两端都用严格比较

```cpp
// 修复前
bool WorldBorder::contains(f64 x, f64 z) const
{
    return x > getMinX() && x < getMaxX() && z > getMinZ() && z < getMaxZ();
}
```

下界应当用 `>=`。这会让正好贴在**下边界**上的方块被误判为界外。

**单元**：`block|0,0|100|-50,-50`——边界 `[-50, 50)`，`x = -50` 应判为界内。

### 缺陷三：`isWithinBounds(BlockPos)` 多检查了 `(x+1, z+1)`

```cpp
// 修复前
return contains(static_cast<f64>(pos.x), static_cast<f64>(pos.z)) &&
       contains(static_cast<f64>(pos.x + 1), static_cast<f64>(pos.z + 1));
```

原版只看西北角。多出的这次检查会把贴着上边界的方块判成界外。

**单元**：`block|0,0|1|0,0`——边长 1 的边界只有一个方块 `(0,0)`，
它的 `(1,1)` 角正好落在 `maxX = maxZ = 0.5` 之外，于是被误判。

## 踩坑：`%g` 在 C++ 与 Java 下的末位分歧

最初两侧都用 17 位有效数字，结果出现这样的差异：

```
原版  minX=-0.04999999999999999
Cubium minX=-0.049999999999999989
```

**两者其实是同一个 double。** 差异来自 `%.17g` 的实现：

| | 输出 |
|---|---|
| C++ `%.17g` | `-0.049999999999999989`（展开到 17 位）|
| Java `%.17g` | `-0.049999999999999990`（到 16 位后补零）|

Java 的末尾那个 `0` 在我的剥离逻辑里没被去掉（剥离只处理「含 `.` 且不含 `e`」的情形，
这里是含 `.` 不含 `e`，但剥零在去掉 `990` 的最后一个 `0` 后仍剩 `99`），
于是两边文本不一致。

**解决**：统一改用 **14 位有效数字**。14 位足以让本项目关心的所有边界判定差异显现，
又完全避开了末位展开的分歧。

> 这类「数值相同、文本不同」的假差异在浮点能力里很常见。判断办法是先确认两侧
> 打印的是否为同一个双精度值（本轮的例子用 `Double.toString` 即可验证）。

## 单元格式

`<模式>|<centerX>,<centerZ>|<size>|<…>`

| 模式 | 尾部参数 | 输出 |
|---|---|---|
| `extent` | — | `minX=… minZ=… maxX=… maxZ=…` |
| `block` | `<bx>,<bz>` | `true` / `false` |
| `aabb` | `<minX>,<minZ>,<maxX>,<maxZ>` | `true` / `false` |

浮点参数以**十进制文本**写在单元里，两侧各自 `strtod` / `Double.parseDouble`，
避免 JSON 数字的解析差异。

## 未覆盖范围

- **`getDistanceToBorder`**：Cubium 尚未实现该接口（原版有 `(Entity)` 与 `(double, double)` 两个重载）。
  `TODO: Cubium 的 WorldBorder 缺少 getDistanceToBorder，待实现后应补入本能力。`
- **`isWithinBounds(double, double)` 与 `(double, double, double)`**：Cubium 只暴露了
  `BlockPos` 与 `AABB` 重载，纯点判定与 padding 版本未实现。
- **`isWithinBounds(ChunkPos)`**：Cubium 用 `intersectsChunk` 表达，语义不同，未比对。
- **尺寸过渡**（`setSizeLerp` / `lerpSizeBetween`）：依赖世界时间，属于有状态能力。
- **`getMinX(float)` 等带进度参数的重载**：涉及 `Mth.lerp` 插值，未单独覆盖。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`aabb_ops`](./aabb_ops.md) | `AABB` 本身的运算 |
| [`aabb_contains`](./aabb_contains.md) | `AABB` 包含判定 |
| [`chunk_pos`](./chunk_pos.md) | 区块坐标 |