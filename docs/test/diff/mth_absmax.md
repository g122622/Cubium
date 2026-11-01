# 差分能力：`mth_absmax`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 162 个能力，27 个用例全部一致。这是 `Mth` 数学函数族的补全——
[`mth_bit`](./mth_bit.md) 覆盖了 `smallestEncompassingPowerOfTwo` / `ceillog2` / `log2`，
本能力补上 `absMax` / `chessboardDistance` / `smallestSquareSide`。

## 核心发现一：`absMax` 是「先各自取绝对值，再取较大者」

```java
public static int absMax(int a, int b) {
    return Math.max(Math.abs(a), Math.abs(b));
}
```

**不是**「先比较 `a` 与 `b`，再对较大者取绝对值」。两者对常规值等价，但**对 `Integer.MIN_VALUE`
天差地别**：

```java
Math.abs(Integer.MIN_VALUE) == Integer.MIN_VALUE   // 溢出，仍为负
```

| 单元 | 原版结果 | 「先比较再取绝对值」会得到 |
|---|---|---|
| **`absMaxInt\|-2147483648\|5`** | **`5`** | **`-2147483648`** |
| **`absMaxInt\|-2147483648\|0`** | **`0`** | **`-2147483648`** |
| **`absMaxInt\|-2147483648\|-2147483648`** | **`-2147483648`** | `-2147483648` |

**第二个用例最直观**：`Math.max(-2147483648, 0) = 0`，而错误实现返回 `Math.abs(-2147483648)`
= `-2147483648`——**正负号都反了**。

### 三个重载行为一致

`absMax(float, float)` 与 `absMax(double, double)` 同样用 `Math.max(Math.abs(a), Math.abs(b))`：

| 单元 | 结果 |
|---|---|
| `absMaxFloat\|3.5\|-5.25` | `5.25` |
| `absMaxFloat\|-1.5\|-2` | `2` |
| `absMaxFloat\|0\|-0` | `0`（`Math.abs(-0.0f) = 0.0f`）|
| `absMaxDouble\|3.5\|-5.25` | `5.25` |

## 核心发现二：`chessboardDistance` 是 **2D** 的，参数顺序是 `(x1, z1, x2, z2)`

```java
public static int chessboardDistance(int x1, int z1, int x2, int z2) {
    return absMax(x2 - x1, z2 - z1);
}
```

**没有 `y` 分量**——它是 X/Z 平面上的切比雪夫距离（国际象棋王的移动步数，斜走算一步）。

| 单元 | 结果 | 说明 |
|---|---|---|
| `chess\|0\|0\|3\|4` | **`4`** | 不是欧氏距离 `5`，而是 `max(3,4)` |
| `chess\|0\|0\|4\|3` | `4` | |
| `chess\|-2\|-3\|1\|1` | **`4`** | `max(1-(-2), 1-(-3)) = max(3,4)` |
| `chess\|5\|5\|5\|5` | `0` | |

**注意参数顺序是 `(x1, z1, x2, z2)`，`z1` 在第二位**——不是 `(x1, x2, z1, z2)`。

若 Cubium 写成 3D 的切比雪夫距离（含 `y`）或欧氏距离，结果完全不同。

### 坐标差会溢出回绕

| 单元 | 结果 | 原因 |
|---|---|---|
| **`chess\|0\|0\|2147483647\|-1`** | **`2147483647`** | `2147483647 - 0` 正常，`-1 - 0 = -1` |

上例没有触发溢出。但若输入是 `(0, 0, -1, 2147483647)`，`2147483647 - (-1)` 在 `int` 下回绕成
`-2147483648`，再取 `absMax` 仍是 `-2147483648`——**结果为负**。

Java 的 `int` 减法静默回绕；C++ 的有符号溢出是 UB。探针必须用无符号减法再转回：

```cpp
static_cast<i32>(static_cast<u32>(a) - static_cast<u32>(b))
```

## 核心发现三：`smallestSquareSide(0)` 返回 `0`，负数抛异常

```java
public static int smallestSquareSide(int itemCount) {
    if (itemCount < 0) {
        throw new IllegalArgumentException("itemCount must be greater than or equal to zero");
    }
    return ceil(Math.sqrt(itemCount));
}
```

| 单元 | 结果 |
|---|---|
| **`square\|0`** | **`0`** |
| `square\|1` | `1` |
| `square\|2` | `2` |
| `square\|3` | `2` |
| `square\|4` | `2` |
| `square\|5` | `3` |
| `square\|9` | `3` |
| `square\|10` | `4` |
| `square\|16` | `4` |
| `square\|17` | `5` |

**`ceil(Math.sqrt(0)) = 0`**——不是 `1`。若实现成 `(int) std::ceil(std::sqrt(n))` 但用了
`Mth.ceil`（对 `double` 的实现可能不同），需要核对。

## 本轮踩坑：浮点格式的科学计数法差异

首次运行：

```
oracle: {"value":"10000000000"}
probe : {"value":"1e+10"}
```

`%.9g` 对 `1e10` 输出 **`1e+10`**，而 oracle 用
`BigDecimal.stripTrailingZeros().toPlainString()` 输出 **`10000000000`**。

**这是 `_patterns.md` 中「尾数与指数必须分开处理」的又一次体现**——本项目历史上已有多次同类问题。

修复：探针改为先用 `%.8e` 取 9 位有效数字，再**手工展开成定点表示**：

```cpp
std::snprintf(buffer, sizeof(buffer), "%.8e", value);   // 1.00000000e+10
// 取出 mantissa 与 exponent，去尾随零，再按 exponent 放置小数点
```

**教训**：凡是 oracle 用 `BigDecimal` 的路径，探针都不能用 `%g`——必须复刻 `toPlainString`
的定点展开语义。`toPlainString` **永不输出科学计数法**。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`mth_bit`](./mth_bit.md) | `smallestEncompassingPowerOfTwo` / `ceillog2` / `log2` |
| [`mth_rounding`](./mth_rounding.md) | `floor` / `ceil` / `lfloor` |
| [`mth_clamp`](./mth_clamp.md) | `clamp` 四个重载 |
| [`mth_frac`](./mth_frac.md) | `frac` |
| [`mth_seed`](./mth_seed.md) | `getSeed` |
| [`blockpos_spiral`](./blockpos_spiral.md) | 切比雪夫距离的另一个使用场景 |

## 未覆盖范围

- `Mth.absMax` 的 `Integer.MIN_VALUE` 之外的溢出组合（如 `absMax(MIN_VALUE, MIN_VALUE)` 已覆盖）
- `Mth.smallestSquareSide` 对 `Integer.MAX_VALUE` 的结果（`ceil(sqrt(2^31-1)) = 46341`）
- `Mth.atan2` / `invSqrt` / `fastInvSqrt`（浮点特殊函数）
- `Mth.getInt(String, int)`（字符串解析，行为类似 `Integer.parseInt` 的容错版）
- `Mth.rotateIfNecessary` / `approachDegrees` / `wrapDegrees` 系列（角度）