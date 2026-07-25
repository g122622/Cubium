# 投掷物实体 (Projectile Entities)

本目录包含所有投掷物实体的实现。

## 目录结构

```
projectile/
├── ProjectileEntity.hpp/cpp       # 投掷物基类（发射、飞行、碰撞检测、偏转）
├── ProjectileDeflection.hpp/cpp   # 弹射物偏转类型与逻辑（None/Reverse/AimDeflect/MomentumDeflect）
├── ThrowableEntity.hpp/cpp        # 可投掷物品基类（雪球、鸡蛋等）
├── AbstractArrowEntity.hpp/cpp    # 抽象箭矢基类 + ArrowEntity, SpectralArrowEntity
├── AbstractFireballEntity.hpp/cpp # 抽象火球基类 + FireballEntity, SmallFireballEntity, DragonFireballEntity, WitherSkullEntity
├── DamagingProjectileEntity.hpp/cpp # 带加速度的投掷物基类（火球类公共层）
├── ProjectileItemEntity.hpp/cpp   # 投掷物品基类 + SnowballEntity, EggEntity, EnderPearlEntity, PotionEntity, ExperienceBottleEntity
├── TridentEntity.hpp/cpp          # 三叉戟实体（支持忠诚返回/激流/引雷）
├── SpearEntity.hpp/cpp            # 长矛投掷实体（可回收，不支持忠诚返回）
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
│   │   ├── SpectralArrowEntity   # 光灵箭
│   │   ├── TridentEntity         # 三叉戟（支持忠诚返回）
│   │   └── SpearEntity           # 长矛（可回收，不支持忠诚返回）
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

### 3.1 烟花火箭生命周期随机化

**爆炸阈值公式**（对应 MC 1.21.11 `FireworkRocketEntity.tick()`）：
```
lifeTime = flightTime * 10 + nextInt(6) + nextInt(7)
```
- `flightTime`：从物品 NBT `Fireworks.Flight` 读取（默认 1）
- `nextInt(6) ∈ [0, 5]`、`nextInt(7) ∈ [0, 6]`，总和范围 `[0, 11]`
- 故 `flightTime=1` 时 `lifeTime ∈ [10, 21]`，`flightTime=2` 时 `∈ [20, 31]`

**懒初始化**（`_ensureLifeTimeComputed()`）：
- `m_lifeTime` 初值为 -1（未计算），首次 `tick()` 时通过 `world.getRandom()` 一次性确定
- **仅服务端执行**：客户端不跑 `FireworkRocketEntity::tick`，爆炸由服务端 `remove` 数据包驱动
- 服务端确定性：`world.getRandom()` 为服务端共享 RNG，同一 tick 内多个烟花火箭调用得到不同序列值
- 客户端回退阈值 `flightTime*10+6` 仅为防御性代码，正常路径不触发（详见 `OtherProjectiles.cpp` 内 TODO 注释）

**物品变更后失效**：`setFireworkItem()` 会将 `m_lifeTime` 重置为 -1，因为不同物品可能有不同 `flightTime`，需重新懒初始化。

### 3.2 烟花火箭 NBT 持久化

**序列化键名**（`EntityNbtKeys.hpp`）：

| 键名 | 类型 | 含义 |
|------|------|------|
| `FireworksItem` | compound | 烟花物品（由 `ItemStack::toNbt` 写入，空物品不写出） |
| `Life` | i32 | 已存在时间（每 tick 递增） |
| `LifeTime` | i32 | 总生命时间（爆炸阈值，未计算时不写出） |
| `ShotAtAngle` | i8 | 是否从弩射出（对应 `m_shotFromCrossbow`） |

**反序列化顺序**（`readAdditionalSaveData`）：
1. 先读 `FireworksItem` → 调用 `setFireworkItem()` 恢复 `m_fireworkItem` 与 `m_flightTime`（此步会将 `m_lifeTime` 重置为 -1）
2. 再读 `LifeTime` → 显式覆盖 `m_lifeTime`（绕过 `setFireworkItem` 的重置）
3. 读 `Life` → 恢复 `m_lifetime`
4. 读 `ShotAtAngle` → 恢复 `m_shotFromCrossbow`

**踩坑点**：
- `setFireworkItem` 必须在 `LifeTime` 恢复之前调用，否则 `setFireworkItem` 的 -1 重置会覆盖 NBT 中恢复的值
- 空 `fireworkItem`（默认 AIR）不会写出 `FireworksItem` 键，反序列化时跳过 `setFireworkItem` 调用
- `LifeTime` 仅在 `m_lifeTime >= 0` 时写出，避免持久化 -1 占位符

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

### 5.1 钓鱼浮标网络同步

对应 MC 1.21.11 `FishingHook.defineSynchedData()` / `onSyncedDataUpdated()`，FishingBobberEntity 通过 `EntityDataManager` 注册两个网络同步数据参数：

| 参数 | 类型 | 含义 | MC 1.21.11 对应 |
|------|------|------|----------------|
| `DATA_HOOKED_ENTITY_PARAM` | i32 | 被钩住实体 ID（+1 偏移，0=无） | `DATA_HOOKED_ENTITY` |
| `DATA_BITING_PARAM` | bool | 是否咬钩 | `DATA_BITING` |

**注册**（`registerData()`）：
- 由于 C++ 虚函数在构造函数中不会派生到子类，`Entity` 基类构造函数中调用的 `registerData()` 只会执行 `Entity::registerData()`。
- `FishingBobberEntity` 构造函数必须也显式调用 `registerData()` 以注册子类专属参数。
- `registerData()` override 先调用 `Entity::registerData()` 注册基础参数（FLAGS/AIR/CUSTOM_NAME 等），再注册上述两个参数。

**写入时机**（服务端）：
| 事件 | 写入 | 位置 |
|------|------|------|
| 钩住实体（`_onEntityHit`） | `DATA_HOOKED_ENTITY = entityId+1` | `_syncCaughtEntityId()` |
| 被钩实体失效 | `DATA_HOOKED_ENTITY = 0` | `tick()` State::Hooked 分支 → `_syncCaughtEntityId()` |
| 鱼咬钩（进入 Fishing 状态） | `DATA_BITING = true` | `_catchingFish()` |
| 咬钩超时（回到 Bobbing） | `DATA_BITING = false` | `tick()` State::Fishing 分支 |

**+1 偏移的设计**：`DATA_HOOKED_ENTITY` 存储的是 `entityId + 1`，0 专门表示"无被钩住实体"，避免与合法的 entityId=0 冲突。客户端读取后需减 1 还原真实实体 ID。

**同步链路**：
```
服务端 m_dataManager.set() → 标记脏数据
  → EntityTracker::tick() 检测 hasDirtyData()
  → EntityMetadataSerializer 序列化 → ir::play::SetEntityData 广播
  → 客户端 ClientEntity::setMetadata() 反序列化到 m_dataManager
  → syncMetadataFromDataManager() 按 typeId 分发
  → fishing_bobber 分支：读取参数写入 m_fishingHookedEntityId / m_fishingBiting 镜像
  → FishingBobberRenderer::generateMesh() 读取镜像字段驱动渲染
```

**踩坑点**：
- `EntityTracker::tick()` 的元数据同步受位置/旋转变化门控（`needsFullUpdate || positionChanged || rotationChanged`）。钓鱼浮标在钩住实体时浮标跟随实体移动（位置变化），在咬钩时浮标在水面上下浮动（位置变化），因此同步通常能及时触发。若浮标完全静止且仅有元数据变化，可能延迟到下次位置变化时同步。
- `DATA_BITING` 的写入必须成对：进入 Fishing 状态时设 true，离开时设 false，否则客户端镜像会永久停留在咬钩状态。

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

**负 inaccuracy 支持**：`ProjectileEntity::shoot()` 内部对 inaccuracy 取 `std::abs` 后再参与高斯散布计算。这是因为 MC 1.21.11 旋风人风弹散布公式 `5 - difficulty.getId() * 4` 在 Normal(-3) 和 Hard(-7) 难度下产生负值。由于高斯分布对称，负值与同绝对值的正值产生相同散布效果。所有调用者无需关心符号，传入正值或负值均可正常产生散布。

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

### 14. 弹射物偏转系统

弹射物命中实体时，在调用 `onEntityHit()` 之前先检查 `Entity::deflection()`：
- 如果返回非 `None` 的偏转类型，弹射物被偏转（改变方向和发射者），**不触发 onEntityHit**
- 如果返回 `None`，正常处理命中

**偏转类型**（对应 MC Java 的 `ProjectileDeflection`）：
| 类型 | 行为 | 用途 |
|------|------|------|
| `None` | 不偏转 | 默认 |
| `Reverse` | 速度 ×(-0.5)，随机偏航170~190° | 潜影贝、旋风人 |
| `AimDeflect` | 速度设为偏转者视线方向 | 玩家攻击可重定向弹射物 |
| `MomentumDeflect` | 速度设为偏转者移动方向 | 特定场景 |

**偏转流程**（`ProjectileEntity::onImpact`）：
1. 命中实体时调用 `hitEntity->deflection(*this)`
2. 如果偏转类型非 None 且偏转者不是上一个偏转者，调用 `deflect()`
3. `deflect()` 调用 `applyProjectileDeflection()` 修改速度/旋转/发射者
4. 记录 `m_lastDeflectedById` 防止同一实体连续偏转
5. 不调用 `onEntityHit()`，弹射物继续飞行

**旋风人偏转规则**（`BreezeEntity::deflection()`）：
- 风弹（`WindChargeEntity`）→ `None`（不偏转自己发射的弹）
- 其他投射物 → `Reverse`（反向偏转 + 播放 `ENTITY_BREEZE_DEFLECT` 音效）
- 前提：旋风人实体类型属于 `#minecraft:deflects_projectiles` 标签

### 15. EvokerFangsEntity Owner UUID 双重追踪

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
