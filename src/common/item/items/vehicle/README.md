# 车辆物品

本目录包含车辆相关的物品类。

## 目录结构

```
vehicle/
├── MinecartItem.hpp/cpp   # 矿车物品
├── BoatItem.hpp/cpp       # 船物品
└── README.md              # 本文档
```

## 物品列表

### 矿车物品

| 物品 | 说明 | 注册名称 |
|------|------|----------|
| MinecartItem | 普通矿车 | minecraft:minecart |
| MinecartItem | 箱子矿车 | minecraft:chest_minecart |
| MinecartItem | 熔炉矿车 | minecraft:furnace_minecart |
| MinecartItem | TNT矿车 | minecraft:tnt_minecart |
| MinecartItem | 漏斗矿车 | minecraft:hopper_minecart |
| MinecartItem | 命令方块矿车 | minecraft:command_block_minecart |

### 船物品

| 物品 | 说明 | 注册名称 |
|------|------|----------|
| BoatItem | 橡木船 | minecraft:oak_boat |
| BoatItem | 云杉木船 | minecraft:spruce_boat |
| BoatItem | 桦木船 | minecraft:birch_boat |
| BoatItem | 丛林木船 | minecraft:jungle_boat |
| BoatItem | 金合欢木船 | minecraft:acacia_boat |
| BoatItem | 深色橡木船 | minecraft:dark_oak_boat |

## MinecartItem

矿车物品用于在铁轨上放置矿车实体。

### 功能
- 检测铁轨方块（AbstractRailBlock）
- 计算正确的放置位置
  - 平轨：Y + 0.0625
  - 斜轨：Y + 0.5625
- 创建对应类型的矿车实体
- 设置自定义名称

### 使用示例

```cpp
// 矿车物品注册（Items.cpp）
MINECART = &registry.registerItem<item::MinecartItem>(
    ResourceLocation("minecraft:minecart"),
    entity::AbstractMinecartEntity::Type::Rideable,
    ItemProperties().maxStackSize(1)
);
```

## BoatItem

船物品用于在水面或陆地上放置船实体。

### 功能
- 检测水面或陆地位置
- 计算正确的放置位置
  - 水面：方块顶部
  - 陆地：击中位置
- 创建对应木材类型的船实体
- 设置船的朝向为玩家朝向
- 碰撞检测确保船可以放置

### 使用示例

```cpp
// 船物品注册（Items.cpp）
OAK_BOAT = &registry.registerItem<item::BoatItem>(
    ResourceLocation("minecraft:oak_boat"),
    entity::BoatEntity::Type::OAK,
    ItemProperties().maxStackSize(1)
);
```

## 依赖

### MinecartItem
- `AbstractMinecartEntity`: 矿车实体基类
- `AbstractRailBlock`: 铁轨方块基类
- `ItemUseContext`: 物品使用上下文
- `IWorld`: 世界接口

### BoatItem
- `BoatEntity`: 船实体类
- `ItemUseContext`: 物品使用上下文
- `IWorld`: 世界接口
- `AxisAlignedBB`: 碰撞箱

## 实现状态

### MinecartItem

| 功能 | 状态 |
|------|------|
| 放置检测 | ✅ 完成 |
| 斜坡高度调整 | ✅ 完成 |
| 实体创建 | ✅ 完成 |
| 自定义名称 | ✅ 完成 |
| 物品消耗 | ✅ 完成 |

### BoatItem

| 功能 | 状态 |
|------|------|
| 水面放置检测 | ✅ 完成 |
| 陆地放置检测 | ✅ 完成 |
| 实体创建 | ✅ 完成 |
| 碰撞检测 | ✅ 完成 |
| 物品消耗 | ✅ 完成 |
