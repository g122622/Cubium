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
| 拾取延迟 | 10 ticks |
| 拾取距离 | 1 格 |

### 行为

1. **物理运动**
   - 受重力影响（0.03 加速度）
   - 地面摩擦（0.98）
   - 在水中/岩浆中有浮力

2. **玩家追踪**
   - 在 8 格范围内检测最近玩家
   - 吸引力公式：`(1 - dist/8)² * 0.1`
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
        -m_trackingPlayer: Player*
        +getXpValue() i32
        +getOrbSize() i32
        +onCollideWithPlayer()
        +tryMergeWith() bool
        +getXPSplit() i32$
    }

    Entity <|-- ExperienceOrbEntity
    ExperienceOrbEntity --> Player : tracks
    ExperienceOrbEntity --> experience::ExperienceManager : gives xp to
```

## 依赖项

- `../../core/Entity.hpp` - 实体基类
- `../experience/ExperienceConstants.hpp` - 经验常量
- `../experience/ExperienceUtils.hpp` - 经验工具函数
- `../experience/ExperienceManager.hpp` - 经验管理器
- `../../world/IWorld.hpp` - 世界接口

## 参考

- Minecraft 1.16.5: `net.minecraft.entity.ExperienceOrbEntity`
- Minecraft Wiki: [Experience Orb](https://minecraft.fandom.com/wiki/Experience_Orb)
