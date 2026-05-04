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
- **HopperMinecartEntity**: `AbstractMinecartEntity`, `IHopper`

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
| RideableMinecartEntity | 可乘坐、物品掉落、激活铁轨弹出乘客 | ✅ 完成 |
| ChestMinecartEntity | 27格库存、物品掉落 | ✅ 完成 |
| FurnaceMinecartEntity | 燃料系统、自动推进、激活铁轨改变方向 | ✅ 完成 |
| TNTMinecartEntity | 激活铁轨点燃、速度影响爆炸威力 | ✅ 完成 |
| HopperMinecartEntity | 物品收集、向下传输 | ✅ 完成 |
| CommandBlockMinecartEntity | 激活铁轨执行命令 | ✅ 完成 |

### 熔炉矿车 (FurnaceMinecartEntity)
- **燃料系统**: 玩家交互添加燃料（3600 tick = 3分钟）
- **自动推进**: 有燃料时自动沿推动方向前进
- **激活铁轨**: 可改变推进方向
- **最大速度**: 0.2（普通矿车为0.4）

### TNT矿车 (TNTMinecartEntity)
- **点燃方式**: 激活铁轨点燃，引信80 tick（4秒）
- **爆炸威力**: 基础4.0，速度加成最大到5.0
- **爆炸模式**: Break模式（破坏方块不掉落）

### 漏斗矿车 (HopperMinecartEntity)
- **库存**: 5格（与漏斗方块相同）
- **物品吸取**: 从上方区域吸取物品实体
- **物品传输**: 向下方容器传输物品
- **冷却时间**: 4 tick
- **实现接口**: `IHopper`

### 命令方块矿车 (CommandBlockMinecartEntity)
- **激活方式**: 通过激活铁轨触发
- **命令存储**: 存储命令字符串
- **输出记录**: 记录上次输出和成功次数

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
| FurnaceMinecartEntity | ✅ 完成 |
| TNTMinecartEntity | ✅ 完成 |
| HopperMinecartEntity | ✅ 完成 |
| CommandBlockMinecartEntity | ✅ 完成 |
| 铁轨逻辑 | ✅ 完成 |
| 库存系统 | ✅ 完成 |
| 爆炸系统对接 | ✅ 完成 |

## 测试覆盖

测试文件位于 `tests/entity/MinecartTests.cpp`，包含：
- RailShape isAscending 辅助函数测试
- ChestMinecartEntity 库存测试
- AbstractMinecartEntity 基础功能测试
- FurnaceMinecartEntity 燃料系统测试
- TNTMinecartEntity 引信系统测试
- MinecartItem 构造测试
