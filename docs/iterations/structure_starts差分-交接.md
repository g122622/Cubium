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
