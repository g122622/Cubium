# 载具渲染器 (Vehicle Renderers)

## 概述

载具渲染器负责渲染船、矿车等可乘坐实体。

## 文件说明

| 文件 | 描述 |
|------|------|
| `VehicleRenderers.hpp` | 载具渲染器头文件 |
| `VehicleRenderers.cpp` | 载具渲染器实现 |

## 支持的载具

### 船 (Boat)
- 橡木船 (Oak Boat)
- 云杉木船 (Spruce Boat)
- 白桦木船 (Birch Boat)
- 丛林木船 (Jungle Boat)
- 金合欢木船 (Acacia Boat)
- 深色橡木船 (Dark Oak Boat)

### 矿车 (Minecart)
- 普通矿车
- 运输矿车
- 动力矿车
- 漏斗矿车
- TNT矿车

## 渲染特性

### 船
- 水面晃动动画
- 受损抖动效果
- 划桨动画
- 不同木材类型的纹理

### 矿车
- 沿轨道方向对齐
- 内容物渲染（箱子、TNT等）
- 受损抖动效果

## 参考

- Minecraft 1.16.5 `net.minecraft.client.renderer.entity.BoatRenderer`
- Minecraft 1.16.5 `net.minecraft.client.renderer.entity.MinecartRenderer`
- Minecraft 1.16.5 `net.minecraft.client.renderer.entity.model.BoatModel`
- Minecraft 1.16.5 `net.minecraft.client.renderer.entity.model.MinecartModel`
