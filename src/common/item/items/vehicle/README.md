# 车辆物品

本目录包含车辆相关的物品类。

## 目录结构

```
vehicle/
├── MinecartItem.hpp/cpp   # 矿车物品
└── README.md              # 本文档
```

## 物品列表

| 物品 | 说明 | 注册名称 |
|------|------|----------|
| MinecartItem | 普通矿车 | minecraft:minecart |
| MinecartItem | 箱子矿车 | minecraft:chest_minecart |
| MinecartItem | 熔炉矿车 | minecraft:furnace_minecart |
| MinecartItem | TNT矿车 | minecraft:tnt_minecart |
| MinecartItem | 漏斗矿车 | minecraft:hopper_minecart |
| MinecartItem | 命令方块矿车 | minecraft:command_block_minecart |

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

## 依赖

- `AbstractMinecartEntity`: 矿车实体基类
- `AbstractRailBlock`: 铁轨方块基类
- `ItemUseContext`: 物品使用上下文
- `IWorld`: 世界接口

## 实现状态

| 功能 | 状态 |
|------|------|
| 放置检测 | ✅ 完成 |
| 斜坡高度调整 | ✅ 完成 |
| 实体创建 | ✅ 完成 |
| 自定义名称 | ✅ 完成 |
| 物品消耗 | ✅ 完成 |
