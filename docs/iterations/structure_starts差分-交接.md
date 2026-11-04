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

## 第四轮：找到要塞生成的结构性差异（关键）

上一节猜测「`generate` 里的 `rng.nextInt` 消耗了随机数」。对照原版后确认，
问题比这严重得多：**两侧的生成算法结构完全不同**。

原版 `StrongholdStructure.generatePieces`：

```java
int i = 0;
do {
    builder.clear();
    context.random().setLargeFeatureSeed(context.seed() + i++, chunkPos.x, chunkPos.z);
    StrongholdPieces.resetPieces();
    StartPiece start = new StartPiece(random, chunkPos.getBlockX(2), chunkPos.getBlockZ(2));
    builder.addPiece(start);
    start.addChildren(start, builder, random);
    List<StructurePiece> pending = start.pendingChildren;
    while (!pending.isEmpty()) {                          // 待处理队列
        int j = random.nextInt(pending.size());
        pending.remove(j).addChildren(start, builder, random);
    }
    builder.moveBelowSeaLevel(seaLevel, minY, random, 10);
} while (builder.isEmpty() || start.portalRoomPiece == null);
```

Cubium `StrongholdStructure::generate`：

```cpp
i32 startX = chunkX * world::CHUNK_WIDTH + 8;
i32 startZ = chunkZ * world::CHUNK_WIDTH + 8;
i32 startY = m_config.minY + rng.nextInt(m_config.maxY - m_config.minY);  // 原版没有
BlockPos startPos(startX, startY, startZ);
_generateStrongholdPieces(rng, startPos, start->pieces());                // 递归
start->recalculateStructureSize();                                         // 无 moveBelowSeaLevel
```

逐条对照：

| 方面 | 原版 | Cubium | 影响 |
|---|---|---|---|
| 起始块位置 | `getBlockX(2)` / `getBlockZ(2)`，即区块内偏移 **2** | `chunkX*16 + 8`，偏移 **8** | 整体平移 6 格 |
| 随机种子 | `setLargeFeatureSeed(seed + i++, ...)`，**i 是重试计数** | 直接使用调用方传入且未重新播种的 rng | 随机序列完全错位 |
| 起始高度 | 无随机高度 | `rng.nextInt(minY..maxY)` **先消耗一个随机数** | 后续选择整体错一位 |
| 组装方式 | **待处理队列 + 随机取下标移除** | 递归 `_generateCorridor` | 生成顺序与 piece 集合不同 |
| 收尾 | `moveBelowSeaLevel(sea, minY, random, 10)` | `recalculateStructureSize()` | 高度不对 |
| 终止条件 | `do-while` 重试直到 **有传送门房间** | 单次递归，可能一个都没有 | 塌缩成 1 piece |

这几条叠加起来，正好解释实测现象：Cubium 侧要塞只有 2 个起点、每个仅 1 个
piece（5×10×4），而原版 8 个起点、平均 143 个 piece（97×48×64）。

### 这不是小修，需要重写

正确做法是按原版重写 `StrongholdStructure::generate` 与
`StrongholdPieces::StartPiece::addChildren`：

1. 起始位置改成 `chunkX * 16 + 2` / `chunkZ * 16 + 2`。
2. 引入重试计数 `i`，每轮 `rng.setLargeFeatureSeed(seed + i, chunkX, chunkZ)`。
3. 去掉随机起始 Y。
4. 把递归走廊改成「待处理队列 + `nextInt(pending.size())` 随机取下标移除」。
5. 收尾接 `moveBelowSeaLevel(seaLevel, minY, rng, 10)`。
6. 外层 `do-while (pieces.empty() || portalRoomPiece == null)`。

工作量集中在 `StrongholdPieces.cpp`（约 1600 行，含全部 piece 类型的
`createPiece` / `addChildren`），属于独立的一次重构。建议单独开一轮做，
并与差分能力 `structure_starts` 配合逐步验证。

### 顺带确认

`StrongholdStructure::canGenerate` 及其在其它结构类（BastionRemnant、
BuriedTreasure、DesertPyramid、EndCity、Fortress、Igloo 等）中的同名方法
**全部没有调用点**，是死代码，应当删除以消除误导。

## 第五轮：找到并修复 createBox 的朝向分支写反（重大）

上一轮记录的要塞塌缩，根因在 `StructureBoundingBox::createBox`。

### 缺陷

`src/common/world/gen/structure/StructureBoundingBox.cpp` 的 North 与 South 分支实现互换了：

| 方向 | 应有的深度展开 | 原实现 |
|---|---|---|
| South（含 default） | `z+offZ .. z+offZ+sizeZ-1`（沿 +Z） | `z-sizeZ-offZ+1 .. z-offZ`（沿 -Z） |
| North | `z-sizeZ+1+offZ .. z+offZ`（沿 -Z） | `z+offZ .. z+offZ+sizeZ-1`（沿 +Z） |

West/East 分支是对的。原版语义见 `BoundingBox.orientBox` 的 NORTH / SOUTH 分支。

### 后果

所有按朝向计算包围盒的结构构件都落在门的另一侧，于是同一结构内相邻构件的包围盒互相重叠，
`findIntersecting` 判相交，后续构件全部创建失败。

要塞受害最明显——`StrongholdStraight::createPiece` 用
`createBox(x, y, z, -1, -1, 0, 5, 5, 7, direction)`，朝北时构件被放到了门的错误一侧，
第二个构件就与起始楼梯盒重叠：

- 修复前：要塞 1 个 piece（只有起始楼梯）
- 修复后：要塞 52 个 piece，递归深度正常展开

### 配套修正

1. `StrongholdStructure::_generateCorridor` 的第一个子构件落点：原先用
   「包围盒中点 `minX + (maxX-minX)/2`」并把朝向改写成 North；原版
   `generateSmallDoorChildForward(start, builder, random, 1, 1)` 用
   `minX+1 / minY+1 / minZ-1` 且沿用构件自身朝向。横向偏 1 格 + 朝向错位，
   使新盒必然与起始楼梯盒在 X 方向重叠。

2. `_generateStrongholdPieces` 改成队列驱动：原版把分支压进
   `StartPiece.pendingChildren`，由 `while (!pending.isEmpty())` 逐个取出继续扩展；
   一条分支被挡住不影响其它分支。现在把 `pieces` 本身当队列，逐个对尚未扩展的
   构件调用 `buildComponent`，等价于原版且无需改动 StrongholdPieces 的组装逻辑。

### 回归

structure_orient 14/14、structure_density 1/1、bounding_box 29/29、
bounding_box_encap 18/18、block_support 1166/1166、piston_movement 523/523 全部通过。

## 仍未收敛的部分

`structure_starts` 仍是 47 处，但构成变了：

| 类别 | 数量 | 说明 |
|---|---|---|
| 要塞缺失 | 8 | 环位置或候选区块判定与原因不同 |
| 都有但内容不同 | 37 | **废弃矿井**（template 全空、程序化生成） |
| 原版空 / Cubium 有 | 2 | |

矿井同样靠 `addChildren` + 队列组装。查过：
- `MAX_DEPTH = 8` 两侧一致；
- 距离限制 `<= 80` 两侧一致（原版 `MineshaftPieces.java:91`）；
- `MineshaftRoom::buildComponent` 里 `depth = genDepth() + 1` 也已正确递增，
  `maxDepth` 参数实际未被使用。

所以矿井的 159 vs 31 差距不在这几处，需要进一步对比 `MineshaftRoom.addChildren`
的原版实现（本轮尚未展开）。下一轮应从那里入手。

## 第六轮：求值顺序缺陷与恶地分支

### 已修复：C++ 实参求值顺序未指定

原版房间构造 `new BoundingBox(x, 50, z, x+7+nextInt(6), 54+nextInt(6), z+7+nextInt(6))`
在 Java 里保证从左到右求值。C++ 的实参求值顺序**未指定**（clang-format 还会把三个
`nextInt(6)` 压到同一行），于是三个跨度按任意顺序抽取。

实测：原版房间 `11x6x13`，Cubium `13x8x11`，三个值都不同。

已改为 `drawRoomSpans()` 按 X -> Y -> Z 显式抽取后经委托构造传入。

### 已修复：缺少恶地（MESA）分支

原版 `generatePiecesAndAdjust` 分两条路径：

```java
if (type == MESA) {
    center = builder.getBoundingBox().getCenter();
    j = getBaseHeight(center.x, center.z, WORLD_SURFACE_WG, ...);
    k = j <= seaLevel ? seaLevel : Mth.randomBetweenInclusive(random, seaLevel, j);
    builder.offsetPiecesVertically(k - center.y);
} else {
    builder.moveBelowSeaLevel(seaLevel, minY, random, 10);
}
```

原实现无条件走 `moveBelowSeaLevel`，恶地矿井被错误压到海平面以下。现已补上 MESA 分支。

### 第六轮逐个排除的怀疑点（都无问题）

| 怀疑点 | 结论 |
|---|---|
| `moveBelowSeaLevel` 的 `topY` 基准 | 与 `getYSpan() + minY + 1` 等价，正确 |
| `seaLevel` / `minY` / `padding` 参数 | 63 / -64 / 10，与原版一致 |
| `canGenerate` 消耗随机数 | 全仓零调用点，是死代码，不影响 |
| `structureRng` 的 `setLargeFeatureSeed` | 逐行对照 `WorldgenRandom.setLargeFeatureSeed`，完全一致 |
| `structureRng` 用的区块坐标 | 当前区块，与原版 `makeRandom(seed, chunkPos)` 一致 |
| `JavaLegacyRandom::nextInt(bound)` | 2 的幂走高位、非 2 的幂走拒绝采样，与原版一致 |
| 房间起点 X | 已与原版一致（如 `overworld/0/-13/-60` 两侧都是 `-206`） |

### 当前阻塞点

房间三个跨度仍与原版不同（原版 `11x6x13` / Cubium `13x8x11` 或 `12x5x12`），
但上面列出的所有随机数入口都已核对一致。

**下一步建议**：在 `MineshaftStructure::generate` 开头和 `drawRoomSpans` 之后各打一次
`rng.nextLong()` 快照（诊断代码，不入库），与 oracle 侧同样位置的快照比对，
直接看出序列是从第几个数开始分歧的。这比继续逐个怀疑点排查快得多。

## 第七、八轮：矿井组装的深挖

### 已修复（第七轮）

**`findGenerationPoint` 开头被丢弃的 `nextDouble()`**

```java
p_227964_.random().nextDouble();   // 返回值丢弃，只推进随机序列
```

漏掉它会让整座矿井的随机序列错一位。补上之后房间尺寸逐位对齐：

| 单元 | 原版首个构件 | Cubium 首个构件 |
|---|---|---|
| (-30,14) | `12x10x11` | `12x10x11` |
| (-13,-60) | `11x6x13` | `11x6x13` |

### 已修复（第八轮）

**交叉点的分支方式是错的**

原版 `MineShaftCrossing.addChildren` 按朝向查一张固定分支表，每方向开三个口，
坐标由包围盒直接算出，**不抽随机数**。原实现遍历四方向、每个方向以 `nextInt(3)`
决定是否跳过，既不符合分支表，又多吃了最多四个随机数。

**缺少双层交叉点分支**

`isTwoFloored = getYSpan() > 3`。双层交叉点在主分支后还要在 `minY + 4` 层再开三个方向，
每个由一次 `nextBoolean()` 决定。原实现完全没有这段。

**交叉点包围盒与楼梯下降量**

- 交叉点原版 `(-1,0,-4)..(3,i,0)` 等，高度 `i = nextInt(4)==0 ? 6 : 2`；原实现没有那次 `nextInt(4)`。
- 楼梯原版下降 5 格，原实现用 `length/2 = 4`。

### 当前实测状态

| 单元 | 原版 | Cubium |
|---|---|---|
| (-30,14) | 125 个 piece，Y=`20..42`，整体 `144x23x116` | 266 个，Y=`2..35`，`118x34x194` |
| (-13,-60) | 159 个，Y=`-25..-4`，整体 `157x22x174` | 152 个，Y=`29..49`，`128x21x173` |

第二个单元的结构已经相当接近：piece 数 152 vs 159、Z 跨度 173 vs 174。
**但 Y 整体偏 54 格**。

### 已排除的怀疑点（都正确）

- 走廊构造的两处消耗（含 `!hasRails &&` 的短路语义）
- `findCorridorSize` 的退避重试长度（`i*5-1`，与原版 `j-1` 一致）
- `moveBelowSeaLevel` 的参数与公式
- `addMineshaftPiece` 的深度逐层递增与 8 层截断
- 距离限制 `<= 80`

### 剩余问题的性质

Y 偏 54 格的原因不是公式错，而是 `moveBelowSeaLevel` 里那次 `nextInt` 取到的值不同，
根子仍是「`moveBelowSeaLevel` 之前消耗的随机数个数不同」——也就是 piece 数与生成顺序的差异。

piece 数 152 vs 159、266 vs 125 说明分支展开仍有出入。要彻底对齐，需要把
「每个 piece 的类型、坐标、朝向」逐个与 oracle 比对，找出第一个分歧点，
再回溯到产生它的那次随机数消耗。这比继续读源码可靠，但需要写一个比对脚本。

### 第七轮的一个教训

调试时曾看到「两个 long 值顺序颠倒」，误判为随机源起点错误。实际是**诊断代码自己
把两个 `rng.nextLong()` 写在同一个 `fprintf` 实参里**，而 C++ 实参求值顺序未指定。
凡是随机数抽取顺序有意义的地方，必须逐个语句取值。

---

## 第九、十轮：构件级差分能力与随机序列定位

### 新增能力 `mineshaft_pieces`

把矿井拆到构件级（顺序、类型、包围盒、朝向），用来把「整座矿井对不上」定位到
「第几个构件开始分歧」。见 `tests/diff/README.md` 的状态表。

### 修掉的三个根本缺陷

1. **构件登记顺序倒置**：`addMineshaftPiece` 先展开子构件、调用方随后才 push，
   导致子构件排在父构件之前。原版是 `addPiece` 在先、`addChildren` 在后。
2. **房间与走廊的组装逻辑张冠李戴**：四边循环是走廊的逻辑，房间实际是
   「`nextInt(4)` 选主口 + 沿一边逐点 `nextInt(5)`」。两处都错位了。
3. **走廊深度不递增导致栈溢出**（`0xC0000005`）：`depth = genDepth()` 不增长，
   `depth > maxDepth` 永不成立。

修复后 `[1]` 与原版 XZ 逐位一致。

### 一个反直觉的坑

原版房间 `addChildren` 里写的是：

```java
if (direction != Direction.NORTH && direction != Direction.SOUTH) {  // 沿 X 扫描
    ...
} else {                                                             // 沿 Z 扫描
```

房间的 `direction` 是 **`null`**。Java 里 `null != NORTH` 为 **true**，
所以房间**恒走沿 X 扫描**，不是落到 else。直觉上会以为「没有朝向 = 默认分支」。

oracle 侧的实测证据：`[0] room orient=null`、`[1] corridor orient=NORTH`。

### 尚未查清的一处

原版源码 L165-168：

```java
int i = this.getGenDepth();
int j = random.nextInt(4);        // ← 字面上在朝向判定之前，应无条件执行
Direction direction = this.getOrientation();
if (direction != null) { ... }
```

但实测结论相反：把这次 `nextInt(4)` 补进来之后，房间第 2 段那次 `nextInt(5)`
从 `0` 变成 `2`，北侧出口随之消失，与 oracle 的 `[1] corridor NORTH (-203,…)` 对不上；
去掉它则完全吻合。

那一次 `nextInt(4)` 到底在什么条件下执行，**尚未查清**。补之前务必先跑
`mineshaft_pieces` 确认房间第 2 段的 `nextInt(5)` 仍为 `0`。

### 当前状态

| 单元 | Cubium | 原版 |
|---|---|---|
| (-13,-60) 构件数 | 140 | 159 |
| (-13,-60) [1] | `corridor` XZ 一致 | `corridor` |
| (-13,-60) [2] | `stairs` `(-212,-33,-968)` | `corridor WEST` `(-218,-15,-973)` |

第 2 个构件起类型分歧。oracle 侧 `[1] hasRails=true`（即构造时 `nextInt(3)==0`），
Cubium 侧同一位置抽到 `1`——说明 `[1]` 构造之前仍有次数差，这是下一步的入口。

## 第十一轮：随机源复算与一处矛盾

### 做了什么

写了一个独立的 Java 复算工具（`tests/diff/tools/MsTrace.java`），按原版调用顺序
从 `setLargeFeatureSeed` 起复算 `overworld/0/-13/-60` 的随机序列：

```
nextDouble
room nextInt(6)x3   = 3,1,5      → 房间 X 跨度 11、Z 跨度 13，与原版包围盒吻合
room nextInt(5)     = 0          → 北侧开口，与原版 [1] 朝北吻合
create nextInt(100) = 57
corridorSize        = 3          → 跨度 14，与原版 [1] 的 Z 跨度 15 吻合
corridorCtor(3)     = 1          → rails = false
corridorCtor(23)    = 6
```

**这份复算结果与 Cubium 的实测逐条一致。**

### 但 oracle 给出相反的值

oracle 实测 `[1] hasRails = true`（即构造时 `nextInt(3)==0`），并因此短路、
**不抽** `nextInt(23)`。Cubium 抽了 `nextInt(23)`，后续整条序列偏移一位，
这正是 `[2]` 类型与位置都不同的根因。

### 试过的变体都不成立

| 变体 | `room nextInt(6)x3` | 房间跨度 | 与原版包围盒 |
|---|---|---|---|
| 有 `nextDouble`（默认） | `3,1,5` | 11×13 | 吻合 |
| 无 `nextDouble` | `5,3,3` | 12×11 | **不吻合** |
| 加房间主口 `nextInt(4)` | `3,1,5`，`nextInt(5)=2` | — | **不开北口，不吻合** |

所以 `nextDouble` 确实被抽了，房间主口那次 `nextInt(4)` 确实没被抽。
在这两个约束下，复算与 Cubium 都给出 `rails=false`，而 oracle 给出 `true`——
**这一处矛盾尚未解决**。

### 已排除

- `LegacyRandomSource` 的算法差异：差分能力 `legacy_random` 已覆盖 `nextInt(bound)`
  的拒绝采样语义并完全通过，两侧逐位一致。
- `setLargeFeatureSeed` 的参数：房间 Z 起点 `-958 = minBlockZ(-960) + 7 + spanZ(5)`
  与复算的 `spanZ=5` 吻合，说明种子坐标没传错。

### 结论

下一次排查应当**直接在 oracle 运行期观察**（例如临时包装 `GenerationContext` 使用的
`WorldgenRandom` 并记录每次 `nextInt` 的实参与返回值），而不是继续靠复算与源码推理。
复算工具先保留在 `tests/diff/tools/`，并在其 README 里写明这个已知矛盾，
避免后来者拿它当权威结论。

## 第十二轮：移除深度提前返回，[8][9] 逐位对齐

### 修复

四个矿井类的 `buildComponent` 开头都有一句多余的

```cpp
const i32 depth = genDepth() + 1;
if (depth > maxDepth) { return; }
```

原版没有这层返回。深度为 8 的构件算出 `depth = 9`，`9 > 8` 成立，整个
`addChildren` 被跳过——**连主口段都不执行**。原版的截断只发生在
`generateAndAddPiece` 的 `p_227714_ > 8`（比较父深度）与走廊第 2 段的 `if (i < 8)`。

驱动程序的实证：原版 `ADD[9] depth=9` 是 `ADD[8] depth=8` 开出来的构件。
去掉这层返回后：

| | 修复前 | 修复后 | 原版 |
|---|---|---|---|
| 构件数 | 160 | 184 | 159 |
| [8] | XZ 一致 | **含 Y 逐位一致** | — |
| [9] | 缺失 | **含 Y 逐位一致** | — |

### 探针加了 `genDepth`

此前探针只输出类型与包围盒，**看不出某个构件有没有继续展开**，而这次的分歧正是深度造成的。
加上 `genDepth` 后一眼就能看到 `[9]` 在 Cubium 里根本不存在。

### 下一处：`[10]` 包围盒对、深度差 1

```
Cubium [10] depth=8  (-260,21,-1028)-(-258,23,-1009)
原版   [10] depth=9  (-260,51,-1028)-(-258,53,-1009)
```

坐标与类型都对，只有深度差 1 —— 说明两侧认定的**父构件**不同：
Cubium 的父构件深度是 7，原版的父构件深度是 8（即 `[8]`）。

### 已试过但不成立的假设

- **把走廊第 2 段判据从 `depth - 1 < 8` 放宽到 `< 9`**：`[10]` 的深度变成 9，
  但坐标跑到 `(-283,16,-1020)`，与原版的 `(-260,-1028)` 完全不同，构件数 179。
  说明第 2 段确实执行了，但此前已有一处抽取偏移，`nextInt(5)` 抽到西侧而不是东侧。
- **`[8]` 是房间**：`[8]` 的第一次抽取是 `nextInt(3)`，看起来像房间四边循环的
  `nextInt(xSpan)`（`xSpan=3`）。但接着是 `nextInt(4)`，与房间的 `nextInt(k)`（`k=1`）
  不符，因此不成立。

### 未完成的一步

驱动程序的 owner 标记（判断每次抽取属于哪个构件）没写对，输出里 owner 全是 -1，
所以没能把 `#50~#61` 那一段归属到具体构件。**这是下一步该补的第一件事**——
补好之后 [10] 的父构件归属可以直接看出来，不必再靠假设。

## 第十三轮：用父子关系定位到交叉点的双层分支

### 方法

给驱动程序补上**父构件推断**：按 `genDepth` 维护「每个深度最近一次登记的构件」，
新构件深度为 `d` 时父构件就是 `latestAtDepth[d-1]`。原版输出因此变成：

```
PIECE 28 depth=6 parent=…   (-192,51,-914)-(-188,57,-910)   ← crossing
PIECE 37 depth=7 parent=28  (-207,55,-913)-(-193,57,-911)
PIECE 38 depth=8 parent=37  (-227,56,-913)-(-208,58,-911)
PIECE 39 depth=9 parent=38  (-227,55,-923)-(-225,57,-914)
PIECE 40 depth=9 parent=38  (-204,55,-933)-(-202,57,-914)
PIECE 41 depth=7 parent=28  (-191,55,-909)-(-189,57,-890)
```

### 发现

`[28]` 是交叉点，**原版它有 4 个子构件，Cubium 只有 3 个**。

`[41]` 的坐标 `(-191,55,-909)`：
- `x = -191 = [28].minX + 1`
- `y = 55 = [28].minY + 4`（`[28].minY = 51`）
- `z = -909 = [28].minZ - 1`（`[28].minZ = -910`）

**`y` 加了 4，正是 `isTwoFloored` 分支的 `minY + 3 + 1`。**

也就是说 `[41]` 是交叉点双层分支的**北向**产物。Cubium 的这段实现在代码上一致
（`ySpan > 3`、三次 `nextBoolean()`、`minY + 4`），但三次 `nextBoolean()` 的命中结果不同——
说明在它之前已经有抽取偏移。

### 当前的收敛进度

| 指标 | 数值 |
|---|---|
| 从头连续一致的构件数 | **41 / 159** |
| Cubium 构件数 | 76 |
| 深度 1~9 逐层数量 | 都少于原版，缺口在中后段 |

分歧点是「第 41 个构件」，比起初的「第 1 个构件」已经推进了 40 个。每一轮修复
（房间/走廊逻辑归属、深度语义、深度提前返回、第 2 段递进、楼梯落点）都把分歧点往后推，
但还没到能整除的程度。

### 下一步

`[28]` 之前的分支必定有一处抽取次数不同，才会让三次 `nextBoolean()` 结果不同。
下一步应当**从 `[28]` 开始单独追踪**：把驱动程序的日志按父构件分段，
只看 `[28]` 的 `addChildren` 那一段的抽取序列，与 Cubium 对应的抽取逐条对齐。

## 第十四轮：把分歧缩到交叉点的三次 nextBoolean

### 实测数据

**Cubium 侧**（临时加打点，已清理）：

```
[cross] (-192,51,-914)-(-188,57,-910) dir=0 depth=6 twoFloor=1
  bool[0]=0    北向：不生成
  bool[1]=1    西向：生成
  bool[2]=0    东向：不生成
```

**原版侧**（`[41]` 的坐标反推）：

```
[41] (-191,55,-909)-(-189,57,-890)
     x = [28].minX + 1    北向分支
     y = [28].minY + 4    isTwoFloored 的 minY + 3 + 1
     z = [28].minZ - 1    北向分支
```

也就是说**原版 `bool[0] = 1`，Cubium 是 0**。三次 `nextBoolean()` 的第一次就不同。

### 交叉点结构本身是对的

Cubium 的 `[28]` 判为 `twoFloor=1` 与原版一致（`ySpan = 7 > 3`），
坐标、深度、朝向都与原版逐位一致；3 个主口产生的子构件 `[29]`、`[37]` 也一致。
差异只在 `isTwoFloored` 段那三次 `nextBoolean()` 的取值。

### 阻塞点

`nextBoolean()` 不抽 `nextInt`，所以它前面必然有一次别的抽取次数不同。
原版的抽取日志里，`[28]` 之后能看到的只有 `nextInt(100)=78`（创建 `[29]`），
紧接 `nextInt(100)=45`、`nextInt(3)`、`nextInt(3)`、`nextInt(23)`——
但 `[29]` 是楼梯，楼梯构造不抽随机数，因此这几条实际属于 `[28]` 的第 2、3 个主口。

**问题在于：`nextBoolean()` 的调用没有出现在这段日志里。** 我在驱动程序的
`Tracing` 里覆写了 `nextBoolean()`，但输出里 `owner=28/29` 的行中一条 `nextBoolean` 都没有。
这说明覆写没有生效（`RandomSource.nextBoolean()` 是接口默认方法，`LegacyRandomSource`
可能不经过它，或者 `WorldgenRandom` 有更具体的重写）。

**这是下一步必须先解决的工具问题**：把 `nextBoolean` 的追踪做对，
才能看到原版那三次的取值，进而定位它们之前哪一次抽取多了或少了。
