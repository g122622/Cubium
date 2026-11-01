# 差分能力：`mth_modulo`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 92 个能力，31 个取模/角度输入全部一致。与上一轮的 `mth_rounding` 同属 `Mth` 数值工具。

#### 原版实现

```java
public static int positiveModulo(int x, int y)        { return Math.floorMod(x, y); }
public static float positiveModulo(float x, float y)  { return (x % y + y) % y; }
public static double positiveModulo(double x, double y) { return (x % y + y) % y; }

public static int wrapDegrees(int deg) {
    int i = deg % 360;
    if (i >= 180)  { i -= 360; }
    if (i < -180)  { i += 360; }
    return i;
}
```

#### 三个关键点

**1. 整数版与浮点版的 `positiveModulo` 不是同一个函数。** 整数走 `Math.floorMod`，浮点
走 `(x % y + y) % y`。实测：

| 单元 | 结果 |
|---|---|
| `modi\|-7\|3` | **2** |
| `modi\|7\|-3` | **-2** |
| `modi\|-6\|3` | **0**（整除时不能变成 3） |

结果的符号**跟随除数 `y`**，不是恒为正——写成「负数就加 `y`」会在 `y < 0` 时出错。

**2. `wrapDegrees` 的区间是 `[-180, 180)`，端点不对称。**

| 输入 | 结果 |
|---|---|
| `180` | **-180** |
| `-180` | **-180** |
| `179.9` | 179.9 |
| `-179.9` | -179.9 |

`180` 被调整成 `-180`，而 `-180` 保持不变。这与函数名暗示的「对称」不符，但正是两个
`if` 条件（`>= 180` 与 `< -180`）的直接结果。

**3. `wrapDegrees` 用两个独立 `if`，依赖 `% 360` 的结果严格落在 `(-360, 360)`。**
正因为这个前提，两次调整至多命中一次，所以 `if`、`if/else if`、`while` 在有限输入下等价。
本能力用 `±720`、`±900`、`±1000000` 把这一前提钉住：

| 单元 | 结果 |
|---|---|
| `-720` | `-0`（双精度负零）/ `0`（整数版） |
| `-900` | `-180` |
| `1000000` | `-80` |

#### 一处实现差异（行为等价）

Cubium 的 `MathUtils::wrapDegrees` 用的是 `if / else if`，原版是两个独立 `if`。由于两个
分支互斥，**行为完全等价**——本能力的 31 个单元把它验证出来了，无需修改。

Cubium 目前**没有** `positiveModulo`，那部分是待实现的行为基准。

#### 负零的保留

`wrap|-720` 在双精度版本得到 `-0`、整数版本得到 `0`。`std::fmod(-720.0, 360.0)` 返回负零，
后续的 `if` 都不命中，负零一路保留到输出。两侧格式化后都显示为 `-0`，一致。
