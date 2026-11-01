# 差分能力：`mth_frac`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 141 个能力，27 个用例全部一致。`Mth.frac` 的浮点版本此前未被覆盖——它是
[`mth_rounding`](./mth_rounding.md)（`floor`/`ceil`）与 [`mth_int_div`](./mth_int_div.md) 的补充，
覆盖了 `frac` 的两个重载**以及它们对 `floor`/`lfloor` 的依赖**。

## 核心发现一：`frac(float)` 在大输入上完全失效

```java
public static float frac(float f) { return f - floor(f); }     // floor 返回 int！
```

`floor(float)` 的返回类型是 **`int`**，实现是「先 `(int)` 转换再比较」。而 **Java 的 `(int)`
转换对超界值是饱和的**。

| 单元 | 结果 | 说明 |
|---|---|---|
| **`floorF\|1e10`** | **`2147483647`** | 饱和 |
| **`floorF\|-1e10`** | **`2147483647`** | **负数也饱和到 `MAX_VALUE`** |
| **`fracF\|1e10`** | **`7.85251635e+09`** | **完全不是小数部分** |
| **`fracF\|3e9`** | **`852516352`** | 同上 |
| **`fracF\|-3e9`** | **`-5.14748365e+09`** | **负的** |

**`fracF|1e10` 得约 `7.85e9`**——因为 `floor(1e10)` 饱和成 `2147483647`，而
`1e10 - 2147483647 ≈ 7.85e9`。

**`fracF|-3e9` 得 `-5.15e9`**：`floor(-3e9)` 同样饱和成 **`+2147483647`**（不是负数！），
所以 `-3e9 - 2147483647 ≈ -5.15e9`。

**`frac(float)` 的返回值可以远大于 1，也可以为负**——与「小数部分」的直觉完全不符。

### 这是 Java 与 C++ 的关键分歧

| 语言 | `(int)` 对超界值 |
|---|---|
| **Java** | **饱和到 `Integer.MAX_VALUE`**（一律，不分正负）|
| **C++** | **未定义行为**（本平台实测得到 `MIN_VALUE`）|

**Cubium 若要复刻 `frac(float)`，必须显式实现 Java 的饱和语义**，不能直接 `static_cast<i32>`。

探针的做法：

```cpp
i32 javaToInt(f32 value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483647.0F)  return 2147483647;
    if (value <= -2147483648.0F) return -2147483648;
    return static_cast<i32>(value);
}
```

## 核心发现二：`frac` 对负数返回**正**的小数部分

`floor` 向负无穷取整，因此 `f - floor(f)` 恒为**非负**。

| 单元 | 结果 |
|---|---|
| `fracF\|1.5` | `0.5` |
| `fracF\|1.25` | `0.25` |
| `fracF\|-1.5` | **`0.5`** |
| `fracF\|-1.25` | **`0.75`** |
| `fracF\|-0.25` | **`0.75`** |
| `fracF\|-2` | `0` |

**`frac(-1.25) = 0.75`**：`-1.25 - (-2) = 0.75`。

若 `floor` 写成向零截断（`(int)`），会得 `-1.25 - (-1) = -0.25`——**符号相反，数值差 1**。

**`frac` 的值域是 `[0, 1)`**，不是 `(-1, 1)`。

## 核心发现三：`frac(double)` 用 `long`，在 `1e300` 上也饱和

```java
public static double frac(double d) { return d - lfloor(d); }   // lfloor 返回 long
```

`lfloor` 的 `(long)` 转换同样**饱和到 `Long.MAX_VALUE`**。

| 单元 | 结果 |
|---|---|
| `fracD\|1.5` | `0.5` |
| `fracD\|-1.5` | `0.5` |
| `fracD\|1e10` | `0`（`1e10` 是整数）|
| **`floorD\|1e10`** | **`10000000000`** |
| **`floorD\|-1e300`** | **`9223372036854775807`** |
| **`fracD\|1e300`** | **`1e+300`** |

**`fracD|1e300` 得到 `1e+300`**：`lfloor(1e300)` 饱和成 `9.223e18`，而 `1e300 - 9.2e18 ≈ 1e300`
（`9.2e18` 在 `1e300` 面前完全可忽略）。

**`floorD|-1e300` 饱和到正数 `Long.MAX_VALUE`**——同样地，负的极大值也饱和到 `MAX_VALUE`。

### `long` 的饱和边界比 `int` 宽得多

| 类型 | 饱和阈值 | 常见坐标范围是否受影响 |
|---|---|---|
| `int`（`floor`） | ±`2.1e9` | **会**（区块坐标可达 `3e7`，但乘上 `2^24` 后会超）|
| `long`（`lfloor`） | ±`9.2e18` | 基本不会 |

**所以 `frac(double)` 在实践中是安全的，而 `frac(float)` 不是。**

## 核心发现四：`float` 的精度阈值是 `2^24`

`float` 只有 24 位有效数字，当 `|f| >= 2^24 = 16777216` 时，`f` 本身已是整数。

| 单元 | 结果 |
|---|---|
| `fracF\|16777216` | `0` |
| `fracF\|16777217` | **`0`**（无法表示 `.0` 以外的小数）|
| **`fracF\|8388608.5`** | **`0`** |

**`8388608.5` 是 `2^23 + 0.5`，结果却是 `0`**——已查清原因并验证：

```
f = std::stof("8388608.5")  ->  f = 8388608, bits = 0x4B000000
```

**`8388608.5` 根本无法被 `float` 表示。** `2^23 = 8388608` 是 `float` 最后一个「每个整数都能精确
表示」的值；从 `2^23` 到 `2^24`，相邻可表示值的**间隔已经是 1.0**。

因此 `8388608.5` 落在 `8388608` 与 `8388609` 正中间，按 **IEEE 754 舍入到偶数**规则，
落到 `8388608`（偶数）。

`frac(8388608) = 0` 是**正确行为**，不是缺陷。

**教训**：设计边界用例时，要确认该值在目标精度下**确实可表示**——否则测的是「字符串到浮点的
舍入」，而不是被测函数本身。

## 本轮踩坑：`%.9g` 的指数形式尾零

首次运行报出 1 处差异：

```
oracle: {"value":"1.00000000e+300"}
probe : {"value":"1e+300"}
```

**`printf("%.9g")` 与 Java 的 `String.format("%.9g")` 在指数形式下都会保留 9 位有效数字**，
而我的去尾零逻辑只在**没有指数**时才执行。

修复方式：把尾数与指数分开处理，只对尾数去尾零。

```cpp
const std::size_t exponentAt = text.find('e') != std::string::npos ? text.find('e') : text.find('E');
const std::string mantissa = exponentAt == std::string::npos ? text : text.substr(0, exponentAt);
const std::string exponent = exponentAt == std::string::npos ? std::string() : text.substr(exponentAt);
// 只对 mantissa 去尾零，再拼回 exponent
```

**这是 `_patterns.md` 里「表示差异」的一个具体实例**——两侧都必须做同样的规范化，否则即使数值
完全一致也会报差异。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`mth_rounding`](./mth_rounding.md) | `floor` / `ceil` 的完整语义（已记录超界饱和的语言差异）|
| [`mth_int_div`](./mth_int_div.md) | 整数除法与 2 的幂工具 |
| [`mth_degrees`](./mth_degrees.md) | 角度差（同样依赖 `clamp` / `absMax`）|
| [`blockpos_dist`](./blockpos_dist.md) | 坐标距离（`frac` 常用于插值偏移）|

## 未覆盖范围

- `8388608.5` 在 `float` 下无法表示（舍入到 `8388608`），因此该用例实际只验证了 `frac(2^23) = 0`
- `frac` 在坐标插值与粒子偏移中的实际调用点（属渲染/客户端，本项目客户端已停止维护）
- `Mth.frac` 对 `NaN` / `Infinity` 的行为（`floor(NaN)` 转 `(int)` 得 0，`NaN - 0` 仍是 `NaN`）
- `Mth.lfloor` / `Mth.lceil` 的完整族（本能力只覆盖了 `lfloor`）