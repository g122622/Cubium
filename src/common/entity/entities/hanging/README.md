# 悬挂实体

本目录包含可以挂在墙上的装饰实体。

## 目录结构

```
hanging/
├── HangingEntity.hpp/cpp    # 悬挂实体基类
└── README.md                # 本文档
```

## 实体列表

| 实体 | 说明 | 特性 |
|------|------|------|
| HangingEntity | 悬挂实体基类 | 管理悬挂位置和方向 |
| PaintingEntity | 画作 | 25种画作，多种尺寸 |
| ItemFrameEntity | 物品展示框 | 展示物品，可旋转 |
| LeashKnotEntity | 拴绳结 | 连接多条拴绳 |

## 悬挂方向

```cpp
enum class Direction : u8 {
    SOUTH = 0,  // 南
    WEST = 1,   // 西
    NORTH = 2,  // 北
    EAST = 3    // 东
};
```

## 画作类型

| 名称 | 尺寸 | 名称 | 尺寸 |
|------|------|------|------|
| Kebab | 1x1 | Pointer | 4x4 |
| Aztec | 1x1 | Pigscene | 4x4 |
| Alban | 1x1 | BurningSkull | 4x4 |
| Wanderer | 1x2 | Skeleton | 4x3 |
| Graham | 1x2 | DonkeyKong | 4x3 |
| Match | 2x2 | Fighters | 4x2 |
| Bust | 2x2 | Pool | 2x1 |
| Stage | 2x2 | Sunset | 2x1 |
| Void | 2x2 | Creebet | 2x1 |
| SkullAndRoses | 2x2 | Courbet | 2x1 |
| Wither | 2x2 | Sea | 2x1 |

## 物品展示框

- 可放置物品
- 8个旋转角度（每45度一个位置）
- 发光物品展示框变体

## 拴绳结

- 连接多条拴绳
- 无绑定时自动消失
- 可被玩家交互

## 实现状态

| 组件 | 状态 |
|------|------|
| HangingEntity | ⚠️ 框架完成，TODO需填充 |
| PaintingEntity | ⚠️ 框架完成，TODO需填充 |
| ItemFrameEntity | ⚠️ 框架完成，TODO需填充 |
| LeashKnotEntity | ⚠️ 框架完成，TODO需填充 |
