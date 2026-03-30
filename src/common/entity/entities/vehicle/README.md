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
- **AbstractMinecartEntity**: `Entity`, `IRideable`

## 船的特性

- 水上行驶
- 可被玩家控制方向
- 受水流影响
- 碰撞推动实体
- 6种木材变体

## 矿车的特性

- 铁轨行驶
- 受动力轨道加速
- 受激活轨道触发
- 各种功能性变体

## 实现状态

| 组件 | 状态 |
|------|------|
| BoatEntity | ⚠️ 框架完成，TODO需填充 |
| MinecartEntity | ⚠️ 框架完成，TODO需填充 |
| 铁轨逻辑 | ❌ 待实现 |
| 库存系统 | ❌ 待实现 |
