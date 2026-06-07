# 末地怪物模块

末地（End）维度的怪物实现，包含末影人和潜影贝。

## 目录结构

```text
src/common/entity/entities/monster/end/
├── EndermanEntity.hpp       # 末影人实体声明
├── EndermanEntity.cpp       # 末影人实体实现
├── ShulkerEntity.hpp        # 潜影贝实体声明
├── ShulkerEntity.cpp        # 潜影贝实体实现
└── README.md                # 本文档
```

## 内部模块关系

```
MonsterEntity (敌对生物基类)
    ├── EndermanEntity (末影人)
    │       └── 实现 IAngerable 接口（愤怒管理）
    │       └── 依赖 EndermanGoals（AI 目标，在 ai/goal/goals/special/ 中）
    │       └── 依赖 Player::isLookingAt()（注视检测）
    └── ShulkerEntity (潜影贝)
            └── 发射 ShulkerBulletEntity（在 entities/projectile/ 中）
```

### EndermanEntity 核心特性

- **中立行为**：默认不攻击玩家，被注视眼睛时激怒
- **瞬移能力**：受攻击或特定条件下瞬移（64格范围）
- **方块搬运**：可拾取和放置方块
- **水敏感**：在水中或雨中受伤并瞬移逃离

### ShulkerEntity 核心特性

- **贝壳防御**：闭合时免疫投射物，获得 +20 护甲
- **悬浮攻击**：发射追踪子弹造成悬浮效果
- **附着方块**：固定在方块表面不移动
- **瞬移**：受伤时概率瞬移寻找新附着点

## 上下游外部依赖关系

### 上游依赖（本模块依赖的外部模块）

| 模块 | 说明 |
|------|------|
| `entity/core/MonsterEntity` | 敌对生物基类，提供敌对行为基础设施 |
| `entity/interfaces/IAngerable` | 愤怒接口，末影人实现此接口 |
| `entity/ai/goal/goals/special/EndermanGoals` | 末影人专用 AI 目标（注视、放方块、拾方块） |
| `entity/entities/projectile/ShulkerBulletEntity` | 潜影贝子弹，由潜影贝发射 |
| `world/World` | 世界接口，用于瞬移位置验证 |
| `Player` | 玩家类，提供注视检测方法 |
| `BlockState` | 方块状态，末影人搬运方块时使用 |

### 下游依赖（依赖本模块的外部模块）

| 模块 | 说明 |
|------|------|
| `entity/core/VanillaEntities` | 注册实体类型，创建末影人/潜影贝实例 |
| `world/spawn/MobSpawner` | 末地维度的怪物生成 |
| `client/renderer/entity/` | 客户端渲染器，渲染末影人和潜影贝模型 |

## 容易踩的坑

- **注视检测需要完整世界环境**：`shouldAttackPlayer()` 使用 `world.canSee()`，单元测试需要 mock
- **眼睛高度差异**：玩家眼睛高度 1.62，末影人眼睛高度 2.55，计算注视向量时务必使用 `getEyePosition()`
- **瞬移冷却**：瞬移后需等待 50 ticks 冷却，连续调用 `teleport()` 可能失败
- **愤怒状态同步**：`setScreaming()` 和 `setAngry()` 需要同步设置，否则客户端状态不一致
- **投射物伤害特殊处理**：末影人对投射物伤害尝试瞬移躲避（最多 64 次），成功则不受伤
- **非生物伤害瞬移**：末影人受非生物伤害（摔落、窒息、岩浆）后 90% 概率瞬移，受生物伤害不瞬移
- **潜影贝护甲计算**：闭合时额外 +20 护甲，需要在伤害计算时考虑
- **潜影贝碰撞箱**：会随开壳程度扩展，`getCollisionBorderSize()` 返回 0 需特殊处理
