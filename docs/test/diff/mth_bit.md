# 差分能力：`mth_bit`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 99 个能力，45 个整数位运算用例全部一致。

## 四个关键点

**1. `smallestEncompassingPowerOfTwo` 对非正输入返回 0。**

```java
public static int smallestEncompassingPowerOfTwo(int x) {
    int i = x - 1;
    i |= i >> 1;  i |= i >> 2;  i |= i >> 4;  i |= i >> 8;  i |= i >> 16;
    return i + 1;
}
```

这是位传播实现，不是 `while` 循环或 `1 << ceil(log2(x))`。对非正输入：

| 输入 | `x - 1` | 传播后 | 返回 |
|---|---|---|---|
| `0` | `-1` | `-1` | **0** |
| `-1` | `-2` | `-1` | **0** |
| `-2` | `-3` | `-1` | **0** |
| `-5` | `-6` | `-1` | **0** |

**返回 0，不是 1。** 若用「向上取到 2 的幂」的常规写法（`while (p < x) p <<= 1`），会得到
1 或进入死循环。这个反直觉的结果必须原样复刻。

**2. `isPowerOfTwo(0)` 与 `isPowerOfTwo(负数)` 都返回 `false`。**

```java
return x != 0 && (x & (x - 1)) == 0;
```

`x != 0` 守卫挡掉了 0；负数因为二进制有多个 1 位（补码），`& (x-1)` 也不为 0。只有**正的**
2 的幂返回 `true`。

**3. `ceillog2` 依赖 De Bruijn 常数 `125613361L` 与 32 项查找表。**

```java
x = isPowerOfTwo(x) ? x : smallestEncompassingPowerOfTwo(x);
return MULTIPLY_DE_BRUIJN_BIT_POSITION[(int)(x * 125613361L >> 27) & 31];
```

这个常数的作用是把「2 的幂」映射到唯一索引。写成循环移位结果相同，但写成 `Integer.numberOfTrailingZeros(x)` 在 `x` 为 2 的幂时等价——需要小心的是 `x` 已被修正为 2 的幂，所以两者其实一致；真正容易错的是**漏掉 `isPowerOfTwo` 判断**，直接对非 2 的幂查表。

**4. `log2` 对非 2 的幂向下取整，`ceillog2` 向上取整。**

```java
public static int log2(int x) {
    return ceillog2(x) - (isPowerOfTwo(x) ? 0 : 1);
}
```

| 输入 | `log2` | `ceillog2` |
|---|---|---|
| `5` | **2**（2²=4 ≤ 5 < 8） | 3 |
| `9` | **3** | 4 |
| `31` | **4** | 5 |
| `1023` | 9 | 10 |
| `1024` | 10 | 10 |
| `1025` | **10** | 11 |

## 与 `blockpos_codec` 的呼应

`sep2(30000000)` 返回 **`33554432`**（2^25）。这正是 `BlockPos` 位宽公式的输入：

```java
PACKED_HORIZONTAL_LENGTH = 1 + Mth.log2(Mth.smallestEncompassingPowerOfTwo(30000000))
                         = 1 + log2(33554432) = 1 + 25 = 26
```

见 [`blockpos_codec.md`](./blockpos_codec.md)。这两个能力互为印证：`mth_bit` 钉住底层位运算，
`blockpos_codec` 钉住它派生出的打包布局。

## 未覆盖范围

- `ceillog2` 的负数行为——依赖 `smallestEncompassingPowerOfTwo` 的负输入返回 0 后查表
  `[(0 * 125613361L) >> 27 & 31] = [0]`，结果是 0，但该路径在原版语义里属未定义使用
- `smallestSquareSide`（依赖 `ceil(Math.sqrt())`，浮点误差另议）