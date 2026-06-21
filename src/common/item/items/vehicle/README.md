# 载具物品

本目录包含载具相关的物品类，用于放置矿车和船实体。

## 目录结构

```
vehicle/
├── BoatItem.hpp/cpp       # 船物品（放置船实体）
├── MinecartItem.hpp/cpp   # 矿车物品（放置各类矿车实体）
└── README.md              # 本文档
```

## 内部模块关系

```
┌────────────────┐
│  MinecartItem  │──── 依赖 ────→ AbstractMinecartEntity (6种矿车类型)
└────────────────┘
┌────────────────┐
│   BoatItem     │──── 依赖 ────→ BoatEntity (10种木材类型)
└────────────────┘
        │
        └──── 两者均依赖 ────→ Item 基类、ItemUseContext、IWorld
```

两个物品类相互独立，无直接依赖关系。

## 上下游外部依赖关系

**依赖上游（本目录依赖）：**
- `item/core/` - Item 基类、ItemStack
- `item/context/` - ItemUseContext 物品使用上下文
- `entity/entities/vehicle/` - AbstractMinecartEntity、BoatEntity
- `world/` - IWorld、BlockPos、BlockState
- `world/block/blocks/redstone/` - AbstractRailBlock 铁轨方块
- `util/AxisAlignedBB.hpp` - 碰撞箱

**被下游依赖（依赖本目录）：**
- `item/Items.cpp` - 物品注册（注册矿车和船的各变体）

## 容易踩的坑

### 矿车物品放置高度

- 平轨放置：Y + 0.0625（1/16格）
- 斜轨放置：Y + 0.5625（额外+0.5）
- 必须通过 `RailShape` 判断是否为斜坡，否则矿车位置会不对

### 铁轨检测逻辑

- 点击铁轨直接放置
- 点击铁轨下方方块时，会尝试在下方一格寻找铁轨
- 必须使用 `dynamic_cast<AbstractRailBlock*>` 检测方块类型

### 船物品碰撞检测

- 放置前必须检查碰撞：`boat->boundingBox().grow(-0.1f)`
- 缩小碰撞箱 0.1 格是为了避免边界问题
- 使用 `IWorld::hasNoCollisions()` 检测

### 船生成位置

- 船生成在玩家视线击中点，而非方块中心
- 船朝向自动设置为玩家朝向（`context.getPlayerYaw()`）

### Spawner Minecart 未实现

- `AbstractMinecartEntity::Type::Spawner` 暂未实现
- 当前使用普通矿车作为降级处理，后续需补充刷怪笼矿车
