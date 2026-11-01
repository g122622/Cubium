# 差分能力：`level_height`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 118 个能力，41 个用例全部一致。世界高度范围与分区索引此前未被覆盖
（[`dimension_type`](./dimension_type.md) 覆盖的是维度配置数据，不是高度判定逻辑）。

## 被验证的原版逻辑

`LevelHeightAccessor` 是一个**接口**，除 `getHeight()` / `getMinY()` 外全部是 `default` 方法：

```java
default int getMaxY() { return this.getMinY() + this.getHeight() - 1; }
default int getMinSectionY() { return SectionPos.blockToSectionCoord(this.getMinY()); }
default int getMaxSectionY() { return SectionPos.blockToSectionCoord(this.getMaxY()); }
default int getSectionsCount() { return this.getMaxSectionY() - this.getMinSectionY() + 1; }
default boolean isInsideBuildHeight(int y) { return y >= this.getMinY() && y <= this.getMaxY(); }
default boolean isOutsideBuildHeight(int y) { return y < this.getMinY() || y > this.getMaxY(); }
default int getSectionIndexFromSectionY(int sectionY) { return sectionY - this.getMinSectionY(); }
default int getSectionIndex(int y) {
    return this.getSectionIndexFromSectionY(SectionPos.blockToSectionCoord(y));
}
default int getSectionYFromSectionIndex(int index) { return index + this.getMinSectionY(); }
```

oracle 用 `LevelHeightAccessor.create(minY, height)` 构造**匿名实现**——是静态工厂，
**不需要注册表**，因此本能力启动很快（3.1 秒，对比需要注册表的能力约 13 秒）。

## 四个关键点

**1. `getMaxY()` 是含上界：`minY + height - 1`。**

| 配置 | maxY | 合法 Y |
|---|---|---|
| `minY=-64, height=384`（主世界） | **319** | `-64..319`，共 384 个 |
| `minY=0, height=256`（下界/末地） | 255 | `0..255`，共 256 个 |

若写成 `minY + height`（漏掉 `-1`），顶端会多放行一个方块。

**2. `isInsideBuildHeight` 与 `isOutsideBuildHeight` 严格互补。**

前者 `y >= minY && y <= maxY`，后者 `y < minY || y > maxY`（德摩根律）。**对任何整数输入
恰好一真一假，不存在空隙。**

7 个 `both` 用例全部证实：

| 单元 | 结果 |
|---|---|
| `both\|-64\|384\|-64` | `inside=true,outside=false` |
| `both\|-64\|384\|-65` | `inside=false,outside=true` |
| `both\|0\|256\|255` | `inside=true,outside=false` |
| `both\|0\|256\|256` | `inside=false,outside=true` |
| `both\|-5\|17\|-6` | `inside=false,outside=true` |
| `both\|-5\|17\|11` | `inside=true,outside=false` |

**3. 分区索引从 `minSectionY` 起算，不是从 0。**

主世界：`minY = -64` → `minSectionY = -64 >> 4 = -4`，所以

| 单元 | 结果 | 说明 |
|---|---|---|
| `sectionIndex\|-64\|384\|-64` | `0` | 最底部方块 → 索引 0 |
| `sectionIndex\|-64\|384\|0` | `4` | Y=0 所在分区 Y 为 0，索引 `0-(-4)=4` |
| `sectionIndex\|-64\|384\|319` | `23` | 最顶部 → 索引 23 |
| `indexToSectionY\|-64\|384\|0` | `-4` | 索引 0 对应分区 Y=-4 |

**负的 `minY` 会让 `minSectionY` 为负**，索引必须减去 `minSectionY` 才能落到 `0..count-1`。

**4. 分区数按跨越的区块算，不是 `height / 16`。**

`getSectionsCount() = maxSectionY - minSectionY + 1`。

| 单元 | 结果 | 说明 |
|---|---|---|
| `range\|-64\|384` | `319,-4,19,24` | 24 个分区 |
| `range\|0\|16` | `15,0,0,1` | 恰 1 个 |
| **`range\|0\|17`** | **`16,0,1,2`** | **17 格跨 2 个分区** |
| `range\|5\|17` | `21,0,1,2` | `5..21` 跨分区 0 与 1 |
| `range\|-16\|16` | `-1,-1,-1,1` | `-16..-1` 恰在同一分区内 |
| `range\|-16\|32` | `15,-1,0,2` | 跨 2 个分区 |
| `range\|-5\|100` | `94,-1,5,7` | `-5..94` 跨分区 `-1..5` |

**`range|0|17` 是最能说明问题的一条**：高度 17 格会跨越 **2 个分区**。若用
`height >> 4`（=1）或 `ceil(height/16)` 之外的近似，会少算一个分区，导致顶层数据丢失。

## 实测完整数据

```text
range|-64|384   -> 319,-4,19,24
range|0|256     -> 255,0,15,16
range|0|16      -> 15,0,0,1
range|-16|16    -> -1,-1,-1,1
range|-16|32    -> 15,-1,0,2
range|0|17      -> 16,0,1,2
range|5|17      -> 21,0,1,2
range|-5|100    -> 94,-1,5,7
```

`sectionIndex` 边界：

```text
sectionIndex|-64|384|-64   -> 0
sectionIndex|-64|384|-1    -> 3
sectionIndex|-64|384|0     -> 4
sectionIndex|-64|384|15    -> 4
sectionIndex|-64|384|16    -> 5
sectionIndex|-64|384|319   -> 23
sectionIndex|0|256|-1      -> -1   （在范围外，索引为负）
```

## 与 `section_pos` 的关系

`getMinSectionY()` 调用 `SectionPos.blockToSectionCoord(minY)`，也就是上一能力
[`section_pos`](./section_pos.md) 里的 `>> 4` 向下取整。**本能力复用了那一语义**：

- `minY = -64` → `-64 >> 4 = -4`（若是向零截断会得到 `-4` 也对，但 `-1 >> 4` 会不同）
- `minY = -5` → `-5 >> 4 = -1`（向零截断会得到 `0`，差一个分区）

`range|-5|100` 的 `minSectionY = -1` 正是这条语义的直接体现。

## 未覆盖范围

- `isOutsideBuildHeight(BlockPos)` 的重载（只取 `getY()`，与整数版等价）
- 各维度实际配置值的来源（那是 `dimension_type` 的职责）
- `getSectionsCount` 在 `LevelChunkSection` 数组分配中的使用点