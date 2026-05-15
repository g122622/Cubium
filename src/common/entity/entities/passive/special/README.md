# 特殊动物

包含特殊行为的被动/中立动物。

## 目录结构

```
special/
├── BeeEntity.hpp/cpp         # 蜜蜂
├── FoxEntity.hpp/cpp         # 狐狸
├── PandaEntity.hpp/cpp       # 熊猫
├── PolarBearEntity.hpp/cpp   # 北极熊
├── StriderEntity.hpp/cpp     # 炽足兽
├── TurtleEntity.hpp/cpp      # 海龟
└── README.md                 # 本文档
```

## 实体列表

| 实体 | 说明 | 接口 | 状态 |
|------|------|------|------|
| BeeEntity | 蜜蜂 | IFlyingAnimal | ✅ 完成 |
| FoxEntity | 狐狸 | - | ✅ 完成 |
| PandaEntity | 熊猫 | - | ✅ 完成 |
| PolarBearEntity | 北极熊 | - | ✅ 完成 |
| StriderEntity | 炽足兽 | IRideable | ✅ 完成 |
| TurtleEntity | 海龟 | - | ✅ 完成 |

## 蜜蜂 (BeeEntity)

### 特性
- 飞行行为（实现 `IFlyingAnimal`）
- 授粉机制
- 蜂巢记忆
- 群体攻击
- 螫刺后死亡

### 繁殖系统
- `isBreedingItem(itemStack)`: 检查物品是否可用于繁殖
  - 使用 `ItemTags::FLOWERS()` 标签检查
  - 支持17种花朵物品（小型13种 + 大型4种）
- `spawnBaby(partner)`: 创建幼蜂
  - 生成新的 BeeEntity 实例
  - 设置为幼体状态 (`setChild(true)`)
  - 位置设为父体位置附近

### 花粉与螫刺状态
- `hasNectar()` / `setHasNectar(bool)`: 花粉状态（携带花蜜）
- `hasStung()` / `setHasStung(bool)`: 螫刺状态（螫刺后死亡）
- `isFlying()` / `setFlying(bool)`: 飞行状态

### 水下溺水机制 (MC 1.16.5)
蜜蜂无法在水下呼吸，会逐渐溺水死亡：
- **成员变量**: `m_underWaterTimer` - 追踪在水中的时间
- **溺水逻辑**:
  ```cpp
  if (isInWater()) {
      ++m_underWaterTimer;
      if (m_underWaterTimer > 20 && m_world != nullptr) {
          auto damageSource = DamageSources::drown();
          hurt(damageSource, 1.0f);
      }
  } else {
      m_underWaterTimer = 0;
  }
  ```
- **时间线**:
  - 在水中超过 20 tick (1秒) 后开始溺水
  - 每 tick 造成 1.0 溺水伤害
  - 离开水后计时器重置为 0

### 螫刺后死亡机制 (MC 1.16.5)
蜜蜂螫刺后会逐渐死亡，实现逻辑：
- **成员变量**: `m_timeSinceSting` - 追踪螫刺后经过的 tick 数
- **死亡概率**: 每 5 tick 检查一次，概率随时间增加
  ```cpp
  // 计算公式: rand.nextInt(clamp(1200 - timeSinceSting, 1, 1200)) == 0
  i32 deathChance = math::clamp(1200 - m_timeSinceSting, 1, 1200);
  if (rng.nextInt(deathChance) == 0) {
      hurt(DamageSources::generic(), health());
  }
  ```
- **时间线**:
  - 螫刺后 0 tick: 1/1200 概率死亡 (约 0.08%)
  - 螫刺后 600 tick: 1/600 概率死亡 (约 0.17%)
  - 螫刺后 1000 tick: 1/200 概率死亡 (约 0.5%)
  - 螫刺后 1199+ tick: 100% 死亡
- **最长存活**: 1200 tick = 60 秒
- **伤害类型**: `DamageSources::generic()`，造成等于当前生命值的伤害

### 蜂巢与花朵系统
- `setHivePos(pos)` / `getHivePos()`: 蜂巢位置
- `hasHive()`: 是否有蜂巢
- `setFlowerPos(pos)` / `getFlowerPos()`: 花朵位置
- `hasFlower()`: 是否有花朵目标

### 愤怒系统 (IAngerable 接口)
- `isAngry()`: 是否愤怒
- `getAngerTime()` / `setAngerTime(time)`: 愤怒计时器
- `setAngry(angry)`: 设置愤怒状态
- `updateAnger()`: 更新愤怒计时器（每 tick 递减）

### 行为
| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 在水中游泳 |
| 1 | AttackGoal | 攻击目标 |
| 2 | EnterBeehiveGoal | 进入蜂巢 |
| 3 | FindPollinationTargetGoal | 寻找授粉目标 |
| 4 | FindBeehiveGoal | 寻找蜂巢 |
| 5 | TemptGoal | 被花朵诱惑 |
| 6 | FollowParentGoal | 幼体跟随父母 |

### 属性
| 属性 | 值 | 说明 |
|------|-----|------|
| MAX_HEALTH | 10.0 | 生命值 |
| MOVEMENT_SPEED | 0.3 | 移动速度 |
| FLYING_SPEED | 0.6 | 飞行速度 |
| 眼睛高度 | 0.3 | - |

### 测试覆盖
- `tests/common/entity/entities/passive/special/BeeEntityTest.cpp`
  - isBreedingItem 花朵检测测试（17种花朵 + 非花朵物品）
  - spawnBaby 幼体生成测试
  - 花粉/螫刺/飞行状态测试
  - 蜂巢/花朵位置测试
  - 属性和眼睛高度测试
  - IAngerable 愤怒系统测试

## 狐狸 (FoxEntity)

### 特性
- **皮肤类型**: 红色狐狸 (Red) 和雪地狐狸 (Snow) 两种变体
- **信任机制**: 最多信任 2 个玩家，信任玩家不会触发逃跑行为
- **繁殖系统**: 使用甜浆果繁殖，幼狐遗传父母皮肤类型和信任玩家
- **叼物品行为**: 可以叼起地上的物品
- **夜间活动**: 白天睡觉，晚上活动
- **潜行狩猎**: 跳跃攻击小动物

### 信任机制
- `trusts(playerId)`: 检查是否信任某玩家
- `addTrustedPlayer(playerId)`: 添加信任玩家（最多 2 个，超出时替换最早的）
- `removeTrustedPlayer(playerId)`: 移除信任玩家
- `getFirstTrustedPlayer()`: 获取第一个信任的玩家
- `getTrustedPlayers()`: 获取所有信任玩家列表

### 繁殖系统
- `isBreedingItem(itemStack)`: 检查物品是否可用于繁殖（仅甜浆果）
- `spawnBaby(partner)`: 创建幼狐
  - 50% 概率继承任一父母的皮肤类型
  - 继承父母的信任玩家（最多 2 个，按添加顺序）
  - 设置为幼体状态

### 叼物品系统
- `isHoldingItem()`: 是否叼着物品
- `getHeldItem()`: 获取叼着的物品
- `setHeldItem(item)`: 设置叼着的物品
- `dropHeldItem()`: 丢弃物品，在世界中生成物品实体（使用 ItemDropHelper）

### 行为目标 (MC 1.16.5)
| 优先级 | Goal | 说明 | 状态 |
|--------|------|------|------|
| 0 | SwimGoal | 在水中游泳 | ✅ |
| 1 | JumpGoal | 跳跃挣扎 | 待实现 |
| 2 | PanicGoal | 恐慌逃跑 | ✅ |
| 3 | MateGoal | 繁殖 | ✅ |
| 3 | TemptGoal | 甜浆果诱惑 | ✅ 已实现 |
| 4 | AvoidEntityGoal | 躲避未信任的玩家 | ✅ 已实现 |
| 4 | AvoidEntityGoal | 躲避狼 | 待实现 |
| 4 | AvoidEntityGoal | 躲避北极熊 | 待实现 |
| 5 | FollowTargetGoal | 追踪猎物 | 待实现 |
| 6 | PounceGoal | 扑击攻击 | 待实现 |
| 6 | FindShelterGoal | 寻找庇护所 | 待实现 |
| 7 | BiteGoal | 咬击攻击 | 待实现 |
| 7 | SleepGoal | 睡觉 | 待实现 |
| 8 | FollowGoal | 跟随父母 | ✅ |
| 9 | StrollGoal | 夜间村庄漫步 | 待实现 |
| 10 | EatBerriesGoal | 吃甜浆果 | 待实现 |
| 10 | LeapAtTargetGoal | 跳向目标 | 待实现 |
| 11 | WaterAvoidingRandomWalkingGoal | 避水随机漫步 | ✅ |
| 11 | FindItemsGoal | 寻找物品 | 待实现 |
| 12 | WatchGoal | 观察玩家 | 待实现 |
| 13 | SitAndLookGoal | 坐下环顾 | 待实现 |

### AvoidEntityGoal 配置 (MC 1.16.5)
- **躲避玩家**: 检测距离 16 格，近距速度 1.6，远距速度 1.4
- **躲避狼**: 检测距离 8 格（未驯服的狼）
- **躲避北极熊**: 检测距离 8 格
- **信任检查**: 信任的玩家、创造模式玩家、旁观者不躲避

### TemptGoal 配置 (MC 1.16.5)
- **跟随速度**: 1.0
- **诱惑物品**: 甜浆果 (Items::SWEET_BERRIES)
- **不被玩家移动吓跑**: false

### 属性
| 属性 | 值 | 说明 |
|------|-----|------|
| MAX_HEALTH | 10.0 | 生命值 |
| MOVEMENT_SPEED | 0.3 | 移动速度 |
| 成体眼睛高度 | 0.4 | - |
| 幼体眼睛高度 | 0.2 | - |

## 熊猫 (PandaEntity)

### 特性
- 7种性格基因
  - 普通 (Normal)
  - 懒惰 (Lazy)
  - 忧郁 (Worried)
  - 顽皮 (Playful)
  - 好斗 (Aggressive)
  - 虚弱 (Weak)
  - 棕色 (Brown)
- 打喷嚏机制（幼年熊猫）
  - 播放预喷嚏音效（第1 tick）
  - 播放喷嚏音效（完成时）
  - 生成喷嚏粒子
  - 让周围10格内的成年熊猫跳跃
  - 1/700概率掉落粘液球（需 doMobLoot 游戏规则）
- 打滚行为
- 吃竹子行为
- 躺下行为（懒惰熊猫）

### 基因系统 (MC 1.16.5)
熊猫有主基因 (mainGene) 和隐藏基因 (hiddenGene)，每个基因值为 0-5：
- 0: Normal（普通）
- 1: Lazy（懒惰）
- 2: Worried（忧愁）
- 3: Playful（顽皮）
- 4: Aggressive（好斗）
- 5: Weak（虚弱）
- 6: Brown（棕色）

**基因表达规则** (MC 1.16.5 Gene.func_221101_b())：
1. 如果主基因是好斗（Aggressive），直接表达好斗（显性）
2. 如果主基因是懒惰（Lazy）且隐藏基因是好斗，表达好斗
3. 否则表达主基因

```cpp
Personality calculateExpressedPersonality() const {
    if (mainGene == AGGRESSIVE) {
        return Personality::Aggressive;  // 显性基因
    }
    if (mainGene == LAZY && hiddenGene == AGGRESSIVE) {
        return Personality::Aggressive;  // 特殊组合
    }
    return static_cast<Personality>(mainGene);
}
```

**基因遗传**：
- 子代从父母各随机继承一个基因
- 1/32 概率发生基因变异（每个基因独立判定）

```cpp
void inheritGenesFromParents(PandaEntity* father, PandaEntity* mother) {
    // 随机决定哪个父母提供主基因/隐藏基因
    if (rng.nextBoolean()) {
        mainGene = father->getOneOfGenesRandomly(rng);
        hiddenGene = mother->getOneOfGenesRandomly(rng);
    } else {
        mainGene = mother->getOneOfGenesRandomly(rng);
        hiddenGene = father->getOneOfGenesRandomly(rng);
    }
    // 变异
    if (rng.nextInt(32) == 0) mainGene = rng.nextInt(0, 5);
    if (rng.nextInt(32) == 0) hiddenGene = rng.nextInt(0, 5);
    updatePersonalityFromGenes();
}
```

**性格生成概率** (MC 1.16.5):
- 普通: 32%
- 懒惰: 32%
- 忧愁: 16%
- 顽皮: 16%
- 好斗: 1.6%
- 虚弱: 0.08%
- 棕色: 2.4%

### 打喷嚏机制
```cpp
// 设置打喷嚏状态（AI Goal 会调用）
panda.setSneezing(true);
panda.setSneezeTimer(20); // 20 ticks = 1秒

// tick() 中自动处理:
// 1. 计时器递减
// 2. 第19 tick 播放预喷嚏音效
// 3. 计时器归零时调用 onSneezeComplete()
```

### 喷嚏完成效果 (onSneezeComplete)
1. 播放 `ENTITY_PANDA_SNEEZE` 音效
2. 在熊猫头部前方生成 `Sneeze` 粒子
3. 搜索周围10格内的成年熊猫，使其跳跃
4. 检查游戏规则 `doMobLoot`，1/700概率掉落粘液球

### 基因遗传
- 子代基因由父母基因随机决定
- 隐性基因（虚弱、棕色）需要双亲携带

## 北极熊 (PolarBearEntity)

### 特性
- **IAngerable 接口**: 实现愤怒管理系统，被攻击后会记住攻击者
- **站立警告**: 近距离威胁时会后腿站立并发出警告声
- **幼崽保护**: 成年熊会攻击靠近幼熊的玩家
- **游泳行为**: 擅长游泳
- **攻击反击**: 被攻击后会反击

### 接口
- 实现 `IAngerable` 接口

### 愤怒系统 (IAngerable)
```cpp
// 设置/获取攻击目标
void setAttackTarget(LivingEntity* target);
LivingEntity* getAttackTarget() const;

// 设置复仇目标（自动设置愤怒时间）
void setRevengeTarget(LivingEntity* target);

// 愤怒状态
bool isAngry() const;
void setAngry(bool angry);

// 愤怒时间
i32 getAngerTime() const;
void setAngerTime(i32 time);
void updateAnger();  // 每tick调用
```

### 行为
| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 在水中游泳 |
| 1 | PolarBearMeleeAttackGoal | 近战攻击（带站立警告） |
| 1 | PolarBearPanicGoal | 恐慌逃跑（仅幼熊或着火） |
| 4 | FollowParentGoal | 幼体跟随父母 |
| 5 | RandomWalkingGoal | 随机漫步 |
| 6 | LookAtGoal | 看向玩家 |
| 7 | LookRandomlyGoal | 随机看向 |

### 目标选择器
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | PolarBearHurtByTargetGoal | 被攻击后反击，幼熊呼唤成年熊 |
| 2 | PolarBearAttackPlayerGoal | 保护幼崽攻击玩家 |
| 3 | NearestAttackableTargetGoal<Player> | 有条件攻击玩家 |
| 4 | NearestAttackableTargetGoal<FoxEntity> | 攻击狐狸 |

### 声音事件
- 环境音效：成年熊和幼熊使用不同音效
- 受伤/死亡音效
- 脚步声
- 警告声（站立时播放）

### 属性
| 属性 | 值 | 说明 |
|------|-----|------|
| MAX_HEALTH | 30.0 | 生命值 |
| FOLLOW_RANGE | 20.0 | 跟随范围 |
| MOVEMENT_SPEED | 0.25 | 移动速度 |
| ATTACK_DAMAGE | 6.0 | 攻击伤害 |

### 常量
| 常量 | 值 | 说明 |
|------|-----|------|
| STAND_DURATION_MIN | 100 | 最小站立时间 (ticks) |
| STAND_DURATION_MAX | 400 | 最大站立时间 (ticks) |
| WARNING_SOUND_COOLDOWN | 40 | 警告声音冷却 (ticks) |
| ANGER_TIME_MIN | 20 | 最小愤怒时间 (ticks) |
| ANGER_TIME_MAX | 39 | 最大愤怒时间 (ticks) |

## 炽足兽 (StriderEntity)

### 特性
- **熔岩行走**: 在熔岩表面行走，不沉入熔岩
- **可骑乘**: 实现 `IRideable` 接口
- **温度敏感**: 离开熔岩会发抖、减速
- **鞍装备**: 需要鞍才能控制方向

### 骑乘速度设置
- 基础速度：`MOVEMENT_SPEED = 0.175`
- **骑乘速度**：
  - 正常状态：`speed * 0.55 = 0.09625`
  - 寒冷状态：`speed * 0.23 = 0.04025`
- **行走速度**：
  - 正常状态：`speed * 1.0 = 0.175`
  - 寒冷状态：`speed * 0.66 = 0.1155`
- 在 `travel()` 中根据寒冷状态选择速度乘数并调用 `setAIMoveSpeed()`
- 通过 `IRideable::ride()` 处理骑乘移动逻辑

### 接口实现
```cpp
class StriderEntity : public AnimalEntity, public entity::IRideable {
public:
    // IRideable 接口
    bool hasSaddle() const override;
    void setSaddle(bool saddle) override;
    void onPlayerStartRiding(Player* player) override;
    void onPlayerStopRiding(Player* player) override;
    f32 getSteeringSpeed() const override;
    bool boost() override;
    bool canBeSteered() const override;
    void travelTowards(const Vector3& travelVec) override;
};
```

### 熔岩行走机制
- 在熔岩表面时设置 `onGround = true`
- 温度系统：检测周围熔岩块
- 发抖动画：离开熔岩时播放
- 速度调整：离开熔岩时速度降低

### 网络同步
- 使用 `PlayerInputPacket` 同步玩家输入
- 使用 `MoveVehiclePacket` 同步载具位置
- 使用 `EntityActionPacket` 同步实体动作

## 海龟 (TurtleEntity)

### 特性
- 出生地记忆
- 产卵行为（完整实现）
- 幼崽成长
- 水陆两栖

### 繁殖系统
- `isBreedingItem(itemStack)`: 检查物品是否可用于繁殖
  - 仅接受海草（SEAGRASS）作为繁殖物品
  - 参考 MC 1.16.5: `item == Items.SEAGRASS`
- `canBreed()`: 检查是否可以繁殖
  - 继承父类 `AnimalEntity::canBreed()` 的检查（成体、非爱心状态）
  - 额外检查：`!hasEgg()` - 有蛋的海龟不能繁殖
  - 参考 MC 1.16.5: `return super.canBreed() && !this.hasEgg();`
- `spawnBaby(partner)`: 生成幼体
  - 创建新的 TurtleEntity 实例
  - 设置为幼体状态 (`setChild(true)`)
  - 继承父母的出生地 (`homePos`)
  - 位置设为父体位置

### 产卵系统
海龟产卵是完整的游戏循环：
1. **交配获得蛋**：喂食海草后，母海龟设置 `hasEgg = true`
2. **返回出生地**：有蛋的海龟会返回出生位置
3. **挖掘下蛋**：在沙子上进行 200 tick (10秒) 的挖掘动画
4. **放置海龟蛋**：调用 `layEgg()` 方法放置 TurtleEggBlock

### layEgg() 方法
```cpp
void TurtleEntity::layEgg();
```
**功能**：在当前位置下方放置海龟蛋方块

**前置条件检查**：
- 世界指针有效
- 脚下是沙子类方块（BlockTags::SAND: 沙子、红沙、灵魂沙）
- 当前位置是空气

**行为**：
- 随机生成 1-4 个蛋
- 放置 TurtleEggBlock 并设置 EGGS_1_4 属性
- 播放 ENTITY_TURTLE_LAY_EGG 音效（音量 0.3，音调 0.9-1.1）

**参考**：MC 1.16.5 `TurtleEntity.LayEggGoal.tick()`

### 移动物理 (travel方法)
海龟在水陆两栖环境中具有不同的移动速度：

**实现位置**：`TurtleEntity::travel()`

**速度调整**：
- **水中**：保持基础速度 0.25，并给予轻微上升动力 (+0.005 y)
  - 远离出生地超过 16 格时：速度减半，最低 0.08
  - 幼体在水中：速度降低为 1/3，最低 0.06
- **陆地**：速度减半，最低 0.06（约为水中速度的 24%）
- **空中**：保持当前 AI 速度（无额外调整）

**参考**：MC 1.16.5 `TurtleEntity.travel()` 和 `MoveHelperController.updateSpeed()`

### AI 目标系统
海龟具有完整的行为目标系统，实现了所有 MC 1.16.5 特有的 AI Goals。

#### 行为目标（按优先级）
| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | TurtlePanicGoal | 恐慌逃跑（优先寻找水源） |
| 1 | TurtleMateGoal | 繁殖（繁殖后获得蛋） |
| 1 | TurtleLayEggGoal | 产卵（有蛋时在出生地附近找沙地） |
| 2 | TurtleTemptGoal | 被海草诱惑 |
| 3 | TurtleGoToWaterGoal | 前往水源（陆地上的海龟找水） |
| 4 | TurtleGoHomeGoal | 返回出生地（有蛋或随机触发） |
| 7 | TurtleTravelGoal | 水中随机游泳 |
| 8 | LookAtGoal | 看向玩家 |
| 9 | TurtleWanderGoal | 陆地随机漫步 |

#### 各 Goal 详细说明

**TurtleGoHomeGoal（返回出生地）**
- 触发条件：
  - 有蛋时：检查是否有出生地
  - 无蛋时：1/700 概率检查，距离出生地超过 64 格触发
- 持续条件：距离出生地 > 7 格 AND 未放弃 AND 未超时（600 ticks）
- 行为：导航返回出生地，远离出生地超过 16 格时速度减半

**TurtleLayEggGoal（产卵）**
- 触发条件：有蛋 AND 距离出生地 ≤ 9 格 AND 找到合适沙地
- 行为：移动到沙地上方，开始 200 tick 的产卵动画
- 完成后：调用 `layEgg()` 放置 1-4 个海龟蛋方块

**TurtleTravelGoal（水中旅行）**
- 触发条件：不在回家状态 AND 没有蛋 AND 在水中
- 行为：在 512 格范围内随机游泳，保持在海平面以下（y ≤ 62）
- 特点：让海龟在海洋中自然游动

**TurtleGoToWaterGoal（前往水源）**
- 触发条件：
  - 幼龟：不在水中
  - 成龟：不在回家 AND 不在水中 AND 没有蛋
- 行为：搜索 24 格内的水源并导航过去
- 超时：1200 ticks

**TurtleMateGoal（繁殖）**
- 继承自 BreedGoal
- 额外条件：没有蛋才能繁殖
- 繁殖后：设置 `hasEgg = true`

**TurtlePanicGoal（恐慌逃跑）**
- 继承自 PanicGoal
- 特点：海龟恐慌时优先寻找水源

**TurtleTemptGoal（海草诱惑）**
- 继承自 TemptGoal
- 触发物品：仅海草（SEAGRASS）

**TurtleWanderGoal（陆地漫步）**
- 继承自 RandomWalkingGoal
- 触发条件：不在水中 AND 不在回家 AND 没有蛋
- 执行概率：1/100

### 状态管理
| 方法 | 说明 |
|------|------|
| `hasEgg()` / `setHasEgg(bool)` | 是否有蛋 |
| `isLayingEgg()` / `setLayingEgg(bool)` | 是否正在下蛋 |
| `startLayEgg()` | 开始下蛋动画（设置状态和计时器） |
| `getHomePos()` / `setHomePos(pos)` | 获取/设置出生位置 |
| `hasHomePos()` | 是否有出生位置 |
| `isGoingHome()` / `setGoingHome(bool)` | 是否正在回家 |
| `isTravelling()` / `setTravelling(bool)` | 是否正在旅行 |

### 常量
| 常量 | 值 | 说明 |
|------|-----|------|
| LAY_EGG_DURATION | 200 | 下蛋动画持续时间 (ticks) |
| 成体眼睛高度 | 0.4f | - |
| 幼体眼睛高度 | 0.2f | - |
| 步高 | 1.0f | 海龟可以走上1格高的方块 |

## 测试覆盖

测试文件位于 `tests/entity/` 和 `tests/common/entity/entities/passive/special/`，包含：
- **StriderEntityTest.cpp**: 炽足兽实体测试（23 个测试）
  - getMountedYOffset 计算测试（11 个测试）
    - 基础偏移计算
    - 公式验证（MC 1.16.5 一致性）
    - 波动幅度限制（limbSwingAmount clamp 到 0.25）
    - cos 周期性验证
    - 高度影响
    - 波动系数验证
    - 边界条件
    - 返回类型验证
  - 基本属性测试（12 个测试）
    - 寒冷状态、鞍状态、熔岩表面状态
    - 骑乘状态、加速状态、高度访问器
- **FoxEntityTest.cpp**: 狐狸实体测试
  - 狐狸类型测试（默认类型、设置/获取）
  - 信任系统测试（添加、移除、最多 2 个、去重）
  - 繁殖物品测试（接受甜浆果、拒绝其他物品）
  - spawnBaby 测试（创建幼狐、遗传类型、遗传信任、位置设置）
  - 属性测试（生命值、移动速度）
  - 眼睛高度测试（成体/幼体差异）
  - 睡眠状态测试
  - 叼物品功能测试
  - dropHeldItem 测试（在世界生成物品实体、空物品不崩溃）
  - AI Goal 注册测试
    - AvoidEntityGoal 注册验证（优先级 4）
    - TemptGoal 注册验证（优先级 3）
    - 基础 AI 目标验证
- **PolarBearEntityTest.cpp**: 北极熊实体测试（20 个测试）
  - 基本属性测试
    - 构造函数默认值
    - 成体/幼体眼睛高度差异
    - 不可繁殖验证
  - 站立状态测试
    - 设置/清除站立状态
    - 站立计时器设置
  - 警告状态测试
    - 设置/获取警告状态
  - IAngerable 接口测试
    - 设置愤怒状态
    - 愤怒时间范围验证（20-39 ticks）
    - 直接设置愤怒时间
    - 设置攻击目标
    - 清除复仇目标
  - 声音事件测试
    - 成体/幼体环境音效差异
    - 受伤音效
    - 死亡音效
  - 属性测试
    - 生命值、跟随范围、移动速度
  - 随机性测试
    - 愤怒时间随机变化
    - 站立计时器随机变化
- **PandaEntityTest.cpp**: 熊猫实体测试（29 个测试）
  - 性格测试
    - 随机生成有效性格
    - 性格枚举值验证
  - 性格访问器测试
    - set/getPersonality
    - isLazy/isAggressive/isPlayful/isWorried/isWeak/isBrown
  - 状态测试
    - set/getSneezing
    - set/getSneezeTimer
    - set/getRolling
    - set/getEating
    - set/getLying
  - 眼睛高度测试
    - 成体眼睛高度 (1.2f)
    - 幼体眼睛高度 (0.6f)
  - 音效常量测试
    - 所有熊猫音效事件定义验证
  - 打喷嚏核心逻辑测试 (onSneezeComplete)
    - 播放喷嚏音效验证
    - 生成喷嚏粒子验证
    - 无世界时不崩溃验证
    - 粒子位置验证（头部附近）
    - 成年熊猫跳跃集成测试
  - 基因系统测试 (7 个测试)
    - getAndSetMainGene/getAndSetHiddenGene
    - calculateExpressedPersonality 好斗显性测试
    - calculateExpressedPersonality 懒惰+好斗组合测试
    - calculateExpressedPersonality 普通主基因测试
    - getOneOfGenesRandomly 随机选择测试
    - updatePersonalityFromGenes 更新测试
  - spawnBaby 测试 (5 个测试)
    - 创建幼体熊猫
    - 位置设置
    - 基因遗传（双亲）
    - 基因遗传（单亲）
- **BeeEntityTest.cpp**: 蜜蜂实体测试（37 个测试）
  - isBreedingItem 花朵检测测试（17 种花朵 + 非花朵物品）
  - spawnBaby 幼体生成测试
    - 创建幼蜂
    - 位置设置
  - 花粉/螫刺/飞行状态测试
  - 蜂巢/花朵位置测试
  - 属性测试
    - MAX_HEALTH (10.0)
    - MOVEMENT_SPEED (0.3)
    - FLYING_SPEED (0.6)
    - ATTACK_DAMAGE (2.0)
    - FOLLOW_RANGE (48.0)
  - 眼睛高度测试 (0.3f)
  - IAngerable 愤怒系统测试
    - setAngerTime/getAngerTime
    - setAngry/isAngry
    - 清除愤怒测试
  - 水下溺水计时器测试
    - 初始状态为 0
- 海龟出生地记忆测试
- 海龟繁殖系统测试
  - isBreedingItem 海草检测测试
  - canBreed 有蛋时返回 false 测试
  - spawnBaby 幼体生成测试
  - spawnBaby 出生地继承测试
