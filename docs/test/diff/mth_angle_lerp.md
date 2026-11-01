# 差分能力：`mth_angle_lerp`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 133 个能力，34 个用例全部一致。这是 [`mth_inverse_lerp`](./mth_inverse_lerp.md) 文档中明确
列出的未覆盖项 `rotLerp` / `rotLerpRad` / `catmullrom` 的补充。

## 核心发现一：`wrapDegrees` 的输出区间是 `(-180, 180]`，不是 `[-180, 180)`

```java
public static float wrapDegrees(float degrees) {
    float f = degrees % 360.0F;
    if (f >= 180.0F)  f -= 360.0F;
    if (f < -180.0F)  f += 360.0F;
    return f;
}
```

| 单元 | 结果 | 说明 |
|---|---|---|
| `wrap\|0` | `0` | |
| `wrap\|90` | `90` | |
| `wrap\|179` | `179` | |
| **`wrap\|180`** | **`-180`** | **上界被绕到下界** |
| `wrap\|-180` | `-180` | 保持 |
| `wrap\|181` | `-179` | |
| `wrap\|360` | `0` | |
| **`wrap\|-360`** | **`-0`** | **负零** |
| `wrap\|540` | `-180` | |
| `wrap\|-540` | `-180` | |
| `wrapInt\|180` | `-180` | `int` 重载结果相同 |

**`180` 被映射成 `-180`**：`>= 180` 的判定把上界排除在外，所以输出区间是 `(-180, 180]`。

**`wrapDegrees(-360)` 返回 `-0`（负零）**——`fmod(-360, 360)` 保留被除数的符号。负零在比较中
等于正零，但**字符串化时不同**，所以本能力把它原样输出。

## 核心发现二：`rotLerp` 走「先 wrap 差值再插值」的短路路径

```java
public static float rotLerp(float delta, float start, float end) {
    return start + delta * wrapDegrees(end - start);
}
```

| 单元 | 结果 | 对比 |
|---|---|---|
| `rotLerp\|0\|0\|90` | `0` | |
| `rotLerp\|1\|0\|90` | `90` | |
| `rotLerp\|0.5\|0\|90` | `45` | |
| **`rotLerp\|0.5\|170\|-170`** | **`180`** | **不是 `0`！** |
| **`rotLerp\|0.5\|-170\|170`** | **`-180`** | **不是 `0`！** |
| `rotLerp\|0\|170\|-170` | `170` | |
| `rotLerp\|1\|170\|-170` | `190` | |
| `rotLerp\|1.5\|0\|90` | `135` | 外推 |
| `rotLerp\|-0.5\|0\|90` | `-45` | 外推 |

**`rotLerp(0.5, 170, -170)` 得 `180`，而不是直接插值的 `0`。**

推导：`wrapDegrees(-170 - 170) = wrapDegrees(-340) = 20`，然后 `170 + 0.5 × 20 = 180`。

**语义**：从 170° 转到 -170°，玩家实际只转 20°（顺时针），不是 340°（逆时针）。直接插值会
让角色「反向绕一大圈」。

**注意结果 `180` 超出了 `[-180, 180]` 区间**——`rotLerp` 不保证输出在 wrap 后的范围内，因为它
是在 `start` 上加一个 wrapped 增量。

## 核心发现三：`rotLerpRad` 用 `while` 循环，边界 `[-π, π)`

```java
public static float rotLerpRad(float delta, float start, float end) {
    float f = end - start;
    while (f < (float) -Math.PI)  f += (float) (Math.PI * 2);
    while (f >= (float) Math.PI)  f -= (float) (Math.PI * 2);
    return start + delta * f;
}
```

| 单元 | 结果 |
|---|---|
| `rotLerpRad\|0.5\|0\|1.5707964` | `0.785398185` |
| **`rotLerpRad\|0.5\|3.0\|-3.0`** | **`3.14159274`** |
| **`rotLerpRad\|0.5\|-3.0\|3.0`** | **`-3.14159274`** |
| `rotLerpRad\|0\|3.0\|-3.0` | `3` |

**`rotLerpRad(0.5, 3, -3)` 得 `+π`**：`-3 - 3 = -6`，加上 `2π ≈ 6.2832` 得 `0.2832`，
再 `3 + 0.5 × 0.2832 = 3.1416`。

**`(float) Math.PI` 与 `(float)(Math.PI * 2)` 不精确**：

- `(float) Math.PI = 3.1415927F`
- `(float)(Math.PI * 2) = 6.2831855F`
- 但 `3.1415927F × 2 = 6.2831854F` ≠ `6.2831855F`

**两者不是精确的 2 倍关系**，因此循环边界的对称为「近似对称」，极端输入下可能有 1 ULP 的差异。

### `while` 循环是性能陷阱

若 `end - start = 1e6`，循环要跑约 `1.6e5` 次。**原版没有用 `%` 优化**。行为上等价于取模，
但 Cubium 若用 `%` 实现，需验证浮点精度是否一致。

## 核心发现四：`catmullrom` 是四点样条，会过冲

```java
public static float catmullrom(float delta, float p0, float p1, float p2, float p3) {
    return 0.5F * (
        2.0F * p1
        + (p2 - p0) * delta
        + (2.0F * p0 - 5.0F * p1 + 4.0F * p2 - p3) * delta * delta
        + (3.0F * p1 - p0 - 3.0F * p2 + p3) * delta * delta * delta);
}
```

| 单元 | 结果 | 说明 |
|---|---|---|
| `catmull\|0\|1\|2\|3\|4` | `2` | `delta=0` 得 `p1=2` |
| `catmull\|1\|1\|2\|3\|4` | `3` | `delta=1` 得 `p2=3` |
| `catmull\|0.5\|1\|2\|3\|4` | `2.5` | 对称配置 |
| **`catmull\|0.5\|0\|1\|2\|3`** | **`1.5`** | **等差配置退化为线性** |
| `catmull\|0.25\|0\|1\|2\|3` | `1.25` | 同上 |
| **`catmull\|0.5\|0\|0\|1\|1`** | `0.5` | |
| **`catmull\|0.25\|0\|0\|1\|1`** | **`0.203125`** | |
| `catmull\|1.5\|1\|2\|3\|4` | `3.5` | **外推** |
| `catmull\|-0.5\|1\|2\|3\|4` | `1.5` | **外推** |

**参数语义**：

- `p1`、`p2` 是**实际端点**（`delta=0` → `p1`，`delta=1` → `p2`）
- `p0`、`p3` 是**切向量参考点**（来自相邻段的端点），决定曲线在端点处的斜率

**当 `p0..p3` 等差时退化为线性插值**（`catmull|0.5|0|1|2|3 = 1.5`，正好是 `p1` 与 `p2` 的中点）。

**`delta` 超出 `[0,1]` 会外推，不做 clamp**（`catmull|1.5` 得 `3.5`，`catmull|-0.5` 得 `1.5`）。

## 本轮情况：一次通过

与 `rotation_info`、`blockpos_geom`、`vec3i_arith` 相同，写代码前完整读取了 `wrapDegrees` /
`rotLerp` / `rotLerpRad` / `catmullrom` 四个方法的实现，并且**用 `javap` 确认了 `wrapDegrees`
有 `int` / `long` / `float` / `double` 四个重载**，补充了 `wrapInt` 用例。

**对比**：`mth_inverse_lerp`（上轮）因参数映射错误错了 3 处。本轮的 `catmullrom` 有 5 个参数，
但因为公式是完整抄录（没有「理解后重写」），一次就对。

**结论**：多参数数学函数应当**逐行抄录原版表达式**，而不是「理解语义后重新实现」——后者正是
上轮 `lerp2` 出错的根源。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`mth_inverse_lerp`](./mth_inverse_lerp.md) | `inverseLerp` / `lerp3` |
| [`mth_lerp`](./mth_lerp.md) | `clampedLerp` / `lerp2` / `lerp` |
| [`direction_rotate`](./direction_rotate.md) | `Direction.fromYRot` / `toYRot`（角度到方向） |

## 未覆盖范围

- `triangleWave`（三角波，2 个参数）
- `smoothstep`（五次多项式缓动）
- `square` / `lerp2` 之外的波形函数
- `rotLerpRad` 在 `end - start` 极大时的循环次数（性能，非正确性）
- `wrapDegrees(long)` 重载（`int` 版已覆盖，`long` 版行为应相同但未验证）