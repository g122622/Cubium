# 水生生物模块

生活在水中的生物实体，包括鱼类、海豚、鱿鱼、美西螈等。

## 目录结构

```text
water/
├── WaterMobEntity.hpp/cpp   # 水生生物基类（反逻辑溺水系统）
├── DolphinEntity.hpp/cpp    # 海豚（宝藏寻找、海豚恩惠、跳跃）
├── AxolotlEntity.hpp/cpp    # 美西螈（装死、支援效果[再生I+移除挖掘疲劳]、桶装）
├── SquidEntity.hpp/cpp      # 鱿鱼（喷墨、移动向量游泳）
├── GlowSquidEntity.hpp/cpp  # 发光鱿鱼（继承鱿鱼；GLOW 粒子、受击暗化、荧光墨汁）
└── README.md                # 本文档
```

## 内部模块关系

```
Entity
└── LivingEntity
    └── MobEntity
        └── CreatureEntity
            └── WaterMobEntity      ← 水生生物基类（反逻辑溺水）
                ├── DolphinEntity   ← 海豚（空气4800tick）
                ├── AxolotlEntity   ← 美西螈（空气6000tick）
                ├── SquidEntity     ← 鱿鱼
                │   └── GlowSquidEntity  ← 发光鱿鱼
                └── fish/
                    └── AbstractFishEntity  ← 鱼类基类
```

## 上下游外部依赖关系

### 上游依赖（本目录依赖）

- `entity/core/CreatureEntity.hpp` - 生物基类
- `entity/ai/goal/` - AI 目标系统（SwimGoal, FindWaterGoal, RandomSwimmingGoal, LookAtGoal, LookRandomlyGoal 等）
- `entity/attribute/Attributes.hpp` - 属性系统（MAX_HEALTH, MOVEMENT_SPEED）
- `world/IWorld.hpp` - 世界接口
- `block/VanillaBlocks.hpp` - 方块定义（BUBBLE_COLUMN 气泡柱检测）
- `sound/SoundEvents.hpp` - 音效事件

### 下游依赖（依赖本目录）

- `entity/entities/passive/fish/` - 鱼类继承 WaterMobEntity
- `entity/VanillaEntities.hpp` - 实体类型注册
- `server/world/spawn/` - 实体生成系统
- `client/renderer/entity/` - 实体渲染

## 容易踩的坑

### 1. 反逻辑溺水系统

WaterMobEntity 的溺水逻辑与陆地生物相反：
- **在水中**：恢复空气到最大值
- **在陆地上**：消耗空气，空气降到 -20 时造成溺水伤害

| 特性 | WaterMobEntity | LivingEntity (陆地生物) | Player |
|------|----------------|------------------------|--------|
| 空气最大值 | 300 tick（可重写） | 300 tick | 300 tick |
| 在水中 | 恢复空气 | 消耗空气 | 消耗空气 |
| 在陆地上 | 消耗空气 | 恢复空气 | 恢复空气 |
| 溺水伤害 | 1.0f | 2.0f | 2.0f |
| 溺水间隔 | 20 tick | 20 tick | 20 tick |

实现新的水生生物时务必正确调用基类的 `updateAirSupply()`。

### 2. 空气储备差异

不同水生生物的最大空气储备不同：
- **WaterMobEntity 默认**：300 tick（15秒）
- **DolphinEntity**：4800 tick（4分钟）
- **AxolotlEntity**：6000 tick（5分钟）
- **AbstractFishEntity**：依赖具体实现

重写 `maxAir()` 方法来设置不同的空气储备。

### 3. 鱿鱼使用移动向量而非导航系统

SquidEntity 不使用标准的导航系统，而是通过 `setMovementVector()` 直接控制移动向量。这是MC原版的行为，不要尝试为鱿鱼添加标准导航目标。

### 4. 发光鱿鱼同步数据与构造顺序

- `GlowSquidEntity` 继承 `SquidEntity`，新增 `DarkTicksRemaining` 同步数据参数（受击暗化 100 tick）。
- **构造函数必须显式调用 `registerData()`**：C++ 虚函数在基类构造函数中不会动态派发到派生类，基类构造期间调用的 `registerData()` 只会调用基类版本，因此 GlowSquidEntity 的同步参数必须在派生类构造函数中显式注册。
- `SquidEntity::getInkParticle()` / `getSquirtSound()` 是虚函数钩子，发光鱿鱼重写以返回 `GlowSquidInk` 粒子与 `ENTITY_GLOW_SQUID_SQUIRT` 音效。
- `SquidEntity::hurt()` 受击成功且存在复仇目标时触发 `sprayInk()`（对应 MC Java `Squid.hurtServer`）。

### 5. 美西螈的特殊行为

- **装死**：水中受击 33% 概率触发，持续 200 tick，期间获得再生 I 效果
- **支援效果**：当美西螈攻击的目标被玩家击杀且玩家在 20 格范围内时，给予玩家再生 I（基础 100 tick + 现有剩余，上限 2400 tick）并移除挖掘疲劳。使用 `EffectInstance::endsWithin()` 判断是否刷新现有效果
- **狩猎冷却**：击杀目标后 2 分钟（2400 tick）不攻击鱼类和鱿鱼，但仍攻击溺尸和守卫者
- **桶装持久化**：来自桶的美西螈 `preventDespawn()` 返回 true，不会消失
- **繁殖食物**：热带鱼桶，不继承 AnimalEntity 的繁殖系统

### 6. AI 目标优先级

优先级数字越小越优先执行。子类必须调用父类 `registerGoals()`，否则丢失基础行为。

### 7. 海豚宝藏寻找系统

- 使用 `setGotFish(true)` 标记玩家喂食状态
- 使用 `setTreasurePos()` 设置宝藏目标位置
- 引导持续时间 1200 tick（60秒）
