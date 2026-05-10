# 经验球实体模块 (Orb Entities)

本目录包含经验球实体的实现。

## 目录结构

```
orb/
├── ExperienceOrbEntity.hpp    # 经验球实体头文件
├── ExperienceOrbEntity.cpp    # 经验球实体实现
└── README.md                  # 本文档
```

## ExperienceOrbEntity

经验球实体，玩家拾取后获得经验值。

### 核心特性

| 特性 | 说明 |
|------|------|
| 最大经验值 | 2477 |
| 存活时间 | 6000 ticks (5分钟) |
| 追踪范围 | 8 格 |
| 拾取延迟 | 0 ticks (构造函数默认值，生成时通常设为10) |
| 拾取距离 | 1 格 |

### 行为

1. **物理运动**
   - 受重力影响（0.03 加速度）
   - 水中浮力：上升速度上限 0.06，水平摩擦 0.99
   - 岩浆中：随机运动 + 上升速度 0.2
   - 地面摩擦：滑度 0.6 * 0.98，Y轴反弹系数 -0.9
   - 空气摩擦：0.98

2. **玩家追踪**
   - 缓存机制：每 `20 + entityId % 100` ticks 搜索一次玩家
   - 在 8 格范围内检测最近玩家
   - 使用 `EntityUtils::findClosestEntity<Player>()` 搜索
   - 吸引力公式：`(1 - dist/8)² * 0.1`
   - Y轴偏移：使用 `eyeHeight / 2` 计算目标点
   - 1 格内触发拾取

3. **合并**
   - 两个经验球在 1 格内可合并
   - 合并后经验值相加
   - 不超过最大值 2477

### 经验球大小

经验球有 11 种大小等级，根据经验值决定：

| 经验值范围 | 大小等级 |
|-----------|---------|
| 2477+ | 10 |
| 1237-2476 | 9 |
| 617-1236 | 8 |
| 307-616 | 7 |
| 149-306 | 6 |
| 73-148 | 5 |
| 37-72 | 4 |
| 17-36 | 3 |
| 7-16 | 2 |
| 3-6 | 1 |
| 1-2 | 0 |

### 使用示例

```cpp
// 创建经验球
auto orb = std::make_unique<ExperienceOrbEntity>(world, x, y, z, 10);

// 设置经验值
orb->setXpValue(100);

// 获取大小等级
i32 size = orb->getOrbSize();  // 返回 0-10

// 分割经验（静态方法）
i32 split = ExperienceOrbEntity::getXPSplit(3000);  // 返回 2477
```

### 关系图

```mermaid
classDiagram
    class Entity {
        +tick()
        +baseTick()
        +position()
        +velocity()
    }

    class ExperienceOrbEntity {
        -m_xpValue: i32
        -m_age: i32
        -m_pickupDelay: i32
        -m_health: i32
        -m_trackingPlayer: Player*
        -m_tickCounter: i32
        -m_lastSearchTick: i32
        +getXpValue() i32
        +getOrbSize() i32
        +onCollideWithPlayer()
        +tryMergeWith() bool
        +getXPSplit() i32$
        -findNearestPlayer() Player*
        -updateMovement()
        -followNearestPlayer()
    }

    Entity <|-- ExperienceOrbEntity
    ExperienceOrbEntity --> Player : tracks
    ExperienceOrbEntity --> experience::ExperienceManager : gives xp to
    ExperienceOrbEntity ..> EntityUtils : uses findClosestEntity
```

## 与 MC 1.16.5 的对齐

| 特性 | MC 1.16.5 | 实现 |
|------|-----------|------|
| 拾取延迟默认值 | 0（构造函数不设置） | ✅ `m_pickupDelay = 0` |
| 玩家搜索缓存 | 每 `20 + entityId % 100` ticks | ✅ `m_tickCounter` / `m_lastSearchTick` |
| 水中浮力 | `vel.y = min(vel.y + 0.0005, 0.06)` | ✅ |
| 岩浆行为 | 随机运动 + 上升 0.2 | ✅ |
| 地面反弹 | Y轴 `-0.9` | ✅ |
| 地面滑度 | 使用脚下方块的 `getSlipperiness()` | ✅ 支持史莱姆块、冰块等 |
| 吸引Y偏移 | `eyeHeight / 2` | ✅ |
| 拾取音效 | `ENTITY_EXPERIENCE_ORB_PICKUP` | ✅ 音量 0.1，随机音调 |
| 附魔消耗 | 直接消耗，不检查 | ✅ `ExperienceManager::onEnchant()` |
| 负经验 | 触发降级 | ✅ `addExperience()` 支持 |

## 依赖项

- `../../core/Entity.hpp` - 实体基类
- `../../core/EntityUtils.hpp` - 实体工具函数（玩家搜索）
- `../experience/ExperienceConstants.hpp` - 经验常量
- `../experience/ExperienceUtils.hpp` - 经验工具函数
- `../experience/ExperienceManager.hpp` - 经验管理器
- `../../world/IWorld.hpp` - 世界接口

## 参考

- Minecraft 1.16.5: `net.minecraft.entity.ExperienceOrbEntity`
- Minecraft Wiki: [Experience Orb](https://minecraft.fandom.com/wiki/Experience_Orb)
