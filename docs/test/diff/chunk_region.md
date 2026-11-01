# 差分能力：`chunk_region`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 152 个能力，25 个用例全部一致。补全了 [`chunk_pos`](./chunk_pos.md) 文档中列出的
`getRegionX` / `getRegionZ` 缺口。

## 核心发现一：区域坐标是算术右移，对负数**向下取整**

```java
public int getRegionX() { return this.x >> 5; }
```

`>>` 是**算术右移**——保留符号位，对负数向负无穷取整。

| 单元 | 结果 |
|---|---|
| `regionX\|0` | `0` |
| `regionX\|31` | `0` |
| `regionX\|32` | `1` |
| `regionX\|63` | `1` |
| `regionX\|64` | `2` |
| **`regionX\|-1`** | **`-1`** |
| **`regionX\|-32`** | **`-1`** |
| **`regionX\|-33`** | **`-2`** |
| **`regionX\|-64`** | **`-2`** |

**`regionX|-1 → -1` 是最关键的一条**：若写成整数除法 `x / 32`（C++ 的 `/` 向零截断），
`-1 / 32 = 0`，与原版差 1。

C++ 的 `>>` 对负数**实现定义**（C++20 起规定为算术右移），但为保险起见应显式用算术右移语义。
在 Cubium 中应确认编译器行为，或显式写成 `x >> 5`（C++20 保证算术右移）。

`regionZ` 完全对称：`regionZ|32 → 1`、`regionZ|-33 → -2`。

## 核心发现二：区域内的局部坐标用 `& 31`，负数得到**非负**结果

```java
public int getRegionLocalX() { return this.x & 31; }
```

`& 31` 等价于「模 32 的非负余数」——与 C++ 的 `%` 对负数行为不同。

| 单元 | 结果 | `% 32` 会得到 |
|---|---|---|
| `localX\|0` | `0` | `0` |
| `localX\|31` | `31` | `31` |
| `localX\|32` | `0` | `0` |
| **`localX\|-1`** | **`31`** | **`-1`** |
| **`localX\|-32`** | **`0`** | **`0`** |
| **`localX\|-33`** | **`31`** | **`-1`** |

**`localX|-1 → 31`** 而 `-1 % 32 = -1`——两者差 32。

Java 的 `%` 也是向零截断（`-1 % 32 = -1`），所以原版**特意用了 `&` 而不是 `%`**。

`localZ` 对称：`localZ|-1 → 31`、`localZ|33 → 1`。

## 核心发现三：`minFromRegion` / `maxFromRegion` 是 `<< 5` 与 `+ 31`

```java
public static ChunkPos minFromRegion(int x, int z) { return new ChunkPos(x << 5, z << 5); }
public static ChunkPos maxFromRegion(int x, int z) {
    return new ChunkPos((x << 5) + 31, (z << 5) + 31);
}
```

| 单元 | 结果 |
|---|---|
| `minFromRegion\|0\|0` | `0,0` |
| `minFromRegion\|1\|-1` | `32,-32` |
| **`minFromRegion\|-1\|-2`** | **`-32,-64`** |
| `maxFromRegion\|0\|0` | `31,31` |
| **`maxFromRegion\|1\|-1`** | **`63,-1`** |
| **`maxFromRegion\|-1\|-2`** | **`-1,-33`** |

**`maxFromRegion|-1|-2 → -1,-33`** 是负数区域的正确结果：
区域 `(-1,-2)` 覆盖 `x ∈ [-32,-1]`、`z ∈ [-64,-33]`。

`<< 5` 对负数是「乘 32」，与 `>> 5` 互为逆运算（在区间内）。

## 核心发现四：`isValid` 未纳入本能力

```java
private static final int SAFETY_MARGIN_CHUNKS =
    (32 + ChunkPyramid.GENERATION_PYRAMID.getStepTo(ChunkStatus.FULL)
               .accumulatedDependencies().size() + 1) * 2;
public static final int MAX_COORDINATE_VALUE =
    SectionPos.blockToSectionCoord(BlockPos.MAX_HORIZONTAL_COORDINATE) - SAFETY_MARGIN_CHUNKS;

public static boolean isValid(int x, int z) {
    return Mth.absMax(x, z) <= MAX_COORDINATE_VALUE;
}
```

`MAX_COORDINATE_VALUE` 依赖 **`ChunkStatus` 的依赖链长度**——这是生成器拓扑的函数，随版本变化。

**若要纳入 `isValid`，必须先实现 `ChunkPyramid`**——那属于「结构生成器」领域的能力。

`isValid` 的**判定形式**（`Mth.absMax(x, z) <= V`，不是分别比较）记录在此，供未来实现参考。

## 本轮踩坑一：`ChunkPos` 的静态初始化需要 bootstrap

首次运行 oracle 崩溃：

```
ExceptionInInitializerError
    at net.minecraft.world.level.chunk.status.ChunkStatus.register(ChunkStatus.java:47)
    at net.minecraft.world.level.chunk.status.ChunkStatus.<clinit>(ChunkStatus.java:31)
    at net.minecraft.world.level.chunk.status.ChunkPyramid.<clinit>(ChunkPyramid.java:12)
    at net.minecraft.world.level.ChunkPos.<clinit>(ChunkPos.java:46)
Caused by: java.lang.IllegalArgumentException: Not bootstrapped (called from registry minecraft:game_event)
```

**`ChunkPos` 的 `MAX_COORDINATE_VALUE` 是个静态 `final` 字段**，它的初始化会拉起整条
`ChunkPyramid → ChunkStatus → BuiltInRegistries` 链。

修复：在 `main` 开头加

```java
net.minecraft.SharedConstants.tryDetectVersion();
net.minecraft.server.Bootstrap.bootStrap();
```

**顺序不能反**——`bootStrap()` 本身要求先设置游戏版本，否则抛 `IllegalStateException: Game version not set`。

这一模式在 [`block_destroy_progress`](./block_destroy_progress.md) 等 oracle 中已有先例。

## 本轮踩坑二：`isValid` 用例导致显式抛错

我最初把 `isValid` 用例留在单元表里，探针侧写成 `throw`。这会让**每次运行都报错**——
即使这是「有意未实现」，也不该污染测试结果。

**改为完全移除该组用例**，把「未覆盖」写进文档。

`_patterns.md` 里「未实现的部分要写 TODO」的原则在此适用，但**不实现的用例不应进入单元表**。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`chunk_pos`](./chunk_pos.md) | 区块坐标的 `long` 编码（本能力补全它的区域换算缺口）|
| [`section_pos`](./section_pos.md) | 方块↔分区的换算与分区编码 |
| [`blockpos_geom`](./blockpos_geom.md) | `BlockPos` 几何运算 |

## 未覆盖范围

- **`ChunkPos.isValid` / `MAX_COORDINATE_VALUE`**（依赖 `ChunkPyramid`，见上文）
- `ChunkPos.INVALID_CHUNK_POS`（`asLong(Integer.MAX_VALUE, Integer.MAX_VALUE)`）
- `ChunkPos.hashCode` / `equals`（内部用 `HASH_A` / `HASH_C` 的 LCG 混合）
- `ChunkPos.getChessboardDistance` / `getMinDistanceSquared`
- `ChunkPos.CODEC` / `STREAM_CODEC`（序列化）