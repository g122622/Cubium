# 差分能力：`mth_interpolation`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 188 个能力。12 个单元，**全部一致**。

过程中确认了 Cubium 在插值/平滑函数上的两处**接口缺口**（不是实现错误，而是函数不存在），
已记录待补。

## 测的是什么

```java
public static float catmullrom(float d, float p0, float p1, float p2, float p3) {
    return 0.5F * (2.0F * p1
        + (p2 - p0) * d
        + (2.0F * p0 - 5.0F * p1 + 4.0F * p2 - p3) * d * d
        + (3.0F * p1 - p0 - 3.0F * p2 + p3) * d * d * d);
}

public static double smoothstep(double x) {
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}
```

## 三个关键点

### 一、`catmullrom` 全程 float

签名是 `float catmullrom(float, ...)`，且字面量全部带 `F` 后缀。
**用 double 计算会得到"更精确"但不同的结果**：

| 单元 | 原版（float）| double 实现 |
|---|---|---|
| `catmullrom\|0,...` | `0.40000000596046` | `0.4` |
| `catmullrom\|0.5,...` | `0.55000001192093` | `0.55` |

这正是本轮首版的错误：探针一开始用 double 实现，4 个单元全错。

### 二、`Mth.smoothstep` 是单参数版本

```java
smoothstep(x) = x³(6x² - 15x + 10)
```

它与 GLSL 风格的三参数版本**完全不是同一个函数**：

| | `Mth.smoothstep(x)` | GLSL `smoothstep(e0, e1, x)` |
|---|---|---|
| 参数 | 1 个 | 3 个 |
| 公式 | `x³(6x²-15x+10)` | `t²(3-2t)`，`t = clamp((x-e0)/(e1-e0), 0, 1)` |
| 定义域外 | 外推（`x=2` 给出 `>1`）| 钳制到 `[0,1]` |

### 三、`Mth.lerp2` / `Mth.lerp3` 的 delta 在**第一个**参数位

```java
public static double lerp(double delta, double a, double b) { return a + delta * (b - a); }
public static double lerp2(double d, double d0, double d1, double d2, double d3, double d4)
public static double lerp3(double d, double d0, ..., double d10)   // 11 个参数
```

Cubium 的 `math::lerp(a, b, t)` 把 delta 放在**末尾**。两种顺序混用会导致参数错位。

## Cubium 的两处接口缺口

### 缺口一：没有 `catmullrom`

`src/common/util/math/MathUtils.hpp` 里没有任何 Catmull-Rom 相关函数。

```cpp
// TODO: Cubium 的 MathUtils 没有 catmullrom；MC 的 Mth.catmullrom 用于实体相机路径
// 与粒子插值。补齐后本探针应改为直接调用它。
```

### 缺口二：`smoothstep` 签名与语义都不同

Cubium 只有三参数版本：

```cpp
[[nodiscard]] inline constexpr f32 smoothstep(f32 edge0, f32 edge1, f32 x) noexcept
{
    const f32 t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
```

**没有任何函数对应 `Mth.smoothstep(x)`**。若后续世界生成代码需要原版语义，
直接调用 `smoothstep(0.0f, 1.0f, x)` 是**不等价**的——公式不同且多了钳制。

```cpp
// TODO: 补 Mth.smoothstep 的单参数版本 x³(6x²-15x+10)，与现有三参数 GLSL 版本区分命名。
```

### 缺口三：`lerp2` 不存在，`lerp3` 语义不同

- Cubium **没有 `lerp2`**
- Cubium 的 `lerp3(sx, sy, sz, d0..d7)` 是**三因子三线性插值**（8 个角、3 个独立因子）
- `Mth.lerp3(delta, d0..d10)` 是**单因子版本**（10 个值、1 个因子）

两者名字相同但语义完全不同，**不能互相替代**。

## 本轮踩的坑

### 坑一：`if` 链里出现了两个相同的 `else if`

编辑过程中产生了：

```cpp
if (mode == "catmullrom") {
    ...
} else if (mode == "smoothstep") {
} else if (mode == "smoothstep") {   // 空块，吞掉了所有 smoothstep 输入
    ...
}
```

第一个是**空块**，所有 `smoothstep` 单元都被它接住，直接返回初始值 `0`。

**现象**：6 个 smoothstep 单元全部输出 `0`，且加进去的诊断打印**完全没有触发**。
诊断不触发这一点是决定性线索——说明代码路径根本没走到。

**教训**：当诊断打印不出现时，优先怀疑控制流，而不是表达式。

### 坑二：Java 的 `%g` 不剥尾随零

与 [`easing_type`](./easing_type.md) 完全相同的问题：

```
原版   0.40000000596046    ← C++ 版输出
Cubium 0.40000000000000    ← Java 版输出
```

两侧必须统一剥离规则，否则全部单元都是假差异。

## 单元格式

`<模式>|<参数…>`

```
catmullrom|0.5,0.1,0.4,0.7,1.0
smoothstep|0.75
```

delta 取 `0`、`0.25`、`0.5`、`0.75`、`1`——正好覆盖五个分段端点；
控制点用单调递增的 `0.1, 0.4, 0.7, 1.0`，便于看出参数次序是否写反。

## 未覆盖范围

- **`Mth.lerp2` / `Mth.lerp3`**：Cubium 无对应实现（`lerp3` 语义不同），无法差分。
  `TODO: 待 Cubium 补齐与原版同语义的 lerp2/lerp3 后，再扩充本能力。`
- **`Mth.smoothstepDerivative`**：原版有该函数，Cubium 未实现。
- **`Mth.wobble` / `triangleWave` / `quantize`**：同样属于插值族，Cubium 均未实现。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`mth_lerp`](./mth_lerp.md) | `Mth.lerp` 本身（Cubium 有对应实现）|
| [`mth_approach`](./mth_approach.md) | 趋近运算 |
| [`mth_sin`](./mth_sin.md) | `Mth.sin` 的查表实现 |
| [`easing_type`](./easing_type.md) | 缓动函数 |