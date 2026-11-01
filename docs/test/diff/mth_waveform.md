# 差分能力：`mth_waveform`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 134 个能力，26 个用例全部一致。这是 [`mth_angle_lerp`](./mth_angle_lerp.md) 文档中明确
列出的未覆盖项 `triangleWave` / `smoothstep` / `square` 的补充。

## 核心发现一：`triangleWave` 负输入会严重越界

```java
public static float triangleWave(float x, float period) {
    return (Math.abs(x % period - period * 0.5F) - period * 0.25F) / (period * 0.25F);
}
```

### `x >= 0` 时输出 `[-1, 1]`

| 单元 | `x % p` | 结果 |
|---|---|---|
| **`tri\|0\|10`** | `0` | **`1`**（峰值）|
| `tri\|2.5\|10` | `2.5` | `0` |
| **`tri\|5\|10`** | `5` | **`-1`**（谷值）|
| `tri\|7.5\|10` | `7.5` | `0` |
| `tri\|10\|10` | `0` | `1` |
| `tri\|1\|4` | `1` | `0` |
| `tri\|3\|4` | `3` | `0` |

**峰值在 `x % p = 0`，谷值在 `x % p = p/2`。**

### `x < 0` 时输出严重越界

| 单元 | 结果 | 说明 |
|---|---|---|
| **`tri\|-3\|10`** | **`2.20000005`** | **远超 `[-1,1]`！** |
| **`tri\|-5\|10`** | **`3`** | 越界到 3 |
| `tri\|-10\|10` | `1` | 恰好是周期的整数倍，回到 1 |

**根因：Java 的 `%` 向零截断。**

- `(-3) % 10 = -3`（数学取模应为 `7`）
- 于是 `|-3 - 5| = 8`，`(8 - 2.5) / 2.5 = 2.2`

**`triangleWave` 的输出区间只在 `x >= 0` 时才保证是 `[-1, 1]`。**

若 Cubium 用数学取模（`((x % p) + p) % p`）实现，负输入会得到 `0` 而不是 `2.2`——**差异明显**。

`tri|-10|10` 恰好返回 `1`，因为 `-10 % 10 = -0`，`|-0 - 5| = 5`，`(5-2.5)/2.5 = 1`。

## 核心发现二：`smoothstep` 是五次多项式，不 clamp

```java
public static double smoothstep(double x) {
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}
```

展开：`6x⁵ - 15x⁴ + 10x³`——这是 **Perlin 的 quintic fade 曲线**，不是常见的三次
`smoothstep`（`3x²-2x³`）。

| 单元 | 结果 | 说明 |
|---|---|---|
| `smooth\|0` | `0` | |
| `smooth\|1` | `1` | `6-15+10 = 1` |
| **`smooth\|0.5`** | **`0.5`** | **不动点** |
| `smooth\|0.25` | `0.103515625` | |
| **`smooth\|-0.5`** | **`-2.375`** | **不 clamp，外推** |
| **`smooth\|2`** | **`32`** | **不 clamp，外推** |

**`x` 超出 `[0,1]` 时不做任何 clamp**，值会继续外推（`smoothstep(2) = 32`）。

五次曲线的一阶与二阶导数在 `x=0` 和 `x=1` 处都为 0，因此相邻噪声单元拼接处**完全没有接缝**。
这是 Perlin 噪声选它而不选三次 `smoothstep` 的原因。

## 核心发现三：`smoothstepDerivative` 与 `smoothstep` 的解析导数一致

```java
public static double smoothstepDerivative(double x) {
    return 30.0 * x * x * (x - 1.0) * (x - 1.0);
}
```

对 `6x⁵ - 15x⁴ + 10x³` 求导：`30x⁴ - 60x³ + 30x² = 30x²(x-1)²`——完全吻合。

| 单元 | 结果 |
|---|---|
| **`smoothD\|0`** | **`0`** |
| **`smoothD\|1`** | **`0`** |
| **`smoothD\|0.5`** | **`1.875`**（最大值）|
| `smoothD\|0.25` | `1.0546875` |

**两端导数为 0** 是这条曲线的重要性质。`(x-1.0)` 出现了**两次**（平方），若漏写一次会得到完全
不同的值。

## 核心发现四：`square` / `cube` 就是字面意义的平方与立方

```java
public static float square(float x) { return x * x; }
public static float cube(float x)   { return x * x * x; }
```

| 单元 | 结果 |
|---|---|
| `square\|3` | `9` |
| `square\|-3` | `9`（负数平方为正）|
| `square\|0.5` | `0.25` |
| `cube\|3` | `27` |
| `cube\|-3` | `-27`（负数立方为负）|
| `cube\|0.5` | `0.125` |

**注意 `Mth.square` 是求平方，不是方波函数**——名字容易误解。

## 本轮情况：一次通过

写代码前完整读取了五个方法的实现，其中 `triangleWave` 的内联嵌套表达式较短，直接照抄。

**唯一需要注意的是 `triangleWave` 里 `x % period` 被算了三次**（原版表达式就是三次），我用
`javaMod` 函数调用而非简单 `%`，**显式复刻 Java 的向零截断语义**（C++ 的 `%` 对负数也是向零
截断，与 Java 一致，所以直接用 `%` 也可以，但抽成命名函数更清晰）。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`mth_angle_lerp`](./mth_angle_lerp.md) | `wrapDegrees` / `rotLerp` / `catmullrom` |
| [`mth_inverse_lerp`](./mth_inverse_lerp.md) | `inverseLerp` / `lerp3` |
| [`mth_lerp`](./mth_lerp.md) | `clampedLerp` / `lerp2` / `lerp` |
| [`mth_int_div`](./mth_int_div.md) | `floorDiv` / `ceillog2` 等整数工具 |

## 未覆盖范围

- `Mth.parabolicRand` / `Mth.nextFloat` / `Mth.nextInt`（需要随机源）
- `Mth.perlinFade` 的其它变体（若存在）
- `triangleWave` 在 `period = 0` 时的行为（`0/0` 产生 `NaN`，但属错误路径）
- `triangleWave` 在 `x` 为 `NaN` / `Infinity` 时的行为