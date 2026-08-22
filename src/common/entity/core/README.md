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
├── EntityDataManager.hpp #实体数据同步管理（客户端 -
    服务端数据同步）
├── EntityPose.hpp #实体姿态枚举（站立、潜行、游泳、睡眠等）
├── EntitySize.hpp #实体尺寸定义（宽度、高度、眼睛高度）
├── EntityClassification.hpp / cpp #实体分类（怪物、动物、环境等）
├── EntityUtils.hpp #模板型实体工具函数（搜索、距离）
├── DataParameter.hpp #数据参数定义（网络同步用）
├── MoverType.hpp #移动类型枚举（自移、活塞、玩家、弹射物等）
└── README.md
```

注：以下文件已按职责迁移至更合适的目录：
- `VanillaEntities.hpp/cpp`（原版实体类型批量注册）→ `common/entity/registry/`
- `VanillaEntityTypeKeys.hpp/cpp`（原版实体类型指针缓存 `const EntityType*`）→ `common/entity/registry/`
- `EntitySpawnPlacementRegistry.hpp/cpp`（生成位置规则）→ `common/world/spawn/`
- `BoostHelper.hpp`（可骑乘实体加速辅助）→ `common/entity/interfaces/`
- `Crackiness.hpp/cpp`（狼铠裂纹渲染）→ `common/entity/entities/passive/tamable/`

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
    EntityRegistry → EntityType → VanillaEntityTypeKeys（类型注册）
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
    | MelonPumpkinBlocks.cpp | | 海龟孵化 | Natural | TurtleEggBlock.cpp | | 嗅探兽蛋孵化 | Natural | TrailsBlocks.cpp（SnifferEggBlock::randomTick） | | 远古守卫者 | Structure
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
            - `m_fire` 正值表示燃烧剩余 tick，负值表示火焰免疫期倒计时
            - `igniteForSeconds(seconds)` 点燃实体指定秒数（推荐使用），内部转换为 tick
            - `igniteForTicks(ticks)` 点燃实体指定 tick 数，仅在新值大于当前值时更新，同时清除冰冻状态
            - `setFire(ticks)` 已弃用，请使用 igniteForTicks() 或
            igniteForSeconds() - `setRemainingFireTicks(ticks)` 直接设置火焰计时器值（含负值免疫期）
            - `getRemainingFireTicks()` 获取剩余火焰 tick（含负值免疫期）
            - `forceFireTicks(ticks)` 直接设置值，用于增减火焰时间（Player 重写以限制创造模式）
            - `clearFire()` 将正值火焰计时器清零，保留负值免疫期 - `extinguishFire()` 如果正在燃烧则播放灭火音效并调用
            clearFire() - `setFireImmunityCooldown()` 设置火焰免疫期（Player 为 20 tick /
                1 秒，普通实体为 0） - `getFireImmuneTicks()` 虚方法，返回火焰免疫期时长。基类返回 0，Player 返回
            20 - `lavaIgnite()` 岩浆点燃：免疫火焰则跳过，否则 igniteForSeconds(15)（15 秒）
            - `lavaHurt()` 岩浆伤害：免疫火焰则跳过，否则造成 4.0 点岩浆伤害 + 播放 GENERIC_BURN 音效
            - `isOnFire()` 火焰免疫的实体永远不会被认为着火（m_fire
        > 0 &&
    !isImmuneToFire()） - 火焰免疫由 `EntityType::immuneToFire()` 标志决定，子类可重写（如 ItemEntity 检查物品是否防火）
                    - 烈焰人、恶魂、岩浆怪、猪灵系、疣猪兽、潜影贝、Boss实体免疫火焰
                    - 火焰免疫期机制：实体离开火焰 / 岩浆后获得短暂免疫期（m_fire 设为负值），防止立即被重新点燃 -
                    doBlockCollisions() 末尾自动检测并设置免疫期（方块碰撞未点燃且当前不燃烧时触发） -
                    雨中灭火会播放音效并设置免疫期
                    - 水中/岩浆状态（inWater/inLava 等七字段）由 `ecs::sys::environmentSensing`（SystemPhase::EnvironmentSense 阶段，在 EntityTick 之前）每帧根据世界流体驱动写 `EnvironmentStateComponent`，对齐 MC Java `Entity.baseTick()` 中 `updateInWaterStateAndDoFluidPushing()` 的时序。B 阶段前由 `Entity::updateEnvironmentState()` 在 `baseTick()` 火焰处理之前内联调用，现已抽成 system（baseTick 不再内联刷新）。`setInWater()`/`setInLava()` setter **已删除**——外部写入会被 system 每帧覆写。测试中要让实体"在水中/岩浆中"：世界流体驱动型测试（如 EntityLavaFireTest）在 baseTick 前手动调 `runEnvironmentSensing()` 消费世界流体桩写组件；纯逻辑测试（如 updateAirSupply 单测）用 `mc::test::setEntityInWater(entity, true)` 直写组件（见 tests/common/TestWorldHelper.hpp）。客户端本地玩家环境感知失效（见 entity/README 坑20）。

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
                     3. 如果防护槽位为空 → `igniteForSeconds(8)` 点燃实体 8 秒

                     ####调用位置

                    - `MonsterEntity::handleDaylightBurning()` — 当 `m_burnsInDaylight
            ==
            true` 时调用 `burnUndead()`（僵尸、骷髅等） - `ZombieHorseEntity::tick()` — 僵尸马在 BURN_IN_DAYLIGHT
                标签中，直接调用 `burnUndead()` - `PhantomEntity::tick()` — 幻翼在 BURN_IN_DAYLIGHT
                标签中，直接调用 `burnUndead()` -
                **注意 **：骷髅马不在 BURN_IN_DAYLIGHT 标签中，不会在阳光下燃烧

                 ####骷髅马与僵尸马的区别

        | 行为 | 骷髅马 | 僵尸马 | | -- -- --| -- -- -- --| -- -- -- --| | BURN_IN_DAYLIGHT | ✗ | ✓ | | 阳光下燃烧
        | ✗ | ✓ | | 阳光防护槽位 | N / A | Chest（马铠） | | canBreatheUnderwater | ✓ | ✗ |

        ####实现注意事项

            - `burnUndead()` 使用 `ItemStack::setDamage()` 直接增加伤害值， **绕过耐久保护附魔 **（与 MC 原版一致）
            - `isInDaylight()` 中 `isWet()` 检查确保雨中和水中的亡灵不会燃烧
            - 物品耐久耗尽后调用 `onEquippedItemBroken` 回调（广播装备破损动画 +
            播放 ENTITY_ITEM_BREAK 音效）

            ## #装备损坏回调

            当装备物品耐久度耗尽时，需要调用 `onEquippedItemBroken` 回调，对应 MC 原版 `LivingEntity
                .onEquippedItemBroken()`。

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

                ##装备变化检测与属性修饰符同步

                LivingEntity 在每个 tick 中自动检测装备变化并同步属性修饰符，对应 MC
                原版 `collectEquipmentChanges()` / `stopLocationBasedEffects()` 机制。

                    **新增方法 **：

            - **`detectEquipmentUpdates()`** — 每tick调用，检测装备变化并应用 / 移除属性修饰符
            - 首次调用初始化装备快照 `m_lastEquipment`（不应用修饰符） -
            后续调用比较当前装备与快照，检测变化后： 1. 对旧物品调用 `stopLocationBasedEffects()` 移除属性修饰符
            2. 对新物品通过 `item->getAttributeModifiers(slot)` 获取修饰符并添加到 `AttributeMap` -
            客户端世界（`isClientSide()`）跳过检测

            - **`stopLocationBasedEffects(stack, slot)`** — 移除物品在指定槽位提供的属性修饰符
            - 遍历 `item->getAttributeModifiers(slot)` 中匹配槽位的条目，逐一调用 `AttributeMap::removeModifier()` -
              同时通过 `EnchantmentHelper::stopLocationBasedEffects()` 停用灵魂疾行、冰霜行者等位置依赖附魔效果

            - **`equipmentHasChanged(a, b)`**(静态) — 比较两个 ItemStack 是否发生变化（物品类型、数量、伤害值）

            - **`onEquippedItemBroken(item, slot)`**(虚方法) — 装备损坏回调，现已集成 `stopLocationBasedEffects()` 调用
            - 在广播破损动画和音效之前，先移除损坏物品的属性修饰符 -
            确保损坏物品不再影响实体的属性值

                **新增成员变量 **：

            - `m_lastEquipment` — `std::array<ItemStack, 6>` 装备快照，记录上次 tick 的装备状态
            - `m_lastEquipmentInitialized` — `bool` 标记快照是否已初始化

                    **Player 子类兼容性 **：

                Player
                重写了 `getEquipment()`/`setEquipment()`/`getMutableEquipment()` 虚方法，通过 `PlayerInventory` 间接管理装备。`detectEquipmentUpdates()` 通过虚方法调用读取装备，因此
                Player 实例的装备快照正确反映 PlayerInventory 中的数据。快照使用 `ItemStack` 值拷贝，与底层存储无关。

                    **可变装备访问**：

                `getMutableEquipment(EquipmentSlot)` 返回 `ItemStack&` 可变引用，用于需要直接修改装备物品的场景（如附魔耐久消耗 `hurtAndBreak`、弩装填 `setCharged` 等）。Player 子类重写此方法以委托到 `PlayerInventory` 的可变引用访问器。便利方法 `getMutableMainHandItem()` 和 `getMutableOffHandItem()` 分别对应主手和副手槽位。所有需要修改装备的代码应使用这些方法而非 `const_cast<ItemStack&>(getEquipment(...))`。

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

        ## #方块速度因子（getBlockSpeedFactor）

        `LivingEntity::getBlockSpeedFactor()` 返回实体脚下方块的有效速度因子，考虑 `MOVEMENT_EFFICIENCY` 属性的插值效果。

        ### 计算公式

        ```
        finalSpeedFactor = lerp(movementEfficiency, blockSpeedFactor, 1.0)
        ```

        即 `blockSpeedFactor + (1.0 - blockSpeedFactor) * movementEfficiency`

        - 当 `movementEfficiency=0.0`（默认）：返回方块原始 `speedFactor`（灵魂沙=0.4，正常方块=1.0）
        - 当 `movementEfficiency=1.0`：返回 1.0，完全忽略方块减速效果

        ### 方块 speedFactor

        大多数方块 `speedFactor=1.0`（无减速）。灵魂沙 `speedFactor=0.4`，蜂蜜块 `speedFactor=0.4`。通过 `BlockState::getSpeedFactor()` 获取。

        ### 调用位置

        - `LivingEntity::travel()` — 地面移动速度计算：`moveFactor *= getBlockSpeedFactor()`
        - `Player::_applyCachedMovementInput()` — 玩家地面移动速度计算：`speedFactor *= getBlockSpeedFactor()`

        ### 与灵魂疾行附魔的关系

        灵魂疾行附魔通过 Addition 操作为 `MOVEMENT_EFFICIENCY` 属性添加 +1.0 修饰符，使 `getBlockSpeedFactor()` 在灵魂沙/灵魂土上返回 1.0，完全抵消减速效果。同时附魔还为 `MOVEMENT_SPEED` 添加加成。

        ### 属性注册

        `MOVEMENT_EFFICIENCY` 在 `LivingEntity::registerAttributes()` 中注册，所有生物实体默认拥有此属性（默认值 0.0，范围 0.0~1.0）。

        ## #空气供应与溺水
        - 空气值通过 `Entity::m_air` / `DATA_AIR_PARAM` 管理，默认最大值 300（15秒），子类可覆写 `maxAir()`
        - 空气处理完全在 `LivingEntity::updateAirSupply()` 中，`Entity::baseTick()` 不包含空气逻辑（与 MC Java 一致）
        - 使用 `isInWater()` 检测水中状态，排除气泡柱（Bubble Column）中的空气消耗
        - 当空气值降到 -20 时，`shouldTakeDrowningDamage()` 返回 true，重置空气为 0 并触发 2.0 点溺水伤害
        - 溺水时通过 `broadcastEntityStatus(id, 67)` 广播实体事件（客户端用于播放溺水动画/音效）
        - 亡灵生物 `canBreatheUnderwater()` 返回 true，不会溺水
        - 水下呼吸附魔通过 `decreaseAirSupply()` 概率性节约空气：I级50%、II级66.7%、III级75%
        - 有水下呼吸/潮涌效果时，在水中不消耗空气且恢复空气（每tick +4）
        - 仅在服务端处理空气逻辑，客户端通过 `DATA_AIR_PARAM` 元数据同步获取空气值
        - `increaseAirSupply()` 每tick恢复4点空气，上限为 `maxAir()`
        - WaterMobEntity 使用反逻辑：水中立即恢复 `maxAir()`，陆地上消耗空气
- **水下强制下坐骑**：当乘客眼睛位置在水中（非气泡柱）且所骑乘的载具类型属于 `EntityTypeTags::DISMOUNTS_UNDERWATER` 标签时，`updateAirSupply()` 会调用 `stopRiding()` 强制乘客下坐骑
  - 对应 MC Java 的 `if (this.isPassenger() && this.getVehicle() != null && this.getVehicle().dismountsUnderwater()) { this.stopRiding(); }`
  - `Entity::dismountsUnderwater()` 虚方法默认实现通过 `EntityTypeTags::DISMOUNTS_UNDERWATER().contains(getTypeId())` 查询标签
  - DISMOUNTS_UNDERWATER 标签包含：骆驼、鸡、驴、快乐恶魂、马、羊驼、骡、猪、劫掠兽、蜘蛛、炽足兽、行商羊驼、僵尸马（共13种）
  - 船不在该标签中（船有自己的水下沉没逻辑 `m_outOfControlTicks`），矿车和玩家也不在其中

## #冰冻系统（Freeze System）

对应 MC Java 的粉末雪冰冻机制（`Entity.ticksFrozen` / `LivingEntity.tickFreeze`），包含冰冻计时器、冰冻伤害、冰冻减速修饰符、冰冻免疫等完整逻辑。

### 核心常量

- `BASE_TICKS_REQUIRED_TO_FREEZE = 140` — 完全冰冻所需 tick 数（7 秒）
- `FREEZE_HURT_FREQUENCY = 40` — 冰冻伤害触发频率（每 40 tick / 2 秒一次）

### Entity 层冰冻状态

冰冻状态数据存于 `ecs::FreezeComponent`（真相源），`DATA_TICKS_FROZEN_PARAM` 退为客户端同步镜像。所有读写经 `tryGetComponent<FreezeComponent>`，不进 `m_builtIn` 缓存（低频）。

- `FreezeComponent.m_ticksFrozen`（i32）— 冰冻计时器，正值表示冰冻进度，达到 `getTicksRequiredToFreeze()` 时完全冰冻
- `FreezeComponent.m_isInPowderSnow`（bool）— 当前 tick 是否处于细雪中，每帧由 `baseTick()` 重置为 false，由 `PowderSnowBlock::onEntityCollision()` 设置为 true
- `DATA_TICKS_FROZEN_PARAM` — 客户端同步镜像（非真相源），`setTicksFrozen()` 内同时写组件与镜像；服务端 `syncMetadataFromDataManager` 不再从镜像回填组件
- `clearFreeze()` — 虚方法，基类调 `setTicksFrozen(0)`，LivingEntity 重写版本额外移除冰冻减速修饰符
- `canFreeze()` — 虚方法，基类检查 `EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES`（安全检查：标签未初始化时默认允许冰冻），LivingEntity 重写版本额外检查 `ItemTags::FREEZE_IMMUNE_WEARABLES`（皮革护甲）
- `getPercentFrozen()` — 返回 `min(getTicksFrozen(), required) / required`，值域 [0.0, 1.0]
- `isFullyFrozen()` — `getTicksFrozen() >= getTicksRequiredToFreeze()`
- `isFreezing()` — `getTicksFrozen() > 0`
- `setTicksFrozen(i32)` / `getTicksFrozen()` — 冰冻计时器存取（setter 同时写组件 + 同步镜像）
- `setIsInPowderSnow(bool)` / `isInPowderSnow()` — 细雪状态存取

### LivingEntity 冰冻逻辑

#### tickFreeze() — 每帧冰冻 tick 处理

对应 MC Java `LivingEntity.baseTick()` 中的 "freezing" 段，在 `LivingEntity::tick()` 中 `tickHealth()` 之后调用：

1. **冰冻计时器递减**：不在细雪中或不可冰冻时，每 tick -2（解冻速度是冰冻速度的两倍）
2. **冰冻减速修饰符更新**：先 `removeFrost()` 移除旧修饰符，再 `tryAddFrost()` 添加新修饰符
3. **冰冻伤害**：每 40 tick 且完全冰冻且可冰冻时，造成 1.0 冰冻伤害。非玩家实体始终受到冰冻伤害，玩家通过 `Player::isInvulnerableTo()` 检查 `FREEZE_DAMAGE` 游戏规则

#### 冰冻减速修饰符

- UUID：`SPEED_MODIFIER_POWDER_SNOW_UUID = "1e7a5c3c-6f4a-4b6b-8c3d-5e2f1a0b9c8d"`
- 操作：`Operation::Addition`，值 = `-0.05 * getPercentFrozen()`
- 完全冰冻时减少 0.05 移动速度（基础速度 0.1，减速 50%）
- 仅在 `ticksFrozen > 0` 且脚下方块非空气时添加（`tryAddFrost()` 检查 `onPos()` 方块状态）

#### 冰冻额外伤害

- 在 `actuallyHurt()` 中：冰冻伤害源（`source.isFreezing()`）+ `EntityTypeTags::FREEZE_HURTS_EXTRA_TYPES` 标签中的实体，伤害 ×5
- 额外伤害标签包含：烈焰人（blaze）、岩浆怪（magma_cube）、炽足兽（strider）

### 冰冻免疫标签

| 标签 | 实体 | 效果 |
|------|------|------|
| `FREEZE_IMMUNE_ENTITY_TYPES` | 流浪者（stray）、北极熊（polar_bear）、雪傀儡（snow_golem）、凋灵（wither） | `canFreeze()` 返回 false，冰冻计时器不递增且不受伤 |
| `FREEZE_HURTS_EXTRA_TYPES` | 烈焰人（blaze）、岩浆怪（magma_cube）、炽足兽（strider） | 冰冻伤害 ×5 |
| `FREEZE_IMMUNE_WEARABLES` | 皮革头盔、皮革胸甲、皮革护腿、皮革靴子、皮革马铠 | 任意一件皮革护甲使 `LivingEntity::canFreeze()` 返回 false |

### 冰冻伤害类型

- `DamageType::Freeze` — 冰冻伤害，绕过护甲（`bypassesArmor()` 返回 true）
- `DamageSources::freeze()` — 创建冰冻伤害源
- 死亡消息键：`"death.attack.freeze"`
- `FREEZE_DAMAGE` 游戏规则仅影响玩家（`Player::isInvulnerableTo()` 检查），非玩家实体始终受到冰冻伤害

### 点燃时清除冰冻

- `igniteForSeconds()` / `igniteForTicks()` — 点燃时自动调用 `clearFreeze()` 清除冰冻状态

### NBT 序列化

- 冰冻计时器键：`"TicksFrozen"`（EntityNbtKeys::TICKS_FROZEN），仅当值 > 0 时写入，读取时使用 `tryGetInt` 容错
- 冰冻计时器通过 `DATA_TICKS_FROZEN_PARAM` 同步到客户端

### 与方块系统的交互

`PowderSnowBlock::onEntityCollision()` 负责冰冻计时器的递增：
1. 设置实体 `setIsInPowderSnow(true)`
2. 如果 `canFreeze()` 为 true，递增 `ticksFrozen`（上限 `getTicksRequiredToFreeze()`）
3. 设置运动减速乘数 `Vector3(0.9, 0.9, 0.9)`

冰冻计时器的递减和伤害处理由 `LivingEntity::tickFreeze()` 负责，递增由 `PowderSnowBlock::onEntityCollision()` 负责。

### 测试覆盖

- `tests/common/entity/core/EntityFreezeTest.cpp` — 冰冻系统完整单元测试
  - Entity 层冰冻状态（getTicksFrozen/setTicksFrozen, getPercentFrozen, isFullyFrozen, isFreezing, canFreeze, isInPowderSnow, clearFreeze）
  - LivingEntity 层冰冻逻辑（tickFreeze 递减/伤害/修饰符, removeFrost, tryAddFrost, clearFreeze）
  - DamageSource::Freeze 伤害类型和 isFreezing() 检测
  - FREEZE_DAMAGE 游戏规则对冰冻伤害的控制
  - 冰冻减速修饰符（SPEED_MODIFIER_POWDER_SNOW_UUID）
  - baseTick 中 isInPowderSnow 重置逻辑

        ## #队伍联盟判断 -
        **`isAlliedTo(const Entity&)`* *-双向联盟检查：this 认为 other 是盟友，或 other 认为 this 是盟友 -
        **`isAlliedTo(const scoreboard::Team*)`* *-队伍级联盟检查（虚方法，TameableEntity 重写以继承主人队伍） -
        **`considersEntityAsAlly(const Entity&)`*
            *-虚方法，自定义单向盟友判定逻辑，默认委托给 `isAlliedTo(other.getTeam())` -
        **`isOnSameTeam(const Entity&)`*
            *-旧版 API，仅单向检查 this 是否属于 other 的队伍，新代码应优先使用 `isAlliedTo()` -
        使用 * *指针相等性 * *比较队伍，而非队伍名称比较 - 两个 Team 对象即使名称相同，指针不同也不算同一队伍 -
        没有队伍的实体（`getTeam()` 返回 nullptr）不会与任何队伍匹配

    ## #位置与高度偏移访问器（getY / getEyeY）

    对应 MC 1.21.11 `Entity.getY(double partialY)` / `Entity.getEyeY()`，提供按高度比例偏移的 Y 坐标访问入口，统一瞄准点、发射位置、视线检测等场景的 Y 坐标计算。

    ### 核心方法

    - **`getY(f64 partialY) const`** — 返回 `position.y + height * partialY`。partialY 取值约定（参考 MC 原版调用）：
      - `0.0`：脚部 Y（等价于 `y()`）
      - `1.0/3.0`：躯干下部，弓/弩/三叉戟瞄准点（`AbstractSkeleton`/`DrownedEntity`/`IllusionerEntity`/`IllagerEntities` 的 `performRangedAttack`）
      - `0.5`：身体几何中心
      - `0.8`：接近头部，旋风人风弹对骑乘目标的瞄准点（`BreezeEntity::shootWindCharge`）、钓鱼钩吸附位置（`FishingHook`）
      - `1.0`：实体头顶
    - **`getEyeY() const`** — 返回 `position.y + eyeHeight`，对应 MC `Entity.getEyeY()`。用于视线检测（`Entity::canSee`）、玩家眼睛位置（`Player::getEyePosition`）、弹射物发射位置（`LlamaEntity`/`DrownedEntity`/`WitchEntity`/`NetherEntities`/`IllagerEntities`）、凋灵侧头瞄准（`WitherEntity::_updateSideHeadRotations`）等。

    ### 与既有访问器的关系

    - `y()` 返回 `f32` 脚部 Y；`getY(partialY)`/`getEyeY()` 返回 `f64` 以匹配 MC 原版精度。
    - `getEyeY()` 使用 `eyeHeight()`（姿态相关，潜行/游泳时降低），`getY(1.0)` 使用 `height()`（碰撞盒高度），两者语义不同。
    - 新增瞄准/发射位置代码应优先使用 `getY(partialY)`/`getEyeY()`，而非内联 `y() + height() * k` / `y() + eyeHeight()`。

        ## #传送系统使用 - `attemptTeleport(x, y, z)` -
        安全传送，自动查找地面 - `randomTeleport(range, playEffects, avoidFluid)` - 随机传送 -
        传送会自动重置运动向量

        ## #类型标识符获取 - `entityType()` 返回 `const EntityType*`（懒缓存），可直接与 `VanillaEntityTypeKeys` 命名空间中的指针常量比较 - `getTypeId()` 返回 `const std::string&`（如 `"minecraft:pig"`，用于字符串型查询如 `getEntitiesByType()`） - `legacyType()` 返回 `LegacyEntityType` 枚举（旧版，仅用于兼容） -
        新代码应使用 `entityType()` 做指针比较，或 `getTypeId()` 做字符串查询

            ## #战利品表ID获取（getLootTableId）

        `Entity::getLootTableId()` 是虚方法，返回实体对应的战利品表资源路径（如 `"minecraft:entities/pig"`）。

            * *方法层次 * *： -
        **`Entity::getLootTableId()`* *（基类）：从 `m_typeId` 推导默认路径，格式为 `<namespace> : entities /
            <path>`（如 `"minecraft:pig"` → `"minecraft:entities/pig"`）。`m_typeId` 为空时返回空字符串。 -
        **`MobEntity::getLootTableId()`*
            *（覆写）：优先返回 NBT 自定义掉落表 `m_deathLootTable`（对应 MC Java 的 `DeathLootTable` NBT 标签），为空或
                 nullopt 时回退到基类实现。逻辑：`Mob.getLootTable()`：`this.lootTable.isPresent()
    ? this.lootTable
    : super.getLootTable()`。 -
            **无战利品表实体覆写 *
                *：以下实体覆写返回空字符串，对应 `EntityType.Builder
                     .noLootTable()`： - `ProjectileEntity`（覆盖所有弹射物子类：箭矢、三叉戟、火球等） - `ItemEntity`（掉落物） - `ExperienceOrbEntity`（经验球） - `BoatEntity`（船） - `AbstractMinecartEntity`（覆盖所有矿车变体） - `HangingEntity`（覆盖画、物品框、拴绳结） - `AreaEffectCloudEntity`、`EnderCrystalEntity`、`LightningBoltEntity`（效果实体） - `FallingBlockEntity`、`TNTEntity`（杂项实体） - `FishingBobberEntity`、`EvokerFangsEntity`、`EyeOfEnderEntity`、`FireworkRocketEntity`（其他弹射物）

                * *使用场景 * *： - `/ loot kill` 命令：从目标实体获取战利品表ID，生成击杀掉落物 -
            实体死亡掉落逻辑：`MobEntity` 死亡时使用 `getLootTableId()` 查询战利品表

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
            数据参数必须在 `registerData()` 中注册。所有数据参数必须通过 `EntityDataManager::createKey<
                T>()` 自动分配唯一 ID，禁止硬编码 ID 值。客户端通过 `EntityDataManager` 同步数据。

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
            判断生物是否可被拴绳拴住，默认实现通过 `hasComponent<MobFlagComponent>()` 判断（敌对生物不可拴绳，IMob 接口的 tag 层） -
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
    [5] = Head, [6] = Body, [7] = Saddle - 默认值为 `DEFAULT_EQUIPMENT_DROP_CHANCE = 0.085f`（8.5 %） -
    大于 1.0 的值表示物品被保留（`PRESERVE_ITEM_DROP_CHANCE = 2.0f`） - `isEquipmentDropPreserved(slot)` 检查掉落概率 > 1.0
    - `setGuaranteedDrop(slot)` 设置掉落概率为 2.0（保整掉落）
    - `dropPreservedEquipment(predicate)` 遍历装备槽位，根据谓词和保留状态处理装备（用于实体转化场景）
    - `dropPreservedEquipment()` 无谓词版本，所有装备都参与掉落/保留判断
    - NBT 序列化格式：
        - 保存时仅写入新格式（MC 1.21.4 +）：`drop_chances`（compound，仅包含非默认值）
        - 读取时优先使用新格式，然后回退到旧格式（`HandDropChances` float[2] + `ArmorDropChances` float[4]）以兼容旧存档
    - `EquipmentSlot::Saddle` 用于铜傀儡天线槽（对应 MC 1.21.11 `CopperGolem.EQUIPMENT_SLOT_ANTENNA`），
      由铁傀儡 `OfferFlowerGoal` 装备罂粟花

        ## #死亡掉落表(
            DeathLootTable) - `m_deathLootTable`：可选字符串，覆盖实体类型的默认掉落表（格式如 `"minecraft:entities/"
                                                                                                "zombi"
                                                                                                "e"`） - `m_lootTableSeed`：确定性种子，0
        表示随机
        - NBT 键：`DeathLootTable`（string，仅在有值时写入）、`DeathLootTableSeed`（long，仅非零时写入）
        - 对应 MC 原版 Mob 的 `lootTable` 和 `lootTableSeed` 字段 -
        通过 `MobEntity::getLootTableId()` 虚方法统一访问：优先返回 `m_deathLootTable`，为空时回退到 `Entity::
            getLootTableId()`（从 typeId 推导默认路径）

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

        遍历实体碰撞箱覆盖的所有方块，对每个非空气方块进行**形状感知检测**：

        1. 获取方块的 `getEntityInsideCollisionShape()` 返回形状
        2. 如果形状为 `fullCube()`（快速路径），视为实体在方块内部
        3. 如果形状非空，检查实体 AABB 与方块形状（偏移到世界坐标后）是否相交
        4. 相交时调用：
           - `Block::onEntityCollision()` — 方块对实体的碰撞回调（仙人掌伤害、蜘蛛网减速、炼药锅灭火等）
           - `Entity::onInsideBlock()` — 实体"在方块内部"的回调（传送门检测等）
           - 自定义方块组件 `onEntity` 事件派发

        对应 MC 原版 `Entity.checkInsideBlocks()`，由 `LivingEntity.aiStep()` 中的
    `applyEffectsFromBlocks()` 调用。

        * *形状感知检测 * *：使用 `getEntityInsideCollisionShape()` 而非简单 AABB-vs-网格位置判断，确保空心方块（如炼药锅）只在实体实际进入内容区域时触发 `onEntityCollision`。大多数方块返回 `fullCube()`，走快速路径无额外开销。炼药锅返回外部壁∪内容区域的填充形状，岩浆炼药锅返回外部壁∪岩浆内容形状。

        * *调用位置 * *： - `LivingEntity::aiStep()` — 在 `travel()` 之后调用，所有 LivingEntity 子类自动继承
    - `Entity::moveWithCollision()` noClip 路径 — 即使 noClip = true 也要触发碰撞
    - `BoatEntity::tick()` — 手动调用（BoatEntity
          .canTriggerWalking() = false） - `ThrowableEntity::tick()` — 投射物需要在 tick 中手动调用

            * *典型方块碰撞效果 * *： |
    方块 | 回调 | 效果 | | -- -- --| -- -- --| -- -- --| | CactusBlock | onEntityCollision
    | 对 LivingEntity 造成 1.0 伤害 | | WebBlock | onEntityCollision | 水平速度 ×0.25，垂直速度 ×0.05 |
    | SweetBerryBushBlock | onEntityCollision | 伤害 + 减速（非潜行时） | | BubbleColumnBlock | onEntityCollision
    | 上推 / 下拉 Y 速度，重置摔落距离 | | NetherPortalBlock | onEntityCollision | 设置传送门状态 | | FireBlock
    | onEntityCollision | 点燃实体 | | HoneyBlock | onEntityCollision | 水平速度 ×0.4，下滑减速 | | CauldronBlock | onEntityCollision | 灭火并降低水位（有水时） | | LavaCauldronBlock | onEntityCollision | 基类行为：点燃+岩浆伤害（始终满） |

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
            - `canAttackType(const entity::EntityType& type)` — 对应 MC 原版 `Mob.canAttackType()` -
            基类默认实现排除恶魂（GHAST），因为恶魂悬浮在高空，大多数近战型 Mob 无法接近，排除恶魂可以避免 Mob
            徒劳地试图攻击一个它们够不着的敌人
            - 子类重写以限制攻击目标类型，例如： - `IronGolemEntity::
                  canAttackType()` — 玩家创建的铁傀儡不攻击玩家，所有铁傀儡不攻击苦力怕，其余委托基类（排除恶魂）
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

## #setYBodyRot / setYHeadRot 虚方法（身体/头部偏航角同步）

对应 MC 1.21.11 `Entity#setYBodyRot` / `Entity#setYHeadRot`，用于同步实体身体与头部朝向。

### 方法层次

- **`Entity::setYBodyRot(f32 yaw)`**（基类，虚方法）：默认空实现（no-op）。对应 MC `Entity.setYBodyRot` 同样为空方法。
- **`Entity::setYHeadRot(f32 yaw)`**（基类，虚方法）：默认空实现（no-op）。对应 MC `Entity.setYHeadRot` 同样为空方法。
- **`LivingEntity::setYBodyRot(f32 yaw)`**（重写）：写入 `m_renderYawOffset` 字段（对应 MC `LivingEntity.yBodyRot`）。
- **`LivingEntity::setYHeadRot(f32 yaw)`**（重写）：写入 `m_rotationYawHead` 字段（对应 MC `LivingEntity.yHeadRot`）。

### 设计意图

基类提供空实现而非纯虚方法，使得通用代码（如结构模板放置实体、NBT 加载）可对任意 `Entity*` 调用 `setYBodyRot` / `setYHeadRot`，无需调用方做 `dynamic_cast<LivingEntity*>` 判断。非 `LivingEntity` 子类（如 `ItemEntity`、`ExperienceOrbEntity`）调用时为 no-op，安全无副作用。

### 字段对照表

| 项目字段 | MC Java 字段 | 含义 |
|----------|-------------|------|
| `m_renderYawOffset` | `yBodyRot` | 身体旋转偏移（视觉身体朝向） |
| `m_rotationYawHead` | `yHeadRot` | 头部旋转（视觉头部朝向） |

注意：项目中等价字段为 `m_renderYawOffset`（body）/ `m_rotationYawHead`（head），与 MC Java 的 `yBodyRot` / `yHeadRot` 一一对应。另提供 `renderYawOffset()` / `setRenderYawOffset()` 和 `rotationYawHead()` / `setRotationYawHead()` 作为直接访问器，但通用代码应优先使用 `setYBodyRot` / `setYHeadRot` 虚方法以支持多态。

### 调用场景

1. **`Template::placeInWorld`**：结构模板放置实体时，在 `setRotation(finalYaw, pitch)` 后调用 `setYBodyRot(finalYaw)` / `setYHeadRot(finalYaw)`，让首帧身体/头部朝向跟随结构旋转。对应 MC 1.21.11 `StructureTemplate#placeEntities` 中的 `setYBodyRot(f)` / `setYHeadRot(f)`。
2. **`Entity::readFromNBT`**：加载 NBT 的 `Rotation` 标签后，调用 `setYBodyRot(m_yaw)` / `setYHeadRot(m_yaw)` 同步身体/头部朝向到 yaw。对应 MC 1.21.11 `Entity#load` 中的 `setYBodyRot(getYRot())` / `setYHeadRot(getYRot())`。
3. **`CopperGolemStatueBlockEntity::removeStatue`**：铜傀儡雕像复活为实体时，调用 `setYBodyRot(yaw)` / `setYHeadRot(yaw)` 同步身体/头部朝向到 FACING 方向。对应 MC 1.21.11 `CopperGolemStatueBlockEntity#initCopperGolem`。

### 与 AI LookController 的关系

`LivingEntity::tick()` 中的 AI LookController 会在每 tick 更新 `m_rotationYawHead`（通过 `setRotationYawHead` 直接写入），让头部朝向当前关注目标。结构模板放置时的 `setYHeadRot` 仅设置首帧朝向，后续 tick 由 AI 接管。

### 测试覆盖

- `tests/common/world/gen/feature/template/TemplateEntityPlacementTest.cpp` — 12 个测试用例覆盖：
  - `setYBodyRot` / `setYHeadRot` 接口直接验证（基类分发、LivingEntity 写入字段）
  - `placeInWorld` 各种旋转（90/180/270）、镜像（FrontBack/LeftRight）、组合场景下 body/head 同步
  - 非零 NBT yaw 的变换验证
  - `readFromNBT` 同步 body/head 验证

            ## #hurtMarked 受伤标记机制
            - `m_hurtMarked`（bool）— 瞬态标记，实体受到伤害或击退时设为 true - `markHurt()` — 设置标记为
            true - `isHurtMarked()` — 查询标记状态
            - `clearHurtMarked()` — 清除标记（由 EntityTracker 速度同步后调用） -
            用途：服务端 EntityTracker 在 tick 中检测 `isHurtMarked()`，为
            true 时向所有追踪玩家发送 EntityVelocityPacket，然后清除标记；AI 目标检测（如
            TradeWithPlayerGoal 检查 `isHurtMarked()` 判断是否中断交易）
            - 字段位于 Entity 类 protected 区域，紧随 `m_invulnerable` 之后 -
            该标记不参与 NBT 序列化，实体重载后从 false 开始

                ## #causeExtraKnockback 额外击退机制

                对应 MC Java 的 `LivingEntity.causeExtraKnockback()` / `Player.causeExtraKnockback()`，用于疾跑
                /
                附魔击退的速度修正。

                ## #基类 LivingEntity::causeExtraKnockback(target, strength, preHurtVelocity)

            - 当 `strength
        > 0` 时：基于攻击者朝向（yaw）对目标施加击退，攻击者水平速度 ×0.6 减速
            - 当 `strength <= 0` 时：不施加击退，攻击者也不减速
            - 注意：基类 **不 **调用 `setSprinting(false)`，那是 Player 子类的行为
            - `preHurtVelocity` 参数在基类中未使用（仅 Player 子类需要）

            ## #Player 子类重写

            - 在基类击退逻辑基础上增加 `setSprinting(false)`（疾跑停止）
            - ServerPlayer 目标速度重复应用修复：当 `target.isHurtMarked() &&
    targetPlayer->sendVelocityPacket()` 返回
                true 时，立即清除 hurtMarked 并恢复 preHurtVelocity，避免 EntityTracker::tick() 重复发送速度包

            ## #getKnockback 击退强度计算

            `LivingEntity::getKnockback(Entity& target)` 返回攻击击退强度，计算公式为：
            ```
            (ATTACK_KNOCKBACK属性 + 击退附魔加成) / 2.0
            ```

            - `ATTACK_KNOCKBACK` 属性默认值为 0.0，大多数生物不注册此属性
            - 击退附魔加成通过 `KnockbackEnchantment::getKnockbackBonus(level)` 计算（每级 +0.5）
            - 除以 2.0 是因为 `hurt()` 中已有 0.4 的基础击退，`causeExtraKnockback` 的击退值需要减半以保持总击退合理
            - 子类可重写此方法以定制击退行为

            **调用位置**：
            - `MobEntity::attackEntityAsMob()` — 调用 `getKnockback(target)` 获取击退强度后传给 `causeExtraKnockback()`
            - `Player::attack()` — 直接内联计算击退强度（疾跑 + 附魔），不调用 `getKnockback()`

            ## #applyKnockback 零向量随机扰动

            `LivingEntity::applyKnockback(strength, ratioX, ratioZ)` 中，当方向向量过小（长度平方 < 1.0E-5）时，
            不直接返回，而是对方向向量添加随机扰动，避免零向量导致无击退的问题。
            扰动方式：`ratioX = (random - random) * 0.01`，`ratioZ = (random - random) * 0.01`。

                ## #sendVelocityPacket() 虚方法

                - Player 基类返回 `false`（空操作） - ServerPlayer 重写版本实际发送 EntityVelocityPacket 并返回 `true` -
                此设计避免 common 层对 server 层的依赖，通过虚方法桥接

                    ## #乘客系统与压力板触发

                    Entity 基类提供两层乘客准入检查和压力板触发控制，对应 MC Java
                    的 `couldAcceptPassenger()` / `canAddPassenger()` / `isIgnoringBlockTriggers()`。

                    ## #乘客准入检查

                -
                **`couldAcceptPassenger()`** — 硬门槛，返回 `false` 表示该实体完全不能接受乘客。基类默认返回 `true`。 -
                **`canAddPassenger(const Entity& passenger)`*
                      * — 软门槛，允许基于乘客身份和当前乘客数量做细粒度判断。基类默认返回 `m_passengers.size() <
            getMaxPassengers()`。 -
                **`getMaxPassengers()`** — 最大乘客数量，基类默认返回 1。

                    *
                    *调用链 *
                        *：`startRiding()` 负责循环检测、准入检查（先 `couldAcceptPassenger()` 再 `canAddPassenger()`），然后设置 vehicle 字段并调用 `addPassenger()`。`addPassenger()` 仅操作乘客列表，不进行循环检测。

                    ## #骑乘关系建立与解除流程

                    ### startRiding() 建立骑乘关系

                    1. 检查不能骑乘自己
                    2. 检查是否已在骑乘同一载具（避免重复骑乘）
                    3. 硬门槛：`couldAcceptPassenger()`
                    4. 循环检测：沿 vehicle 链向上遍历，检查是否形成环（需要 World 环境）
                    5. 软门槛：`canBeRidden()`（潜行状态、骑乘冷却）和 `canAddPassenger()`
                    6. 如已在骑乘其他载具，先调用 `stopRiding()`
                    7. 设置 `m_vehicle = vehicle.id()`（**先于** addPassenger）
                    8. 调用 `vehicle.addPassenger(*this)`；失败时回滚 `m_vehicle = INVALID_ENTITY_ID`

                    ### addPassenger() 添加乘客

                    - 验证 `passenger.getVehicle() == m_id`（前置检查，对齐 MC Java）
                    - 若 `passenger.getVehicle() != m_id`，触发 `MC_ASSERT_RELEASE_MSG` 断言并返回 false
                      （MC Java 抛出 IllegalStateException，C++ 项目使用断言对齐此行为）
                    - 检查 `couldAcceptPassenger()` 和 `canAddPassenger()`
                    - 将 passenger 添加到乘客列表（服务端玩家插入头部）
                    - 设置骑乘冷却 `rideCooldown = 60`

                    ### removePassenger() 移除乘客

                    - 按 passenger id 在乘客列表中查找并移除
                    - 验证 `passenger.getVehicle() != m_id`（对齐 MC Java：stopRiding 先清空 vehicle 再调用 removePassenger）
                    - 若 `passenger.getVehicle()` 仍指向本载具，触发断言（调用顺序错误）
                    - 设置骑乘冷却 `rideCooldown = 60`

                    ### isRidingOrBeingRiddenBy() 双向骑乘关系检查

                    - 向下搜索：检查 other 是否是 this 的间接乘客（遍历 passengers 子树）
                    - 向上搜索：检查 other 是否是 this 的间接载具（沿 vehicle 链向上遍历）
                    - 需要 World 环境（`m_world != nullptr`），无 World 时返回 false

                        **子类覆写示例 **：

        | 实体 | couldAcceptPassenger | canAddPassenger | getMaxPassengers | 说明 | | -- -- --|
        -- -- -- -- -- -- -- -- -- -- -| -- -- -- -- -- -- -- -- -| -- -- -- -- -- -- -- -- --| -- -- --|
        | Entity（基类） | true | passengers < max | 1 | 默认单乘客 | | BoatEntity | true（继承） | passengers < 2 &&
    非水下 | — | 覆写 canAddPassenger，水下禁止乘客 | | OminousItemSpawnerEntity | **false * *|
        **false * *| — | 完全禁止乘客 | | AbstractHorseEntity | true（继承） | passengers < max |
        2（马类） | 覆写 getMaxPassengers |

        ## #压力板触发控制

            - **`doesEntityNotTriggerPressurePlate()`** — 返回 `true` 表示该实体不触发压力板。基类默认返回 `false`。

                   **覆写情况 **（对应 MC Java 的 `isIgnoringBlockTriggers()`）：

        | 实体 | 返回值 | 说明 | | -- -- --| -- -- -- --| -- -- --| | Entity（基类） | false | 默认触发压力板 |
        | BatEntity | true | 蝙蝠不触发压力板 |
        | ArmorStandEntity（marker = true） | true | 标记模式盔甲架不触发压力板 | | ArmorStandEntity（marker = false） |
    false | 普通盔甲架触发压力板 | | OminousItemSpawnerEntity | true | 不祥物品生成器不触发压力板 |

    **压力板灵敏度分类 * *：

        - **石质 / 磨制黑石压力板 *
            *（MOBS 灵敏度）：使用 `dynamic_cast<
                LivingEntity*>` 过滤，只检测生物实体，同时排除 `doesEntityNotTriggerPressurePlate()` 返回 true 的实体。
        -
        **木质 / 铜 / 铁 / 金等压力板 *
            *（EVERYTHING 灵敏度）：检测所有实体类型，但排除 `doesEntityNotTriggerPressurePlate()` 返回 true 的实体。
        -
        **测重压力板 *
            *：与木质压力板相同，检测所有实体并排除不触发的实体。

            注意：物品实体和投射物
            * *不 * *覆写 `doesEntityNotTriggerPressurePlate()`。在 MC 原版中，木质 /
            测重压力板可检测所有实体（包括物品），石质压力板通过 LivingEntity 类型过滤自动排除非生物实体。

            ## #setAttackTarget 虚方法
        - `MobEntity::setAttackTarget()` 和 `MobEntity::
            attackTarget()` 现为 `virtual` 方法，允许IAngerable实体在设置攻击目标时同步更新愤怒状态
        -
        **IAngerable实体统一使用MobEntity::m_attackTarget *
            *：所有实现IAngerable接口的实体（PiglinEntity、GolemEntity、EndermanEntity、BeeEntity、PolarBearEntity、TameableEntity）不再声明独立的`m_attackTarget`成员，而是复用`MobEntity::
                m_attackTarget` -
        通过`MobEntity
            *`指针调用`setAttackTarget()`时，虚函数派发会正确到达子类的override，确保愤怒状态与攻击目标始终同步
        - 子类override `setAttackTarget` 时应调用 `MobEntity::setAttackTarget(target)` 设置基类的 `m_attackTarget`

## #寻路惩罚值系统（Pathfinding Malus）

`MobEntity` 提供三个公有接口对应 MC Java `Mob.setPathfindingMalus` / `getPathfindingMalus` / `shouldPassengersInheritMalus`，允许子类在构造函数中声明实体对特定地形（水、岩浆、火焰等）的寻路代价偏好。

### 核心接口

- **`setPathfindingMalus(PathNodeType pathType, f32 malus)`**：设置指定路径节点类型的惩罚值。
  - 负值（如 `-1.0F`）：完全不可通行，节点会被排除出寻路
  - `0.0F`：无惩罚，按默认路径类型代价处理
  - 正值（如 `8.0F`/`16.0F`）：高代价但可通行，A* 会优先选择更便宜的路径

- **`getPathfindingMalus(PathNodeType pathType) const`**：查询指定类型的惩罚值。
  - **默认值回退**：若未通过 `setPathfindingMalus` 显式设置，回退到 `getPathCostPenalty(pathType)`（对应 MC Java 的 `PathType.getMalus()`）。
  - **乘客继承**：若当前实体骑乘在重写 `shouldPassengersInheritMalus()` 返回 `true` 的 Mob 载具上（如炽足兽 Strider），返回载具的 malus 值；否则返回自身的 malus 值。载具通过 `getVehicle()` + `world()->getEntity()` 解引用（与 `MobEntity::isInDaylight()` 既有模式一致）。

- **`shouldPassengersInheritMalus() const`**（虚方法）：乘客是否继承载具的寻路惩罚值。默认返回 `false`。炽足兽（Strider）等载具重写为 `true`，使骑乘者能在岩浆上寻路。

### 存储实现

`MobEntity` 内部使用 `std::array<f32, pathNodeTypeCount()>` 存储（`pathNodeTypeCount()` 定义于 `PathNodeType.hpp`，因枚举底层类型 `u8` 最大 255，Count 值 256 必须独立于枚举之外）。构造时填充 NaN 表示"未设置"，`getPathfindingMalus` 通过 `std::isnan(stored)` 判断是否回退到默认值，对应 MC Java `EnumMap<PathType, Float>` 的 `get()` 返回 `null` 时回退到 `getMalus()` 的语义。

### 与 WalkNodeProcessor 的集成

`WalkNodeProcessor::createNode()` 创建新节点时调用 `mob->getPathfindingMalus(type)` 设置 `PathPoint::costMalus`；`_addNeighbor()` 在节点类型变更时以 `std::max(node->costMalus(), newCostMalus)` 更新（对应 MC Java `WalkNodeEvaluator.getNodeAndUpdateCostToMax` 的 `Math.max` 语义——保守保留较高代价，避免后续更"便宜"的类型判定削弱已有的危险标记）。

### 典型用例

- `CopperGolemEntity` 构造函数设置 `DANGER_FIRE=16.0`、`DANGER_OTHER=16.0`、`DAMAGE_FIRE=-1.0`（对应 MC 1.21.11 `CopperGolem.java:91-93`），使铜傀儡避开火焰周边但可在紧急时穿过。
- 其他实体（如僵尸、骷髅）可在各自构造函数或 AI 初始化时按需调用 `setPathfindingMalus` 自定义路径偏好。

### 测试覆盖

- `tests/common/entity/core/PathfindingMalusTest.cpp` — 寻路惩罚值系统完整单元测试
  - 默认值回退（Water/DangerFire/DamageFire/Walkable/Lava）
  - 设置/覆盖/多类型独立/重复设置
  - `shouldPassengersInheritMalus` 默认 false（TestMobEntity + PigEntity）
  - CopperGolemEntity 构造函数初始化验证（DangerFire/DangerOther/DamageFire）
  - 乘客继承场景（`shouldPassengersInheritMalus=true` / `false` / 无载具）

## #激怒状态同步系统（MOB_FLAG_AGGRESSIVE）

对应 MC 1.21.11 `Mob.MOB_FLAG_AGGRESSIVE` 与 `Mob.isAggressive` / `Mob.setAggressive`，通过 `DATA_MOB_FLAGS_PARAM`（i8）的位 2（0x04）在服务端-客户端之间同步 Mob 的"激怒"状态（僵尸抬臂攻击姿态、近战 AI 锁定目标等）。

### 核心字段与方法

- **`MobEntity::DATA_MOB_FLAGS_PARAM`**（`protected static` 成员）：`EntityDataManager::createKey<i8>()` 在 `registerData()` 中分配的同步参数 ID，存储 Mob 标志位。
- **`MobEntity::MOB_FLAG_AGGRESSIVE`**（`protected static constexpr i8 = 0x04`）：位 2 掩码，对应 MC `MOB_FLAG_AGGRESSIVE = 4`。
- **`getMobFlagsParamId()`** / **`getAggressiveFlagMask()`**（`public static`）：暴露给客户端同步层（ClientEntity）和测试代码使用的访问器，避免直接访问 protected 成员。
- **`isAggressive() const`**：读取 `DATA_MOB_FLAGS_PARAM` 的位 2，`(value & 0x04) != 0`。
- **`setAggressive(bool)`**：位运算修改 `DATA_MOB_FLAGS_PARAM`：`aggressive ? (b0 | 4) : (b0 & -5)`，仅修改位 2，保留位 0 (NO_AI) 和位 1 (LEFTHANDED)。
- **`isAggroed()` / `setAggroed(bool)`**：向后兼容委托方法，分别委托给 `isAggressive()` / `setAggressive()`。MC 1.21.11 已统一为 `isAggressive`/`setAggressive`，旧代码可继续使用 `isAggroed`/`setAggroed` 而不破坏行为。

### 位布局

| 位 | 掩码 | 含义 | 对应 MC |
|----|------|------|---------|
| 0  | 0x01 | NO_AI（无 AI） | `MOB_FLAG_NO_AI` |
| 1  | 0x02 | LEFTHANDED（左撇子） | `MOB_FLAG_LEFTHANDED` |
| 2  | 0x04 | AGGRESSIVE（激怒状态） | `MOB_FLAG_AGGRESSIVE` |
| 3-7 | —   | 未使用 | — |

### registerData 注册时机

`MobEntity::MobEntity(...)` 构造函数**显式**调用 `registerData()`（重写 `Entity::registerData` 虚方法）注册 `DATA_MOB_FLAGS_PARAM`。由于 C++ 虚函数在构造函数中不进行动态派发，派生类构造时必须自行调用 `MobEntity::registerData()` 或在自身 `registerData` 重写中调用基类版本，否则 `DATA_MOB_FLAGS_PARAM` 不会被注册。

### 数据流

```
服务端 MobEntity::setAggressive(true)
  → DATA_MOB_FLAGS_PARAM |= 0x04（位 2 置位）
  → EntityTracker 广播 ir::play::SetEntityData
  → 客户端 ClientEntity::setMetadata → EntityMetadataSerializer::deserialize
  → 写入 ClientEntity::m_dataManager
  → syncMetadataFromDataManager 读取 DATA_MOB_FLAGS_PARAM & 0x04
  → setIsAggressive(aggressive)
  → EntityRendererManager::_applyZombieState 推送到 ZombieModel::setAggressive
  → ZombieModel::setAngles 按 animateZombieArms 计算手臂角度（aggressive=true → -PI/1.5）
```

### 关键调用位置

- **`MeleeAttackGoal::resetTask()`**：攻击目标丢失或目标死亡时调用 `m_mob->setAggroed(false)` 清除激怒状态，对应 MC 1.21.11 `MeleeAttackGoal#stop` 中 `mob.setAggressive(false)`。注意该方法不再为 `noexcept`，因为 `setAggroed → setAggressive → m_dataManager.set` 涉及互斥锁，理论上可抛异常。
- **`ZombieEntity` / `HuskEntity` / `DrownedEntity` 等**：在 AI 锁定玩家或进入攻击范围时通过 `setAggressive(true)` 启用激怒状态，驱动客户端抬臂动画。

### 测试覆盖

- `tests/entity/MobEntityAggressiveFlagTest.cpp` — DATA_MOB_FLAGS_PARAM 位 2 同步完整单元测试
  - 默认状态（isAggressive=false、原始字节=0）
  - setAggressive 往返（true→false、多次切换幂等）
  - 位隔离（保留 NO_AI/LEFTHANDED，只修改位 2）
  - getAggressiveFlagMask / getMobFlagsParamId 静态访问器
  - isAggroed / setAggroed 向后兼容委托

## #swing(Hand) 挥动动画广播

对应 MC 1.21.11 `LivingEntity.swing(InteractionHand)`，在服务端调用时通过 `IWorld::broadcastEntityAnimation` 广播挥动动画事件（经 IR `ir::play::Animate` 传输），客户端收到后启动本地 6 tick 挥动动画。

### 核心方法

- **`swing(Hand hand)`** — 触发一次挥动动画。条件判断 `!m_swingInProgress || m_swingProgressInt >= getArmSwingAnimationEnd()/2 || m_swingProgressInt < 0` 允许在动画进行到一半时重新触发（避免连击被节流）。
- **`swingArm()`** — 便捷方法，等价于 `swing(Hand::MainHand)`。
- **`getArmSwingAnimationEnd()`** — 返回挥动动画总 tick 数（默认 6，受急迫/挖掘疲劳效果影响）。
- **`isSwingInProgress()` / `swingProgressInt()`** — 挥动状态访问器，供测试验证节流逻辑。

### 网络广播

```cpp
void LivingEntity::swing(Hand hand)
{
    if (!m_swingInProgress || m_swingProgressInt >= getArmSwingAnimationEnd() / 2 || m_swingProgressInt < 0) {
        m_swingProgressInt = -1;
        m_swingInProgress = true;
        m_swingingHand = hand;

        // 服务端广播挥动动画事件
        if (m_world != nullptr && !m_world->isClientSide()) {
            const u8 animation = (hand == Hand::MainHand)
                ? static_cast<u8>(network::EntityAnimation::SwingMainHand)  // 0
                : static_cast<u8>(network::EntityAnimation::SwingOffHand); // 3
            m_world->broadcastEntityAnimation(m_id, animation);
        }
    }
}
```

### 动画值映射

| Hand | `network::EntityAnimation` 枚举 | 网络值 |
|------|----------------|--------|
| MainHand | `SwingMainHand` | 0 |
| OffHand | `SwingOffHand` | 3 |

`network::EntityAnimation` 枚举定义在 `common/network/protocol/EntityEvents.hpp`，动画事件经 IR `ir::play::Animate` 包传输。

### 边界条件

- **客户端世界**（`isClientSide()==true`）：不广播动画事件，仅更新本地挥动状态。
- **无世界**（`m_world==nullptr`）：不广播，不崩溃。
- **节流**：挥动进行中（`m_swingInProgress==true` 且 `m_swingProgressInt ∈ [0, half)`）时重复调用 `swing` 不会重新广播。

### 数据流

```
服务端 LivingEntity::swing(Hand::MainHand)
  → 检测 !m_world->isClientSide()
  → m_world->broadcastEntityAnimation(entityId, SwingMainHand=0)
  → IR ir::play::Animate 传输到客户端
  → ClientPlayVisitor 分流到 ClientEntity::triggerSwingAnimation 启动本地 6 tick 挥动动画
  → ClientEntity::tick 推进 swingProgress
  → BipedModel::setSwingProgress(swingProgress) 应用到手臂旋转
```

### 测试覆盖

- `tests/entity/LivingEntitySwingBroadcastTest.cpp` — swing 广播完整单元测试
  - MainHand/OffHand 广播正确的 animation 值（0/3）
  - 客户端世界（isClientSide=true）不广播
  - 无世界（m_world=nullptr）不崩溃
  - 节流状态验证（swingInProgress、swingProgressInt）
  - swingArm() 便捷方法等价于 swing(MainHand)

