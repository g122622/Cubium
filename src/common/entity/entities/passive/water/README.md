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

水生生物的基类，继承自 CreatureEntity，实现反逻辑溺水系统（在陆地上溺水，在水中恢复）。

**空气供应系统（MC 1.16.5 对齐）**

水生生物使用基类 `LivingEntity` 的空气管理接口：
- `air()` / `setAir(i32)` - 获取/设置当前空气值
- `maxAir()` - 获取最大空气值（默认 300 tick = 15 秒）
- `isDrowning()` - 是否正在溺水（空气值 <= 0）

**便捷方法（委托到基类）**
- `getAirSupply()` - 委托到 `air()`
- `setAirSupply(i32)` - 委托到 `setAir()`
- `getMaxAirSupply()` - 委托到 `maxAir()`

**溺水机制（反逻辑）**
- 在水中：空气恢复到最大值
- 在陆地上：消耗空气，空气降到 -20 时造成溺水伤害
- 溺水伤害间隔：20 tick
- 溺水伤害量：1.0f（玩家为 2.0f）

```cpp
// WaterMobEntity::updateAirSupply() override
void WaterMobEntity::updateAirSupply() {
    if (!isAlive()) return;

    bool inWater = isInWater();

    if (inWater) {
        // 在水中恢复空气
        setAir(maxAir());
        m_drownDamageTimer = 0;
    } else {
        // 在陆地上消耗空气
        setAir(decreaseAirSupply(air()));
        if (air() <= -20) {
            setAir(0);
            m_drownDamageTimer++;
            if (m_drownDamageTimer >= DROWN_DAMAGE_INTERVAL) {
                m_drownDamageTimer = 0;
                hurt(DamageSources::drown(), DROWN_DAMAGE_AMOUNT);
            }
        }
    }
}
```

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
            └── WaterMobEntity      ← 水生生物基类（反逻辑溺水）
                ├── DolphinEntity   ← 海豚
                ├── SquidEntity     ← 鱿鱼
                └── fish/
                    └── AbstractFishEntity  ← 鱼类（更长的空气储备）
```

## 与陆地生物的溺水对比

| 特性 | WaterMobEntity | LivingEntity (陆地生物) | Player |
|------|----------------|------------------------|--------|
| 空气最大值 | 300 tick | 300 tick | 300 tick |
| 在水中 | 恢复空气 | 消耗空气 | 消耗空气 |
| 在陆地上 | 消耗空气 | 恢复空气 | 恢复空气 |
| 溺水伤害 | 1.0f | 2.0f | 2.0f |
| 溺水间隔 | 20 tick | 20 tick | 20 tick |
| 效果免疫 | 无 | WaterBreathing/ConduitPower | WaterBreathing/ConduitPower/创造模式 |

## 测试用例

- [tests/entity/LivingEntityTests.cpp](../../../../../../tests/entity/LivingEntityTests.cpp) - 基类溺水测试
- [tests/common/entity/PlayerSwimTest.cpp](../../../../../../tests/common/entity/PlayerSwimTest.cpp) - 玩家游泳和溺水测试
- [tests/common/test_entity_physics.cpp](../../../../../../tests/common/test_entity_physics.cpp) - 物理常量测试

## 待实现功能

- 海豚 AI 目标（需要 AI 系统完善）
- 鱿鱼 AI 目标（需要 AI 系统完善）

## 已实现功能

### DolphinEntity（2026-05-10）
- ✅ `canJumpOutOfWater()`: 检查海豚是否接近水面（上方有空气）
- ✅ `isFoodItem()`: 检测鳕鱼、鲑鱼、河豚、热带鱼

### WaterMobEntity（2026-05-10）
- ✅ `isInWaterOrBubble()`: 检测实体是否在水中或气泡柱中，使用 `VanillaBlocks::BUBBLE_COLUMN` 检测气泡柱方块

### 测试用例

- [tests/common/entity/DolphinEntityTest.cpp](../../../../../../tests/common/entity/DolphinEntityTest.cpp) - 海豚实体测试

## 参考

- MC 1.16.5 WaterMobEntity / WaterCreatureEntity
- MC 1.16.5 DolphinEntity
- MC 1.16.5 SquidEntity
