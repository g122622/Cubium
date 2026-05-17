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
| ItemFrameEntity | 物品展示框 | 展示物品，可旋转，红石信号输出 |
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

### 基本功能
- 可放置物品（使用 ItemStack 存储）
- 8个旋转角度（每45度一个位置，rotation 值 0-7）
- 发光物品展示框变体（Glow Item Frame）

### 红石信号输出
物品展示框可以向红石比较器输出模拟信号：

| 条件 | 信号强度 |
|------|----------|
| 无物品 | 0 |
| 有物品，rotation=0 | 1 |
| 有物品，rotation=1 | 2 |
| ... | ... |
| 有物品，rotation=7 | 8 |

**关键方法**：
- `getAnalogOutput()` - 返回红石比较器信号强度（0-8）
- `getHorizontalFacing()` - 返回物品展示框朝向（mc::Direction）
- `setDisplayedItem(const ItemStack&)` - 设置展示物品
- `getDisplayedItem()` - 获取展示物品
- `hasItem()` - 检查是否有展示物品
- `rotateItem()` - 旋转物品（右键交互）

### 比较器检测规则
参考 MC 1.16.5，红石比较器检测物品展示框的位置关系：

```
[物品展示框] --> [完整方块] --> [比较器]
      ↑              ↑            ↑
   朝向相同      普通方块      检测方向
```

- 物品展示框必须附着在比较器前方完整方块的另一侧
- 物品展示框的朝向必须与比较器的朝向相同
- 该位置只能有一个物品展示框

## 拴绳结

- 连接多条拴绳
- 无绑定时自动消失
- 可被玩家交互

## 实现状态

| 组件 | 状态 |
|------|------|
| HangingEntity | ✅ 基本功能完成 |
| PaintingEntity | ⚠️ 框架完成，TODO需填充 |
| ItemFrameEntity | ✅ 红石信号功能完成 |
| LeashKnotEntity | ⚠️ 框架完成，TODO需填充 |
