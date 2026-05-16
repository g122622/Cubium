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
- 与玩家同游：跟随游泳玩家并给予"海豚的恩惠"效果
- 玩物品：拾取水中物品并扔出玩耍
- 救助行为：将溺水玩家推向水面
- 群居行为：形成小群体
- 掉落：生鳕鱼

**AI 目标（MC 1.16.5 优先级）**:

| 优先级 | 目标 | 说明 |
|--------|------|------|
| 0 | SwimGoal | 浮出水面呼吸 |
| 0 | FindWaterGoal | 寻找水源 |
| 1 | SwimToTreasureGoal | 游向宝藏（喂食鱼后触发） |
| 2 | SwimWithPlayerGoal | 与游泳玩家同游，给予海豚恩惠 |
| 4 | RandomSwimmingGoal | 随机游泳 |
| 4 | LookRandomlyGoal | 随机看向 |
| 5 | LookAtGoal | 看向玩家 |
| 5 | DolphinJumpGoal | 跳出水面 |
| 6 | MeleeAttackGoal | 近战攻击 |
| 8 | PlayWithItemsGoal | 玩物品 |
| 8 | FollowBoatGoal | 跟随玩家驾驶的船 |
| 9 | AvoidEntityGoal | 避开守卫者（待实现） |

**宝藏寻找系统**:

```cpp
// 设置鱼标记（玩家喂食后调用）
dolphin.setGotFish(true);

// 检查是否得到鱼
bool hasFish = dolphin.hasGotFish();

// 设置宝藏位置
dolphin.setTreasurePos(BlockPos(100, 50, 200));

// 清除宝藏目标
dolphin.clearTreasureTarget();
```

**海豚的恩惠效果**:

当玩家在水中游泳时，附近的海豚会给予玩家 `DolphinsGrace` 效果，提高游泳速度。

**导航辅助方法**:

```cpp
// 检查是否接近导航目标
bool close = dolphin.closeToTarget();

// 检查是否有路径
bool hasPath = dolphin.hasPath();

// 清除导航路径
dolphin.clearNavigationPath();

// 尝试移动到实体
dolphin.tryMoveToEntity(targetEntity, speed);
```

**空气储备**:

海豚有 4800 tick (4分钟) 的空气储备，远超普通水生生物的 300 tick。

### SquidEntity

鱿鱼实体，海洋无脊椎动物：

- 喷墨行为：受到攻击时喷出墨汁
- 游泳行为：在水中优雅游动
- 挣扎行为：离开水会扑腾
- AI 目标：随机游泳 (SquidMoveRandomGoal) 和逃跑 (SquidFleeGoal)
- 掉落：墨囊

**移动向量系统**:

鱿鱼使用自定义的移动向量系统进行游泳，而不是标准的导航系统。

```cpp
// 设置移动向量
void setMovementVector(f32 x, f32 y, f32 z);

// 检查是否有移动向量
bool hasMovementVector() const;
```

**AI 目标**:

| 优先级 | 目标 | 说明 |
|--------|------|------|
| 0 | SquidMoveRandomGoal | 随机游泳，始终可执行 |
| 1 | SquidFleeGoal | 受攻击时逃跑，水中 && 距离 < 10 格 |

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
- [tests/common/entity/DolphinEntityTest.cpp](../../../../../../tests/common/entity/DolphinEntityTest.cpp) - 海豚实体测试
- [tests/common/entity/DolphinGoalsTest.cpp](../../../../../../tests/common/entity/DolphinGoalsTest.cpp) - 海豚 AI 目标测试
- [tests/common/entity/SquidGoalsTest.cpp](../../../../../../tests/common/entity/SquidGoalsTest.cpp) - 鱿鱼目标和移动向量测试

## 已实现功能

### DolphinEntity（2026-05-15）
- ✅ `maxAir()`: 返回 4800 tick（4分钟）空气储备
- ✅ `hasGotFish()` / `setGotFish()`: 鱼标记系统
- ✅ `setTreasurePos()` / `getTreasurePos()` / `hasTreasureTarget()`: 宝藏目标管理
- ✅ `setGuidingPlayer()` / `isGuidingPlayer()`: 引导玩家状态
- ✅ `closeToTarget()` / `hasPath()` / `clearNavigationPath()`: 导航辅助方法
- ✅ `DolphinJumpGoal`: 海豚跳出水面跳跃行为
- ✅ `SwimToTreasureGoal`: 喂食后引导玩家到宝藏结构
- ✅ `SwimWithPlayerGoal`: 跟随游泳玩家并给予海豚恩惠效果
- ✅ `PlayWithItemsGoal`: 拾取水中物品并扔出玩耍
- ✅ `FollowBoatGoal`: 跟随玩家驾驶的船（2026-05-16）

### WaterMobEntity（2026-05-10）
- ✅ `isInWaterOrBubble()`: 检测实体是否在水中或气泡柱中，使用 `VanillaBlocks::BUBBLE_COLUMN` 检测气泡柱方块

### SquidEntity（2026-05-15）
- ✅ `setMovementVector()`: 设置移动向量
- ✅ `hasMovementVector()`: 检查是否有移动向量
- ✅ `SquidMoveRandomGoal`: 随机游泳目标
- ✅ `SquidFleeGoal`: 受攻击时逃跑目标

## 参考

- MC 1.16.5 WaterMobEntity / WaterCreatureEntity
- MC 1.16.5 DolphinEntity
- MC 1.16.5 SquidEntity
