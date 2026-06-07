# 功能方块模块 (Functional Blocks)

本目录包含各种功能性方块的实现，这些方块提供了特定的游戏功能，如存储、合成、红石交互等。

## 目录结构

```
functional/
├── BedBlock.hpp/cpp              # 床方块（16色，双格结构，设置重生点）
├── BrewingStandBlock.hpp/cpp     # 酿造台（药水酿造，3瓶槽位）
├── CauldronBlock.hpp/cpp         # 炼药锅（储水、物品清洗，4级水位）
├── CompostableItems.hpp/cpp      # 可堆肥物品注册表（~66种物品概率表）
├── ComposterBlock.hpp/cpp        # 堆肥桶（8层填充，产出骨粉）
├── CakeBlock.hpp/cpp             # 蛋糕（可食用，7片）
├── BeaconBlock.hpp/cpp           # 信标（增益效果，金字塔基座）
├── BarrelBlock.hpp/cpp           # 木桶（存储容器，6方向放置）
├── LecternBlock.hpp/cpp          # 讲台（书籍展示，红石输出）
├── GrindstoneBlock.hpp/cpp       # 砂轮（修复/祛魔，3种附着面）
├── StonecutterBlock.hpp/cpp      # 切石机（石材切割配方）
├── LoomBlock.hpp/cpp             # 织布机（旗帜图案制作）
├── BellBlock.hpp/cpp             # 钟（声音/动画，多方向附着）
├── JukeboxBlock.hpp/cpp          # 唱片机（音乐播放）
├── RespawnAnchorBlock.hpp/cpp    # 重生锚（下界重生点，4级充能）
├── LodestoneBlock.hpp/cpp        # 磁石（指南针绑定）
├── CartographyTableBlock.hpp/cpp # 制图台（地图复制/扩展）
├── FletchingTableBlock.hpp/cpp   # 制箭台（箭矢制作）
├── SmithingTableBlock.hpp/cpp    # 锻造台（装备升级）
├── TrailsBlocks.hpp/cpp          # 考古方块（雕纹书架、饰纹陶罐、可疑沙/砾、嗅探兽蛋）
└── README.md
```

## 内部模块关系

```
Block (基类)
├── BedBlock
│   └── 双方块结构处理
├── BrewingStandBlock
│   └── 容器方块实体
├── CauldronBlock
│   ├── 音效系统
│   └── 物品交互
├── ComposterBlock
│   ├── CompostableItems (可堆肥物品注册表)
│   ├── TickManager (tick调度)
│   └── ItemDropHelper (物品掉落)
├── CakeBlock
│   └── 可食用方块
├── BeaconBlock
│   └── 增益效果系统
├── BarrelBlock
│   └── 容器方块实体
├── LecternBlock
│   └── 红石信号输出
├── GrindstoneBlock
│   └── 附着检测
├── BellBlock
│   ├── 多方向附着
│   └── 支撑检测
├── JukeboxBlock
│   └── 音乐播放
├── RespawnAnchorBlock
│   └── 充能系统
├── TrailsBlocks
│   ├── ChiseledBookshelfBlock (红石比较器检测)
│   ├── DecoratedPotBlock (IWaterLoggable)
│   ├── BrushableBlock (FallingBlock子类)
│   └── SnifferEggBlock (randomTick孵化)
└── 其他工作站方块
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 依赖模块 | 用途 |
|----------|------|
| `Block.hpp` | 方块基类 |
| `BlockState.hpp` | 状态管理 |
| `Material.hpp` | 材质定义 |
| `CollisionShape.hpp` | 碰撞形状 |
| `Properties.hpp` | 方块属性（HORIZONTAL_FACING, LEVEL_0_8等） |
| `IWorld` | 世界接口 |
| `BlockItemUseContext` | 放置上下文 |
| `Player` | 玩家实体 |
| `TickManager` | tick调度（ComposterBlock） |
| `ItemDropHelper` | 物品掉落工具 |
| `IWaterLoggable` | 含水接口（DecoratedPotBlock） |
| `FallingBlock` | 下落方块基类（BrushableBlock） |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `VanillaBlocks.hpp` | 注册原版方块 |
| `BlockRegistry` | 方块注册表 |
| 世界生成 | 使用方块实例 |
| 渲染系统 | 方块渲染 |

## 容易踩的坑

### 1. BedBlock 双方块结构

床头和床脚是两个独立的方块，放置时需要正确处理 `BED_PART` 属性。睡眠前需要检查：
- 床是否被占用（`OCCUPIED` 属性）
- 玩家距离床是否超过3格（水平）/2格（垂直）
- 床上方空间是否被阻挡
- 周围是否有怪物（非创造模式）
- 在下界/末地使用会爆炸（爆炸强度5.0）

### 2. ComposterBlock 堆肥延迟

等级7→8的转换需要20 tick延迟，通过 `TickManager` 调度。不要在 `onBlockActivated` 中直接产出骨粉。

### 3. GrindstoneBlock 附着面

支持3种附着面（FLOOR, CEILING, WALL），共12种VoxelShape（3附着面×4朝向）。放置时需要检测支撑方块是否存在，支撑失效时掉落。

### 4. RespawnAnchorBlock 维度检测

只能在下界设置重生点。非下界使用会爆炸。使用 `Dimension::respawnAnchorWorks()` 判断当前维度是否允许重生锚。

### 5. TrailsBlocks 文件包含多个类

`TrailsBlocks.hpp/cpp` 包含4个方块类：
- `ChiseledBookshelfBlock` - 雕纹书架
- `DecoratedPotBlock` - 饰纹陶罐（实现 IWaterLoggable）
- `BrushableBlock` - 可刷方块（继承 FallingBlock）
- `SnifferEggBlock` - 嗅探兽蛋（randomTick 孵化）

### 6. 方块实体注册

部分方块需要方块实体支持：
- `BrewingStandBlock` → `BrewingStandEntity`
- `BarrelBlock` → `BarrelEntity`
- `JukeboxBlock` → `JukeboxEntity`
- `LecternBlock` → `LecternEntity`
- `BeaconBlock` → `BeaconEntity`

### 7. 红石比较器信号

多个方块支持比较器信号输出，需要正确实现 `hasComparatorInputOverride()` 和 `getComparatorInputOverride()`：
- `ComposterBlock`: 输出 = 填充等级
- `BrewingStandBlock`: 输出基于槽位状态
- `BeaconBlock`: 输出基于效果激活状态
- `LecternBlock`: 输出基于书籍页面
- `ChiseledBookshelfBlock`: 输出基于书籍数量

### 8. CauldronBlock 水位限制

水位范围是0-3（`LEVEL_0_3`），不是0-15。雨天自动填充需通过 `randomTick` 实现。
