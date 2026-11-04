# `structure_starts` 差分现状与排查记录

日期：2026-11-04。当前状态：**47 处差异 / 540 个单元**，未收敛。

## 能力测什么

`structure_starts` 比对「STRUCTURE_STARTS 阶段产物」：给定种子与区块坐标，
两侧各自算出该区块的结构起点（start）列表，逐字段比对包围盒、piece 列表、
rotation、template、projection、junctions、groundLevelDelta。

- 探针：`tests/diff/probes/structure_starts.cpp`
- 原版：`tests/diff/java/dev/horizonlab/diff/StructureStartsOracle.java`
- 编排：`tests/diff/harness/harnesses/structure_starts.ts`

刻意设计：候选区块由**原版**给出（`beforeUnits` 里跑 oracle 的 `candidates` 子命令），
而不是由探针导出。这样「本项目的放置规则是否与原版一致」才能被暴露——
若让探针自己列单元，放置偏差会被单元选取本身掩盖。

## 差异分类（本轮实测）

| 类别 | 数量 | 特征 |
|---|---|---|
| 要塞缺失 | 8 | 原版有 `minecraft:stronghold` 起点，Cubium 返回空数组 |
| 试炼密室模板不同 | 5 | 首个 piece 的 `template` 不同，如原版 `trial_chambers/corridor/end_2` vs Cubium `end_1` |
| 双方 template 均大量为空 | 31 | 见下节 |
| 其它 | 3 | `overworld/0/36/103`、`overworld/9248738493917/-69/-67`、`overworld/9248738493917/13/-52` |

另有 2 处原版出现「退化单点 start」（`box` 六值中 min==max），疑似跨区块结构被
错误归入当前区块，占比很小不是主因。

## 已查明的事实

### 1. template 为空不是「模板没加载」

在差异项范围内统计：

- 原版 4752 个 piece，其中 3303 个 `template` 为空（69.5%）
- Cubium 1646 个 piece，其中 426 个为空（25.9%）

**两侧都有大量空 template**，所以这不是某一侧的资源加载失败，而是这些 piece
本来就来自不含 `StructureTemplatePoolElement` 的池（如 `ListPoolElement`、
`FeaturePoolElement`），它们的 `template` 字段本就为空。

数据包侧确认完整：`data/minecraft/structure` 下有 1202 个模板文件。

### 2. 试炼密室的首个 piece 不同 → 随机序列消费顺序不一致

原版 `trial_chambers/corridor/end_2` vs Cubium `end_1`。同一池的两个候选被
选中不同项，说明在挑选 piece 之前消费的随机数个数不同。这类偏差会连锁放大——
一个 piece 选错，后续所有 piece 的位置、rotation、junctions 全部错位，
所以 37 处「都有但内容不同」大概率同源。

### 3. 要塞完全缺失

8 处原版有 stronghold 起点而 Cubium 为空。要塞由 `ConcentricRingsStructurePlacement`
按同心环规则放置，与普通随机散布的 structure set 走不同路径。Cubium 侧可能是
该 placement 类型未实现或环计算参数不同。

## 建议的收敛顺序

1. **先查要塞**：8 处，现象最干净（有/无），且 `ConcentricRingsStructurePlacement`
   是独立算法，与随机序列问题正交。先修它能把差异从 47 降到 39。
2. **再查随机序列消费顺序**：对比同一 `trial_chambers` 池在挑选前两侧的
   `RandomSource` 状态与已消费次数。可以用一个最小的池（只有一个两元素列表）
   构造对照，定位第一处分歧。
3. **最后处理退化单点**：确认那 2 处是否真的属于当前区块，可能是探针的
   区块归属判定问题。

## 未做的事

本轮只做了分类与成因排查，**没有修改任何生产代码**。原因是 47 处差异集中在
世界生成的随机序列层面，盲改风险远大于收益。需要按上面第 2 步的做法先定位到
第一处随机数分歧点再动手。

## 后续深入排查（第二轮，重要更新）

上一节把 47 处差异归为四类。进一步排查后，**要塞问题的根因已定位到具体代码位置**。

### 要塞：piece 数塌缩为 1

按结构类型统计两侧的 start 与平均 piece 数：

| 结构 | Cubium | 原版 |
|---|---|---|
| mineshaft | 14 个 start，平均 16.8 pieces | — |
| trial_chambers | 5 个 start，平均 **225.8** | — |
| ancient_city | 1 个 start，平均 **98.0** | — |
| village_plains | 1 个 start，平均 **112.0** | — |
| end_city | 6 个 start，平均 24.8 | — |
| **stronghold** | **2 个 start，平均 1.0** | **8 个 start，平均 143.4** |

**除要塞外所有结构的拼图组装都正常**。要塞是唯一异常项：Cubium 侧只有 2 个
要塞起点，且每个都只有 1 个 piece（5×10×4 的小盒）；原版是 97×48×64、上百 piece。

### 已排除的假设

1. **不是模板池缺失**。`minecraft:stronghold` 的 `type` 是专用类型
   `minecraft:stronghold`，不是 `minecraft:jigsaw`，所以它不使用模板池，
   piece 由 `StrongholdStructure` 类程序化生成。数据包中确实没有
   `worldgen/template_pool/stronghold/` 目录，但这符合预期。
2. **不是方向枚举错位**。`StrongholdStartStairs` 用
   `static_cast<Direction>(rng.nextInt(4) + 2)` 取方向，值域 2..5；Cubium 的
   `Direction` 枚举 Down=0/Up=1/North=2/South=3/West=4/East=5，与原版 id 一一对应，
   落在水平方向上，不会走进 `_generateCorridor` 的 `default: return;`。
3. **不是随机流 fork 问题**。`StructurePlacementState` 已用
   `generateRingLayout` 同一条流取 fork 种子，生物群系搜索（半径 112、
   8 参数重载、步长 1、findClosest=false）也按原版实现。

### 下一步该看的位置

`src/server/world/gen/structure/structures/StrongholdStructure.cpp`：

- `_generateCorridor`（约 L155 起）：递归入口。`depth > 50 || pieces.size() > 100`
  是防无限递归的截断，但首轮就不产出第二个 piece，说明在更早的地方返回了。
  需要确认 `getCoordBaseMode()` 的返回值、`minX/maxX/minY/maxZ` 是否被正确设置
  （`StrongholdStartStairs` 构造时 box 是 `x..x+4`、`64..74`、`z..z+4`）。
- `canSpawnMoreStructuresOfType(depth)`：若权重表在当前 depth 上全部返回 false，
  `totalWeight == 0`，会直接生成传送门房间并 return。原版此时也会这样做，
  但原版在此之前已经递归生成了大量走廊。**怀疑权重表的 `instancesLimit` 或
  `groupCount` 初值与原版不同**。
- `initializeStrongholdPieceWeights`：对照原版 `StrongholdPieces` 的静态权重表
  （`PieceWeight(Class, weight, limit)` 列表，含 `STRAIGHT`、`PRISON_HALL`、
  `LIBRARY` 等，以及 `allowedDepth` 分组）。

### 要塞缺失（8 处）与 piece 塌缩是同一个问题

原版在这些区块有要塞、Cubium 返回空数组。由于要塞起点数量在 Cubium 侧只有 2 个
（原版候选里至少 8 个），这不是「有起点但被丢弃」，而是**环位置或结构集归属不同**，
使得 Cubium 根本没为这些区块判定过要塞。修完 piece 塌缩后需要重新测这一项。

## 第三轮：`canGenerate` 是死代码

顺着上一节的线索继续查，发现一处**结构性不一致**：

`StrongholdStructure::canGenerate`（`StrongholdStructure.cpp` L89-111）用的是**过时的旧算法**：

```cpp
for (i32 index = 0; index < 65; ++index) {          // 硬编码 65 个
    const auto [cx, cz] = calculateStrongholdPos(index, worldSeed);
    if (cx == chunkX && cz == chunkZ) return true;
}
```

而 `calculateStrongholdPos` 用硬编码的 8 环表 `RING_COUNTS = {3,6,10,15,21,28,36,9}`。
数据包配置却是 `count: 128, distance: 32, spread: 3`。代码注释里自己写着
`TODO 当前实现仍未完成 Java 版 biome locate 校正`。

**但这个方法是死代码**：全仓搜索 `->canGenerate(` / `.canGenerate(` 零命中。
实际生成链是

```
generateStructureStarts
  -> _generateStructureStartsForSet    // 先 placementState.isStructureChunk(...) 筛选
      -> _selectStructureStart         // 轮盘抽签 + 群系校验
          -> structure->generate(...)  // 直接生成，不经过 canGenerate
```

所以要塞缺失与 piece 塌缩**都不是 `canGenerate` 造成的**。

### 应当删除 `canGenerate`

它保留了旧算法与 TODO，但从未被调用。另外各结构类都有同名方法（BastionRemnant、
BuriedTreasure、DesertPyramid、EndCity、Fortress、Igloo…），全部都是死代码。
删掉它们不影响行为，但能消除「这里似乎还在用旧算法」的误导。

建议作为独立的小改动提交，与本差分收敛解耦。

### 剩下一处可疑点

`_selectStructureStart` 传给 `structure->generate()` 的 `structureRng` 是
`setLargeFeatureSeed(seed, chunkX, chunkZ)` 之后的状态；而
`StrongholdStructure::generate` 里第一件事就是

```cpp
i32 startY = m_config.minY + rng.nextInt(m_config.maxY - m_config.minY);
```

即**先消耗一个随机数决定起始 Y**，再调 `_generateStrongholdPieces`。
原版 `StrongholdStructure.addPieces` 里并没有这个「随机高度」步骤——它直接
用一个固定的起始 Y。若原版不消耗这个 `nextInt`，后续所有 piece 的位置、
旋转、选择都会整体错一位，正好能解释「只有 1 个 piece 且盒子很小」。

这是下一轮应当首先验证的点：把 `startY` 改成不消耗随机数的固定值，
重跑 `structure_starts` 看要塞 piece 数是否恢复。
