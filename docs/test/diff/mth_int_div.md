# 差分能力：`mth_int_div`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 125 个能力，51 个用例全部一致。`Mth` 的整数除法与 2 的幂工具此前未被覆盖。

## 四个关键点

**1. `floorDiv` 是向下取整，不是向零截断。**

```java
public static int floorDiv(int x, int y) { return Math.floorDiv(x, y); }
```

| 单元 | 结果 | 说明 |
|---|---|---|
| `floorDiv\|7\|2` | `3` | |
| **`floorDiv\|-7\|2`** | **`-4`** | **不是 `-3`**（Java 整除是 -3）|
| **`floorDiv\|7\|-2`** | **`-4`** | 除数为负也向下取整 |
| **`floorDiv\|-7\|-2`** | **`3`** | |
| `floorDiv\|-6\|2` | `-3` | 整除时无差异 |

**负被除数时 `floorDiv` 与 Java 整除差 1。** 这是最容易抄错的地方——`-7 / 2` 在 Java 里
得 `-3`，而 `Math.floorDiv(-7, 2)` 得 `-4`。

**2. `isPowerOfTwo(0)` 是 `false`，负数也是 `false`。**

```java
return value != 0 && (value & value - 1) == 0;
```

| 单元 | 结果 |
|---|---|
| **`isPow2\|0`** | **`false`** |
| `isPow2\|1` | `true` |
| `isPow2\|4` | `true` |
| `isPow2\|16384` | `true` |
| `isPow2\|3` | `false` |
| `isPow2\|16383` | `false` |
| **`isPow2\|-4`** | **`false`**（负数不是 2 的幂）|

**只有正的真 2 的幂返回 `true`。** 零被第一个条件挡住；负数因为符号位使 `value & (value-1)`
不为 0。

**3. `smallestEncompassingPowerOfTwo` 对 `value <= 0` 返回 `0`，不做保护。**

```java
int i = value - 1;
i |= i >> 1;  i |= i >> 2;  i |= i >> 4;  i |= i >> 8;  i |= i >> 16;
return i + 1;
```

| 单元 | 结果 | 说明 |
|---|---|---|
| **`smallest\|0`** | **`0`** | **不是 1！** |
| **`smallest\|-1`** | **`0`** | 负数返回 0 |
| `smallest\|1` | `1` | |
| `smallest\|2` | `2` | |
| `smallest\|3` | `4` | |
| `smallest\|4` | `4` | |
| `smallest\|5` | `8` | |
| `smallest\|17` | `32` | |

`value = 0` 时 `i = -1`（32 位全 1），位扩散后仍是 `-1`，`-1 + 1 = 0`。

**函数名暗示「至少返回 1」，实际对 `0` 返回 `0`。** 这是本能力最反直觉的一条——若调用方
用它的结果做数组长度或掩码，会得到 0 而非 1。

**4. `ceillog2` 与 `log2` 是一对上下取整。**

```java
public static int ceillog2(int value) {
    value = isPowerOfTwo(value) ? value : smallestEncompassingPowerOfTwo(value);
    return MULTIPLY_DE_BRUIJN_BIT_POSITION[(int)(value * 125613361L >> 27) & 31];
}

public static int log2(int value) {
    return ceillog2(value) - (isPowerOfTwo(value) ? 0 : 1);
}
```

| 输入 | `ceillog2` | `log2` |
|---|---|---|
| `1` | 0 | 0 |
| `2` | 1 | 1 |
| **`3`** | **2** | **1** |
| `4` | 2 | 2 |
| **`5`** | **3** | **2** |
| `16` | 4 | 4 |
| **`17`** | **5** | **4** |
| `1024` | 10 | — |

**`ceillog2` 向上取整，`log2` 向下取整。** `ceillog2(3) = 2`（因为 `2^1 < 3 ≤ 2^2`），
`log2(3) = 1`（因为 `2^1 ≤ 3 < 2^2`）。

**De Bruijn 技巧**：

```java
MULTIPLY_DE_BRUIJN_BIT_POSITION[(int)(value * 125613361L >> 27) & 31]
```

`value * 125613361` 用 `long` 计算（避免 32 位溢出），右移 27 位取高位作为索引。这是经典的
「乘常数 + 移位」求 2 的幂指数的方法，比循环移位快，但表格与常数**必须逐字准确**。

## `positiveCeilDiv` 与 `roundToward`

```java
public static int positiveCeilDiv(int x, int y) { return -Math.floorDiv(-x, y); }
public static int roundToward(int value, int divisor) {
    return positiveCeilDiv(value, divisor) * divisor;
}
```

| 单元 | 结果 |
|---|---|
| `posCeilDiv\|7\|2` | `4` |
| `posCeilDiv\|6\|2` | `3` |
| `posCeilDiv\|1\|5` | `1` |
| **`posCeilDiv\|0\|5`** | `0` |
| **`posCeilDiv\|-1\|5`** | **`0`** |
| `roundToward\|7\|2` | `8` |
| `roundToward\|1\|5` | `5` |
| **`roundToward\|-1\|5`** | **`0`** |
| `roundToward\|15\|16` | `16` |
| `roundToward\|16\|16` | `16` |

**`posCeilDiv(-1, 5) == 0`**：`-floorDiv(1, 5) = -0 = 0`。负数被「向上取整到 0」而不是保持
负数——名字里的 `positive` 就体现在这里。

`roundToward` 把 `value` **向上取整到 `divisor` 的倍数**，对负数同样会拉到 0。

## 未覆盖范围

- `Mth.nextInt(RandomSource, int, int)`（需要随机源）
- `Mth.frac` 的浮点版本（与 [`mth_rounding`](./mth_rounding.md) 部分重叠）
- 除数为 0 的行为（`Math.floorDiv` 抛 `ArithmeticException`，属错误路径）