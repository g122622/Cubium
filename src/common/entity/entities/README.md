# Entities 目录

本目录包含所有具体实体实现，按类别组织到子目录中。

## 目录结构树

```
entities/
├── passive/                    # 被动/中立生物
│   ├── basic/                  # 普通动物
│   │   ├── AnimalEntity.hpp/cpp    # 动物基类
│   │   ├── PigEntity.hpp/cpp       # 猪
│   │   ├── CowEntity.hpp/cpp       # 牛
│   │   ├── SheepEntity.hpp/cpp     # 羊
│   │   ├── ChickenEntity.hpp/cpp   # 鸡
│   │   ├── RabbitEntity.hpp/cpp    # 兔子
│   │   └── MooshroomEntity.hpp/cpp # 哞菇
│   ├── tamable/                # 可驯服动物
│   │   ├── TameableEntity.hpp/cpp  # 可驯服基类
│   │   ├── WolfEntity.hpp/cpp      # 狼
│   │   ├── CatEntity.hpp/cpp       # 猫
│   │   ├── OcelotEntity.hpp/cpp    # 豹猫
│   │   ├── ParrotEntity.hpp/cpp    # 鹦鹉
│   │   └── ShoulderRidingEntity.hpp/cpp # 肩膀骑乘基类
│   ├── special/                # 特殊动物
│   │   ├── FoxEntity.hpp/cpp       # 狐狸
│   │   ├── PandaEntity.hpp/cpp     # 熊猫
│   │   ├── PolarBearEntity.hpp/cpp # 北极熊
│   │   ├── TurtleEntity.hpp/cpp    # 海龟
│   │   ├── BeeEntity.hpp/cpp       # 蜜蜂
│   │   └── StriderEntity.hpp/cpp   # 炽足兽
│   ├── horse/                  # 马类
│   │   ├── AbstractHorseEntity.hpp/cpp     # 马类基类
│   │   ├── AbstractChestedHorseEntity.hpp/cpp # 可装箱马类基类
│   │   ├── HorseEntity.hpp/cpp     # 马
│   │   ├── DonkeyEntity.hpp/cpp    # 驴
│   │   ├── MuleEntity.hpp/cpp      # 骡
│   │   ├── SkeletonHorseEntity.hpp/cpp # 骷髅马
│   │   ├── ZombieHorseEntity.hpp/cpp # 僵尸马
│   │   ├── LlamaEntity.hpp/cpp     # 羊驼
│   │   ├── TraderLlamaEntity.hpp/cpp # 商贩羊驼
│   │   ├── CoatColors.hpp/cpp      # 马毛色
│   │   └── CoatTypes.hpp/cpp       # 马花纹
│   ├── fish/                   # 鱼类
│   │   ├── AbstractFishEntity.hpp/cpp   # 鱼类基类
│   │   ├── AbstractGroupFishEntity.hpp/cpp # 群游鱼类基类
│   │   ├── CodEntity.hpp/cpp       # 鳕鱼
│   │   ├── SalmonEntity.hpp/cpp    # 鲑鱼
│   │   ├── PufferfishEntity.hpp/cpp # 河豚
│   │   └── TropicalFishEntity.hpp/cpp # 热带鱼
│   ├── water/                  # 水生生物
│   │   ├── WaterMobEntity.hpp/cpp  # 水生生物基类
│   │   ├── SquidEntity.hpp/cpp     # 鱿鱼
│   │   └── DolphinEntity.hpp/cpp   # 海豚
│   ├── ambient/                # 环境生物
│   │   ├── AmbientEntity.hpp/cpp   # 环境生物基类
│   │   └── BatEntity.hpp/cpp       # 蝙蝠
│   └── golem/                  # 傀儡
│       ├── GolemEntity.hpp/cpp     # 傀儡基类
│       ├── IronGolemEntity.hpp/cpp # 铁傀儡
│       └── SnowGolemEntity.hpp/cpp # 雪傀儡
│
├── monster/                    # 敌对生物
│   ├── MonsterEntity.hpp/cpp       # 敌对生物基类
│   ├── undead/                 # 亡灵类
│   │   ├── AbstractSkeletonEntity.hpp/cpp # 骷髅基类
│   │   ├── SkeletonEntity.hpp/cpp  # 骷髅
│   │   ├── StrayEntity.hpp/cpp     # 流浪者
│   │   ├── WitherSkeletonEntity.hpp/cpp # 凋灵骷髅
│   │   ├── ZombieEntity.hpp/cpp    # 僵尸
│   │   ├── HuskEntity.hpp/cpp      # 尸壳
│   │   ├── DrownedEntity.hpp/cpp   # 溺尸
│   │   ├── ZombieVillagerEntity.hpp/cpp # 僵尸村民
│   │   └── PhantomEntity.hpp/cpp   # 幻翼
│   ├── arthropod/              # 节肢类
│   │   ├── SpiderEntity.hpp/cpp    # 蜘蛛
│   │   ├── CaveSpiderEntity.hpp/cpp # 洞穴蜘蛛
│   │   ├── SilverfishEntity.hpp/cpp # 蠹虫
│   │   └── EndermiteEntity.hpp/cpp # 末影螨
│   ├── nether/                 # 地狱生物
│   │   ├── BlazeEntity.hpp/cpp     # 烈焰人
│   │   ├── GhastEntity.hpp/cpp     # 恶魂
│   │   ├── MagmaCubeEntity.hpp/cpp # 岩浆怪
│   │   └── NetherEntities.hpp      # 猪灵系、疣猪兽系
│   ├── end/                    # 末地生物
│   │   ├── EndermanEntity.hpp/cpp  # 末影人
│   │   └── ShulkerEntity.hpp/cpp   # 潜影贝
│   ├── basic/                  # 基础怪物
│   │   ├── CreeperEntity.hpp/cpp   # 苦力怕
│   │   ├── SlimeEntity.hpp/cpp     # 史莱姆
│   │   └── GiantEntity.hpp/cpp     # 巨人
│   ├── ocean/                  # 海洋怪物
│   │   ├── GuardianEntity.hpp/cpp  # 守卫者
│   │   └── ElderGuardianEntity.hpp/cpp # 远古守卫者
│   └── illager/                # 灾厄村民
│       ├── AbstractIllagerEntity.hpp/cpp # 灾厄村民基类
│       ├── AbstractRaiderEntity.hpp/cpp  # 袭击者基类
│       ├── SpellcastingIllagerEntity.hpp/cpp # 施法灾厄村民基类
│       ├── IllagerEntities.hpp     # 掠夺者、卫道士
│       ├── EvokerEntity.hpp/cpp    # 唤魔者
│       ├── IllusionerEntity.hpp/cpp # 幻术师
│       ├── RavagerEntity.hpp/cpp   # 劫掠兽
│       ├── VexEntity.hpp/cpp       # 恼鬼
│       ├── WitchEntity.hpp/cpp     # 女巫
│       └── PatrollerEntity.hpp/cpp # 巡逻队基类
│
├── boss/                       # Boss实体
│   ├── EnderDragonEntity.hpp/cpp   # 末影龙 + EnderDragonPartEntity
│   ├── WitherEntity.hpp/cpp        # 凋灵
│   └── README.md
│
├── villager/                   # 村民实体
│   ├── AbstractVillagerEntity.hpp/cpp # 村民基类
│   ├── VillagerEntity.hpp/cpp      # 村民 + VillagerData
│   └── README.md
│
├── projectile/                 # 投掷物实体
│   ├── ProjectileEntity.hpp/cpp    # 投掷物基类
│   ├── ThrowableEntity.hpp/cpp     # 可投掷物基类
│   ├── AbstractArrowEntity.hpp/cpp # 箭矢基类
│   ├── DamagingProjectileEntity.hpp/cpp # 加速度投掷物基类
│   ├── AbstractFireballEntity.hpp/cpp # 火球基类
│   ├── ProjectileItemEntity.hpp/cpp # 投掷物品基类
│   ├── TridentEntity.hpp/cpp       # 三叉戟
│   ├── WindChargeEntity.hpp/cpp    # 风弹
│   ├── ProjectileHelper.hpp/cpp    # 投掷物辅助工具
│   ├── OtherProjectiles.hpp/cpp    # 其他投掷物
│   └── README.md
│
├── vehicle/                    # 交通工具
│   ├── BoatEntity.hpp/cpp          # 船
│   ├── MinecartEntity.hpp/cpp      # 矿车及变种
│   └── README.md
│
├── hanging/                    # 悬挂实体
│   ├── HangingEntity.hpp/cpp       # 悬挂实体基类
│   └── README.md
│
├── effect/                     # 效果实体
│   ├── EffectEntities.hpp/cpp      # 闪电、末影水晶等
│   └── README.md
│
├── misc/                       # 杂项实体
│   ├── MiscEntities.hpp/cpp        # 下落方块、TNT、寂守者警告效果
│   ├── OminousItemSpawnerEntity.hpp/cpp # 不祥物品生成器
│   └── README.md
│
├── orb/                        # 经验球
│   └── ExperienceOrbEntity.hpp/cpp # 经验球实体
│
├── item/                       # 物品相关实体
│   └── ItemEntity.hpp/cpp          # 掉落物品（含伤害处理、防火物品检测、NBT生命值序列化）
│
└── player/                     # 玩家实体
    ├── Player.hpp/cpp              # 玩家实体
    ├── GameModeUtils.hpp/cpp       # 游戏模式工具
    ├── ChatVisibility.hpp          # 聊天可见性
    ├── PlayerModelPart.hpp         # 玩家模型部件
    ├── SpawnLocationHelper.hpp/cpp # 重生点辅助
    └── README.md
```

## 内部模块关系

```
Entity (core/Entity.hpp)
├── LivingEntity (core/LivingEntity.hpp)
│   ├── MobEntity (core/MobEntity.hpp)
│   │   ├── CreatureEntity (core/CreatureEntity.hpp)
│   │   │   ├── AgeableEntity (core/AgeableEntity.hpp)
│   │   │   │   └── AnimalEntity (passive/basic/AnimalEntity.hpp)
│   │   │   │       ├── basic/ (Pig, Cow, Sheep, Chicken, Rabbit, Mooshroom)
│   │   │   │       ├── tamable/TameableEntity (+ IAngerable)
│   │   │   │       ├── special/ (Fox, Panda, PolarBear, Turtle, Bee, Strider)
│   │   │   │       └── horse/AbstractHorseEntity (+ IRideable, IJumpingMount)
│   │   │   └── WaterMobEntity (passive/water/WaterMobEntity.hpp)
│   │   │       ├── water/ (Squid, Dolphin)
│   │   │       └── fish/AbstractFishEntity
│   │   │           └── fish/ (Cod, Salmon, Pufferfish, TropicalFish)
│   │   ├── AmbientEntity (passive/ambient/AmbientEntity.hpp)
│   │   │   └── ambient/BatEntity
│   │   ├── GolemEntity (passive/golem/GolemEntity.hpp + IAngerable)
│   │   │   ├── IronGolemEntity
│   │   │   └── SnowGolemEntity
│   │   └── MonsterEntity (monster/MonsterEntity.hpp)
│   │       ├── undead/ (Zombie系, Skeleton系, Phantom)
│   │       ├── arthropod/ (Spider, CaveSpider, Silverfish, Endermite)
│   │       ├── nether/ (Blaze, Ghast, MagmaCube, Piglin系, Hoglin系)
│   │       ├── end/ (Enderman, Shulker)
│   │       ├── basic/ (Creeper, Slime, Giant)
│   │       ├── ocean/ (Guardian, ElderGuardian)
│   │       └── illager/ (Vindicator, Evoker, Illusioner, Pillager, Ravager, Vex, Witch)
│   └── Player (player/Player.hpp)
├── ItemEntity (item/ItemEntity.hpp)
├── ExperienceOrbEntity (orb/ExperienceOrbEntity.hpp)
├── VehicleEntity (vehicle/)
│   ├── BoatEntity (+ IRideable)
│   └── AbstractMinecartEntity (+ IRideable)
├── HangingEntity (hanging/)
│   ├── PaintingEntity
│   ├── ItemFrameEntity
│   └── LeashKnotEntity
├── EffectEntity (effect/)
│   ├── EnderCrystalEntity
│   ├── LightningBoltEntity
│   ├── AreaEffectCloudEntity
│   └── ArmorStandEntity
├── MiscEntity (misc/)
│   ├── FallingBlockEntity
│   ├── TNTEntity
│   ├── WardenWarningEffect
│   └── OminousItemSpawnerEntity
└── ProjectileEntity (projectile/)
    ├── ThrowableEntity
    │   └── ProjectileItemEntity (Snowball, Egg, EnderPearl, Potion)
    ├── AbstractArrowEntity (Arrow, SpectralArrow)
    ├── DamagingProjectileEntity
    │   └── AbstractFireballEntity (Fireball, SmallFireball, DragonFireball, WitherSkull)
    ├── TridentEntity
    ├── WindChargeEntity
    └── OtherProjectiles (LlamaSpit, ShulkerBullet, FishingBobber, FireworkRocket)
```

## 上下游外部依赖关系

**上游依赖（本目录依赖的模块）**：
- `entity/core/` - Entity、LivingEntity、MobEntity、AgeableEntity 等基类
- `entity/interfaces/` - IRideable、IShearable、IAngerable、IRangedAttackMob 等接口
- `entity/ai/` - Goal 系统、Brain 系统、寻路系统
- `entity/attribute/` - 属性系统（MAX_HEALTH、MOVEMENT_SPEED 等）
- `entity/damage/` - DamageSource、DamageSources
- `entity/inventory/` - 背包系统
- `entity/loot/` - 掉落表系统
- `world/IWorld.hpp` - 世界接口
- `world/block/Block.hpp` - 方块状态
- `item/ItemStack.hpp` - 物品堆

**下游依赖（依赖本目录的模块）**：
- `server/world/ServerWorld.hpp` - 服务端实体生成和管理
- `server/network/` - 实体同步包
- `client/renderer/entity/` - 客户端实体渲染器
- `item/` - 物品使用时生成实体（弓箭、雪球、末影珍珠等）
- `world/chunk/` - 区块实体存储

## 容易踩的坑

### 实体类型标识符

使用 `entity->entityType()` 与 `VanillaEntityTypeKeys` 命名空间常量比较，不要使用已废弃的 `LegacyEntityType` 枚举。`VanillaEntityTypeKeys` 中的指针在 `VanillaEntities::registerAll()` 后初始化，确保在实体类型注册后使用。

### 继承层次与接口

- `TameableEntity` 同时实现 `IAngerable` 接口，愤怒状态需要正确同步
- `AbstractHorseEntity` 实现 `IRideable` 和 `IJumpingMount`，骑乘时步高会动态变化
- `GolemEntity` 实现 `IAngerable`，铁傀儡被攻击后会愤怒
- 怪物类继承 `MonsterEntity`，自动获得怪物生成光照检查

### AI 目标注册

AI 目标在 `registerGoals()` 中注册，优先级数字越小越优先。互斥标志（GoalFlag）冲突时，高优先级目标会抢占。常见坑：
- 多个移动类目标使用相同标志但优先级设置不当
- 未检查 `shouldContinueExecuting()` 导致目标无法正常结束
- 目标中使用裸指针引用实体，实体已销毁后访问悬空指针

### 数据参数 ID

数据参数（DataParameter）ID 在实体类继承链中必须唯一。子类注册数据参数时，ID 不能与基类冲突。实体 ID 分配参考 `core/DataParameter.hpp`。

### NBT 序列化

实体保存/加载时注意：
- 新字段要同时实现 `writeAdditional()` 和 `readAdditional()`
- UUID 使用 `getUUID()` / `setUUID()` 方法，不要直接操作 NBT
- 乘客列表由 `Entity` 基类管理，子类不要自行序列化

### 世界边界伤害

`Player::tick()` 中有世界边界伤害检测，非观察者模式、非无敌状态的玩家在边界外受伤害。伤害公式：`max(1, floor(-(distance + buffer) * damagePerBlock))`。

### SlimeEntity 分裂逻辑

分裂逻辑应在 `remove()` 中执行而非 `die()`，因为 `die()` 时实体还未被标记为移除。经验值更新应在 `updateSizeAttributes()` 中执行，确保 `registerAttributes()` 初始化时也能正确设置。

### 群游鱼类

`AbstractGroupFishEntity` 实现群游行为，新鱼加入群体时需要检查 `groupLeader` 是否已满员。群体最大数量由 `getMaxGroupSize()` 返回。

## 相关文档

- [实体系统总览](../README.md)
- [核心实体层](../core/README.md)
- [实体接口](../interfaces/README.md)
- [AI系统](../ai/README.md)
- [玩家模块说明](player/README.md)
