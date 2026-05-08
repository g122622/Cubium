# 装饰性方块模块

提供玻璃板、地毯、灯笼、染色玻璃等装饰性方块的实现。

## 目录结构

```
decorative/
├── PaneBlock.hpp/cpp           # 玻璃板/铁栏杆基类
├── StainedGlassBlock.hpp/cpp   # 染色玻璃（信标光束颜色提供者）
├── CarpetBlock.hpp/cpp         # 地毯
├── GlazedTerracottaBlock.hpp/cpp # 釉面陶瓦
├── FlowerPotBlock.hpp/cpp      # 花盆
├── LanternBlock.hpp/cpp        # 灯笼
├── ChainBlock.hpp/cpp          # 锁链
├── LadderBlock.hpp/cpp         # 梯子
├── ScaffoldingBlock.hpp/cpp    # 脚手架
├── CampfireBlock.hpp/cpp       # 营火
└── README.md
```

## 文件详解

### PaneBlock.hpp/cpp

**职责**：玻璃板和铁栏杆基类。

**状态属性**:
```cpp
- NORTH/WEST/EAST/SOUTH: bool  // 各方向连接
- WATERLOGGED: bool
```

**实现要点**:
- 形状按 4 位连接掩码缓存为 16 种组合，避免每次重算
- 连接判定接受同类 Pane、墙方块和有实体面的相邻方块
- `WATERLOGGED` 会直接回传水流体状态，并在邻居更新时调度水 tick

**衍生方块**:
- GLASS_PANE（玻璃板）
- WHITE_STAINED_GLASS_PANE ~ BLACK_STAINED_GLASS_PANE（染色玻璃板）
- IRON_BARS（铁栏杆）

### StainedGlassBlock.hpp/cpp

**职责**：染色玻璃方块。

**特性**:
- 实现 `IBeaconBeamColorProvider` 接口
- 为信标光束提供颜色
- 16种染料颜色

**接口实现**:
```cpp
class StainedGlassBlock : public Block, public IBeaconBeamColorProvider {
public:
    // 返回此染色玻璃的染料颜色
    [[nodiscard]] DyeColor getBeaconColor() const override;

    // 返回 RGB 颜色数组用于信标光束渲染
    [[nodiscard]] const std::array<f32, 3>* getBeaconColorMultiplier(
        const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr,
        const BlockPos* beaconPos = nullptr) const override;

    // 玻璃非固体
    [[nodiscard]] bool isSolid(const BlockState& state) const override;
};
```

**信标光束颜色混合**:
- 信标光束穿过染色玻璃时，颜色会与当前光束颜色平均混合
- 混合算法: `newColor = (currentColor + blockColor) / 2.0`
- 多层染色玻璃会依次混合

**衍生方块**: 16种染色玻璃
- WHITE_STAINED_GLASS, ORANGE_STAINED_GLASS, MAGENTA_STAINED_GLASS
- LIGHT_BLUE_STAINED_GLASS, YELLOW_STAINED_GLASS, LIME_STAINED_GLASS
- PINK_STAINED_GLASS, GRAY_STAINED_GLASS, LIGHT_GRAY_STAINED_GLASS
- CYAN_STAINED_GLASS, PURPLE_STAINED_GLASS, BLUE_STAINED_GLASS
- BROWN_STAINED_GLASS, GREEN_STAINED_GLASS, RED_STAINED_GLASS, BLACK_STAINED_GLASS

### CarpetBlock.hpp/cpp

**职责**：地毯方块。

**特性**:
- 单层高度（1/16格）
- 可放置在任何非空气方块上
- 16种颜色

**放置逻辑**:
- `isValidPosition()`: 检查下方是否为非空气方块
- `updatePostPlacement()`: 下方方块被移除时，地毯自动变为空气

### GlazedTerracottaBlock.hpp/cpp

**职责**：釉面陶瓦。

**状态属性**:
```cpp
- FACING: Direction (NORTH, SOUTH, EAST, WEST)  // 图案方向
```

**衍生方块**: 16种颜色的釉面陶瓦

### FlowerPotBlock.hpp/cpp

**职责**：花盆方块。

**方块实体**: `FlowerPotEntity`（存储植物内容）

**放置逻辑**:
- `isValidPosition()`: 花盆可放置在任何完整方块上（默认返回 true）
- `updatePostPlacement()`: 下方方块被移除时，花盆自动变为空气

**衍生方块**:
- 空花盆
- 盆栽树苗（6种）
- 盆栽花（多种）
- 盆栽蘑菇
- 盆栽仙人掌

### LanternBlock.hpp/cpp

**职责**：灯笼方块。

**状态属性**:
```cpp
- HANGING: bool  // 是否悬挂
- WATERLOGGED: bool
```

**放置逻辑**:
- `isValidPosition()`: 根据悬挂状态检查支撑
  - 悬挂时检查上方方块是否有实体底面
  - 站立时检查下方方块是否有实体顶面
- `updatePostPlacement()`: 支撑方块被移除时，灯笼自动变为空气

**衍生方块**:
- LANTERN（灯笼）
- SOUL_LANTERN（灵魂灯笼）

### ChainBlock.hpp/cpp

**职责**：锁链方块。

**状态属性**:
```cpp
- AXIS: Axis (X, Y, Z)  // 锁链方向
- WATERLOGGED: bool
```

### LadderBlock.hpp/cpp

**职责**：梯子方块。

**状态属性**:
```cpp
- HORIZONTAL_FACING: Direction (NORTH, SOUTH, EAST, WEST)  // 附着方向
- WATERLOGGED: bool
```

**放置逻辑**:
- `isValidPosition()`: 检查背面是否有固体方块（使用 `isSolidSide()` 判断）
- `updatePostPlacement()`: 背面方块被移除时，梯子自动变为空气

**特性**:
- 可攀爬（`isLadder()` 始终返回 true）
- 无碰撞箱

### ScaffoldingBlock.hpp/cpp

**职责**：脚手架方块。

**状态属性**:
```cpp
- DISTANCE_0_7: IntegerProperty(0-7)  // 距离支撑点的距离，0=直接支撑，7=过远需掉落
- BOTTOM: bool  // 是否显示底部支撑柱
- WATERLOGGED: bool  // 是否含水
```

**特性**:
- 可攀爬（`isLadder()` 始终返回 true）
- 距离支撑过远时会掉落
- 支持含水

**距离计算** (`calculateDistance` 静态方法):
- 检查下方方块：
  - 若为固体方块顶面，返回 0（直接支撑）
  - 若为脚手架，继承其距离值
- 检查下方水平方向的脚手架：
  - 遍历北东南西四个方向
  - 若 `pos.down().offset(dir)` 位置有脚手架，取其距离+1
- 返回最小距离值（最大为 7）

**底部支撑柱显示** (`shouldShowBottom` 静态方法):
- 当 `distance > 0` 且下方不是脚手架时返回 true
- 用于渲染脚手架底部的站立平台

**Tick 更新机制**:
- `onBlockAdded`: 方块放置时调度 1 tick 延迟的 tick
- `updatePostPlacement`: 邻居更新时调度 tick
- `tick`: 
  - 重新计算距离和底部状态
  - 若 `distance == 7`：
    - 如果之前 `distance == 7`：破坏方块并掉落脚手架物品（使用 `ItemDropHelper`）
    - 如果之前 `distance != 7`：创建 `FallingBlockEntity` 下落实体
  - 若状态改变：更新方块状态

**放置检测** (`isValidPosition`):
- 只有当 `calculateDistance(world, pos) < 7` 时才能放置

**碰撞形状** (`getCollisionShape`):
- `distance == 0`: 无碰撞（玩家可穿过）
- `distance != 0 && bottom == true`: 底部平台碰撞（玩家可站立）
- 其他情况: 无碰撞（玩家可穿过）

**渲染形状** (`getShape`):
- `bottom == true`: 完整形状（含角支柱和底部平台）
- `bottom == false`: 顶部平台形状

**参考**: MC 1.16.5 `net.minecraft.block.ScaffoldingBlock`

**测试**: `tests/common/world/block/blocks/decorative/ScaffoldingBlockTest.cpp`

## 依赖项

### 内部依赖
- `world/block/Block.hpp` - 方块基类
- `world/block/BlockState.hpp` - 方块状态
- `world/block/BlockProperties.hpp` - 方块属性构建器
- `world/block/VanillaBlocks.hpp` - 空气方块常量

### 外部依赖
- `<memory>` - 智能指针

## 使用方法

### 创建玻璃板

```cpp
// 创建玻璃板
auto glassPane = std::make_unique<PaneBlock>(
    BlockProperties::create()
        .mapColor(MapColor::WHITE)
        .solid(false)
);

// 检查连接状态
bool connectedNorth = state.get(BlockStateProperties::NORTH());
```

### 创建地毯

```cpp
// 创建红色地毯
auto redCarpet = std::make_unique<CarpetBlock>(
    BlockProperties::create()
        .mapColor(MapColor::RED)
        .solid(false)
);
```

## 测试用例

测试文件位于 `tests/common/world/block/blocks/decorative/`：

- `PaneBlockTest.cpp` - 玻璃板测试
- `CarpetBlockTest.cpp` - 地毯测试
- `LanternBlockTest.cpp` - 灯笼测试
- `DecorativeBlockTest.cpp` - 梯子、花盆等装饰性方块测试
- `StainedGlassBlockTest.cpp` - 染色玻璃和信标颜色工具类测试
  - `BeaconColorsTest` - BeaconColors 工具类测试（16种颜色的 RGB 值验证）
  - `StainedGlassBlockTest` - StainedGlassBlock 功能测试
    - 构造和属性验证
    - `isSolid()` 返回 false
    - `getBeaconColor()` 返回正确的 DyeColor
    - `getBeaconColorMultiplier()` 返回正确的 RGB 值
    - IBeaconBeamColorProvider 接口验证
- `ScaffoldingBlockTest.cpp` - 脚手架方块测试
  - 构造和默认状态验证
  - 攀爬属性 (`isLadder` 始终返回 true)
  - 形状和碰撞形状测试
  - 含水状态测试
  - DISTANCE_0_7 属性范围测试
  - BOTTOM 属性切换测试
