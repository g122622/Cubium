# 装饰性方块模块

提供玻璃板、地毯、灯笼等装饰性方块的实现。

## 目录结构

```
decorative/
├── PaneBlock.hpp/cpp           # 玻璃板/铁栏杆基类
├── CarpetBlock.hpp/cpp         # 地毯
├── GlazedTerracottaBlock.hpp/cpp # 釉面陶瓦
├── FlowerPotBlock.hpp/cpp      # 花盆
├── LanternBlock.hpp/cpp        # 灯笼
├── ChainBlock.hpp/cpp          # 锁链
├── LadderBlock.hpp/cpp         # 梯子
├── ScaffoldingBlock.hpp/cpp    # 脚手架
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

**衍生方块**:
- GLASS_PANE（玻璃板）
- WHITE_STAINED_GLASS_PANE ~ BLACK_STAINED_GLASS_PANE（染色玻璃板）
- IRON_BARS（铁栏杆）

### CarpetBlock.hpp/cpp

**职责**：地毯方块。

**特性**:
- 单层高度（1/16格）
- 可放置在任何固体上
- 16种颜色

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
- FACING: Direction (NORTH, SOUTH, EAST, WEST)  // 附着方向
```

**特性**:
- 可攀爬
- 只能附在固体方块侧面

### ScaffoldingBlock.hpp/cpp

**职责**：脚手架方块。

**状态属性**:
```cpp
- DISTANCE: IntegerProperty(0-7)  // 距离支撑的距离
- BOTTOM: bool  // 是否在底部
- WATERLOGGED: bool
```

**特性**:
- 可攀爬
- 可下沉
- 无需支撑可扩展距离

## 依赖项

### 内部依赖
- `world/block/Block.hpp` - 方块基类
- `world/block/BlockState.hpp` - 方块状态
- `world/block/BlockProperties.hpp` - 方块属性构建器

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
