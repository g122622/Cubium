# 树干放置器 (Trunk Placers)

树干放置器负责生成树木的树干部分，返回树叶附着点列表供树叶放置器使用。

## 目录结构

```
trunk/
├── TrunkPlacer.hpp/cpp             # 树干放置器基类（定义 FoliagePosition、placeTrunk 接口）
├── BendingTrunkPlacer.hpp/cpp      # 弯曲树干（杜鹃树）
├── StraightTrunkPlacer.hpp/cpp     # 直树干（橡树/白桦/云杉/丛林木）
├── CherryTrunkPlacer.hpp/cpp       # 樱花树干
├── TrunkPlacers.hpp/cpp            # 聚合头文件（深色橡树/精美/金合欢/巨型云杉/巨型丛林木）
└── README.md
```

## 内部模块关系

```
TrunkPlacer（基类）
    ├── BendingTrunkPlacer      弯曲树干，先垂直后水平延伸
    ├── StraightTrunkPlacer     纯垂直树干
    ├── CherryTrunkPlacer       樱花树干，带弯曲分支
    ├── DarkOakTrunkPlacer      深色橡树，2x2 粗干带弯曲
    ├── FancyTrunkPlacer        精美橡树，弯曲分支
    ├── ForkyTrunkPlacer        金合欢，分叉树干
    ├── GiantTrunkPlacer        巨型云杉，2x2 粗干
    └── MegaJungleTrunkPlacer   巨型丛林木，2x2 粗干带分支
```

**生成流程**：`TreeFeature::place()` → `TrunkPlacer::placeTrunk()` 返回 `FoliagePosition` 列表 → `FoliagePlacer::placeFoliage()` 生成树叶

## 上下游外部依赖关系

### 上游依赖

| 模块 | 用途 |
|------|------|
| `FeatureSpread` | 树叶放置器的半径/偏移配置 |
| `IntProvider`（valueprovider） | BendingTrunkPlacer 的弯曲长度随机化 |
| `WorldGenRegion` | 区块区域访问 |
| `VanillaBlocks` | 原版方块常量 |
| `math::Random` | 随机数生成器 |

### 下游依赖

- `TreeFeature` — 创建树干放置器实例（由数据包 configured_feature JSON 驱动）
- `RootSystemFeature`（杜鹃树）— 使用 BendingTrunkPlacer
- `FoliagePlacer` — 消费 placeTrunk() 返回的 FoliagePosition

## 容易踩的坑

### 1. IntProvider 与 FeatureSpread 的区别

`IntProvider` 是多态的整数值提供器（堆分配、虚函数分派），而 `FeatureSpread` 是值类型（栈分配、无虚函数）。BendingTrunkPlacer 是唯一使用 `IntProvider` 的树干放置器，`placeTrunk()` 接收 `math::Random&` 可隐式转为 `math::IRandom&` 传入 `IntProvider::sample()`。

### 2. BendingTrunkPlacer 水平弯曲高度

垂直循环结束后光标位于 `startPos.y + height`（比最后放置的垂直方块高一格），水平弯曲方块位于该高度。这是 MC 原版行为，不是 bug。

### 3. FoliagePosition 的 trunkTop 标志

BendingTrunkPlacer 的所有树叶附着点均使用 `trunkTop = false`，因为弯曲树干没有传统意义上的"树干顶部"——树叶沿弯曲段连续分布。

### 4. TrunkPlacer 聚合文件

`TrunkPlacers.hpp` 是聚合头文件，**只转出** DarkOak、Fancy、Forky、Giant、MegaJungle 各自的头文件，自身不声明任何类——历史上它曾内联重复声明这五个类，与独立头文件构成 ODR 隐患（改一处漏一处即产生不一致的类定义）。`TrunkPlacers.cpp` 以 `#include "XxxTrunkPlacer.cpp"` 的方式聚合实现。BendingTrunkPlacer、CherryTrunkPlacer 和 StraightTrunkPlacer 拥有独立头文件，由 `TreeFeature.cpp` 直接包含。

### 5. FancyTrunkPlacer 的高度必须全程用相对偏移

`FancyTrunkPlacer::placeTrunk` 中，扫描分支的高度、`_getBranchLength` 的入参、分支末端坐标三者必须统一为**相对** `startPos.y` 的偏移量。一旦混入绝对 Y，`|halfHeight - relY| >= halfHeight` 会恒成立而使 `_getBranchLength` 恒返回 0，分支长度退化为 0、全部重合在树干正上方，**且不产生任何报错**。

### 6. 零长度 limb 除零会得到越界坐标

`_makeLimb` 以 `max(|dx|,|dy|,|dz|)` 作除数。若 `start == end` 且 `place == true` 走到除法处，步数 0 会得到 `0.0f / 0.0f = NaN`；`NaN` 转整型在 arm64 上饱和为 0、在 x86 上为 `INT_MIN`，前者会写入 `(0,0,0)` 并触发 `WorldGenRegion` 的访问窗口断言。故 `if (!place && start == end) return true;` 是承重判断：所有**探测**必须传 `place = false`，真正的放置留到扫描结束，并由 `!(basePos == branchEnd)` 守卫。
