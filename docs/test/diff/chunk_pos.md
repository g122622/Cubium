# 差分能力：`chunk_pos`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 119 个能力，33 个用例全部一致。区块坐标的 `long` 编码与区域换算此前未被覆盖
（[`section_pos`](./section_pos.md) 覆盖的是方块↔分区的换算与分区的 22/20 位编码）。

## 四个关键点

**1. `asLong` 用两个 32 位字段无符号拼接，与 `SectionPos` 的窄字段 + 符号扩展完全不同。**

```java
public static long asLong(int x, int z) {
    return x & 4294967295L | (z & 4294967295L) << 32;
}
```

| 类 | 字段位宽 | 有无符号扩展 | `(-1, -1)` 的编码 |
|---|---|---|---|
| `ChunkPos` | X 32 位、Z 32 位 | **无**（`& 0xFFFFFFFF`） | `-1L` |
| `SectionPos` | X/Z 22 位、Y 20 位 | **有** | 见 [`section_pos`](./section_pos.md) |

`ChunkPos` 两个字段各占满 32 位，**恰好铺满整个 `long`**，没有空隙也没有符号扩展余地。

实测：

| 单元 | 结果 | 说明 |
|---|---|---|
| `asLong\|1\|0` | `1` | |
| `asLong\|0\|1` | `4294967296` | `1 << 32` |
| **`asLong\|-1\|0`** | **`4294967295`** | **`-1` 被转成无符号，不是 `-1`** |
| `asLong\|0\|-1` | `-4294967296` | |
| **`asLong\|-1\|-1`** | **`-1`** | 两个 32 位全 1 拼成 64 位全 1 |
| `asLong\|2147483647\|-2147483648` | `-9223372034707292161` | 极值 |

**`asLong(-1, 0) == 4294967295`** 是最易错的一条。若按 `SectionPos` 的符号扩展习惯写，
会得到 `-1`，与 `(0, -1)` 的编码混淆。

**2. 反解 X 用 `&`，反解 Z 用 `>>>`。**

```java
public static int getX(long packed) { return (int)(packed & 4294967295L); }
public static int getZ(long packed) { return (int)(packed >>> 32 & 4294967295L); }
```

低位直接掩码即可；**高位必须逻辑右移**（`>>>`）把符号位补 0，再掩码取低 32 位。若用算术
右移 `>>`，负数打包值会把高 32 位补满 1，`getZ` 恒为 `-1`。

实测 `decode|-1` 得 `-1,-1`——若用 `>>`，Z 会得到 `-1`（碰巧对），但 `decode|0|-1` 即
`-4294967296` 就会出错。往返用例 `roundtrip|2147483647|-2147483648` 覆盖了这一点。

**3. 区域是 32×32 个区块，用 `<< 5` / `+ 31` 换算。**

```java
public static ChunkPos minFromRegion(int rx, int rz) { return new ChunkPos(rx << 5, rz << 5); }
public static ChunkPos maxFromRegion(int rx, int rz) {
    return new ChunkPos((rx << 5) + 31, (rz << 5) + 31);
}
```

**`REGION_SIZE = 32` 而不是 16**——区域是区块级的 2 的幂，与方块级的分区尺寸（16）不同。

`+ 31` 而不是 `+ 32`：区域 `r` 覆盖区块 `r*32 .. r*32+31`，共 32 个。

| 单元 | 结果 |
|---|---|
| `minRegion\|0\|0` | `0,0` |
| `maxRegion\|0\|0` | `31,31` |
| `minRegion\|1\|2` | `32,64` |
| `maxRegion\|1\|2` | `63,95` |
| **`minRegion\|-1\|-1`** | **`-32,-32`** |
| **`maxRegion\|-1\|-1`** | **`-1,-1`** |

负区域号同样正确：`-1 << 5 = -32`，区域 `-1` 覆盖区块 `-32..-1`。

**4. `hash` 是两个独立的 LCG 混合后异或，X 与 Z 处理不对称。**

```java
public static int hash(int x, int z) {
    int i = 1664525 * x + 1013904223;
    int j = 1664525 * (z ^ -559038737) + 1013904223;
    return i ^ j;
}
```

**Z 先与 `-559038737` 异或**再走同样的 LCG。这保证 `(0,0)` 与 `(0,1)` 之类的小坐标也能散开，
不会因为 `z = 0` 让 `j` 退化成常数。

| 单元 | 结果 |
|---|---|
| `hash\|0\|0` | `1455762653` |
| `hash\|0\|1` | `1459391018` |
| `hash\|1\|0` | `1445168878` |
| `hash\|1\|2` | `1458653700` |
| `hash\|-1\|-1` | `843918461` |

`(0,0)` 与 `(0,1)` 差异明显，证实魔数异或在起作用。Java 的 `int` 乘法溢出环绕（与
[`mth_bit`](./mth_bit.md)、[`mth_color`](./mth_color.md) 同类语义）。

## 方块边界换算

```java
public int getMinBlockX() { return SectionPos.sectionToBlockCoord(this.x); }
public int getBlockX(int offset) { return SectionPos.sectionToBlockCoord(this.x, offset); }
public int getMiddleBlockX() { return this.getBlockX(8); }
public int getMaxBlockX() { return this.getBlockX(15); }
```

| 单元 | 结果（minX,midX,maxX,minZ,midZ,maxZ） |
|---|---|
| `blocks\|0\|0` | `0,8,15,0,8,15` |
| `blocks\|1\|2` | `16,24,31,32,40,47` |
| `blocks\|-1\|-1` | `-16,-8,-1,-16,-8,-1` |

中点用偏移 **8**，上界用偏移 **15**（不是 16）——区块覆盖 16 个方块，`+15` 才是最后一个。

## 公开常量

```java
public static final long INVALID_CHUNK_POS = asLong(1875066, 1875066);
```

实测 `invalid` 得 `8053347149716602`，与 `asLong|1875066|1875066` 一致——常量的定义与编码
逻辑自洽。

## 一个实现细节

`ChunkPosOracle` 需要调用 `DataPackRegistries.load()`，因为 `ChunkPos` 的**静态初始化块**会
访问 `BlockPos.MAX_HORIZONTAL_COORDINATE` 与 `ChunkPyramid`：

```java
public static final int MAX_COORDINATE_VALUE =
    SectionPos.blockToSectionCoord(BlockPos.MAX_HORIZONTAL_COORDINATE) - SAFETY_MARGIN_CHUNKS;
```

即使本能力不直接用到 `MAX_COORDINATE_VALUE`，类的静态初始化也会执行它。这使本能力耗时约
14.8 秒（对比不需要注册表的 `level_height` 仅 2.9 秒）。

## 未覆盖范围

- `isValid` / `MAX_COORDINATE_VALUE`（依赖 `ChunkPyramid` 的累积依赖数量，是生成器拓扑的函数）
- `getRegionX` / `getRegionZ`
- `getBlockZ` 系列（与 X 完全对称）
- `ChunkPos.INVALID_CHUNK_POS` 在各处的判等用法