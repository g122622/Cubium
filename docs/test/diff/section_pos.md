# 差分能力：`section_pos`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 117 个能力，40 个用例全部一致。区块坐标换算此前未被覆盖
（[`blockpos_codec`](./blockpos_codec.md) 只覆盖了方块坐标的编码）。

## 核心语义：`>> 4` 是向下取整，不是截断

```java
public static int blockToSectionCoord(int blockCoord) { return blockCoord >> 4; }
```

| 方块坐标 | 区块坐标 |
|---|---|
| `15` | `0` |
| `16` | `1` |
| `0` | `0` |
| `-1` | **`-1`** |
| `-15` | **`-1`** |
| `-16` | **`-1`** |
| `-17` | **`-2`** |
| `-32` | **`-2`** |
| `-33` | **`-3`** |

**`-1 >> 4 == -1`，不是 `0`。** 方块 `-1` 属于区块 `-1`（它覆盖方块 `-16..-1`）。

若错用整数除法 `/ 16`（向零截断），`-1 / 16 == 0`，整个负半轴会**整体错位一个区块**。
这是世界生成、区块加载与方块实体定位的核心语义，错一位就会导致大规模错位。

`-16 >> 4 == -1` 也是关键点：区块 `-1` 覆盖 `-16..-1`，所以 `-16` 属于区块 `-1` 而不是
`-1` 的相邻区块。

## 其余三个关键点

**1. `blockToSectionCoord(double)` 先 `floor` 再右移。**

```java
public static int blockToSectionCoord(double blockCoord) { return Mth.floor(blockCoord) >> 4; }
```

| 单元 | 结果 |
|---|---|
| `blockToSectionD\|0.0` | `0` |
| `blockToSectionD\|15.9` | `0` |
| `blockToSectionD\|-0.1` | **`-1`** |
| `blockToSectionD\|-16.5` | **`-2`** |

`-0.1` 先 `floor` 成 `-1` 再右移，得到区块 `-1`。若不 `floor` 直接截断成 `0`，会落到错误的区块。

**2. `sectionToBlockCoord` 是精确乘 16。**

| 单元 | 结果 |
|---|---|
| `sectionToBlock\|1` | `16` |
| `sectionToBlock\|-1` | `-16` |
| `sectionToBlock\|-2` | `-32` |
| `sectionToBlockOff\|-1\|8` | `-8` |

与 `>> 4` 构成互逆映射：`sectionToBlockCoord(blockToSectionCoord(x)) <= x`，差值在 `0..15`。

**3. `asLong` 的三个字段位宽不同。**

| 字段 | 位宽 | 掩码 | 起始位 | 取值范围 |
|---|---|---|---|---|
| X | 22 位 | `4194303` (`0x3FFFFF`) | 42 | `-2097152..2097151` |
| Z | 22 位 | `4194303` | 20 | `-2097152..2097151` |
| Y | **20 位** | `1048575` (`0xFFFFF`) | 0 | `-524288..524287` |

**Y 只有 20 位，X 与 Z 各 22 位**——世界高度远小于水平范围。

```java
public static long asLong(int x, int y, int z) {
    long i = 0L;
    i |= (x & 4194303L) << 42;
    i |= (y & 1048575L) << 0;
    return i | (z & 4194303L) << 20;
}
```

| 单元 | 结果 |
|---|---|
| `asLong\|0\|0\|0` | `0` |
| `asLong\|1\|2\|3` | `4398049656834` |
| `asLong\|-1\|-1\|-1` | **`-1`** |
| `asLong\|-5\|10\|-20` | `-17592207015926` |
| `asLong\|100\|200\|-300` | `444202383048904` |

**`asLong(-1,-1,-1) == -1`**：三个字段全 `-1`，掩码后全为 1，拼起来就是 64 位全 1，即 `-1L`。
**符号扩展贯穿整个编码。**

**4. 反解用 `<< n >> m` 保留符号，不是直接掩码。**

```java
public static int x(long packed) { return (int)(packed << 0  >> 42); }
public static int y(long packed) { return (int)(packed << 44 >> 44); }
public static int z(long packed) { return (int)(packed << 22 >> 42); }
```

若写成 `(int)((packed >>> offset) & mask)`，负数的符号位会丢失，得到「无符号解码」。
`<<` 后再 `>>` 让符号位贯穿整个移位过程。

`decode|4398046511104`（即 `1L << 42`）得 `1,0,0`，验证 X 字段起始位正确。

往返用例含极值，全部可逆：

| 单元 | 结果 |
|---|---|
| `roundtrip\|2097151\|524287\|2097151` | `2097151,524287,2097151` |
| `roundtrip\|-2097152\|-524288\|-2097152` | `-2097152,-524288,-2097152` |

## 本轮流程改进

注册索引文件时严格按 `_patterns.md` 第十四条执行：先 `Copy-Item` 备份 → 改完立即校验三张列表
长度与逐项顺序 → 删除备份。

**校验脚本本身也出过一次错**：把 `-replace` 直接用在数组上（运算符优先级问题），一度显示
`SUFFIXES=1`、`SOURCES=113`，看起来像文件被破坏。改用
`$line | Where-Object { -like ... } | Select-Object -First 1` 取出单行后再替换，才得到正确结果
（`113/113/113`，错位 0）。

**教训**：校验脚本报告异常时，先确认**脚本本身**是否正确，再怀疑文件。这与第十四条「改完
必须校验」配套——校验工具也要可信。

## 未覆盖范围

- `SectionPos.of(ChunkPos, int)`、`of(EntityAccess)`、`of(Position)` 等重载
- `aroundChunk`、`cube`、`blocksInside`（依赖 `Stream`）
- `MIN_SECTION` / `MAX_SECTION` 常量本身
- `offset`、`relativeTo` 等实例方法（行为是纯加法，本能力的编码部分已隐含覆盖）