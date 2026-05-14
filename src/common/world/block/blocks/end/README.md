# 末地方块模块 (End Blocks)

末地方块模块提供末地维度相关方块的实现，包括末地传送门、紫颂植物、龙蛋等。

## 目录结构

```
end/
├── EndPortalBlock.hpp/cpp # 末地传送门、传送门框架、折跃门、紫颂植物、紫颂花、龙蛋
└── README.md              # 本文档
```

## 方块类型

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `EndPortalBlock` | 末地传送门方块 | 无 |
| `EndPortalFrameBlock` | 末地传送门框架 | EYE, HORIZONTAL_FACING |
| `EndGatewayBlock` | 末地折跃门 | 无 |
| `ChorusPlantBlock` | 紫颂植物 | NORTH/SOUTH/EAST/WEST/DOWN/UP |
| `ChorusFlowerBlock` | 紫颂花 | AGE_0_5 |
| `DragonEggBlock` | 龙蛋 | 无 |

## 核心机制

### EndPortalBlock（末地传送门方块）

- **状态属性**：无
- **行为**：
  - 实体碰撞时触发传送（主世界↔末地）
  - 传送冷却 300 ticks（15 秒）
  - 无碰撞箱
- **参考**：MC 1.16.5 `net.minecraft.block.EndPortalBlock`

### EndPortalFrameBlock（末地传送门框架）

- **状态属性**：
  - `EYE`：是否放置了末影之眼
  - `HORIZONTAL_FACING`：框架朝向
- **行为**：
  - 可放置末影之眼
  - 框架朝向决定传送门结构
- **参考**：MC 1.16.5 `net.minecraft.block.EndPortalFrameBlock`

### EndGatewayBlock（末地折跃门）

- **状态属性**：无
- **行为**：
  - 实体碰撞时触发折跃传送
  - 无碰撞箱
- **TODO**：实现折跃门传送逻辑

### ChorusPlantBlock（紫颂植物）

- **状态属性**：6 个布尔方向属性（DOWN, UP, NORTH, SOUTH, WEST, EAST）
- **形状系统**：
  - 参考 MC 1.16.5 `SixWayBlock` 实现
  - 预计算 64 种形状组合（2^6）
  - 使用位掩码索引：Down=bit0, Up=bit1, North=bit2, South=bit3, West=bit4, East=bit5
  - 中心柱尺寸：apothem=0.3125（5 像素）
- **连接规则**：
  - 所有方向：连接到紫颂植物和紫颂花
  - 仅下方：额外连接到末地石（作为生长基底）
- **方法**：
  - `getShapeIndex(state)`：静态方法，根据连接状态计算形状索引
  - `canConnect(world, pos, direction)`：检查指定方向是否应该连接
  - `isValidPosition(state, world, pos)`：检查是否有至少一个连接
  - `updatePostPlacement(...)`：邻居更新时更新连接状态
- **参考**：MC 1.16.5 `net.minecraft.block.ChorusPlantBlock`

### ChorusFlowerBlock（紫颂花）

- **状态属性**：`AGE_0_5`（生长阶段 0-5）
- **行为**：
  - 随机刻时有概率生长（增加年龄）
  - 最大年龄为 5
- **位置验证（isValidPosition）**：
  - 下方是紫颂植物（ChorusPlantBlock）→ 可以放置
  - 下方是末地石（EndStone）→ 可以放置（作为生长基底）
  - 下方是紫颂花（ChorusFlowerBlock）→ 可以放置（水平分支生长）
  - 下方是空气 → 检查水平四个方向：
    - 必须恰好有一个水平方向是紫颂植物
    - 其他三个水平方向必须是空气
  - 下方是其他方块 → 无法放置
- **参考**：MC 1.16.5 `net.minecraft.block.ChorusFlowerBlock`

### DragonEggBlock（龙蛋）

- **状态属性**：无
- **行为**：
  - 继承自 FallingBlock，下方无支撑时会下落
  - 点击（左键或右键）会随机传送到附近位置
  - 下落延迟为 5 tick（比普通下落方块的 2 tick 更长）
  - 传送范围：X/Z 方向 -15 ~ +15，Y 方向 -7 ~ +7
  - 最多尝试 1000 次寻找有效位置
- **参考**：MC 1.16.5 `net.minecraft.block.DragonEggBlock`

## 使用方法

### 注册末地方块

```cpp
#include "world/block/blocks/end/EndPortalBlock.hpp"
#include "world/block/VanillaBlocks.hpp"

// 在 VanillaBlocks::initialize() 中注册
auto& endPortal = BlockRegistry::instance().registerBlock<EndPortalBlock>(
    ResourceLocation("minecraft:end_portal"),
    BlockProperties(Material::PORTAL).noCollision().hardness(-1.0f)
);
```

### 检查紫颂植物连接

```cpp
#include "world/block/blocks/end/EndPortalBlock.hpp"

// 检查紫颂植物是否可以连接到指定方向
ChorusPlantBlock* plant = static_cast<ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT);
bool canConnectDown = plant->canConnect(world, pos, Direction::Down);
bool canConnectNorth = plant->canConnect(world, pos, Direction::North);

// 获取形状索引
size_t index = ChorusPlantBlock::getShapeIndex(state);
const CollisionShape& shape = plant->getShape(state);
```

### 计算紫颂植物形状索引

```cpp
// 位掩码索引计算（Direction 枚举顺序：Down=0, Up=1, North=2, South=3, West=4, East=5）
size_t index = 0;
if (state.get(BlockStateProperties::DOWN()))  index |= 1ULL << 0;  // bit 0
if (state.get(BlockStateProperties::UP()))    index |= 1ULL << 1;  // bit 1
if (state.get(BlockStateProperties::NORTH())) index |= 1ULL << 2;  // bit 2
if (state.get(BlockStateProperties::SOUTH())) index |= 1ULL << 3;  // bit 3
if (state.get(BlockStateProperties::WEST()))  index |= 1ULL << 4;  // bit 4
if (state.get(BlockStateProperties::EAST()))  index |= 1ULL << 5;  // bit 5

// 索引范围：0-63，预计算形状数组大小：64
```

## 依赖项

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/Material` | 材质系统 |
| `world/IWorld` | 世界接口 |
| `world/block/VanillaBlocks` | 方块注册 |
| `util/property/Properties` | 方块属性 |
| `util/Direction` | 方向枚举 |
| `physics/collision/CollisionShape` | 碰撞形状 |

## 容易踩的坑

### 1. 紫颂植物形状索引顺序

**问题**：形状索引的位顺序必须与 Direction 枚举顺序一致。

**Direction 枚举顺序**：Down=0, Up=1, North=2, South=3, West=4, East=5

```cpp
// 正确的索引计算
// Down = bit 0, Up = bit 1, North = bit 2, South = bit 3, West = bit 4, East = bit 5
```

### 2. 紫颂植物连接检查

**问题**：`canConnect` 使用指针比较 `adjState->is(this)` 检查相邻方块是否是紫颂植物。

**解决方案**：测试时必须使用 `VanillaBlocks::CHORUS_PLANT` 指针，而不是创建新的 `ChorusPlantBlock` 实例。

### 3. 末地传送门传送

**问题**：传送逻辑需要 `ServerDimensionManager` 处理实际的维度切换。

**注意**：`EndPortalBlock::onEntityCollision` 只设置传送请求标志，实际传送由服务端处理。

### 4. isValidPosition 的方块检查

**已解决**：`ChorusFlowerBlock::isValidPosition` 已完整实现，检查下方方块是否是紫颂植物、紫颂花、末地石或空气（水平支撑）。

## 测试用例

测试文件：`tests/common/world/block/blocks/end/ChorusPlantBlockTest.cpp`

### 测试覆盖

| 测试类别 | 测试数量 |
|---------|---------|
| 基础属性测试 | 9 |
| canConnect 测试 | 5 |
| isValidPosition 测试 | 7 |
| updatePostPlacement 测试 | 2 |
| **总计** | **23** |

### 测试要点

1. **形状索引计算**：验证 64 种连接状态的索引正确
2. **canConnect 连接规则**：验证紫颂植物、紫颂花、末地石（仅向下）的连接
3. **isValidPosition 位置验证**：验证需要有至少一个有效连接
4. **updatePostPlacement 状态更新**：验证邻居更新时连接状态正确更新

## 参考资料

- MC 1.16.5 `net.minecraft.block.EndPortalBlock`
- MC 1.16.5 `net.minecraft.block.EndPortalFrameBlock`
- MC 1.16.5 `net.minecraft.block.EndGatewayBlock`
- MC 1.16.5 `net.minecraft.block.ChorusPlantBlock`（SixWayBlock 实现）
- MC 1.16.5 `net.minecraft.block.ChorusFlowerBlock`
- MC 1.16.5 `net.minecraft.block.DragonEggBlock`
