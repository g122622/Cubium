# 差分能力：`portal_relative`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 153 个能力，12 个用例全部一致。这是**首个跨维度传送领域**的能力——此前该子系统完全未覆盖。

`PortalShape` 的其余方法需要 `LevelAccessor`（world 型），但 `getRelativePosition` 是纯计算。

## 核心发现一：宽度不足与高度不足的兜底值**不同**（`0.5` vs `0.0`）

```java
if (widthSpace > 0.0) {
    ...
} else {
    d2 = 0.5;                                  // 水平居中
}

if (heightSpace > 0.0) {
    ...
} else {
    d5 = 0.0;                                  // 贴底
}
```

| 单元 | 结果 | 说明 |
|---|---|---|
| **`0\|0\|0\|0\|3\|x\|1.0\|1.8\|0\|0\|0`** | **`0.5,0,-0.5`** | 宽度 0 < 实体宽 1 → `x=0.5` |
| **`0\|0\|0\|1\|3\|x\|2.0\|1.8\|0\|0\|0`** | **`0.5,0,-0.5`** | 宽度 1 < 实体宽 2 → `x=0.5` |
| **`0\|0\|0\|2\|0\|x\|0.6\|1.8\|0\|0\|0`** | **`0,0,-0.5`** | 高度 0 < 实体高 1.8 → **`y=0`** |
| **`0\|0\|0\|2\|1\|x\|0.6\|2.0\|0\|0\|0`** | **`0,0,-0.5`** | 高度 1 < 实体高 2 → **`y=0`** |
| **`0\|0\|0\|1\|1\|x\|2.0\|2.0\|0\|0\|0`** | **`0.5,0,-0.5`** | **两者都不足** |

**最后一个用例是最关键的**：宽度和高度都不足，但 `x=0.5` 而 `y=0`——**这是极易写错的一处**
（很多人会以为两者都取 `0.5` 或都取 `0`）。

语义上：**水平方向居中**（传送门宽度不够时把实体放中间），**垂直方向贴底**（高度不够时站在最下面的
方块上）。

## 核心发现二：返回的 `Vec3` 三个分量语义完全不同

| 分量 | 语义 | 范围 |
|---|---|---|
| `x` | 沿**宽度轴**的归一化插值 | `[0, 1]` |
| `y` | 沿**高度轴**的归一化插值 | `[0, 1]` |
| **`z`** | 沿**垂直轴**的**带符号绝对偏移** | 无界 |

```java
Direction.Axis other = axis == Direction.Axis.X ? Direction.Axis.Z : Direction.Axis.X;
double d4 = entityPos.get(other) - (corner.get(other) + 0.5);
return new Vec3(d2, d5, d4);
```

**`z` 分量没有除以任何尺寸**——它是「实体到传送门中心平面的距离」。

| 单元 | 结果 |
|---|---|
| `0\|0\|0\|2\|3\|x\|0.6\|1.8\|1\|0\|0` | `0.5,0,-0.5`（`z` 分量是 `0 - 0.5 = -0.5`）|
| **`0\|0\|0\|2\|3\|z\|0.6\|1.8\|1\|0\|0`** | **`0,0,0.5`**（Z 轴传送门，`z` 分量变 X 偏移）|

注意 `x` 轴传送门下 `z` 分量是 Z 坐标偏移；`z` 轴传送门下 `z` 分量变成 **X 坐标偏移**。

## 核心发现三：`other` 轴的选择——`X` 与 `Z` 互换，其他轴得 `X`

```java
Direction.Axis other = axis == Direction.Axis.X ? Direction.Axis.Z : Direction.Axis.X;
```

| `axis` | `other` |
|---|---|
| `X` | `Z` |
| `Z` | `X` |
| **`Y`** | **`X`**（不是 `Z`）|

| 单元 | 结果 |
|---|---|
| **`0\|0\|0\|2\|3\|y\|0.6\|1.8\|0\|1\|2`** | **`0.5,0.8333333,-0.5`** |

`axis = Y` 时 `other = X`，所以 `z` 分量是 `entityPos.x - (corner.x + 0.5) = 0 - 0.5 = -0.5`。

下界传送门不会用 `Y` 轴，但代码允许，**行为必须对齐**。

## 核心发现四：插值结果被钳到 `[0, 1]`

```java
d2 = Mth.clamp(Mth.inverseLerp(...), 0.0, 1.0);
```

| 单元 | 结果 | 说明 |
|---|---|---|
| `0\|0\|0\|2\|3\|x\|0.6\|1.8\|-1\|0\|0` | `0,0,-0.5` | 钳到下界 |
| **`0\|0\|0\|2\|3\|x\|0.6\|1.8\|3\|0\|0`** | **`1,0,-0.5`** | 钳到上界 |
| `0\|0\|0\|2\|3\|x\|0.6\|1.8\|1\|0\|0` | `0.5,0,-0.5` | 居中 |

实体在传送门外侧时，归一化值会被钳到 `0` 或 `1`，**不会溢出**。

## 其余实测

### 非原点角

| 单元 | 结果 |
|---|---|
| **`10\|64\|-20\|3\|4\|x\|0.6\|1.8\|12\|64\|-20`** | **`0.708333335,0,-0.5`** |

`corner.x = 10`，`widthSpace = 3 - 0.6 = 2.4`，`center = 10 + 0.3 = 10.3`，
`entityPos.x - center = 12 - 10.3 = 1.7`，`1.7 / 2.4 = 0.7083333333`。

`z` 分量仍是 `-20 - (-20 + 0.5) = -0.5`——**与 corner 无关，只与「实体 vs 中心平面」有关**。

## 本轮踩坑一：`FoundRectangle` 的尺寸字段是 `int`

首次编译报错：

```
error: incompatible types: possible lossy conversion from double to int
```

```java
public final int axis1Size;
public final int axis2Size;
public FoundRectangle(BlockPos minCorner, int axis1Size, int axis2Size)
```

我按 `double` 传入。修正后单元表里的小数尺寸（`0.5`）也要改成整数。

## 本轮踩坑二：`EntityDimensions` 的宽高是 `float`，必须降精

修正尺寸类型后仍差一处：

```
oracle: 0.5,0.8333333,-0.5
probe : 0.5,0.833333333,-0.5
```

根因：`EntityDimensions.scalable(float width, float height)` 接收 **`float`**。

`1.8f` 的精确值是 `1.7999999523162842`，因此 `heightSpace = 3 - 1.7999999523162842 = 1.2000000476837158`，
`1 / 1.2000000476837158 = 0.8333333`（而非 `0.8333333333`）。

**探针用 `std::stod` 得到 double 精度输入，跳过了 float 降精**——这与
[`mth_sqrt`](./mth_sqrt.md) 中是同一类错误。

修复：`static_cast<f64>(std::stof(parts[6]))`。

## 本轮踩坑三：Java 的 `%.Ng` 与 C 的 `%.Ng` 语义不同

我曾怀疑 `String.format("%.9g")` 与 `printf("%.9g")` 不一致，实测证明**两者一致**：

```
String.format("%.9g", 5.0/6.0) = 0.833333333
```

Java 的 `%.Ng` 中 `N` 也是**有效数字位数**，与 C 相同。**但 Java 不会去掉尾随零**
（`%.9g` 对 `2.0` 输出 `2.00000000`），而 C 会输出 `2`。

因此 oracle 侧仍需手工去尾零——本能力改用 `BigDecimal.round(MathContext(9))` +
`stripTrailingZeros()`，语义更清晰。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`mth_inverse_lerp`](./mth_inverse_lerp.md) | `Mth.inverseLerp`（本能力的核心算子）|
| [`mth_clamp`](./mth_clamp.md) | `Mth.clamp` |
| [`aabb_factory`](./aabb_factory.md) | `AABB.ofSize`（实体尺寸相关）|
| [`entity_dimensions`](./entity_dimensions.md) | `EntityDimensions` 的尺寸计算 |

## 未覆盖范围

- `PortalShape.findEmptyPortalShape` / `findPortalShape` / `findAnyShape`（需要 `LevelAccessor`，world 型）
- `PortalShape.createPortalBlocks`（写入方块，world 型）
- `PortalShape.isComplete` / `isValid`（依赖方块的 `TagKey.PORTAL_FRAME` 与形状判定）
- `PortalShape.findCollisionFreePosition`（需要 `ServerLevel`，world 型）
- `NetherPortalBlock` / `DimensionTransition` 的完整传送流程（world 型）
- `MAX_WIDTH` / `MAX_HEIGHT` 常量