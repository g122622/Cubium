# 投掷物实体 (Projectile Entities)

本目录包含所有投掷物实体的实现。

## 目录结构

```
projectile/
├── ProjectileEntity.hpp/cpp       # 投掷物基类（发射、飞行、碰撞检测）
├── ThrowableEntity.hpp/cpp        # 可投掷物品基类（雪球、鸡蛋等）
├── AbstractArrowEntity.hpp/cpp    # 抽象箭矢基类 + ArrowEntity, SpectralArrowEntity
├── AbstractFireballEntity.hpp/cpp # 抽象火球基类 + FireballEntity, SmallFireballEntity, DragonFireballEntity, WitherSkullEntity
├── DamagingProjectileEntity.hpp/cpp # 带加速度的投掷物基类（火球类公共层）
├── ProjectileItemEntity.hpp/cpp   # 投掷物品基类 + SnowballEntity, EggEntity, EnderPearlEntity, PotionEntity, ExperienceBottleEntity
├── TridentEntity.hpp/cpp          # 三叉戟实体
├── WindChargeEntity.hpp/cpp       # 风弹实体
├── ProjectileHelper.hpp/cpp       # 投掷物辅助工具（朝向更新、移动搜索盒、最近实体命中检测）
├── OtherProjectiles.hpp/cpp       # 其他投掷物（LlamaSpitEntity, FishingBobberEntity, ShulkerBulletEntity, EvokerFangsEntity, EyeOfEnderEntity, FireworkRocketEntity）
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
│   ├── DamagingProjectileEntity  # 带加速度的投掷物基类
│   │   └── AbstractFireballEntity # 火球基类
│   │       ├── FireballEntity    # 恶魂火球
│   │       ├── SmallFireballEntity # 烈焰人火球
│   │       ├── DragonFireballEntity # 龙火球
│   │       └── WitherSkullEntity # 凋灵之首
│   ├── AbstractArrowEntity       # 抽象箭矢基类
│   │   ├── ArrowEntity           # 普通箭矢
│   │   └── SpectralArrowEntity   # 光灵箭
│   ├── TridentEntity             # 三叉戟
│   ├── WindChargeEntity          # 风弹
│   ├── LlamaSpitEntity           # 羊驼唾液
│   ├── ShulkerBulletEntity       # 潜影贝子弹
│   └── FireworkRocketEntity      # 烟花火箭
├── FishingBobberEntity           # 钓鱼浮标（独立实体）
├── EvokerFangsEntity             # 唤魔者尖牙（独立实体）
└── EyeOfEnderEntity              # 末影之眼（独立实体）
```

## 内部模块关系

```
                    ┌─────────────────┐
                    │ ProjectileEntity│
                    │   (投掷物基类)   │
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│ ThrowableEntity │ │DamagingProjectile│ │AbstractArrowEntity│
│ (可投掷物品)     │ │Entity(加速度投掷物)│ │   (箭矢类)       │
└────────┬────────┘ └────────┬────────┘ └─────────────────┘
         │                   │
         ▼                   ▼
┌─────────────────┐ ┌─────────────────┐
│ProjectileItemEntity│ │AbstractFireballEntity│
│ (投掷物品)        │ │   (火球类)       │
└─────────────────┘ └─────────────────┘
```

**核心类职责**：
- **ProjectileEntity**：投掷物基类，提供发射者追踪、射线追踪碰撞检测、重力和空气阻力、命中回调
- **ThrowableEntity**：可投掷物品基类，增加传送门检测逻辑
- **DamagingProjectileEntity**：带加速度的投掷物基类，火球类使用加速度而非速度驱动
- **AbstractArrowEntity**：箭矢基类，提供暴击、穿透附魔、拾取状态、插入方块状态
- **ProjectileHelper**：投掷物辅助工具，提供朝向更新、移动搜索盒、最近实体命中检测、获取武器持有手

## 上下游外部依赖关系

**上游依赖（本目录依赖的模块）**：
- `common/entity/core/Entity.hpp` - 实体基类
- `common/entity/core/LivingEntity.hpp` - 生物实体基类（发射者、伤害目标）
- `common/entity/damage/DamageSource.hpp` - 伤害来源
- `common/entity/effect/EffectInstance.hpp` - 药水效果
- `common/world/IWorld.hpp` - 世界接口（射线追踪、粒子、方块查询）
- `common/world/block/Block.hpp` - 方块状态
- `common/item/ItemStack.hpp` - 物品堆（拾取、药水箭）
- `common/util/math/Vector3.hpp` - 向量运算
- `common/util/math/random/Random.hpp` - 随机数生成

**下游依赖（依赖本目录的模块）**：
- `common/item/Item.hpp` 及其子类 - 物品发射投掷物（弓、弩、雪球、末影珍珠等）
- `common/entity/entities/monster/` - 怪物发射投掷物（骷髅射箭、恶魂发射火球等）
- `common/entity/entities/player/Player.hpp` - 玩家使用物品发射投掷物
- `client/renderer/trident/entity/renderer/projectile/` - 投掷物渲染器
- `server/world/ServerWorld.hpp` - 服务端世界生成投掷物实体

## 容易踩的坑

### 1. 三叉戟附魔系统

三叉戟与弓箭不同，不使用弓类附魔（力量、冲击、火焰）。三叉戟有四个专属附魔：

| 附魔 | 实现位置 | 效果 |
|------|---------|------|
| 忠诚 (Loyalty) | `TridentEntity::setItemStack()` | 投掷后自动返回 |
| 穿刺 (Impaling) | `TridentEntity::onEntityHit()` | 对水生生物额外伤害 |
| 引雷 (Channeling) | `TridentEntity::onEntityHit()` | 雷暴天气召唤闪电 |
| 激流 (Riptide) | `TridentItem::onPlayerStoppedUsing()` | 雨天/水中携带玩家冲刺 |

`TridentEntity::setBaseDamageFromMob()` 方法继承自 `AbstractArrowEntity`，但三叉戟只需计算基础伤害，不考虑弓类附魔。

### 2. 火球类粒子效果

所有火球类投掷物继承自 `DamagingProjectileEntity`，具有以下粒子效果：

**水下气泡粒子**（`DamagingProjectileEntity::spawnWaterParticles()`）：
- 触发条件：`isInWater()` 为 true
- 粒子数量：每 tick 生成 4 个气泡
- 位置计算：`pos - velocity * 0.25`

**拖尾粒子**（`DamagingProjectileEntity::spawnTrailParticles()`）：
| 实体类型 | 拖尾粒子 |
|---------|---------|
| FireballEntity | SMOKE |
| DragonFireballEntity | DRAGON_BREATH |
| WitherSkullEntity | SMOKE |

### 3. 烟花火箭伤害机制

仅当 `shotFromCrossbow = true` 时造成伤害：
- 基础伤害 = 5 + 爆炸效果数量 × 2
- 实际伤害 = 基础伤害 × sqrt((5 - distance) / 5)
- 视线检测：两条射线（脚部 y=0, 腰部 y=0.5×height）

### 4. 潜影贝子弹追踪算法

子弹沿轴向移动（X、Y、Z），每 step 选择最优轴向接近目标。速度公式：`velocity *= 1.025`（每tick加速2.5%）。

### 5. 钓鱼浮标状态机

| 状态 | 描述 |
|------|------|
| Flying | 飞行中，未入水 |
| Hooked | 钩住实体 |
| Bobbing | 浮在水面 |
| Fishing | 咬钩状态 |

**耐久消耗**：
| 情况 | 耐久消耗 |
|------|----------|
| 钩住物品实体 | 3 |
| 钩住其他实体 | 5 |
| 钓到鱼 | 1 |
| 落地 | 2 |

**开放水域检测**（`_checkOpenWater`）：
对应 MC Java `FishingHook.isOpenOrWaterAround`，采用分层检测算法：
- 以浮标为中心，检测 Y-1 到 Y+2 共 4 层的 5×5 区域
- 每个方块分类为三种 `WaterType`：
  - `AboveWater`：空气或睡莲方块
  - `InsideWater`：水源方块（空碰撞箱 + 水流体标签 + source）
  - `Invalid`：不满足以上条件的方块
- 每层内所有方块必须属于同一类型
- 层间过渡规则：`Invalid` 直接判定失败；`AboveWater` 前面不能有 `Invalid`；`InsideWater` 前面不能有 `AboveWater`
- 开放水域影响钓鱼宝藏表的概率

### 6. 箭矢伤害计算

箭矢伤害由两个独立方法控制：

- **`setBaseDamageFromMob(f32 power)`**：生物射出箭矢时调用，公式为 `power * 2.0 + triangle(difficulty * 0.11, 0.57425)`。骷髅、幻术师等怪物射箭时使用此方法。三叉戟重写此方法使用相同公式但默认伤害更高（8.0）。
- **`applyBowEnchantments(LivingEntity& shooter)`**：玩家射出箭矢时调用，读取射手主手武器的附魔等级：力量（每级 +0.5 伤害 + 基础 0.5）、冲击（每级 +1 击退强度）、火焰（着火 100 ticks）。BowItem 中调用此方法，怪物射箭不调用。

### 7. 箭矢拾取条件

箭矢拾取需要满足：
- 必须插在方块中或处于穿甲状态（noClip）
- 箭矢不能处于抖动状态
- PickupStatus 必须允许拾取
- Allowed 状态会检查背包空间，CreativeOnly 状态不检查背包

### 8. 投掷物发射者追踪

投掷物同时存储发射者的 UUID 和 Entity ID，用于跨区块追踪。投掷物需要离开发射者的碰撞箱才能伤害发射者。

### 9. 箭矢计数系统

箭矢命中生物实体时会增加目标身上的箭矢计数：
- 只有非穿透箭（`pierceLevel <= 0`）才增加计数
- 穿透箭会记录已穿透的实体 ID，避免重复命中
- 箭矢计数用于渲染层 `ArrowLayer` 显示插在身上的箭矢

### 10. 随机数生成规范

所有投掷物的随机数必须使用 `mc::math::Random`，严禁使用 `rand()` 或 `mt19937`：
```cpp
math::Random rng(seed);
i32 value = rng.nextInt(100);    // [0, 100)
f32 f = rng.nextFloat();          // [0.0, 1.0)
f32 g = rng.nextGaussian(0.0, 1.0); // 正态分布
```

发射时使用 `world->getRandom().nextGaussian()` 计算散布精度，符合 MC 1.16.5 投掷物不精确度计算：`inaccuracy * 0.0075 * nextGaussian()`。

### 11. 投射物传送门处理

`ThrowableEntity::tick()` 实现投射物的传送门检测逻辑：
- 下界传送门（NETHER_PORTAL）：设置 `setInPortal(true)` 和 `setPortalPos()`
- 末地折跃门（END_GATEWAY）：调用 `EndGatewayEntity::teleportEntity()` 立即传送
- 末地传送门（END_PORTAL）：由 `EndPortalBlock.onEntityCollision()` 直接传送

### 12. 重力和阻力系数

| 投掷物类型 | 重力 | 空气阻力 | 水中阻力 |
|-----------|------|---------|---------|
| ProjectileEntity（默认） | 0.03 | 0.99 | 0.8 |
| AbstractArrowEntity | 0.05 | 0.99 | 0.6 |
| DamagingProjectileEntity | 0.0 | - | - |

### 13. 碰撞检测

投掷物使用射线追踪进行碰撞检测：
1. **方块碰撞**：通过 `rayTraceBlocks` 检测与方块的碰撞
2. **实体碰撞**：通过 `rayTraceEntities` 检测与实体的碰撞
3. **碰撞处理**：调用 `onImpact`，分发到 `onEntityHit` 或 `onBlockHit`

**命中过滤**（`canHitEntity`）：
对应 MC Java `Projectile.canHitEntity`，过滤可命中的实体：
- 不可被弹射物命中的实体（`canBeHitByProjectile()` 返回 false）不可命中
  - `canBeHitByProjectile()` 默认为 `isAlive() && canBeCollidedWith()`（对应 MC Java 的 `isAlive() && isPickable()`）
  - `Player` 重写为 `!isSpectator() && Entity::canBeHitByProjectile()`，旁观者不可被弹射物命中
- 发射者未离开碰撞箱前，不能命中与发射者骑乘同一载具的实体（`isRidingSameEntity`）

### 14. EvokerFangsEntity Owner UUID 双重追踪

EvokerFangsEntity 使用双重追踪模式（缓存指针 + UUID）追踪 owner 实体，参考 AreaEffectCloudEntity 的模式：

| 方法 | 行为 |
|------|------|
| `setOwner(LivingEntity*)` | 同时设置缓存指针和 UUID，nullptr 时清空两者 |
| `getOwner()` | 非const：缓存有效直接返回，失效后通过 UUID 在 64 格范围 + 玩家列表中懒加载查找 |
| `owner() const` | 直接返回缓存指针，不触发懒加载 |
| `ownerUuid() const` | 返回 UUID 字符串（32字符十六进制） |
| `setOwnerUuid(const string&)` | 仅设置 UUID，清空指针（NBT 反序列化入口） |

**NBT 序列化键名**：`Warmup`（i32）、`OwnerUUIDMost`（i64）、`OwnerUUIDLeast`（i64）

**踩坑点**：
- `_damageEntities()` 必须使用 `getOwner()` 而非直接访问 `m_owner`，否则 owner 死亡/卸载后指针悬空导致崩溃
- `setOwner(nullptr)` 会同时清空 UUID 和指针，确保两者同步
- NBT 反序列化后指针为 nullptr，需调用 `getOwner()` 触发 UUID 懒加载查找
- 参考 MC 1.21.11 `EvokerFangs` 使用 `EntityReference<LivingEntity>` 追踪 owner

## 参考

- MC 1.16.5 ProjectileEntity
- MC 1.16.5 AbstractArrowEntity
- MC 1.16.5 ThrowableEntity
- MC 1.16.5 DamagingProjectileEntity
- MC 1.16.5 FishingBobberEntity
