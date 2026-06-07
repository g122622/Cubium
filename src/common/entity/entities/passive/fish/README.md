# 鱼类实体模块

## 目录结构

```text
fish/
├── AbstractFishEntity.hpp/cpp         # 所有鱼共享的基础游泳/离水扑腾语义
├── AbstractGroupFishEntity.hpp/cpp    # 群游鱼中间层（含招募和导航方法）
├── CodEntity.hpp/cpp                  # 鳕鱼
├── SalmonEntity.hpp/cpp               # 鲑鱼
├── PufferfishEntity.hpp/cpp           # 河豚（膨胀/中毒语义）
├── TropicalFishEntity.hpp/cpp         # 热带鱼
└── README.md                          # 本文档
```

## 内部模块关系

```
WaterMobEntity
└── AbstractFishEntity        # 基础层：游泳、离水扑腾、空气供应、FromBucket 标签
    ├── AbstractGroupFishEntity  # 中间层：群首引用、群体大小、跟随距离
    │   ├── CodEntity           # 鳕鱼（最大群体 8）
    │   ├── SalmonEntity        # 鲑鱼（最大群体 5）
    │   └── TropicalFishEntity  # 热带鱼
    └── PufferfishEntity       # 河豚（不群游，有膨胀/中毒机制）
```

## 上下游外部依赖关系

### 上游依赖（本目录依赖）

- `src/common/entity/entities/passive/water/WaterMobEntity.hpp` - 水生生物基类
- `src/common/entity/core/Entity.hpp` - 实体核心
- `src/common/entity/attribute/Attributes.hpp` - 属性系统
- `src/common/sound/SoundEvents.hpp` - 音效事件

### 下游依赖（依赖本目录）

- `src/common/entity/ai/goal/` - AI 目标（FishSwimGoal、FollowSchoolLeaderGoal、PuffGoal）
- `src/common/item/special/FishBucketItem.hpp` - 鱼桶物品（使用 FromBucket 标签）
- `src/server/world/spawn/` - 实体生成系统

## 容易踩的坑

1. **不要把群游字段塞回 AbstractFishEntity**
   - 这会让 PufferfishEntity 落到错误层次，河豚不应该有群游行为。

2. **SalmonEntity 的最大群体大小是 5，不是默认值 8**
   - vanilla 1.16.5 鲑鱼固定为 5，CodEntity 和 TropicalFishEntity 使用默认值 8。

3. **群游 AI 已完整实现**
   - `FollowSchoolLeaderGoal` 已接入，使用 `EntityUtils::findEntities<AbstractGroupFishEntity>()` 搜索附近鱼群。
   - 初始生成分组逻辑和桶/NBT 同步仍未实现。

4. **FromBucket 机制**
   - 从桶放出的鱼永远不会消失（`preventDespawn()` 返回 true，`canDespawn()` 返回 false）。
   - 有自定义名称的鱼也不会消失。

5. **河豚膨胀碰撞箱**
   - 河豚碰撞箱根据膨胀状态动态变化：Deflated(0.35)、SemiPuffed(0.49)、FullyPuffed(0.7)。
   - 膨胀状态通过 DataParameter 同步到客户端。
