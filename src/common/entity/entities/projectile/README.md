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

### 火球类 (Fireballs)

| 实体 | 伤害 | 爆炸 | 特性 |
|------|------|------|------|
| FireballEntity | 6.0 | 是 (威力1) | 恶魂发射 |
| SmallFireballEntity | 5.0 | 否 | 烈焰人发射，点燃目标 |
| DragonFireballEntity | 12.0 | 否 | 末影龙发射，生成龙息 |
| WitherSkullEntity | 8.0 | 是 | 凋灵发射，凋零效果 |

### 投掷物品类 (Throwable Items)

| 实体 | 伤害 | 特性 |
|------|------|------|
| SnowballEntity | 0 (烈焰人3) | 无伤害，击退 |
| EggEntity | 0 | 12.5%孵化小鸡 |
| EnderPearlEntity | 5 (传送伤害) | 传送发射者 |
| PotionEntity | 变化 | 药水效果 |
| ExperienceBottleEntity | 0 | 释放经验球 |

### 其他投掷物

| 实体 | 用途 |
|------|------|
| LlamaSpitEntity | 羊驼攻击狼 |
| FishingBobberEntity | 钓鱼机制 |
| ShulkerBulletEntity | 潜影贝跟踪攻击 |
| EvokerFangsEntity | 唤魔者召唤尖牙 |
| EyeOfEnderEntity | 寻找要塞 |
| FireworkRocketEntity | 烟花/弩弹药 |

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
auto fireball = std::make_unique<FireballEntity>(LegacyEntityType::Unknown, id);
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
auto snowball = std::make_unique<SnowballEntity>(LegacyEntityType::Unknown, id);
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
