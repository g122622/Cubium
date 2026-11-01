# 差分能力：`blockpos_codec`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 96 个能力，21 个打包/偏移用例全部一致。`BlockPos` 的位打包此前没有被比对过。

## 三个关键点

**1. 位段宽度是 `X/Z: 26`、`Y: 12`，偏移是 `X << 38`、`Y << 0`、`Z << 12`。**

```java
public static final int PACKED_HORIZONTAL_LENGTH =
    1 + Mth.log2(Mth.smallestEncompassingPowerOfTwo(30000000));   // = 26
public static final int PACKED_Y_LENGTH = 64 - 2 * PACKED_HORIZONTAL_LENGTH;  // = 12

public static long asLong(int x, int y, int z) {
    long i = 0L;
    i |= (x & PACKED_X_MASK) << X_OFFSET;   // 38
    i |= (y & PACKED_Y_MASK) << 0;
    return i | (z & PACKED_Z_MASK) << Z_OFFSET;   // 12
}
```

实测：`pack|-1|0|0` → `-274877906944`（X 左移 38），`pack|0|0|-1` → `274877902848`
（Z 左移 12），`pack|0|-1|0` → `4095`（Y 占低 12 位）。

**2. 解包用「左移再算术右移」还原符号。**

```java
public static int getX(long p) {
    return (int)(p << 64 - X_OFFSET - PACKED_HORIZONTAL_LENGTH >> 64 - PACKED_HORIZONTAL_LENGTH);
}
```

C++ 侧必须用**有符号** `i64` 右移。用 `u64` 无符号右移会把负数坐标变成巨大的正数。

**3. `asLong` 先掩码再移位，所以超范围坐标被静默翻转。**

`pack|0|2048|0` 得到 `packed=2048`，解包回来是 `(0, -2048, 0)`；`pack|33554432|0|0` 得到
`packed=-9223372036854775808`，解包是 `(-33554432, 0, 0)`。

既不报错也不饱和，而是**按位段宽度回绕**。这是原版行为，必须复刻。

| 输入 | 打包后 | 解包结果 |
|---|---|---|
| `y = 2047` | 2047 | `y = 2047`（上界内） |
| `y = 2048` | 2048 | `y = -2048`（翻转） |
| `y = -2049` | 2047 | `y = 2047`（翻转） |
| `x = 33554431` | ... | `x = 33554431`（上界内） |
| `x = 33554432` | ... | `x = -33554432`（翻转） |

坐标范围：`MAX_HORIZONTAL_COORDINATE = (1 << 26) / 2 - 1 = 33554431`。

## 输出格式注意

打包结果以**字符串**输出。`long` 的范围超出 JavaScript 的安全整数范围（2^53），用 JSON
number 传递会在 harness 侧退化为 `double` 并丢失低位——`pack|100|64|200` 的
`27487791513664` 已经超过 2^44，接近危险区间，而三轴极值的组合会直接超过 2^53。

## 未覆盖范围

- `betweenClosed`（包围盒迭代顺序）
- `distManhattan` / `distSqr` / `distChessboard`（简单距离公式）
- `containing`（依赖 `Mth.floor`，已有 `mth_rounding` 覆盖）