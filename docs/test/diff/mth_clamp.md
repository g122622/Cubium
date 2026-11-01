# 差分能力：`mth_clamp`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 120 个能力，42 个用例全部一致。`Mth` 的钳制/映射/平方函数族此前未被系统覆盖
（[`mth_lerp`](./mth_lerp.md)、[`mth_rounding`](./mth_rounding.md)、`mth_bit` 覆盖的是相邻的
函数族）。

## 四个关键点

**1. 浮点 `clamp` 与整数 `clamp` 的写法不同。**

| 版本 | 实现 |
|---|---|
| `int` / `long` | `Math.min(Math.max(v, min), max)` |
| `float` / `double` | `v < min ? min : Math.min(v, max)` |

浮点版本用 `<` 比较：`NaN < min` 为 `false`，于是走 `Math.min(NaN, max)`，而 `Math.min`
对 `NaN` 返回 **`NaN`**。

**实测 `clampf|NaN|0|10 → NaN`**——**`NaN` 会穿透钳制，不会退化成 `min`。**

| 单元 | 结果 |
|---|---|
| `clampf\|5\|0\|10` | `5` |
| `clampf\|-5\|0\|10` | `0` |
| `clampf\|NaN\|0\|10` | **`NaN`** |
| `clampf\|Inf\|0\|10` | `10` |
| `clampi\|5\|10\|0` | `0`（区间反向：先 `max(5,10)=10`，再 `min(10,0)=0`） |

**2. `clampedLerp` 有两个短路分支，顺序是「先下界、后上界」。**

```java
if (delta < 0.0) { return start; }
return delta > 1.0 ? end : lerp(delta, start, end);
```

| 单元 | 结果 | 说明 |
|---|---|---|
| `clampedLerpD\|-0.5\|10\|20` | `10` | 短路返回 `start` |
| `clampedLerpD\|1.5\|10\|20` | `20` | 短路返回 `end` |
| **`clampedLerpD\|0\|10\|20`** | `10` | **不短路，走 `lerp`** |
| **`clampedLerpD\|1\|10\|20`** | `20` | **不短路，走 `lerp`** |
| `clampedLerpD\|0.5\|10\|20` | `15` | 正常插值 |
| `clampedLerpF\|0.25\|10\|20` | `12.5` | float 版本一致 |

**`delta = 0.0` 与 `delta = 1.0` 不短路**，会走 `start + delta * (end - start)`。
平时结果相同，但当 `end - start` 为 `Infinity` 时，`0 * Infinity = NaN`，**`clampedLerp(0, a, Inf)`
会得 `NaN` 而不是 `a`**。这是窗口函数里 `Infinity` 边界容易踩的坑。

**3. `clampedMap` 是先算 `inverseLerp` 再钳制的组合。**

```java
clampedLerp(inverseLerp(value, inMin, inMax), outMin, outMax)
```

| 单元 | 结果 |
|---|---|
| `clampedMapD\|15\|10\|20\|0\|100` | `50` |
| `clampedMapD\|5\|10\|20\|0\|100` | `0` |
| `clampedMapD\|25\|10\|20\|0\|100` | `100` |
| **`clampedMapD\|10\|10\|10\|0\|100`** | **`NaN`** |

**输入区间退化（`inMin == inMax`）时**：`inverseLerp` 产生 `0.0 / 0.0 = NaN`，`clampedLerp`
的两个短路分支都不成立，最终 `lerp(NaN, ...)` → **`NaN`**。

`inverseLerp` 自身的退化更直接：**`inverseLerpD|5|10|10 → -Infinity`**（`-5 / 0`）。

**4. `square(int)` 与 `square(long)` 会溢出环绕，且阈值不同。**

| 单元 | 结果 | 说明 |
|---|---|---|
| `squareI\|46340` | `2147395600` | 未溢出（`46340² < 2^31`） |
| **`squareI\|46341`** | **`-2147479015`** | **32 位溢出环绕** |
| **`squareL\|46341`** | **`2147488281`** | **64 位正确** |
| `squareD\|2.5` | `6.25` | |

`46340² = 2147395600 < 2^31`，`46341² = 2147488281 > 2^31`。**越过 `46340` 后 `square(int)`
必然溢出**，而 `square(long)` 在 64 位下安然无恙。两者只差在字段宽度。

Java 的整数溢出是定义良好的二补数环绕（与 [`mth_bit`](./mth_bit.md)、
[`chunk_pos`](./chunk_pos.md) 同类语义）。

## 其余实测

| 单元 | 结果 |
|---|---|
| `absMaxI\|-5\|3` | `5` |
| `absMaxI\|5\|-3` | `5` |
| `absMaxD\|-2.5\|1.5` | `2.5` |
| `chessboard\|0\|3\|0\|4` | `1` |
| `chessboard\|10\|3\|10\|4` | `1` |
| `inverseLerpD\|15\|10\|20` | `0.5` |
| `inverseLerpD\|10\|10\|20` | `0` |
| `inverseLerpD\|20\|10\|20` | `1` |

`chessboardDistance(x1, x2, z1, z2) = absMax(z1 - x1, z2 - x2)`——**参数顺序是
`(x1, x2, z1, z2)`，内部按 `z1-x1`、`z2-x2` 相减**。名字叫 chessboard 但实际上是「两轴差值的
绝对值取最大」，即切比雪夫距离。

## 浮点格式化的对齐

C++ 侧用 `%.9g`（去掉尾随零、指数形式只对尾数去零），Java 侧用
`BigDecimal(value).round(MathContext(9))` 再 `stripTrailingZeros`。

本能力的输出值（`0.5`、`12.5`、`6.25`、`2.5`、`15` 等）都是短十进制小数，9 位有效数字下
两侧必然一致，**42/42 全部通过证实了这一点**。

若将来加入需要更长位数的浮点用例（如 `1/3`），两侧的舍入模式必须复核——C 的 `%g` 用
round-half-even，`MathContext(9, HALF_EVEN)` 与之相同，但进位位置可能有差异。

## 未覆盖范围

- `Mth.map`（非钳制版）
- `Mth.cube`
- `Mth.positiveModulo` 的浮点版本
- `Mth.lerpInt`
- `Mth.wrapDegrees`（已由 [`mth_rot`](./mth_rot.md) 覆盖相关部分）