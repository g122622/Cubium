# 世界边界模块 (World Border)

提供 Minecraft 1.16.5 风格的世界边界系统，支持边界大小设置、中心点设置、伤害计算、警告效果等功能。

## 目录结构

```
border/
├── WorldBorder.hpp       # 世界边界类定义
├── WorldBorder.cpp       # 世界边界实现
└── README.md             # 本文件
```

## 核心类

### WorldBorder

世界边界管理类，支持：

- **边界大小**：立即设置或渐变过渡
- **边界中心**：设置边界中心坐标
- **伤害参数**：每格伤害量、伤害缓冲距离
- **警告参数**：警告时间、警告距离
- **边界检测**：点检测、AABB检测、区块检测

```cpp
#include "common/world/border/WorldBorder.hpp"

mc::world::border::WorldBorder border;

// 设置边界大小
border.setSize(1000.0);  // 立即设置为 1000 格

// 渐变设置边界大小
border.setSizeLerp(1000.0, 500.0, 60000);  // 60秒内从 1000 缩小到 500

// 设置边界中心
border.setCenter(100.0, 200.0);

// 设置伤害参数
border.setDamagePerBlock(0.2);  // 每格 0.2 伤害
border.setDamageBuffer(5.0);    // 5 格缓冲区

// 设置警告参数
border.setWarningTime(15);      // 15 秒警告时间
border.setWarningDistance(5);   // 5 格警告距离

// 检测点是否在边界内
bool inside = border.contains(x, z);

// 获取点到边界的距离（正数=在内，负数=在外）
double distance = border.getClosestDistance(x, z);
```

### IBorderState（状态模式）

边界大小使用状态模式实现：

- **StationaryBorderState**：静止边界，固定大小
- **MovingBorderState**：移动边界，线性插值过渡

状态转换：
- `setSize()` 创建静止状态
- `setSizeLerp()` 创建移动状态
- `tick()` 更新移动状态，过渡完成后转为静止状态

### IBorderListener（监听器）

边界变化监听器接口，用于网络同步：

```cpp
class MyListener : public IBorderListener {
    void onSizeChanged(double newSize) override {
        // 发送 WorldBorderPacket(SetSize)
    }
    void onTransitionStarted(double oldSize, double newSize, u64 timeMs) override {
        // 发送 WorldBorderPacket(LerpSize)
    }
    void onCenterChanged(double x, double z) override {
        // 发送 WorldBorderPacket(SetCenter)
    }
    // ...
};
```

## 边界参数

### 默认值（MC 1.16.5 兼容）

| 参数 | 默认值 | 说明 |
|------|--------|------|
| 初始大小 | 60,000,000 | 约 6000 万格 |
| 最大大小 | 29,999,872 | 约 3000 万格半径 |
| 伤害每格 | 0.2 | 越界每格伤害量 |
| 伤害缓冲 | 5.0 | 越界缓冲距离（不受伤） |
| 警告时间 | 15 秒 | 边界收缩前警告时间 |
| 警告距离 | 5 格 | 接近边界时警告距离 |

### 伤害计算公式

```
距离 = getClosestDistance(entity) + damageBuffer
如果 距离 < 0:
    伤害 = max(1, floor(-距离 * damagePerBlock))
```

示例：
- 越界 3 格，damageBuffer = 5：距离 = -3 + 5 = 2，不受伤
- 越界 10 格，damageBuffer = 5，damagePerBlock = 0.2：距离 = -10 + 5 = -5，伤害 = max(1, floor(5 * 0.2)) = 1

## 使用方法

### 在 ServerWorld 中使用

```cpp
// ServerWorld 已集成 WorldBorder
ServerWorld world;
auto& border = world.worldBorder();

// 设置边界
border.setSize(1000.0);
border.setCenter(0.0, 0.0);

// 在 tick 中更新（自动处理过渡动画）
world.tick();  // 内部调用 border.tick()
```

### 命令系统

```cpp
/worldborder set <size> [time]     // 设置边界大小
/worldborder add <distance> [time] // 增加边界大小
/worldborder center <x> <z>        // 设置边界中心
/worldborder get                   // 获取边界大小
/worldborder damage amount <value> // 设置每格伤害
/worldborder damage buffer <value> // 设置伤害缓冲
/worldborder warning time <seconds>  // 设置警告时间
/worldborder warning distance <blocks> // 设置警告距离
```

## 网络同步

通过 `WorldBorderPacket` 同步到客户端：

| Action | 说明 | 数据 |
|--------|------|------|
| SetSize | 立即设置大小 | size |
| LerpSize | 渐变设置大小 | oldSize, newSize, timeMs |
| SetCenter | 设置中心 | x, z |
| Initialize | 完整初始化 | 所有参数 |
| SetWarningTime | 设置警告时间 | warningTime |
| SetWarningDistance | 设置警告距离 | warningDistance |
| SetDamageBuffer | 设置伤害缓冲 | damageBuffer |
| SetDamagePerBlock | 设置每格伤害 | damagePerBlock |

## 参考

- MC 1.16.5 `net.minecraft.world.border.WorldBorder`
- MC 1.16.5 `net.minecraft.network.play.server.SWorldBorderPacket`
