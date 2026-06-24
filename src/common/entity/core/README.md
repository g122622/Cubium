#Entity Core Module

实体系统的核心框架，包含所有实体的基类和基础设施。

##目录结构树

``` src / common / entity / core /
├── Entity.hpp / cpp #所有实体的基类（位置、运动、碰撞、火焰、队伍联盟判断、持久化随机数生成器等）
├── LivingEntity.hpp / cpp #有生命值的生物实体基类（生命值、吸收值、装备、药水效果）
├── MobEntity.hpp / cpp #有AI的生物实体基类（AI系统、目标选择）
├── CreatureEntity.hpp / cpp #陆地生物基类（寻路、生成条件判断）
├── FlyingEntity.hpp / cpp #飞行生物基类
├── AgeableEntity.hpp / cpp #成长系统基类（幼体→成体）
├── EntityType.hpp / cpp #实体类型定义
├── EntityRegistry.hpp #实体注册表（工厂模式创建实体）
├── EntityTypeIdNumber.hpp / cpp #实体类型ID常量（网络同步用）
├── EntityDataManager.hpp #实体数据同步管理（客户端 -
    服务端数据同步）
├── EntityPose.hpp #实体姿态枚举（站立、潜行、游泳、睡眠等）
├── EntitySize.hpp #实体尺寸定义（宽度、高度、眼睛高度）
├── EntityClassification.hpp / cpp #实体分类（怪物、动物、环境等）
├── EntitySpawnPlacementRegistry.hpp /
        cpp #生成位置规则、SpawnReason枚举
├── EntityUtils.hpp #模板型实体工具函数（搜索、距离）
├── DataParameter.hpp #数据参数定义（网络同步用）
├── MoverType.hpp #移动类型枚举（自移、活塞、玩家、弹射物等）
├── BoostHelper.hpp #可骑乘实体的鞍和加速管理（猪、炽足兽等）
├── VanillaEntities.hpp #原版实体类型注册声明
└── README.md
```

        ##内部模块关系

``` 继承层次： Entity（基类：位置、运动、碰撞、火焰、流体检测）
├── LivingEntity（生命值、装备、药水效果、击退、空气供应、setLastHurtBy虚方法）
│   ├── MobEntity（AI系统、目标选择、控制器、日光检测）
│   │   ├── CreatureEntity（陆地移动、寻路、生成条件判断）
│   │   │   ├── AgeableEntity（成长系统）
│   │   │   │   └── AnimalEntity（繁殖系统，在../
        passive /）
│   │   │   └── MonsterEntity（敌对行为，在../ monster /）
│   │   └── FlyingEntity（飞行移动）
│   └── Player（玩家特有功能，在../../ player /）
└── ItemEntity（掉落物，在../）

        辅助类依赖关系： -
    Entity → EntityDataManager → DataParameter（数据同步） - Entity → EntitySize → AxisAlignedBB（碰撞箱） -
    EntityRegistry → EntityType → EntityTypeIdNumber（类型注册） - BoostHelper → EntityDataManager（鞍和加速状态同步） -
    EntitySpawnPlacementRegistry → SpawnReason（生成规则判断） -
    EntitySpawnPlacementRegistry → ISpawnWorldReader（世界状态查询接口） -
    EntitySpawnPlacementRegistry → BiomeTags（生物群系标签查询，如地表史莱姆生成） -
    EntitySpawnPlacementRegistry → InternalLightUtils（月相、光照计算） -
    EntitySpawnPlacementRegistry → SlimeChunkChecker（史莱姆区块判断）
```

    ##上下游外部依赖关系

        **上游依赖（本目录依赖的外部模块） **： - `core / Types.hpp` -
    基础类型定义（EntityId、Vector3等） - `entity / attribute /` -
    属性系统（generic.max_health等） - `entity / damage /` -
    伤害系统（DamageSource、DamageSources） - `world / IWorld.hpp` -
    世界级声音和位置查询入口 - `world / block / Block.hpp` - 方块交互回调 - `physics / PhysicsEngine.hpp` -
    物理引擎（移动、碰撞） - `util / math / random / Random.hpp` - 随机数生成器 - `util / nbt /` -
    NBT序列化 - `scoreboard / Team.hpp` -
    队伍系统

        **下游依赖（依赖本目录的外部模块） **： - `entity / passive /` -
    被动生物（动物） - `entity / monster /` - 敌对生物 - `entity / projectile /` - 弹射物实体 - `entity / item /` -
    物品实体 - `entity / vehicle /` - 载具实体 - `player /` - 玩家实体 - `world / World.hpp` -
    世界实体管理 - `server /` - 服务端实体调度 - `client / renderer / entity /` -
    客户端实体渲染

    ##MobEntity 生成初始化系统

        MobEntity 提供 `finalizeSpawn()` 方法，在实体被创建并设置好位置后调用，用于根据难度和生成原因进行初始化。

    ## #finalizeSpawn 调用链

``` finalizeSpawn(world, difficulty, spawnReason)
├── 设置 canPickUpLoot（概率 = 0.55 * specialMultiplier）
├── populateDefaultEquipmentSlots(random, difficulty)
│   └── 护甲生成概率 = 0.15 * specialMultiplier
│       └── getEquipmentForSlot(slot, armorLevel) → 护甲物品映射
└── populateDefaultEquipmentEnchantments(random, difficulty)
    ├── 武器附魔概率 = 0.25 *specialMultiplier
    └── 护甲附魔概率 = 0.5 *
            specialMultiplier（每个槽位独立）
```

            ## #子类覆写示例

        - **ZombieEntity * *：覆写 `populateDefaultEquipmentSlots()` 添加铁剑 / 铁锹 -
        **MonsterEntity *
            *：调用 `finalizeSpawn()` 设置 `isDespawnPeaceful()`

            ## #finalizeSpawn 调用位置

            所有 MobEntity 生成路径必须在 `spawnEntity()` 之前调用 `finalizeSpawn()`：

    | 生成路径 | SpawnReason | 文件 | | -- -- -- -- -| -- -- -- -- -- -- -| -- -- --| | 自然生成 | Natural
    | NaturalSpawner.cpp | | 村庄围攻 | Event | VillageSiege.cpp | | 刷怪蛋 | SpawnEgg | SpawnEggItem.cpp | |
    / summon 命令 | Command | SummonCommand.cpp | | 陷阱骷髅马 | Trigger | SkeletonHorseEntity.cpp | | 袭击 | Event
    | Raid.cpp | | 繁殖 | Breeding | BreedGoal.cpp | | 史莱姆分裂 | Reinforcement | SlimeEntity.cpp | | 恼鬼召唤
    | MobSummons | EvokerEntity.cpp | | 蠹虫生成 | Event | InfestedBlock.cpp | | 雪傀儡 / 铁傀儡 | Event
    | MelonPumpkinBlocks.cpp | | 海龟孵化 | Natural | TurtleEggBlock.cpp | | 远古守卫者 | Structure
    | OceanMonumentPieces.cpp | | 结构模板 | Structure | Template.cpp | | 区块生成 | ChunkGeneration | ServerWorld.cpp,
          MinecraftServer.cpp | | 僵尸村民治愈 | Conversion | ZombieVillagerEntity.cpp |

    ## #getEquipmentForSlot 护甲等级映射

    | armorLevel | 材质 | Head | Chest | Legs | Feet | | -- -| -- -| -- -| -- -| -- -| -- -| | 0 | 皮革 | LEATHER_HELMET
    | LEATHER_CHESTPLATE | LEATHER_LEGGINGS | LEATHER_BOOTS | | 1 | 铜 | COPPER_HELMET | COPPER_CHESTPLATE
    | COPPER_LEGGINGS | COPPER_BOOTS | | 2 | 金 | GOLDEN_HELMET | GOLDEN_CHESTPLATE | GOLDEN_LEGGINGS | GOLDEN_BOOTS | |
    3 | 锁链 | CHAINMAIL_HELMET | CHAINMAIL_CHESTPLATE | CHAINMAIL_LEGGINGS | CHAINMAIL_BOOTS | | 4 | 铁 | IRON_HELMET
    | IRON_CHESTPLATE | IRON_LEGGINGS | IRON_BOOTS | | 5 | 钻石 | DIAMOND_HELMET | DIAMOND_CHESTPLATE | DIAMOND_LEGGINGS
    | DIAMOND_BOOTS |

    对应 MC 1.21.11 原版 `Mob.getEquipmentForSlot()`，armorLevel = 1 为铜护甲。

                                                                   ##CreatureEntity 寻路权重与生成条件

                                                                   ## #getPathWeight 寻路权重系统

`CreatureEntity::getPathWeight(f32 x, f32 y, f32 z)` 返回实体偏好移动到该位置的程度，正值越高越偏好，负值越避开，0为中性。对应
                                                                   MC `PathfinderMob
                                                                       .getWalkTargetValue`，被 `RandomPositionGenerator` 用于
                                                                   AI 随机位置选择。

    | 实体类 | 重写逻辑 | 对应 MC | | -- -- -- --| -- -- -- -- --| -- -- -- -- -| | CreatureEntity（默认）
    | 返回 0.0f | `PathfinderMob` 返回 0.0F | | AnimalEntity
    | 草方块 → 10.0f，否则 brightness - 0.5f | `Animal.getWalkTargetValue` | | MonsterEntity | 0.5f - brightness
    | `Monster.getWalkTargetValue` | | WaterMobEntity
    | 水中 → 10.0f，否则 → 0.0f | MC 中未重写（返回0.0F），但水生子类重写 | | GuardianEntity
    | 水中 → 10.0f + lightCost，否则委托 MonsterEntity | `Guardian.getWalkTargetValue` | | StriderEntity
    | 岩浆中 → 10.0f，非岩浆但自身在岩浆 → - ∞，否则 → 0.0f | `Strider.getWalkTargetValue` | | MooshroomEntity
    | 菌丝 → 10.0f，否则委托 AnimalEntity | `MushroomCow.getWalkTargetValue` | | EndermanEntity
    | 返回 0.0f | `EnderMan.getWalkTargetValue` |

    **待实现 * *：TurtleEntity（水陆两栖）、HoglinEntity（诡异菌排斥 /
                绯红菌岩偏好）、SilverfishEntity（虫蚀方块偏好）、GiantEntity（亮度不取反）。

                ## #canSpawnAt 生成条件

`CreatureEntity::canSpawnAt(f32 x, f32 y, f32 z)` 基于 `getPathWeight
            >= 0.0f` 判断位置是否适合生成，对应 MC `PathfinderMob.checkSpawnRules`。

                已集成到以下生成路径：
                -
                **NaturalSpawner::_trySpawnAt *
                    *：自然生成时，在实体创建并设置位置后、`finalizeSpawn` 之前调用 `canSpawnAt` 检查
                -
                **ServerWorld::spawnEntitiesFromChunkGeneration *
                    *：区块生成时，在实体创建并设置位置后、`finalizeSpawn` 之前调用 `canSpawnAt` 检查

                    注意：WorldGenSpawner
                    在区块生成阶段只记录 `SpawnedEntityData`（无实体实例），无法执行实例级检查，因此实例级检查延迟到 `ServerWorld::
                        spawnEntitiesFromChunkGeneration` 创建实体时执行。

                    ##容易踩的坑

                    ## #实体随机数生成器（getRandom）
                - `Entity::getRandom()` 返回 `math::Random
        &`（持久化引用），每个实体拥有独立的 `m_random` 成员
            - 旧 `MobEntity::getRandom()` 按值返回临时对象（每次调用重新构造 RNG），已移除
            - 获取随机数时应使用引用赋值 `math::Random &rng = entity->getRandom()`，而非值赋值 `math::Random rng =
                                                                       entity->getRandom()` -
    值赋值会拷贝 RNG 状态，导致原 RNG 不推进，且同一 tick 内多次值赋值得到相同序列
    - `getRandom()` 在 `const` 方法中也可调用（`mutable` 成员），这是有意为之——随机数生成是逻辑操作而非状态查询 -
    Brain 系统的 `Task::start()` 接收的 `random` 参数仅用于计算任务持续时间，AI 逻辑中的随机数应使用 `owner->getRandom()`

    ## #区块高度常量混淆
    - `CHUNK_HEIGHT = MAX_BUILD_HEIGHT - MIN_BUILD_HEIGHT`（当前值 384） - `MAX_BUILD_HEIGHT` 是世界最大建筑高度（当前值
    320），`MIN_BUILD_HEIGHT` 是最低高度（当前值
    - 64） -
    两者语义完全不同，务必根据语义选择正确常量，不要硬编码

    ## #步进高度（stepHeight）设置
    - Entity 基类默认 stepHeight = 0.0f（无步进能力） - LivingEntity 默认 stepHeight = 0.6f（可走台阶） -
                铁傀儡、马、末影人、溺尸、劫掠兽、海龟等为 1.0f（可走完整方块） - 盔甲架为 0.0f（无法步进） -
                骑乘时步高会动态变化（IRideable）

                ## #火焰系统注意事项
                - `setFire(seconds)` 将秒转换为 tick（×20），只在当前值较小时更新
                - `forceFireTicks(ticks)` 直接设置值，用于增减火焰时间
                - 火焰免疫由 `EntityType::immuneToFire()` 标志决定 -
                烈焰人、恶魂、岩浆怪、猪灵系、疣猪兽、潜影贝、Boss实体免疫火焰

                ## #亡灵日光燃烧系统(burnUndead)

                    对应 MC 原版 `Mob.burnUndead()`，统一处理亡灵生物在阳光下的燃烧逻辑。

                ####核心方法

                - **`isInDaylight()`* * — 检查实体是否暴露在日光下。条件：白天（dayTime <
            12000）、亮度 > 0.4、天空可见、不在水中且不在雨中（`isWet()`）、随机检查通过 -
                **`sunProtectionSlot()`** — 虚方法，返回阳光防护装备槽位。默认 `EquipmentSlot::
                        Head`（头盔），僵尸马覆写为 `EquipmentSlot::Chest`（马铠
                    / 胸甲槽位）
                -
                **`burnUndead()`** — 亡灵日光燃烧主逻辑： 1. 如果不在日光下 → 直接返回
                 2. 如果防护槽位有可损坏物品 → 物品承受耐久损耗（`setDamage()`，绕过耐久保护附魔）
                 3. 如果防护槽位为空 → `setFire(8)` 点燃实体 8 秒

                 ####调用位置

                - `MonsterEntity::handleDaylightBurning()` — 当 `m_burnsInDaylight
        ==
        true` 时调用 `burnUndead()`（僵尸、骷髅等） - `ZombieHorseEntity::tick()` — 僵尸马在 BURN_IN_DAYLIGHT
            标签中，直接调用 `burnUndead()` - `PhantomEntity::tick()` — 幻翼在 BURN_IN_DAYLIGHT
            标签中，直接调用 `burnUndead()` -
            **注意 **：骷髅马不在 BURN_IN_DAYLIGHT 标签中，不会在阳光下燃烧

             ####骷髅马与僵尸马的区别

    | 行为 | 骷髅马 | 僵尸马 | | -- -- --| -- -- -- --| -- -- -- --| | BURN_IN_DAYLIGHT | ✗ | ✓ | | 阳光下燃烧 | ✗ | ✓ |
    | 阳光防护槽位 | N / A | Chest（马铠） | | canBreatheUnderwater | ✓ | ✗ |

    ####实现注意事项

        - `burnUndead()` 使用 `ItemStack::setDamage()` 直接增加伤害值， **绕过耐久保护附魔 **（与 MC 原版一致）
        - `isInDaylight()` 中 `isWet()` 检查确保雨中和水中的亡灵不会燃烧
        - 物品耐久耗尽后调用 `onEquippedItemBroken` 回调（广播装备破损动画 +
        播放 ENTITY_ITEM_BREAK 音效）

        ## #装备损坏回调

        当装备物品耐久度耗尽时，需要调用 `onEquippedItemBroken` 回调，对应 MC 原版 `LivingEntity.onEquippedItemBroken()`。

            **推荐使用 `LivingEntity::hurtAndBreak()` 静态方法 **： -
        封装了 "保存物品指针 → 调用 attemptDamageItem → 检查是否损坏 → 触发回调"的完整流程 -
        对应 MC 原版 `ItemStack.hurtAndBreak(int, LivingEntity, EquipmentSlot)` -
        适用于绝大多数物品耐久损耗场景（工具攻击、方块破坏、武器射击等）
        - 参数：`(ItemStack& stack, i32 amount, LivingEntity* entity, EquipmentSlot slot)` -
        返回值：`true` 表示物品损坏，`false` 否则

            **辅助方法 *
                *： - `LivingEntity::handToEquipmentSlot(Hand)` — 将 `Hand` 枚举转换为 `EquipmentSlot`

                          **调用链 **： 1. `hurtAndBreak()` 在调用 `attemptDamageItem` 之前保存物品指针（因为损坏后
                      ItemStack 会被清空） 2. 调用 `attemptDamageItem(amount, entity)` 处理耐久保护附魔和实际伤害
                      3. 若物品损坏，调用 `entity->onEquippedItemBroken(item, slot)`

                          **特殊场景 **： - `MobEntity::
                    burnUndead()` — 使用 `setDamage()` 直接增加伤害值（绕过耐久保护附魔），手动实现回调逻辑
        - `PlayerInventory::damageArmor()` — 使用 `hurtAndBreak()` 统一处理

            **方法实现 **： - `LivingEntity::onEquippedItemBroken()` — 广播 EntityStatus 装备破损状态码
        + 播放 ENTITY_ITEM_BREAK 音效
        - `ServerPlayer::onEquippedItemBroken()` — 额外更新物品损坏统计（`minecraft.broken:
{
    item_id
}
`）

        **EntityStatus 状态码 **： -
    47 = MainHand,
    48 = OffHand, 49 = Head, 50 = Chest, 51 = Legs,
    52 = Feet

            * *客户端处理 * *： -
        收到状态码 47 - 52 时，播放 ENTITY_ITEM_BREAK 音效 +
        Breaking 粒子效果

        ## #空气供应与溺水 -
        空气值可从正数变成负数（用于溺水计时） - 当空气值降到 - 20 时重置为 0 并触发溺水伤害 -
        亡灵生物 `canBreatheUnderwater()` 返回 true，不会溺水 -
        WaterMobEntity 使用反逻辑：水中恢复，陆地上消耗

        ## #队伍联盟判断 -
        **`isAlliedTo(const Entity&)`* *-双向联盟检查：this 认为 other 是盟友，或 other 认为 this 是盟友 -
        **`isAlliedTo(const scoreboard::Team*)`* *-队伍级联盟检查（虚方法，TameableEntity 重写以继承主人队伍） -
        **`considersEntityAsAlly(const Entity&)`*
            *-虚方法，自定义单向盟友判定逻辑，默认委托给 `isAlliedTo(other.getTeam())` -
        **`isOnSameTeam(const Entity&)`*
            *-旧版 API，仅单向检查 this 是否属于 other 的队伍，新代码应优先使用 `isAlliedTo()` -
        使用 * *指针相等性 * *比较队伍，而非队伍名称比较 - 两个 Team 对象即使名称相同，指针不同也不算同一队伍 -
        没有队伍的实体（`getTeam()` 返回 nullptr）不会与任何队伍匹配

        ## #传送系统使用 - `attemptTeleport(x, y, z)` -
        安全传送，自动查找地面 - `randomTeleport(range, playEffects, avoidFluid)` - 随机传送 -
        传送会自动重置运动向量

        ## #类型标识符获取 - `typeId()` 返回 `EntityTypeIdNumber` 命名空间中的常量 - `legacyType()` 返回 `LegacyEntityType` 枚举（旧版，仅用于兼容） -
        新代码应使用 `typeId()`

        ## #战利品表ID获取（getLootTableId）

        `Entity::getLootTableId()` 是虚方法，返回实体对应的战利品表资源路径（如 `"minecraft:entities/pig"`）。

        **方法层次**：
        - **`Entity::getLootTableId()`**（基类）：从 `m_typeId` 推导默认路径，格式为 `<namespace>:entities/<path>`（如 `"minecraft:pig"` → `"minecraft:entities/pig"`）。`m_typeId` 为空时返回空字符串。
        - **`MobEntity::getLootTableId()`**（覆写）：优先返回 NBT 自定义掉落表 `m_deathLootTable`（对应 MC Java 的 `DeathLootTable` NBT 标签），为空或 nullopt 时回退到基类实现。对齐 MC Java 的 `Mob.getLootTable()`：`this.lootTable.isPresent() ? this.lootTable : super.getLootTable()`。
        - **无战利品表实体覆写**：以下实体覆写返回空字符串，对齐 MC Java 的 `EntityType.Builder.noLootTable()`：
          - `ProjectileEntity`（覆盖所有弹射物子类：箭矢、三叉戟、火球等）
          - `ItemEntity`（掉落物）
          - `ExperienceOrbEntity`（经验球）
          - `BoatEntity`（船）
          - `AbstractMinecartEntity`（覆盖所有矿车变体）
          - `HangingEntity`（覆盖画、物品框、拴绳结）
          - `AreaEffectCloudEntity`、`EnderCrystalEntity`、`LightningBoltEntity`（效果实体）
          - `FallingBlockEntity`、`TNTEntity`（杂项实体）
          - `FishingBobberEntity`、`EvokerFangsEntity`、`EyeOfEnderEntity`、`FireworkRocketEntity`（其他弹射物）

        **使用场景**：
        - `/loot kill` 命令：从目标实体获取战利品表ID，生成击杀掉落物
        - 实体死亡掉落逻辑：`MobEntity` 死亡时使用 `getLootTableId()` 查询战利品表

        ## #ISpawnWorldReader 接口 -
        定义在 `EntitySpawnPlacementRegistry.hpp` 中，用于生成检查的最小世界读取接口 -
        主要方法： - `getBlockState(x, y, z)` - 获取方块状态 - `isInWorldBounds(x, y, z)` -
        检查位置是否在世界范围内 - `getHeight(type, x, z)` - 获取高度图值 - `getBiome(x, y, z)` -
        获取生物群系ID - `seed()` - 获取世界种子（史莱姆区块判断等确定性生成条件） - `difficulty()` -
        获取世界难度（和平难度拒绝怪物生成） - `dayTime()` -
        获取游戏日时间（月相计算等基于时间的生成条件） - `getMaxLocalRawBrightness(x, y, z)` -
        获取最大原始亮度（光照等级生成条件） -
        适配器实现：`NaturalSpawner::ServerWorldAdapter`（服务端）、`WorldGenRegionAdapter`（世界生成）

        ## #数据参数注册 -
        数据参数必须在 `registerData()` 中注册。所有数据参数必须通过 `EntityDataManager::createKey<T>()` 自动分配唯一
            ID，禁止硬编码 ID 值。客户端通过 `EntityDataManager` 同步数据。

        ## #鞍与加速系统（BoostHelper） - `BoostHelper` 的加速状态不保存到 NBT（MC 1.16.5 行为） -
        只有鞍状态会持久化 -
        客户端通过 `syncFromDataManager()` 同步状态

        ## #玩家交互 - `processInitialInteract()` -
        处理玩家与生物的交互链，按优先级依次处理：命名牌 → 刷怪蛋 → 拴绳 → 子类交互 -
        命名牌交互：委托 `NameTagItem::itemInteractionForEntity()`，成功后设置自定义名称并启用持久化 -
        刷怪蛋交互：调用 `_spawnOffspringFromSpawnEgg()`，生成同类型幼体（仅 AgeableEntity 子类支持） -
        **依赖 *
            *：此路径依赖 SpawnEggItem 在 Items 注册表中的注册（如 `Items::PIG_SPAWN_EGG`），当前
                Items 注册表尚未注册任何刷怪蛋物品，待 `Items::registerSpawnEggs()` 实现后可通过正常游戏流程触发 -
        拴绳交互：基本拴绳附着逻辑已实现（`setLeashedToEntity`、`setLeashedToFence`、`clearLeash`）， 完整的拴绳系统（Leashable接口、tickLeash物理、LeashKnotEntity交互、网络同步包等）待后续实现 - `canBeLeashed()` -
        判断生物是否可被拴绳拴住，默认实现通过 `dynamic_cast<IMob*>` 判断（敌对生物不可拴绳） -
        拴绳数据序列化：NBT 中 `Leash` 标签已实现，支持实体 UUID（UUIDMost / UUIDLeast）和栅栏柱坐标（X / Y /
            Z）两种格式 -
        拴绳延迟绑定：`LeashDelayInfo` 存储从 NBT
            读取的原始数据，待目标实体加载后通过 `restoreLeashFromSave()` 完成实际绑定 - `_spawnOffspringFromSpawnEgg()` -
        使用刷怪蛋生成幼体，类型匹配 + AgeableEntity 检查 -
        **测试覆盖 *
            *：核心逻辑（实体生成、物品消耗、类型匹配）的单元测试待
                Items 注册 SpawnEggItem 后补充 - `applyPlayerInteraction()` -
        有位置信息的交互（盔甲架装备槽） -
        基类默认返回 `ActionResultType::Pass`

        ## #水花溅射效果 -
        速度因子 f1 决定水花强度和声音选择 - f1 <
    0.25 用普通溅水声，f1 >= 0.25 用高速溅水声 -
        观察者模式玩家不产生水花效果

        ## #发光效果判断 - `isGlowing()` 客户端检查数据参数标志位，服务端检查 m_glowing 字段 -
        发光效果来源：发光药水、Entity发光标志、团队发光规则

        ## #箭矢计数与脱落 -
        箭矢数量越多，脱落越快 - 脱落计时器公式：`20 * (30 - arrowCount)` ticks -
        箭矢计数仅用于渲染，不影响游戏逻辑

        ## #吸收值（金苹果额外生命） -
        使用 `absorptionAmount()` 获取吸收值，`setAbsorptionAmount(
            f32)` 设置吸收值 - `setAbsorptionAmount` 会将值限制在 `[0, maxAbsorption]` 范围内（与
            MC 原版一致） - `maxAbsorption` 来自 `generic.max_absorption` 属性，默认值为 0.0 -
        Player 不再需要单独定义吸收值方法，统一使用 LivingEntity 基类的实现 -
        吸收值在 `actuallyHurt` 中通过 `setAbsorptionAmount` 消耗，确保限制逻辑生效 -
        NBT 序列化键为 `"AbsorptionAmount"`，反序列化也使用 `setAbsorptionAmount`

        ## #装备掉落概率(DropChances) -
        对应 MC 原版的 `DropChances` 记录，存储在 `m_equipmentDropChances` 数组中 -
        索引与 `EquipmentSlot` 枚举值对应：[0] = MainHand,
    [1] = OffHand, [2] = Feet, [3] = Legs, [4] = Chest,
    [5] = Head - 默认值为 `DEFAULT_EQUIPMENT_DROP_CHANCE = 0.085f`（8.5 %） -
    大于 1.0 的值表示物品被保留（`PRESERVE_ITEM_DROP_CHANCE = 2.0f`） - `isEquipmentDropPreserved(slot)` 检查掉落概率 >
    1.0 - `setGuaranteedDrop(slot)` 设置掉落概率为 2.0（保整掉落） - NBT 序列化格式：
        - 保存时仅写入新格式（MC 1.21.4 +）：`drop_chances`（compound，仅包含非默认值）
        - 读取时优先使用新格式，然后回退到旧格式（`HandDropChances` float[2] + `ArmorDropChances` float[4]）以兼容旧存档

        ## #死亡掉落表(DeathLootTable)
        - `m_deathLootTable`：可选字符串，覆盖实体类型的默认掉落表（格式如 `"minecraft:entities/zombie"`）
        - `m_lootTableSeed`：确定性种子，0 表示随机
        - NBT 键：`DeathLootTable`（string，仅在有值时写入）、`DeathLootTableSeed`（long，仅非零时写入）
        - 对应 MC 原版 Mob 的 `lootTable` 和 `lootTableSeed` 字段
        - 通过 `MobEntity::getLootTableId()` 虚方法统一访问：优先返回 `m_deathLootTable`，为空时回退到 `Entity::getLootTableId()`（从 typeId 推导默认路径）

        ## #拴绳系统(Leash) - `m_isLeashed`：是否被拴绳拴住
        - `m_leashHolderUuid`：拴绳目标实体的 UUID（拴在实体上时）
        - `m_leashFencePos`：拴绳目标栅栏柱坐标（拴在栅栏上时）
        - `LeashDelayInfo`：延迟绑定信息，用于 NBT 加载后目标实体尚未就绪的情况
        - NBT 键：`Leash`（compound），包含 `UUIDMost`+`UUIDLeast`（实体）或 `X`+`Y`+`Z`（栅栏柱）
        - `setLeashedToEntity(uuid)`：拴到实体 - `setLeashedToFence(pos)`：拴到栅栏柱 - `clearLeash()`：解除拴绳

        ## #摔落伤害流程

        实体着地时的摔落伤害由 `Block::onFallenUpon` 驱动，而非由 Entity 自行施加。流程如下：

``` Entity::move() → updateFallDistance() → _handleLandingOnBlock() → Block::onFallenUpon()
```

        1. `Entity::updateFallDistance()`：当实体着地且 `fallDistance
    > 0` 时，调用 `_handleLandingOnBlock()` 2. `Entity::_handleLandingOnBlock()`：获取实体脚下方块，调用 `Block::
            onFallenUpon()` 3. `Block::onFallenUpon()` 默认实现：调用 `entity.causeFallDamage(
                fallDistance, 1.0f, DamageSources::fall())` 施加普通摔落伤害
        4. 方块子类可重写 `onFallenUpon()` 自定义摔落行为：
        -
        **石笋（PointedDripstoneBlock） **：调用 `causeFallDamage(
            dist + 2.5, 2.0, stalagmite())` 但不调用父类，替代普通摔落伤害
        - **耕地（FarmlandBlock） **：先执行踩踏逻辑，再调用父类 `Block::onFallenUpon`，保留普通摔落伤害 -
        **海龟蛋（TurtleEggBlock） **：先执行踩破逻辑，再调用父类 `Block::onFallenUpon`，保留普通摔落伤害 -
        **蜂蜜块 / 史莱姆块 **：通过 `onLanded()` 重置 `fallDistance = 0`，根本不会触发 `onFallenUpon`

        * *重要 *
        *：`Entity::updateFallDistance()` 不再直接调用 `handleFallDamage()`，摔落伤害完全由 `Block::onFallenUpon` 负责。

         ####摔落伤害传播给乘客

`Entity::causeFallDamage` 基类实现会先将摔落伤害传播给所有乘客（`propagateFallToPassengers`），然后对自身不施加伤害。`LivingEntity::
             causeFallDamage` 重写时先调用 `Entity::causeFallDamage`（传播给乘客），再对自身计算伤害。

``` Entity::causeFallDamage(distance, multiplier, source)
  → propagateFallToPassengers(distance, multiplier, source) // 传播给所有乘客
                                                             // 基类不对自身施加伤害

         LivingEntity::causeFallDamage(distance, multiplier, source)
  → Entity::causeFallDamage(distance, multiplier, source) // 先传播给乘客
  → 计算自身摔落伤害（缓降免疫、跳跃增强减距、摔落保护附魔减伤）
  → hurt(source, damage)
```

         参考 MC 1.21.11：`Entity.causeFallDamage` → `propagateFallToPassengers`，`LivingEntity
             .causeFallDamage` → `super.causeFallDamage` +
    自身伤害计算。

        ## #方块碰撞检测系统

        实体与方块的碰撞检测由两个互补方法组成，对应 MC 原版中 Entity.tick() 和 LivingEntity
            .aiStep() 的不同碰撞阶段。

        ## #doBlockCollisions() — 遍历碰撞箱内所有方块

        遍历实体碰撞箱覆盖的所有方块，对每个非空气方块调用：
        1. `Block::onEntityCollision()` — 方块对实体的碰撞回调（仙人掌伤害、蜘蛛网减速等）
        2. `Entity::onInsideBlock()` — 实体 "在方块内部"的回调（传送门检测等） 3. 自定义方块组件 `onEntity` 事件派发

        对应 MC 原版 `Entity.checkInsideBlocks()`，由 `LivingEntity.aiStep()` 中的
    `applyEffectsFromBlocks()` 调用。

        * *调用位置 * *： - `LivingEntity::aiStep()` — 在 `travel()` 之后调用，所有 LivingEntity 子类自动继承
    - `Entity::moveWithCollision()` noClip 路径 — 即使 noClip = true 也要触发碰撞
    - `BoatEntity::tick()` — 手动调用（BoatEntity
          .canTriggerWalking() = false） - `ThrowableEntity::tick()` — 投射物需要在 tick 中手动调用

            * *典型方块碰撞效果 * *： |
    方块 | 回调 | 效果 | | -- -- --| -- -- --| -- -- --| | CactusBlock | onEntityCollision
    | 对 LivingEntity 造成 1.0 伤害 | | WebBlock | onEntityCollision | 水平速度 ×0.25，垂直速度 ×0.05 |
    | SweetBerryBushBlock | onEntityCollision | 伤害 + 减速（非潜行时） | | BubbleColumnBlock | onEntityCollision
    | 上推 / 下拉 Y 速度，重置摔落距离 | | NetherPortalBlock | onEntityCollision | 设置传送门状态 | | FireBlock
    | onEntityCollision | 点燃实体 | | HoneyBlock | onEntityCollision | 水平速度 ×0.4，下滑减速 |

    ## #doBlockCollisionsAfterMove() — 移动后触发的方块回调

            在 `Entity::moveWithCollision()` 正常路径（非 noClip）移动完成后调用，处理与
            移动方向和着地状态相关的方块回调： 1. `Block::onLanded()` — 着陆回调（粘液块弹跳、蜂蜜块取消摔落等）
            2. `Block::onEntityWalk()` — 行走回调（农田踩踏、海龟蛋踩踏、红石粉更新等）
            3. `Block::onStepOn` / `Block::onStepOff` — 自定义方块组件步进事件
            4. `Entity::onInsideBlock()` — 遍历碰撞箱内所有方块的 "内部"回调

            * *重要区别 *
            *：`doBlockCollisionsAfterMove()` 不调用 `onEntityCollision()`， 因此不会触发仙人掌伤害、蜘蛛网减速等效果。这些效果仅由 `doBlockCollisions()` 触发。

            ## #两个方法的调用时序

    ``` LivingEntity::tick()
      → LivingEntity::aiStep()
        → travel() // 物理移动
        → doBlockCollisions() // onEntityCollision + onInsideBlock（每tick）
      → Entity::moveWithCollision() // 碰撞检测移动
        → doBlockCollisionsAfterMove() // onLanded + onEntityWalk + onStepOn/Off + onInsideBlock
    ```

        - `doBlockCollisions()` 在每 tick 的 `aiStep()` 中调用，保证持续碰撞效果
        - `doBlockCollisionsAfterMove()` 仅在物理移动发生时调用，处理着陆和行走事件

            ## #步声系统（Step Sound）

            对应 MC 原版 `Entity.walkingStepSound()` / `Entity.playStepSound()` / `Player.playStepSound()` 逻辑。

                                                                                  ## #核心方法

        - **`playStepSound(pos, blockState)`* * — 播放脚步声的入口（虚方法，Player 重写） -
        **`getPrimaryStepSoundBlockPos(pos)`*
            * — 检查 pos
            上方方块是否属于 `INSIDE_STEP_SOUND_BLOCKS` 或 `COMBINATION_STEP_SOUND_BLOCKS`，如果是则返回上方位置
        - **`playCombinationStepSounds(aboveState, belowState)`* * — 播放组合步声：上方方块正常步声（0.15x 音量）
        + 下方方块沉闷步声 - **`playMuffledStepSound(blockState)`* * — 播放沉闷步声（0.05x 音量，0.8x 音调） -
        **`shouldPlayAmethystStepSound(blockState)`*
            * — 检查方块是否属于 `CRYSTAL_SOUND_BLOCKS`

            ## #步声类型与标签

    | 标签 | 方块示例 | 步声行为 | | -- -- --| -- -- -- -- --| -- -- -- -- --|
    | `COMBINATION_STEP_SOUND_BLOCKS` | 羊毛地毯、苔藓地毯、苍白苔藓地毯、雪层、下界苗、诡异菌索、绯红菌索、树脂团
    | 播放上方正常步声 + 下方沉闷步声 |
    | `INSIDE_STEP_SOUND_BLOCKS` | 细雪、幽匿脉络、发光地衣、睡莲、小型紫水晶芽、粉红色花瓣、野花、落叶层
    | 只播放上方方块的步声（替代脚下方块的步声） | | `CRYSTAL_SOUND_BLOCKS` | 紫水晶块、紫水晶母岩
    | 播放紫水晶共振铃声（`block.amethyst_block.chime`），带强度累积和 20 tick 冷却 |

    ## #Entity::playStepSound 逻辑流程

    ``` playStepSound(pos, blockState)
      → getPrimaryStepSoundBlockPos(pos)
      → 如果 primaryPos != pos（上方方块是 COMBINATION 或 INSIDE）
        → primaryState = world.getBlockState(primaryPos)
        → 如果 COMBINATION_STEP_SOUND_BLOCKS
          → playCombinationStepSounds(primaryState, blockState) // 上方正常 + 下方沉闷
        → 否则 INSIDE_STEP_SOUND_BLOCKS
          → 播放上方方块正常步声
        → 如果 shouldPlayAmethystStepSound(blockState) // 始终检查脚下方块
          → playAmethystStepSound() // 强度累积 + 20 tick 冷却
        → return
      → 否则（普通方块）
        → 播放脚下方块正常步声
        → 如果 shouldPlayAmethystStepSound(blockState)
          → playAmethystStepSound() // 强度累积 + 20 tick 冷却
    ```

        * *重要 * *：紫水晶铃声始终检查脚下方块（`blockState`），而非上方方块（`primaryState`），与 MC 原版一致。

    - **紫水晶铃声强度累积机制 * *-字段：`m_crystalSoundIntensity`（f32，0 ~1）和 `m_lastCrystalSoundPlayTick`（i32）
    - 惰性衰减模型：强度不在每 tick 更新，仅在 `playAmethystStepSound()` 被调用时一次性补偿衰减
    - 流程：elapsedTicks → intensity
    *= 0.997 ^ elapsedTicks → intensity = min(1.0, intensity + 0.07) → pitch = 0.5 +
    intensity *random * 1.2 → volume = 0.1 + intensity * 1.2 → playSound → lastCrystalSoundPlayTick = ticksExisted -
    冷却：`shouldPlayAmethystStepSound()` 要求距上次播放 ≥ 20 tick -
    这两个字段不参与 NBT 序列化，实体重载后从零开始

    ## #Player::playStepSound 重写

    Player 重写 `playStepSound` 以处理水中步声：
    - 在水中：播放游泳声 + 沉闷步声 -
    非水中：委托给 `Entity::playStepSound`（已包含 COMBINATION / INSIDE /
        紫水晶逻辑）

        ## #容易踩的坑

    - **紫水晶铃声检查对象 * *
        ：`shouldPlayAmethystStepSound` 必须检查脚下方块（`blockState`），不是上方方块（`primaryState`）。站在紫水晶块上铺有地毯时，地毯是 COMBINATION，紫水晶是脚下方块，铃声应触发。
    - **紫水晶铃声强度不持久化 * *
        ：`m_crystalSoundIntensity` 和 `m_lastCrystalSoundPlayTick` 不参与 NBT 序列化，这是 MC 原版行为。实体重载后强度从零开始重新累积。
    -
    **Player 不需要重复 INSIDE / COMBINATION 判断 *
        *：Player 的 `playStepSound` 非水中情况直接调用 `Entity::playStepSound`，避免重复判断导致双重步声。
    - **组合步声音量 * *：上方正常步声 0.15x 音量，下方沉闷步声 0.05x 音量 +
    0.8x 音调。

    ## #canAttackType 攻击类型判断
    - `canAttackType(EntityTypeId typeId)` — 对应 MC 原版 `Mob.canAttackType()` -
    基类默认实现排除恶魂（GHAST），因为恶魂悬浮在高空，大多数近战型 Mob 无法接近，排除恶魂可以避免 Mob
    徒劳地试图攻击一个它们够不着的敌人
    - 子类重写以限制攻击目标类型，例如：
    - `IronGolemEntity::canAttackType()` — 玩家创建的铁傀儡不攻击玩家，所有铁傀儡不攻击苦力怕，其余委托基类（排除恶魂）
    - `PhantomEntity::canAttackType()` — 返回 `true`，覆盖基类排除恶魂的限制，因为幻翼本身会飞行
    - `BreezeEntity::canAttackType()` — 白名单模式，仅允许攻击玩家和铁傀儡
    - 在 `TargetGoal::isSuitableTarget()` 中自动调用，所有目标选择器继承此过滤逻辑 -
    自定义目标选择器（如 `IronGolemNearestAttackableTargetGoal`）需手动调用 `canAttackType()`

    ## #setLastHurtBy 虚方法
    - `LivingEntity::setLastHurtBy()` 现为 `virtual` 方法，允许子类在受到其他实体攻击时执行自定义逻辑 -
    **子类覆写时必须调用基类实现 *
        * `LivingEntity::
            setLastHurtBy()`，否则基类的 `m_lastHurtByPlayer`、`m_lastHurtByMob`、`m_lastHurtByPlayerAtTime` 等字段不会更新，可能导致依赖这些字段的逻辑（如复仇目标选择、击退方向等）失效
    -
    典型覆写示例：`VillagerEntity::
        setLastHurtBy()` 在被玩家攻击时，调用基类实现后额外广播 `VillagerAngry` 粒子并添加 `MinorNegative` 流言

    ## #hurtMarked 受伤标记机制
    - `m_hurtMarked`（bool）— 瞬态标记，实体受到伤害或击退时设为 true
    - `markHurt()` — 设置标记为 true
    - `isHurtMarked()` — 查询标记状态
    - `clearHurtMarked()` — 清除标记（由 EntityTracker 速度同步后调用）
    - 用途：服务端 EntityTracker 在 tick 中检测 `isHurtMarked()`，为 true 时向所有追踪玩家发送 EntityVelocityPacket，然后清除标记；AI 目标检测（如 TradeWithPlayerGoal 检查 `isHurtMarked()` 判断是否中断交易）
    - 字段位于 Entity 类 protected 区域，紧随 `m_invulnerable` 之后
    - 该标记不参与 NBT 序列化，实体重载后从 false 开始

    ## #setAttackTarget 虚方法
    - `MobEntity::setAttackTarget()` 和 `MobEntity::
        attackTarget()` 现为 `virtual` 方法，允许IAngerable实体在设置攻击目标时同步更新愤怒状态
    -
    **IAngerable实体统一使用MobEntity::m_attackTarget *
        *：所有实现IAngerable接口的实体（PiglinEntity、GolemEntity、EndermanEntity、BeeEntity、PolarBearEntity、TameableEntity）不再声明独立的`m_attackTarget`成员，而是复用`MobEntity::
            m_attackTarget` -
    通过`MobEntity *`指针调用`setAttackTarget()`时，虚函数派发会正确到达子类的override，确保愤怒状态与攻击目标始终同步
    - 子类override `setAttackTarget` 时应调用 `MobEntity::setAttackTarget(target)` 设置基类的 `m_attackTarget`
