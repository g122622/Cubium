# 水生生物模块

生活在水中的生物实体，包括鱼类、海豚、鱿鱼等。

## 目录结构

```text
src/common/entity/entities/passive/water/
├── WaterMobEntity.hpp  # 水生生物基类
├── WaterMobEntity.cpp  # 水生生物实现
├── DolphinEntity.hpp   # 海豚实体
├── DolphinEntity.cpp   # 海豚实现
├── SquidEntity.hpp     # 鱿鱼实体
├── SquidEntity.cpp     # 鱿鱼实现
└── README.md           # 本文档
```

## 核心类

### WaterMobEntity

水生生物的基类，提供水下生存能力：

**空气供应系统（MC 1.16.5 对齐）**
- `isInWater()` - 检测是否在水中（调用基类 Entity::isInWater()）
- `isInWaterOrBubble()` - 检测是否在水中或气泡柱中
- `updateAirSupply()` - 更新空气供应
  - 在水中：恢复空气至最大值
  - 水外：消耗空气，空气耗尽时造成溺水伤害（每 20 tick 1.0 伤害）
- `isDrowning()` - 是否正在溺水

**溺水伤害常量**
- 空气最大值：300 tick（15 秒）
- 溺水伤害间隔：20 tick（1 秒）
- 溺水伤害量：1.0f（玩家为 2.0f）

### DolphinEntity

海豚实体，海洋哺乳动物：

- 游泳行为：快速游泳，可跳出水面
- 宝藏寻找：喂食鱼后引导玩家到宝藏
- 救助行为：将溺水玩家推向水面
- 群居行为：形成小群体
- 掉落：生鳕鱼

### SquidEntity

鱿鱼实体，海洋无脊椎动物：

- 喷墨行为：受到攻击时喷出墨汁
- 游泳行为：在水中优雅游动
- 挣扎行为：离开水会扑腾
- 掉落：墨囊

## 模块关系

```
Entity
└── LivingEntity
    └── MobEntity
        └── CreatureEntity
            └── WaterMobEntity      ← 水生生物基类
                ├── DolphinEntity   ← 海豚
                └── SquidEntity     ← 鱿鱼
```

## 关键实现细节

### 溺水机制

```cpp
// WaterMobEntity::updateAirSupply()
if (!inWater) {
    m_airSupply--;
    if (m_airSupply <= -20) {
        m_airSupply = 0;
        m_drownDamageTimer++;
        if (m_drownDamageTimer >= 20) {
            m_drownDamageTimer = 0;
            hurt(DamageSources::drown(), 1.0f);
        }
    }
} else {
    m_airSupply = m_maxAirSupply;
    m_drownDamageTimer = 0;
}
```

### 与玩家的区别

| 特性 | WaterMobEntity | Player |
|------|----------------|--------|
| 空气最大值 | 300 tick | 300 tick |
| 溺水伤害 | 1.0f | 2.0f |
| 溺水间隔 | 20 tick | 20 tick |
| 效果免疫 | 无 | WaterBreathing/ConduitPower |

## 测试用例

目前水生生物模块的测试通过 Entity 基类测试覆盖：
- [tests/common/test_entity_physics.cpp](../../../../../../tests/common/test_entity_physics.cpp)

## 待实现功能

- 气泡柱检测（需要 Blocks::BUBBLE_COLUMN 实现）
- 海豚 AI 目标（需要 AI 系统完善）
- 鱿鱼 AI 目标（需要 AI 系统完善）
- 喷墨粒子效果（需要粒子系统）

## 参考

- MC 1.16.5 WaterMobEntity / WaterCreatureEntity
- MC 1.16.5 DolphinEntity
- MC 1.16.5 SquidEntity
