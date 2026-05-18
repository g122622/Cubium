# 投掷物实体 (Projectile Entities)

本目录包含所有投掷物实体的实现。

## 目录结构

```
projectile/
├── ProjectileEntity.hpp/cpp       # 投掷物基类
├── ThrowableEntity.hpp/cpp        # 可投掷物品基类
├── AbstractArrowEntity.hpp/cpp    # 抽象箭矢基类 + ArrowEntity, SpectralArrowEntity
├── AbstractFireballEntity.hpp/cpp # 抽象火球基类 + FireballEntity, SmallFireballEntity, DragonFireballEntity, WitherSkullEntity
├── ProjectileItemEntity.hpp/cpp   # 投掷物品基类 + SnowballEntity, EggEntity, EnderPearlEntity, PotionEntity, ExperienceBottleEntity
├── TridentEntity.hpp/cpp          # 三叉戟实体
├── OtherProjectiles.hpp/cpp       # 其他投掷物 (LlamaSpitEntity, FishingBobberEntity, ShulkerBulletEntity, EvokerFangsEntity, EyeOfEnderEntity, FireworkRocketEntity)
└── README.md                      # 本文档
```

## 继承层次

```
Entity (core/Entity.hpp)
├── ProjectileEntity              # 投掷物基类
│   ├── ThrowableEntity           # 可投掷物品基类
│   │   └── ProjectileItemEntity  # 投掷物品基类
│   │       ├── SnowballEntity    # 雪球
│   │       ├── EggEntity         # 鸡蛋
│   │       ├── EnderPearlEntity  # 末影珍珠
│   │       ├── PotionEntity      # 药水
│   │       └── ExperienceBottleEntity # 经验瓶
│   ├── AbstractArrowEntity       # 抽象箭矢基类
│   │   ├── ArrowEntity           # 普通箭矢
│   │   ├── SpectralArrowEntity   # 光灵箭
│   │   └── TridentEntity         # 三叉戟
│   ├── AbstractFireballEntity    # 抽象火球基类
│   │   ├── FireballEntity        # 恶魂火球
│   │   ├── SmallFireballEntity   # 烈焰人火球
│   │   ├── DragonFireballEntity  # 龙火球
│   │   └── WitherSkullEntity     # 凋灵之首
│   ├── LlamaSpitEntity           # 羊驼唾液
│   ├── ShulkerBulletEntity       # 潜影贝子弹
│   └── FireworkRocketEntity      # 烟花火箭
├── FishingBobberEntity           # 钓鱼浮标 (独立实体)
├── EvokerFangsEntity             # 唤魔者尖牙 (独立实体)
└── EyeOfEnderEntity              # 末影之眼 (独立实体)
```

## 实体列表

### 箭矢类 (Arrows)

| 实体 | 伤害 | 特性 |
|------|------|------|
| ArrowEntity | 2.0 (可变) | 弓/弩发射，可拾取，可附魔 |
| SpectralArrowEntity | 2.0 | 光灵箭，使目标发光 |
| TridentEntity | 8.0 | 可投掷/近战，忠诚附魔可返回 |

#### 三叉戟附魔系统 (2026-05-17)

**重要说明**：三叉戟与弓箭不同，不使用弓类附魔（力量、冲击、火焰）。

三叉戟有四个专属附魔，实现位置如下：

| 附魔 | 实现位置 | 效果 |
|------|---------|------|
| **忠诚 (Loyalty)** | `TridentEntity::setItemStack()` | 投掷后自动返回，等级影响返回速度 |
| **穿刺 (Impaling)** | `TridentEntity::onEntityHit()` | 对水生生物造成额外伤害（每级 +2.5） |
| **引雷 (Channeling)** | `TridentEntity::onEntityHit()` | 雷暴天气命中时召唤闪电 |
| **激流 (Riptide)** | `TridentItem::onPlayerStoppedUsing()` | 雨天/水中携带玩家冲刺 |

**为什么三叉戟不使用弓类附魔**：

1. **力量附魔**（Power）：只能应用于弓（`EnchantmentType::Bow`），不影响三叉戟伤害
2. **冲击附魔**（Punch）：只能应用于弓，不影响三叉戟击退
3. **火焰附魔**（Flame）：只能应用于弓，不影响三叉戟点燃

`TridentEntity::setEnchantmentEffectsFrom()` 方法继承自 `AbstractArrowEntity`，但三叉戟只需计算基础伤害，不考虑弓类附魔。这符合 MC 1.16.5 的设计——`TridentEntity.java` 中没有 `setEnchantmentEffectsFromEntity` 方法。

**代码参考**：
- `TridentEntity.cpp` 第307-328行：`setEnchantmentEffectsFrom()` 方法注释说明
- `TridentEntity.cpp` 第196-203行：穿刺附魔伤害计算
- `TridentEntity.cpp` 第240-266行：引雷附魔闪电召唤

### 火球类 (Fireballs)

| 实体 | 伤害 | 爆炸 | 特性 |
|------|------|------|------|
| FireballEntity | 6.0 | 是 (威力1) | 恶魂发射，爆炸产生火焰 |
| SmallFireballEntity | 5.0 | 否 | 烈焰人发射，点燃目标5秒，可点燃方块 |
| DragonFireballEntity | 无直接伤害 | 否 | 末影龙发射，生成龙息区域效果云 |
| WitherSkullEntity | 8.0/5.0 | 是 | 凋灵发射，凋零II效果，目标死亡治疗发射者 |

#### 火球类粒子效果 (2026-05-17)

**MC 1.16.5 粒子效果实现**：

所有火球类投掷物继承自 `DamagingProjectileEntity`，具有以下粒子效果：

**水下气泡粒子**（`DamagingProjectileEntity::spawnWaterParticles()`）：
- 触发条件：`isInWater()` 为 true
- 粒子数量：每 tick 生成 4 个气泡
- 粒子类型：`ParticleTypeId::Bubble`
- 位置计算：`pos - velocity * 0.25`（沿运动反方向偏移）
- 速度：与实体运动速度相同

**拖尾粒子**（`DamagingProjectileEntity::spawnTrailParticles()`）：
- 触发条件：每 tick 调用
- 位置：实体位置 Y+0.5 偏移
- 速度：(0, 0, 0) - 粒子静止

| 实体类型 | 拖尾粒子 | Y轴偏移 | 粒子速度 |
|---------|---------|--------|---------|
| DamagingProjectileEntity (基类) | SMOKE | +0.5 | (0,0,0) |
| FireballEntity | SMOKE (继承) | +0.5 | (0,0,0) |
| DragonFireballEntity | **DRAGON_BREATH** | +0.5 | (0,0,0) |
| WitherSkullEntity | SMOKE (继承) | +0.5 | (0,0,0) |

**参考**: MC 1.16.5 `DamagingProjectileEntity.tick()` 第 88-98 行

#### 火球类实现详情 (2026-05-16)

**FireballEntity (恶魂火球)**：
- ✅ `onEntityHit`: 调用 `LivingEntity::hurt()` 造成 6.0 伤害
- ✅ `onBlockHit`: 触发爆炸（半径 1.0，产生火焰）
- ✅ 使用 `DamageSources::fireball()` 创建火焰投射物伤害来源
- ✅ 支持 `mobGriefing` 游戏规则控制爆炸模式

**SmallFireballEntity (烈焰人火球)**：
- ✅ `onEntityHit`: 造成 5.0 伤害，点燃目标 5 秒
- ✅ 支持 `isImmuneToFire()` 检查
- ✅ 伤害失败时恢复原燃烧时间
- ✅ `onBlockHit`: 在碰撞方块上方放置火焰方块（受 `mobGriefing` 控制）

**DragonFireballEntity (末影龙火球)**：
- ✅ 生成龙息区域效果云 (`AreaEffectCloudEntity`)
- ✅ 云参数：半径 3.0，持续时间 600 ticks，逐渐扩展到 7.0
- ✅ 添加瞬间伤害 II 效果

**WitherSkullEntity (凋灵之首)**：
- ✅ 有发射者时造成 8.0 投射物伤害，无发射者时造成 5.0 魔法伤害
- ✅ 难度相关凋零效果：
  - 简单难度：无效果
  - 普通难度：凋零 II 10 秒 (200 ticks)
  - 困难难度：凋零 II 40 秒 (800 ticks)
- ✅ 目标死亡时治疗发射者 5.0 HP
- ✅ 触发爆炸（半径 1.0，不产生火焰）
- ✅ 蓝色凋灵之首运动因子 0.73（普通 0.95）

### 投掷物品类 (Throwable Items)

| 实体 | 伤害 | 特性 |
|------|------|------|
| SnowballEntity | 0 (烈焰人3) | 无伤害，击退 |
| EggEntity | 0 | 12.5%孵化小鸡 |
| EnderPearlEntity | 5 (传送伤害) | 传送发射者 |
| PotionEntity | 变化 | 药水效果 |
| ExperienceBottleEntity | 0 | 释放经验球 |

### 其他投掷物

| 实体 | 用途 | 实现状态 |
|------|------|----------|
| LlamaSpitEntity | 羊驼攻击狼 | ⏳ 框架完成 |
| FishingBobberEntity | 钓鱼机制 | ✅ 完整实现 |
| ShulkerBulletEntity | 潜影贝跟踪攻击 | ✅ 完整实现 |
| EvokerFangsEntity | 唤魔者召唤尖牙 | ✅ 完整实现（含队伍伤害检查）|
| EyeOfEnderEntity | 寻找要塞 | ⏳ 框架完成 |
| FireworkRocketEntity | 烟花/弩弹药 | ✅ 爆炸伤害已实现 |

#### 烟花火箭 (FireworkRocketEntity) 详细实现 (2026-05-16)

烟花火箭是从弩发射的投射物，可以对周围实体造成爆炸伤害。

**核心特性**:
| 特性 | 值 |
|------|-----|
| 宽度/高度 | 0.25f / 0.25f |
| 爆炸半径 | 5.0 格 |
| 基础伤害 | 5.0 点 |
| 每爆炸效果增加 | +2 点伤害 |
| 距离衰减 | damage × sqrt((5 - distance) / 5) |

**伤害机制** (MC 1.16.5):
- 仅当 `shotFromCrossbow = true` 时造成伤害
- 爆炸效果数量从物品 NBT `Fireworks.Explosions` 数组大小获取
- 基础伤害 = 5 + 爆炸效果数量 × 2
- 实际伤害 = 基础伤害 × sqrt((5 - distance) / 5)
- 视线检测：两条射线（脚部 y=0, 腰部 y=0.5×height），任一未被方块阻挡即可造成伤害

**NBT 数据读取**:
```cpp
// 设置烟花物品
firework.setFireworkItem(itemStack);

// 从 NBT 读取
// Fireworks.Flight: 飞行时间（1-3）
// Fireworks.Explosions: 爆炸效果数组
i32 flightTime = firework.flightTime();
i32 explosionCount = firework.getExplosionCount();
```

**CrossbowItem 集成**:
```cpp
// 在 CrossbowItem::fireProjectiles() 中
firework->setShotFromCrossbow(true);
firework->setFireworkItem(projectile);  // 传递烟花物品数据
```

**参考**: MC 1.16.5 `FireworkRocketEntity.dealExplosionDamage()`

#### 烟花火箭粒子效果实现 (2026-05-16)

烟花火箭具有两种粒子效果：飞行粒子和爆炸粒子。

**飞行粒子** (`tick()` 中生成):
| 参数 | 值 |
|------|-----|
| 生成频率 | 每 2 ticks 一次 |
| 粒子类型 | Firework |
| 生成位置 | 火箭位置向下偏移 0.3 格 |
| 水平速度 | 高斯分布 × 0.05 |
| 垂直速度 | -velocity.y × 0.5 |
| 执行条件 | 仅客户端 (`isClientSide()`) |

**爆炸粒子** (`explode()` 中生成):

无爆炸效果的烟花火箭：
| 参数 | 值 |
|------|-----|
| 粒子类型 | Poof |
| 数量 | 2-4 个（随机） |
| 速度 | 高斯分布 × 0.05 |

有爆炸效果的烟花火箭：
| 参数 | 值 |
|------|-----|
| Flash 粒子 | 1 个，火箭位置 |
| Firework 粒子 | 20-40 个，球形分布 |
| Poof 粒子 | 5-10 个，高斯分布 |

**球形分布算法**:
```cpp
// 生成球面均匀分布的粒子
for (i32 i = 0; i < fireworkCount; ++i) {
    f32 theta = rng.nextFloat() * 2.0f * PI;           // 方位角
    f32 phi = std::acos(2.0f * rng.nextFloat() - 1.0f); // 极角
    f32 speed = rng.nextFloat() * 0.5f;                 // 速度大小
    
    Vector3 velocity(
        speed * std::sin(phi) * std::cos(theta),
        speed * std::sin(phi) * std::sin(theta),
        speed * std::cos(phi)
    );
    m_world->addParticle(ParticleTypeId::Firework, pos, velocity);
}
```

**随机数生成器**:
使用 `createRandomFromEntity()` 辅助函数创建确定性随机数生成器：
```cpp
math::Random createRandomFromEntity(const Entity& entity) {
    u64 seed = static_cast<u64>(entity.id()) << 32 | 
               static_cast<u64>(entity.ticksExisted());
    return math::Random(seed);
}
```

**参考**: MC 1.16.5 `FireworkRocketEntity.tick()`, `FireworkParticle.Starter`

#### 潜影贝子弹 (ShulkerBulletEntity) 详细实现

潜影贝子弹是潜影贝发射的追踪子弹，具有独特的轴向移动机制。

**核心特性**:
| 特性 | 值 |
|------|-----|
| 宽度/高度 | 0.3125f / 0.3125f |
| 伤害 | 4.0 |
| 漂浮效果 | 200 ticks (10秒) |
| 速度 | 0.15 (基础) × 1.025^steps |

**追踪算法** (MC 1.16.5):
```cpp
void ShulkerBulletEntity::updateFlight() {
    // 1. 获取目标位置
    Vector3d targetPos = m_target->position();
    
    // 2. 计算到目标的方向增量
    m_targetDelta = (targetPos - position()).normalize() * 0.15;
    
    // 3. 根据当前方向更新速度
    if (m_direction == Direction::Up || m_direction == Direction::Down) {
        // Y轴移动：保持XZ平面方向
        m_velocity.x += m_targetDelta.x * 0.05;
        m_velocity.z += m_targetDelta.z * 0.05;
    } else {
        // 水平移动：保持Y方向
        m_velocity.y += m_targetDelta.y * 0.05;
    }
    
    // 4. 加速
    m_velocity *= 1.025;
}
```

**方向选择**:
- 子弹沿轴向移动（X、Y、Z三个轴）
- 每 step 选择最优轴向接近目标
- 碰撞方块时重新选择方向

**命中效果**:
- 对目标造成 4 点魔法伤害
- 施加漂浮效果（Levitation，200 ticks = 10秒）
- 播放 ENTITY_SHULKER_BULLET_HIT 音效

**碰撞检测**:
- 可以被玩家击中（canBeCollidedWith = true）
- 命中方块时重新选择方向继续移动
- 命中实体时应用伤害和效果

**参考**: MC 1.16.5 ShulkerBulletEntity

#### 钓鱼浮标 (FishingBobberEntity) 详细实现 (2026-05-16)

钓鱼浮标是钓鱼竿的投射物，控制钓鱼机制的完整流程。

**核心状态机**:
| 状态 | 描述 |
|------|------|
| `Flying` | 飞行中，未入水 |
| `Hooked` | 钩住实体 |
| `Bobbing` | 浮在水面 |
| `Fishing` | 咬钩状态 |

**钓鱼掉落表集成**:
```cpp
// 钓鱼掉落表结构
minecraft:gameplay/fishing (主表)
├── minecraft:gameplay/fishing/fish (鱼表, 权重85, 质量-1)
│   ├── cod (鳕鱼, 权重60)
│   ├── salmon (鲑鱼, 权重25)
│   ├── tropical_fish (热带鱼, 权重2)
│   └── pufferfish (河豚, 权重13)
├── minecraft:gameplay/fishing/junk (垃圾表, 权重10, 质量-2)
│   ├── leather_boots (皮革靴, 权重12)
│   ├── leather (皮革, 权重10)
│   ├── bone (骨头, 权重10)
│   └── ... (其他垃圾物品)
└── minecraft:gameplay/fishing/treasure (宝藏表, 权重5, 质量2)
    ├── name_tag (命名牌, 权重17, 需开放水域)
    ├── saddle (鞍, 权重10, 需开放水域)
    ├── bow (弓, 权重15, 需开放水域)
    └── ... (其他宝藏物品)
```

**开放水域检测**:
- 浮标周围 5x4x5 区域检查
- 水面上方层：必须是空气或睡莲
- 水层：必须是水源方块
- 宝藏物品只有在开放水域才能钓到

**海之眷顾附魔效果**:
- 每级增加 1 点幸运值（海之眷顾附魔等级）
- 幸运值影响掉落表的选择概率
- 质量(weight + luck * quality)用于加权随机

**幸运值计算** (2026-05-17):
```cpp
// MC 1.16.5: .withLuck((float)this.luck + playerentity.getLuck())
f32 totalLuck = luckBonus + player.getAttributeValue(Attributes::LUCK, 0.0);
```
- `luckBonus`: 海之眷顾附魔等级（0-3）
- `player.getAttributeValue(LUCK)`: 玩家基础幸运属性（受幸运/霉运药水影响）
- 幸运值影响钓鱼掉落表的概率权重

**经验球生成**:
```cpp
// MC 1.16.5 经验分割算法
static constexpr i32 XP_SPLIT_VALUES[] = {1, 3, 7, 17, 37, 79, 169, 347, 703, 1415};
// 总经验值 1-6，分割成多个经验球
```

**收杆流程** (`reelIn()`):
1. 检查当前状态
2. 如果在钓鱼状态，调用 `spawnCatchItems()` 生成物品和经验
3. 如果在钩住实体状态，调用 `bringInHookedEntity()` 拉动实体，返回耐久消耗（物品=3，其他=5）
4. 返回耐久消耗值

**钩住实体功能** (2026-05-16):

钓鱼浮标可以钩住实体，包括物品实体和普通实体。

**核心成员变量**:
- `m_caughtEntity`: 被钩住的实体指针
- `m_caughtEntityId`: 被钩住实体ID（用于网络同步，存储时+1，0表示无）

**可钩住实体** (`canHitEntity`):
- 物品实体 (`ItemEntity`)
- 普通可碰撞实体（不包括钓鱼者自己）

**状态转换**:
```
Flying → (命中实体) → Hooked
Hooked → (实体移除) → Flying
Hooked → (收杆) → 移除浮标
```

**钩住实体后行为**:
- 浮标速度清零
- 浮标位置跟随实体（Y偏移 = 实体高度 × 0.8）
- 实体被移除时浮标恢复 Flying 状态

**拉动算法** (`bringInHookedEntity`):
```cpp
// MC 1.16.5 FishingBobberEntity.bringInHookedEntity()
Vector3d direction = angler.position() - bobber.position();
direction *= 0.1;  // 缩放到10%
caughtEntity->addVelocity(direction.x, direction.y, direction.z);
```

**耐久消耗**:
| 情况 | 耐久消耗 |
|------|----------|
| 钩住物品实体 | 3 |
| 钩住其他实体 | 5 |
| 钓到鱼 | 1 |
| 落地 | 2 |
| 未咬钩收杆 | 0 |

**参考**: MC 1.16.5 FishingBobberEntity

## 核心类设计

### ProjectileEntity

投掷物基类，提供：
- 发射者追踪（UUID + Entity ID）
- 射线追踪碰撞检测
- 重力和空气阻力
- 命中回调（onEntityHit, onBlockHit）

```cpp
class ProjectileEntity : public Entity {
public:
    // 发射
    void shoot(f32 x, f32 y, f32 z, f32 velocity, f32 inaccuracy);
    void shootFrom(Entity& shooter, f32 pitch, f32 yaw, f32 pitchOffset, f32 velocity, f32 inaccuracy);

    // 命中处理
    virtual void onEntityHit(const RayTraceResult& result);
    virtual void onBlockHit(const RayTraceResult& result);
    virtual void onImpact(const RayTraceResult& result);

    // 物理
    virtual f32 getGravity() const;
    virtual f32 getAirDrag() const;
    virtual f32 getWaterDrag() const;
};
```

### AbstractArrowEntity

箭矢特有功能：
- 暴击系统
- 穿透附魔
- 拾取状态
- 插入方块状态

```cpp
class AbstractArrowEntity : public ProjectileEntity {
public:
    // 箭矢属性
    f32 damage() const;
    void setDamage(f32 damage);
    bool isCritical() const;
    void setCritical(bool critical);
    u8 pierceLevel() const;
    void setPierceLevel(u8 level);
    PickupStatus pickupStatus() const;
    void setPickupStatus(PickupStatus status);

    // 附魔效果
    void setEnchantmentEffectsFrom(LivingEntity& shooter, f32 baseVelocity);
};
```

### AbstractFireballEntity

火球特有功能：
- 加速度驱动（非速度）
- 持续追踪
- 爆炸威力

```cpp
class AbstractFireballEntity : public ProjectileEntity {
public:
    // 加速度
    f32 accelerationX() const;
    f32 accelerationY() const;
    f32 accelerationZ() const;
    void setAcceleration(f32 x, f32 y, f32 z);

    // 火球不受重力
    f32 getGravity() const override { return 0.0f; }
};
```

## 使用示例

### 创建箭矢

```cpp
// 从射手创建箭矢
auto arrow = ArrowEntity::createFromShooter(shooter, world);
arrow->shoot(target.x - shooter.x,
             target.y - shooter.y,
             target.z - shooter.z,
             1.5f,  // 速度
             0.0f); // 精度
world->spawnEntity(std::move(arrow));
```

### 创建火球

```cpp
// 恶魂发射火球
auto fireball = std::make_unique<FireballEntity>(id);
fireball->setPosition(ghast.x, ghast.y + ghast.eyeHeight, ghast.z);
fireball->setShooter(&ghast);
fireball->setAcceleration(
    (target.x - ghast.x) * 0.1f,
    (target.y - ghast.y) * 0.1f,
    (target.z - ghast.z) * 0.1f
);
world->spawnEntity(std::move(fireball));
```

### 创建雪球

```cpp
// 玩家投掷雪球
auto snowball = std::make_unique<SnowballEntity>(id);
snowball->setPosition(player.x, player.y + player.eyeHeight - 0.1f, player.z);
snowball->setShooter(&player);
snowball->shootFrom(player, player.pitch(), player.yaw(), 0.0f, 1.5f, 0.0f);
world->spawnEntity(std::move(snowball));
```

## 碰撞检测

投掷物使用射线追踪进行碰撞检测：

1. **方块碰撞**：通过 `rayTraceBlocks` 检测与方块的碰撞
2. **实体碰撞**：通过 `rayTraceEntities` 检测与实体的碰撞
3. **碰撞处理**：调用 `onImpact`，根据碰撞类型分发到 `onEntityHit` 或 `onBlockHit`

## 伤害系统

投掷物使用 `IndirectEntityDamageSource` 创建伤害来源：

```cpp
auto damageSource = std::make_unique<IndirectEntityDamageSource>(
    DamageType::Arrow,  // 伤害类型
    shooter,            // 伤害来源（射箭者）
    this,               // 直接来源（箭矢实体）
    isPlayer            // 是否来自玩家
);
```

### 箭矢计数系统（MC 1.16.5）

箭矢命中生物实体时会增加目标身上的箭矢计数：

```cpp
// AbstractArrowEntity::onEntityHit() 中
LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(target);
if (livingTarget != nullptr) {
    bool hurt = livingTarget->hurt(*damageSource, static_cast<f32>(damage));
    // 只有非穿透箭在造成伤害后才增加箭矢计数
    if (hurt && m_pierceLevel <= 0) {
        livingTarget->setArrowCountInEntity(livingTarget->getArrowCount() + 1);
    }
}
```

**计数规则**：
- 只有非穿透箭（`pierceLevel <= 0`）才增加计数
- 穿透箭会记录已穿透的实体 ID，避免重复命中
- 箭矢计数用于渲染层 `ArrowLayer` 显示插在身上的箭矢

## 注意事项

1. **发射者追踪**：投掷物同时存储发射者的UUID和Entity ID，用于跨区块追踪
2. **离开发射者检测**：投掷物需要离开发射者的碰撞箱才能伤害发射者
3. **重力**：不同投掷物有不同的重力值（箭矢0.05，雪球0.03，火球0）
4. **阻力**：水中阻力0.6-0.8，空气中阻力0.99
5. **穿透**：只有箭矢支持穿透附魔，需要追踪已穿透的实体
6. **拾取机制**：箭矢拾取需要满足以下条件：
   - 必须插在方块中或处于穿甲状态（noClip）
   - 箭矢不能处于抖动状态
   - PickupStatus 必须允许拾取
   - Allowed 状态会检查背包空间，CreativeOnly 状态不检查背包

## 参考

- MC 1.16.5 ProjectileEntity
- MC 1.16.5 AbstractArrowEntity
- MC 1.16.5 ThrowableEntity
- MC 1.16.5 FireballEntity
## 最新补充
- 已补 `DamagingProjectileEntity.hpp/.cpp`，承接 1.16.5 火球类加速度投掷物公共层。
- 已补 `ProjectileHelper.hpp/.cpp`，当前已接入朝向更新、移动搜索盒和最近实体命中检测。
- `AbstractFireballEntity` 已改为继承 `DamagingProjectileEntity`。
- `ProjectileEntity` 已接入方块/实体射线追踪、发射者过滤和基础碰撞边界语义。
- `ProjectileItemEntity` 已完善实现（2026-05-02）：
  - SnowballEntity：对烈焰人造成3点伤害，粒子效果完整
  - EggEntity：12.5%概率孵化小鸡，粒子效果完整
  - EnderPearlEntity：传送发射者并造成5点摔落伤害
  - ExperienceBottleEntity：生成3-11个经验球实体，粒子效果完整
  - PotionEntity：框架已就绪，待药水系统完善后实现效果应用
- 投掷物品(ThrowableItem)已实现完整的createProjectile实体生成逻辑。
- 水中粒子效果已完善实现（2026-05-08）：
  - AbstractArrowEntity：水中每tick生成4个气泡粒子（参考MC 1.16.5 第239-244行）
  - TridentEntity：返回状态下的三叉戟在水中生成气泡轨迹
  - ThrowableEntity：水中移动时生成气泡粒子
  - ProjectileItemEntity：气泡粒子已在ThrowableEntity中处理，无需重复
  - FishingBobberEntity：钓鱼粒子效果完整（水花、气泡、涟漪）
  - 新增FishingParticle粒子类型用于水面涟漪效果
- **箭矢拾取功能已完善实现（2026-05-09）**：
  - `AbstractArrowEntity::onPlayerPickup()`：完整实现箭矢拾取逻辑
  - `AbstractArrowEntity::onCollideWithPlayer()`：玩家碰撞检测入口
  - `getArrowStack()`：新增纯虚方法，子类返回对应的物品堆
  - `ArrowEntity::getArrowStack()`：返回普通箭矢或药水箭
  - `SpectralArrowEntity::getArrowStack()`：返回光灵箭
  - `TridentEntity::getArrowStack()`：返回三叉戟物品
  - 拾取逻辑参考 MC 1.16.5 AbstractArrowEntity.onCollideWithPlayer()
  - 支持 PickupStatus 三种状态：Disallowed、Allowed、CreativeOnly
  - CreativeOnly 状态不检查背包空间（创造模式无限物品）
  - 拾取成功后播放 ENTITY_ITEM_PICKUP 音效
  - **碰撞检测集成（2026-05-09）**：
    - `Entity::onCollideWithPlayer()`：新增虚方法，默认无操作
    - `Player::checkEntityCollisions()`：玩家tick中检测附近实体碰撞
    - 搜索范围为玩家碰撞箱扩展1格（水平和垂直）
    - 自动调用附近实体的 `onCollideWithPlayer()` 方法
- **随机数生成规范化（2026-05-12）**：
  - `FishingBobberEntity`：发射时使用 `world->getRandom().nextGaussian()` 替代 `rand()`
  - 符合 MC 1.16.5 投掷物不精确度计算：`inaccuracy * 0.0075 * nextGaussian()`
  - 保证跨平台随机数一致性和可测试性
- **滞留药水区域效果云实现（2026-05-16）**：
  - `PotionEntity::onImpact()` 为滞留型药水添加 `AreaEffectCloudEntity` 创建逻辑
  - 参考 MC 1.16.5 `PotionEntity.makeAreaOfEffectCloud()`
  - 效果云参数：
    - 初始半径：3.0
    - radiusOnUse：-0.5（每次应用效果半径减少0.5格）
    - waitTime：10 ticks（0.5秒等待时间）
    - duration：600 ticks（30秒默认持续时间）
    - radiusPerTick：-radius/duration = -0.005
  - 效果持续时间：原持续时间的 1/4（MC 1.16.5 规则）
  - 设置拥有者为投射物发射者
  - 颜色从药水物品自动获取
- **光灵箭完整实现（2026-05-17）**：
  - `SpectralArrowEntity::tick()`：飞行时生成 InstantSpell 粒子效果
    - 仅客户端执行（`m_world->isClientSide()`）
    - 粒子位置：箭矢当前位置，速度为零
    - 参考 MC 1.16.5 SpectralArrowEntity.tick() 第31-36行
  - `SpectralArrowEntity::onEntityHit()`：命中生物时施加发光效果
    - 目标必须是 LivingEntity
    - 施加 EffectType::Glowing 效果，持续时间 200 ticks（10秒）
    - 参考 MC 1.16.5 SpectralArrowEntity.arrowHit() 第43-46行

- **药水箭完整实现（2026-05-16）**：
  - `ArrowEntity::tick()`：飞行时生成彩色粒子效果
    - 仅客户端执行（`m_world->isClientSide()`）
    - 使用 EntityEffect 粒子，颜色通过速度参数传递（RGB）
    - 每 tick 生成 2 个粒子，位置在箭矢周围随机偏移
    - 参考 MC 1.16.5 ArrowEntity.spawnPotionParticles()
  - `ArrowEntity::onEntityHit()`：命中生物时施加药水效果
    - 遍历 m_effects 向量，对目标施加所有药水效果
    - 参考 MC 1.16.5 ArrowEntity.arrowHit() 第210-215行
  - `ArrowEntity::getArrowStack()`：拾取时保留药水效果
    - 使用 `PotionUtils::setCustomEffects()` 设置效果到 NBT
    - 使用 `PotionUtils::setCustomPotionColor()` 设置自定义颜色
    - 参考 MC 1.16.5 ArrowEntity.getArrowStack() 第195-208行
  - `ArrowEntity` 成员变量：
    - `m_color`：药水箭颜色（ARGB 格式，默认 0xFFFFFFFF）
    - `m_effects`：药水效果列表
    - `m_glowing`：发光标记（预留）

- **箭矢碰撞检测完善（2026-05-16）**：
  - `AbstractArrowEntity::tick()`：使用 CollisionShape 精确检测箭矢是否在方块碰撞箱内
    - 替代之前的简化 `isSolid()` 检测
    - 使用 `blockState->getCollisionShape()` 获取碰撞形状
    - 使用 `AxisAlignedBB::contains()` 检测箭矢位置
    - 正确处理非完整方块（如台阶、楼梯、地毯等）

- **投射物传送门处理实现（2026-05-17）**：
  - `ThrowableEntity::tick()`：实现投射物的传送门检测逻辑
    - 参考 MC 1.16.5 ThrowableEntity.tick() 第56-69行
    - 下界传送门（NETHER_PORTAL）：设置 `setInPortal(true)` 和 `setPortalPos()`
    - 末地折跃门（END_GATEWAY）：调用 `EndGatewayEntity::teleportEntity()` 传送实体
    - 末地传送门（END_PORTAL）：不在此处理，由 EndPortalBlock.onEntityCollision() 直接传送
  - 传送门处理流程：
    1. 射线追踪检测方块碰撞
    2. 检查命中方块是否为传送门类型
    3. 对于下界传送门，设置实体传送门状态等待传送
    4. 对于末地折跃门，调用方块实体的传送方法立即传送
    5. 设置 `handledPortal = true` 阻止后续碰撞处理
  - 测试覆盖：`tests/entity/ThrowablePortalTest.cpp`

- **龙息火球粒子效果实现（2026-05-17）**：
  - `DragonFireballEntity::getParticleType()`：返回 `ParticleTypeId::DragonBreath`
    - 参考 MC 1.16.5 DragonFireballEntity.getParticle()
    - 龙息火球飞行时产生龙息粒子尾迹（而非普通火球的烟雾粒子）
  - `DragonFireballEntity::createDragonBreathCloud()`：创建龙息区域效果云
    - 生成 `AreaEffectCloudEntity`
    - 设置龙息粒子类型（`ParticleTypeId::DragonBreath`）
    - 参考 MC 1.16.5 DragonFireballEntity.func_213284_a()
  - 测试覆盖：`tests/common/entity/entities/projectile/DragonFireballCloudTest.cpp`

- **潜影贝子弹爆炸粒子实现（2026-05-17）**：
  - `ShulkerBulletEntity::onBlockHit()`：命中方块时生成爆炸粒子
    - 参考 MC 1.16.5 ShulkerBulletEntity.func_230299_a_()
    - 粒子类型：`ParticleTypeId::Explosion`
    - 粒子数量：2
    - 随机偏移范围：(0.2, 0.2, 0.2)
    - 初始速度：(0, 0, 0)
    - 通过 `IWorld::addParticle()` 广播粒子到客户端
  - 测试覆盖：`tests/common/entity/entities/projectile/ShulkerBulletParticleTest.cpp`
