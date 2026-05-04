# 车辆实体

本目录包含可乘坐的车辆实体。

## 目录结构

```
vehicle/
├── BoatEntity.hpp/cpp       # 船实体
├── MinecartEntity.hpp/cpp   # 矿车实体（包含多种变体）
└── README.md                # 本文档
```

## 实体列表

| 实体 | 说明 | 变体 |
|------|------|------|
| BoatEntity | 船 | 橡木、云杉、白桦、丛林、金合欢、深色橡木 |
| RideableMinecartEntity | 普通矿车 | - |
| ChestMinecartEntity | 箱子矿车 | 带库存 |
| FurnaceMinecartEntity | 熔炉矿车 | 可添加燃料 |
| TNTMinecartEntity | TNT矿车 | 可被激活爆炸 |
| HopperMinecartEntity | 漏斗矿车 | 可收集物品 |
| CommandBlockMinecartEntity | 命令方块矿车 | 可执行命令 |

## 接口继承

- **BoatEntity**: `Entity`, `IRideable`
- **AbstractMinecartEntity**: `Entity`

## 船的特性

- 水上行驶
- 可被玩家控制方向
- 受水流影响
- 碰撞推动实体
- 6种木材变体

## 矿车的特性

### 基础功能
- 铁轨行驶（支持10种铁轨形状）
- 受动力轨道加速
- 受激活轨道触发
- 斜坡高度调整
- 物品掉落

### 各变体特性

| 变体 | 特性 | 实现状态 |
|------|------|----------|
| RideableMinecartEntity | 可乘坐、物品掉落 | ✅ 完成 |
| ChestMinecartEntity | 27格库存、物品掉落 | ✅ 完成 |
| FurnaceMinecartEntity | 燃料系统、自动推进 | ⚠️ 基础完成 |
| TNTMinecartEntity | 激活铁轨点燃、爆炸 | ⚠️ 基础完成 |
| HopperMinecartEntity | 物品收集、传输 | ⚠️ 基础完成 |
| CommandBlockMinecartEntity | 命令执行 | ⚠️ 框架完成 |

### 铁轨系统
- AbstractRailBlock: 铁轨基类，支持10种形状
- RailBlock: 普通铁轨，自动连接
- PoweredRailBlock: 动力铁轨，红石加速
- DetectorRailBlock: 探测铁轨，矿车检测
- ActivatorRailBlock: 激活铁轨，触发矿车

### 矿车物品
- MinecartItem: 放置矿车物品
- 6种矿车物品注册（Items::MINECART等）
- 斜坡高度调整（Y + 0.0625 或 Y + 0.5625）

## 实现状态

| 组件 | 状态 |
|------|------|
| BoatEntity | ⚠️ 框架完成 |
| AbstractMinecartEntity | ✅ 完成 |
| RideableMinecartEntity | ✅ 完成 |
| ChestMinecartEntity | ✅ 完成 |
| FurnaceMinecartEntity | ⚠️ 基础完成 |
| TNTMinecartEntity | ⚠️ 基础完成 |
| HopperMinecartEntity | ⚠️ 基础完成 |
| CommandBlockMinecartEntity | ⚠️ 框架完成 |
| 铁轨逻辑 | ✅ 完成 |
| 库存系统 | ✅ 完成 |

## 测试覆盖

测试文件位于 `tests/entity/MinecartTests.cpp`，包含：
- RailShape isAscending 辅助函数测试
- ChestMinecartEntity 库存测试
- AbstractMinecartEntity 基础功能测试
- MinecartItem 构造测试
