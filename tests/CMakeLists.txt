# tests/CMakeLists.txt
# 娴嬭瘯閰嶇疆

find_package(GTest CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)

function(mc_disable_all_warnings_for_tests target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -w)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W0)
    endif()
endfunction()

# 通用测试可执行文件
add_executable(mc_tests
    common/test_math.cpp
    common/test_network.cpp
    common/test_world.cpp
    common/test_core.cpp
    common/GameDirectoryTest.cpp
    common/DefaultValuesTest.cpp
    common/PackTypeTest.cpp
    common/DataPackListTest.cpp
    # common/test_blockid.cpp  # 宸茬Щ闄?- 浣跨敤鏂扮殑鏂瑰潡绯荤粺
    # common/test_noise.cpp     # 宸茬Щ闄?- Noise 绫诲凡鍒犻櫎
    common/test_entity.cpp
    common/test_entity_physics.cpp
    common/test_chunksync.cpp
    common/test_localconnection.cpp
    common/test_chunkloadticket.cpp
    # common/test_collision.cpp  # 鏆傛椂绂佺敤 - 渚濊禆鏃х殑纰版挒绯荤粺
    common/test_property.cpp
    common/TestWorldHelper.cpp
    common/BaseTestServer.cpp
    common/test_block.cpp
    common/test_chunk_generation.cpp
    common/test_chunk_level.cpp
    common/test_biome.cpp
    common/test_ore_feature.cpp
    common/test_carver.cpp
    common/test_tree_feature.cpp
    common/test_new_tree_features.cpp
    common/test_decoration_stage.cpp
    common/test_time.cpp
    common/test_item.cpp
    common/test_block_item.cpp
    common/test_inventory.cpp
    common/test_item_entity.cpp
    common/test_container.cpp
    common/item/crafting/IngredientTest.cpp
    common/item/crafting/RecipeBookTest.cpp
    common/item/crafting/RecipeManagerTest.cpp
    common/item/crafting/ShapedRecipeTest.cpp
    common/item/crafting/ShapelessRecipeTest.cpp
    common/item/crafting/RecipeLoaderTest.cpp
    common/item/crafting/RecipeSerializersTest.cpp
    common/item/crafting/BannerDuplicateRecipeTest.cpp
    common/item/crafting/ShieldDecorationRecipeTest.cpp
    common/item/tool/ToolTests.cpp
    common/item/tool/ShearsItemTest.cpp
    # ItemTags 花朵标签测试
    common/item/tag/ItemTagsTest.cpp
    common/item/potion/GlassBottleItemTest.cpp
    common/item/potion/PotionUtilsTest.cpp
    common/item/weapon/ThrowableItemTest.cpp
    common/item/weapon/WeaponItemTest.cpp
    common/item/special/BucketItemTest.cpp
    common/item/special/FishBucketItemTest.cpp
    common/item/special/BoneMealItemTest.cpp
    common/item/special/OnAStickItemTest.cpp
    common/world/map/MaterialColorTest.cpp
    common/world/map/MapDataTest.cpp
    common/item/food/ChorusFruitItemTest.cpp
    common/item/NewItemTest.cpp
    common/item/enchantment/EnchantmentTest.cpp
    common/item/enchantment/EnchantmentCallbackTest.cpp
    common/entity/inventory/CraftingInventoryTest.cpp
    common/entity/inventory/IRecipeHolderTest.cpp
    common/entity/inventory/container/ChestContainerTest.cpp
    common/entity/inventory/container/HopperContainerTest.cpp
    common/entity/inventory/container/FurnaceContainerTest.cpp
    common/entity/inventory/container/EnchantmentContainerTest.cpp
    common/entity/inventory/container/BrewingStandContainerTest.cpp
    common/entity/inventory/container/AnvilContainerTest.cpp
    common/entity/inventory/container/LoomContainerTest.cpp
    common/entity/entities/passive/basic/ChickenEntityTest.cpp
    common/entity/entities/passive/basic/RabbitEntityTest.cpp
    common/entity/entities/passive/basic/MooshroomEntityTest.cpp
    common/entity/entities/passive/basic/AnimalEntityTest.cpp
    common/entity/entities/passive/tamable/WolfEntityTest.cpp
    common/entity/entities/passive/tamable/ParrotEntityTest.cpp
    common/entity/entities/passive/tamable/OcelotEntityTest.cpp
    # BegGoal 驯服物品乞求测试
    entity/BegGoalTest.cpp
    # CatEntity 猫实体测试（驯服物品、繁殖物品、CatTemptGoal、CatAvoidPlayerGoal）
    entity/CatEntityTest.cpp
    common/entity/entities/passive/special/FoxEntityTest.cpp
    # PandaEntity 打喷嚏、性格、状态测试
    common/entity/entities/passive/special/PandaEntityTest.cpp
    # PolarBearEntity IAngerable接口、站立警告、声音测试
    common/entity/entities/passive/special/PolarBearEntityTest.cpp
    # BeeEntity 花朵繁殖、幼体生成、花粉/螫刺状态测试
    common/entity/entities/passive/special/BeeEntityTest.cpp
    # TurtleEntity 下蛋逻辑测试
    common/entity/entities/passive/special/TurtleEntityTest.cpp
    # 海龟 AI 目标测试
    entity/TurtleGoalsTest.cpp
    # 蜜蜂 AI 目标测试
    common/entity/BeeGoalsTest.cpp
    # SnowGolemEntity 雪傀儡测试
    common/entity/entities/passive/golem/SnowGolemEntityTest.cpp
    common/entity/DolphinEntityTest.cpp
    # 鱿鱼 AI 目标测试
    common/entity/SquidGoalsTest.cpp
    # 村民 AI 目标测试
    common/entity/VillagerGoalsTest.cpp
    # 蝙蝠 AI 目标测试
    common/entity/BatGoalsTest.cpp
    # 海豚 AI 目标测试
    common/entity/DolphinGoalsTest.cpp
    # 幻翼 AI 目标测试
    common/entity/PhantomGoalsTest.cpp
    # PufferfishEntity 膨胀目标和敌人检测测试
    common/entity/entities/passive/fish/PufferfishEntityTest.cpp
    common/entity/entities/monster/SlimeEntityTest.cpp
    # SlimeChunkChecker 史莱姆区块判断测试
    common/world/spawn/SlimeChunkCheckerTest.cpp
    # canSlimeSpawn 史莱姆生成条件测试
    common/world/spawn/CanSlimeSpawnTest.cpp
    # MagmaCubeEntity 岩浆怪测试（继承自史莱姆）
    common/entity/entities/monster/MagmaCubeEntityTest.cpp
    # 史莱姆 AI 目标测试
    common/entity/SlimeGoalsTest.cpp
    # 铁傀儡 AI 目标测试
    common/entity/IronGolemGoalsTest.cpp
    common/entity/entities/monster/EndermiteEntityTest.cpp
    # SpiderEntity AI 目标、光照敏感、中毒攻击测试
    entity/SpiderEntityTest.cpp
    # RavagerEntity 劫掠兽测试
    entity/RavagerEntityTest.cpp
    common/entity/entities/monster/ZombieVillagerEntityTest.cpp
    # ZombieVillagerEntity 床和铁栏杆加速治愈测试
    entity/ZombieVillagerConversionTest.cpp
    # ZombieEntity 僵尸转溺尸测试
    common/entity/entities/monster/undead/ZombieEntityTest.cpp
    # SkeletonEntity 骷髅射箭测试
    common/entity/entities/monster/undead/SkeletonEntityTest.cpp
    # VexEntity 穿墙能力和生命周期测试
    common/entity/entities/monster/illager/VexEntityTest.cpp
    # EvokerEntity 尖牙攻击和召唤恼鬼测试
    common/entity/entities/monster/illager/EvokerEntityTest.cpp
    # PillagerEntity 弩远程攻击和 VindicatorEntity 近战攻击测试
    common/entity/entities/monster/illager/PillagerEntityTest.cpp
    # WitchEntity 药水决策和治疗效果测试
    common/entity/entities/monster/illager/WitchEntityTest.cpp
    # SpellcastingIllager 施法粒子颜色测试
    common/entity/entities/monster/illager/SpellcastingIllagerTest.cpp
    # ShulkerEntity 贝壳状态、瞬移和投射物免疫测试
    common/entity/entities/monster/end/ShulkerEntityTest.cpp
    # EndermanEntity 注视检测测试（Player::getLookVector、isLookingAt、shouldAttackPlayer）
    common/entity/entities/monster/end/EndermanStareDetectionTest.cpp
    # EndermanEntity 方块放置和拾取测试（BlockTags::ENDERMAN_HOLDABLE、EndermanPlaceBlockGoal、EndermanTakeBlockGoal）
    common/entity/entities/monster/end/EndermanBlockGoalsTest.cpp
    # EndermanEntity 攻击末影螨目标测试（NearestAttackableTargetGoal<EndermiteEntity>、isSpawnedByPlayer）
    common/entity/entities/monster/end/EndermanEndermiteTargetTest.cpp
    common/entity/entities/misc/TNTEntityTest.cpp
    # FallingBlockEntity 落地放置方块、物品掉落、伤害实体测试
    common/entity/entities/misc/FallingBlockEntityTest.cpp
    common/entity/entities/HangingEntityTest.cpp
    # EnderCrystalEntity 治愈末影龙测试
    common/entity/entities/effect/EnderCrystalHealTest.cpp
    # MonsterEntity 生成位置检查测试
    common/entity/entities/monster/MonsterSpawnTest.cpp
    # BoostHelper NBT 序列化测试
    common/entity/core/BoostHelperTest.cpp
    # LivingEntity 箭矢计数和发光效果测试
    common/entity/core/LivingEntityArrowCountTest.cpp
    # LivingEntity 吸收值 getter/setter 和边界 clamping 测试
    common/entity/core/LivingEntityAbsorptionTest.cpp
    # StepHeight 步高系统测试（Entity、LivingEntity、特定实体步高设置）
    common/entity/core/StepHeightTest.cpp
    # Entity::isInRain() 双位置检测和 EndermanEntity::isInWaterOrRain() 测试
    common/entity/core/EntityIsInRainTest.cpp
    common/entity/EntitySerializationTest.cpp
    # Entity::isOnSameTeam() 队伍系统测试
    entity/EntityTeamTest.cpp
    # Player::attack() 横扫攻击过滤、荆棘附魔反伤、武器损耗测试
    entity/PlayerAttackTest.cpp
    # IRideable 速度设置逻辑测试
    common/entity/interfaces/IRideableTest.cpp
    # DamageSource isExplosion() 和 isFire() 测试
    common/entity/damage/DamageSourceExplosionTest.cpp
    # 矿车 dropItem() 伤害源检测测试
    common/entity/entities/vehicle/MinecartDropItemTest.cpp
    # 船实体 getBoatItem() 和 dropItem() 测试
    common/entity/entities/vehicle/BoatEntityDropItemTest.cpp
    # BoatItem 船物品测试
    common/item/vehicle/BoatItemTest.cpp
    # 成就系统测试
    advancement/AdvancementTest.cpp
    # PlayerKilledEntityTrigger 触发器测试
    advancement/PlayerKilledEntityTriggerTest.cpp
    # PlacedBlockTrigger 触发器测试
    advancement/PlacedBlockTriggerTest.cpp
    # CuredZombieVillagerTrigger 触发器测试
    advancement/CuredZombieVillagerTriggerTest.cpp
    # PlayerInteractedWithEntityTrigger 触发器测试
    advancement/PlayerInteractedWithEntityTriggerTest.cpp
    # ChanneledLightningTrigger 引雷附魔触发器测试
    advancement/ChanneledLightningTriggerTest.cpp
    # ItemTriggers 物品触发器测试（消耗、耐久、附魔、填充桶）
    advancement/ItemTriggersTest.cpp
    # MobEffectsPredicate 效果谓词测试
    advancement/MobEffectsPredicateTest.cpp
    # DamageSourcePredicate 伤害源谓词测试
    advancement/DamageSourcePredicateTest.cpp
    # EntityPredicate 实体谓词测试（包含 EntityFlagsPredicate、EntityEquipmentPredicate、NBTPredicate、DistancePredicate）
    advancement/EntityPredicateTest.cpp
    # ItemPredicate 物品谓词测试（含药水匹配）
    advancement/ItemPredicateTest.cpp
    # BredAnimalsTrigger 动物繁殖触发器测试
    advancement/BredAnimalsTriggerTest.cpp
    # AdvancementEventHandler 测试（getServerPlayer 架构验证）
    server/advancement/AdvancementEventHandlerTest.cpp
    # PlayerDestroyItemEvent 物品销毁事件测试
    server/event/PlayerDestroyItemEventTest.cpp
    # 村庄声望治愈僵尸村民测试
    server/advancement/VillageReputationTest.cpp
    # ItemDropHelper 测试
    common/entity/utils/ItemDropHelperTest.cpp
    # 经验系统测试
    common/entity/experience/ExperienceTest.cpp
    common/entity/experience/ExperienceManagerTest.cpp
    common/entity/experience/ExperienceOrbEntityTest.cpp
    # 箭矢拾取测试
    common/entity/ArrowPickupTest.cpp
    # ArrowEntity 药水箭测试（效果、颜色、NBT标签）
    common/entity/ArrowEntityTest.cpp
    # 难度系统测试
    common/entity/combat/DifficultyHelperTest.cpp
    # DifficultyInstance 区域难度测试
    common/entity/combat/DifficultyInstanceTest.cpp
    # MobEntity 装备生成和finalizeSpawn测试
    common/entity/combat/MobEntityEquipmentTest.cpp
    # MobEntity 交互测试（processInitialInteract、canBeLeashed、_spawnOffspringFromSpawnEgg）
    common/entity/core/MobEntityInteractTest.cpp
    # 战斗追踪器测试
    common/entity/combat/CombatTrackerTest.cpp
    # CombatRules EPF 测试
    common/entity/combat/CombatRulesEPFTest.cpp
    # 横扫攻击静止检测条件测试
    common/entity/combat/SweepAttackConditionTest.cpp
    # GameRules 游戏规则测试
    common/world/gamerule/GameRulesTest.cpp
    common/world/lighting/InternalLightUtilsTest.cpp
    common/world/LightLevelTest.cpp
    common/world/blockentity/BlockEntityTest.cpp
    common/world/blockentity/ChestEntityTest.cpp
    common/world/blockentity/FurnaceEntityTest.cpp
    common/world/blockentity/HopperEntityTest.cpp
    common/world/blockentity/DoubleSidedInventoryTest.cpp
    common/world/blockentity/BrewingStandEntityTest.cpp
    common/world/blockentity/BeaconEntityTest.cpp
    common/world/blockentity/BlockEntityTodoTest.cpp
    # ConduitEntity 潮涌核心测试（敌对生物检测、魔法伤害）
    common/world/blockentity/ConduitEntityTest.cpp
    # SignEntity 告示牌测试（类型注册、文本功能）
    common/world/blockentity/SignEntityTest.cpp
    # SignEntity::executeCommand 测试（告示牌点击事件命令执行）
    common/world/blockentity/SignEntityCommandTest.cpp
    # EndGatewayEntity 末地折跃门测试（类型注册、传送功能、序列化）
    common/world/blockentity/EndGatewayEntityTest.cpp
    # CampfireBlockEntity 营火方块实体测试（烹饪逻辑、槽位管理、序列化）
    common/world/blockentity/CampfireBlockEntityTest.cpp
    # ShulkerBoxEntity 潜影盒测试（动画状态、比较器信号、防递归嵌套）
    common/world/blockentity/ShulkerBoxEntityTest.cpp
    # CommandBlockEntity 命令方块实体测试（命令执行、红石触发、连锁执行、条件检测）
    common/world/blockentity/CommandBlockEntityTest.cpp
    # BannerEntity 旗帜方块实体测试（图案存储、底色、自定义名称、NBT序列化）
    common/world/blockentity/BannerEntityTest.cpp
    # BannerPattern 旗帜图案工具类测试（hashname、文件名、图案物品判断）
    common/world/blockentity/BannerPatternTest.cpp
    common/world/block/blocks/DoorBlockTest.cpp
    common/world/block/blocks/TrapDoorBlockTest.cpp
    common/world/block/blocks/BlockSoundTest.cpp
    common/world/block/blocks/CauldronBlockTest.cpp
    common/world/block/blocks/MobBlocksTest.cpp
    common/world/block/DispenseItemBehaviorTest.cpp
    common/world/storage/StorageTaskTest.cpp
    common/world/storage/PlayerStorageTest.cpp
    server/world/ServerWorldPersistenceTest.cpp
    common/world/storage/WorldNameSanitizerTest.cpp
    common/world/storage/JavaChunkReaderTest.cpp
    common/world/block/blocks/EnchantingTableBlockTest.cpp
    common/world/block/blocks/ChestBlockTest.cpp
    common/world/block/blocks/decorative/PaneBlockTest.cpp
    common/world/block/blocks/decorative/DecorativeBlockTest.cpp
    common/world/block/blocks/decorative/StainedGlassBlockTest.cpp
    common/world/block/blocks/decorative/ScaffoldingBlockTest.cpp
    common/world/block/blocks/functional/BellBlockTest.cpp
    common/world/block/blocks/functional/GrindstoneBlockTest.cpp
    common/world/block/WaterLoggableTest.cpp
    common/world/block/blocks/HopperBlockTest.cpp
    # SignBlock 告示牌方块测试（onBlockActivated、状态属性）
    common/world/block/blocks/SignBlockTest.cpp
    common/world/block/blocks/IceBlockTest.cpp
    common/world/block/blocks/SpreadableSnowyDirtBlockTest.cpp
    common/world/block/blocks/SoulFireBlockTest.cpp
    common/world/block/blocks/FireBlockCollisionTest.cpp
    common/world/block/blocks/FireBlockTest.cpp
    common/world/block/blocks/NetherPortalBlockTest.cpp
    common/world/block/blocks/BubbleColumnBlockTest.cpp
    common/world/block/blocks/FallingBlockTest.cpp
    common/world/block/blocks/VegetationBlockTest.cpp
    common/world/block/blocks/SweetBerryBushBlockTest.cpp
    common/world/block/blocks/SeagrassBlockTest.cpp
    common/world/block/blocks/end/ChorusPlantBlockTest.cpp
    common/world/block/blocks/end/DragonEggBlockTest.cpp
    common/world/block/BlockTagsTest.cpp
    common/entity/EntityMotionMultiplierTest.cpp
    common/world/block/blocks/CropBlocksTest.cpp
    common/world/block/blocks/StemBlocksTest.cpp
    common/world/block/blocks/GolemBlockTest.cpp
    common/world/block/blocks/PumpkinCarveTest.cpp
    common/world/block/blocks/RespawnAnchorBlockTest.cpp
    common/world/block/blocks/SpongeBlockTest.cpp
    common/world/block/CompostableItemsTest.cpp
    common/world/CanSeeSkyTest.cpp
    common/command/test_command_dispatcher.cpp
    entity/EntityCoreTests.cpp
    entity/EntityTagsTests.cpp
    entity/FlyingEntityTests.cpp
    entity/EntitySensesTest.cpp
    entity/RidingSystemTests.cpp
    entity/StriderEntityTest.cpp
    entity/PigEntityTest.cpp
    entity/FlingingSupportTypesTest.cpp
    entity/AttributeTests.cpp
    # Player LUCK 属性测试
    entity/PlayerLuckAttributeTest.cpp
    entity/LivingEntityTests.cpp
    entity/AnimalSoundTests.cpp
    entity/GoalTests.cpp
    entity/EatGrassGoalTest.cpp
    entity/CreeperSwellGoalTest.cpp
    entity/AreaEffectCloudEntityTest.cpp
    entity/BlazeFireballAttackGoalTest.cpp
    # 火球类投射物测试（伤害、爆炸、区域效果云）
    entity/FireballEntityTest.cpp
    # DamagingProjectileEntity 粒子效果测试（水下气泡、拖尾粒子）
    entity/DamagingProjectileParticleTest.cpp
    # LookAtGoal 类型过滤测试
    common/entity/ai/goal/LookAtGoalTest.cpp
    # FishSwimGoal 鱼类游泳目标测试
    common/entity/ai/goal/FishSwimGoalTest.cpp
    entity/GhastEntityTest.cpp
    # 恶魂 AI 目标测试
    common/entity/GhastGoalsTest.cpp
    entity/GuardianAttackGoalTest.cpp
    # 猪灵类实体测试（金装备检测、AI 目标）
    entity/PlayerGoldArmorTest.cpp
    entity/NetherEntitiesTest.cpp
    # PatrolGoal 测试
    common/entity/ai/goal/PatrolGoalTest.cpp
    entity/WitherEntityTest.cpp
    # EndermanEntity 受伤后瞬移逻辑测试
    entity/EndermanHurtTeleportTest.cpp
    # EndermanEntity AI 目标测试（ResetAngerGoal, WaterAvoidingRandomWalkingGoal）
    entity/EndermanAIGoalsTest.cpp
    entity/EvokerGoalsTest.cpp
    # EvokerFangsEntity 队伍伤害检查测试
    entity/EvokerFangsTest.cpp
    entity/VexGoalsTest.cpp
    entity/BrainScheduleTests.cpp
    entity/SensorTests.cpp
    entity/DamagingProjectileSupportTypesTest.cpp
    entity/FishSupportTypesTest.cpp
    entity/HorseAppearanceSupportTypesTest.cpp
    entity/HorseSupportTypesTest.cpp
    entity/HorseTamingTest.cpp
    entity/HorseBreedingTest.cpp
    entity/HorseJumpBoostTest.cpp
    entity/HorseAiGoalsTest.cpp
    common/entity/entities/passive/horse/HorseCanEquipTest.cpp
    common/entity/entities/passive/horse/SkeletonHorseEntityTest.cpp
    # 骷髅马陷阱触发集成测试（triggerTrap 完整流程、困难模式额外骷髅马）
    common/entity/entities/passive/horse/SkeletonHorseTrapIntegrationTest.cpp
    entity/PatrollerSupportTypesTest.cpp
    entity/PlayerSupportTypesTest.cpp
    entity/PlayerPoseCollisionTest.cpp
    entity/PlayerDiggingTest.cpp
    common/entity/PlayerSwimTest.cpp
    common/entity/player/SpawnPointValidatorTest.cpp
    common/entity/player/CooldownTrackerTest.cpp
    common/entity/player/PlayerMessageTest.cpp
    common/entity/player/PlayerSleepTest.cpp
    entity/ProjectileHelperTest.cpp
    # ThrowableEntity 传送门处理测试（下界传送门、末地折跃门）
    entity/ThrowablePortalTest.cpp
    # FishingBobberEntity 钩住实体测试
    common/entity/entities/projectile/FishingBobberHookEntityTest.cpp
    # FireworkRocketEntity 爆炸伤害测试
    common/entity/entities/projectile/FireworkRocketExplosionTest.cpp
    # DragonFireballEntity 龙息云和粒子类型测试
    common/entity/entities/projectile/DragonFireballCloudTest.cpp
    # ShulkerBulletEntity 爆炸粒子效果测试
    common/entity/entities/projectile/ShulkerBulletParticleTest.cpp
    entity/SkeletonSupportTypesTest.cpp
    entity/SpellcastingIllagerSupportTypesTest.cpp
    entity/SpawnLocationHelperTest.cpp
    entity/ShoulderRidingSupportTypesTest.cpp
    # 鹦鹉 AI 目标测试（LandOnOwnersShoulderGoal, WaterAvoidingRandomFlyingGoal, FollowMobGoal）
    entity/ParrotGoalsTest.cpp
    entity/TraderLlamaSupportTypesTest.cpp
    entity/PathfindingTests.cpp
    entity/WalkNodeProcessorTests.cpp
    common/entity/ai/pathfinding/PathNavigatorTest.cpp
    entity/RandomWalkingGoalTest.cpp
    common/entity/MoveToLavaGoalTest.cpp
    common/entity/ai/goal/goals/special/TriggerSkeletonTrapGoalTest.cpp
    common/entity/ai/goal/goals/special/FoxGoalsTest.cpp
    # PandaGoals 熊猫打滚 AI 目标测试
    common/entity/ai/goal/goals/special/PandaGoalsTest.cpp
    common/entity/ai/goal/goals/special/SilverfishGoalsTest.cpp
    # DolphinGoals 海豚 AI 目标测试
    common/entity/ai/goal/goals/special/DolphinGoalsTest.cpp
    # WanderingTraderGoals 流浪商人 AI 目标测试
    common/entity/ai/goal/goals/special/WanderingTraderGoalsTest.cpp
    # RandomPositionGenerator 随机位置生成器测试
    common/entity/ai/util/RandomPositionGeneratorTest.cpp
    entity/AiGoalRegressionTest.cpp
    entity/PathWeightTest.cpp
    entity/MinecartTests.cpp
    entity/ElytraAndSpinAttackTest.cpp
    common/entity/entities/EntityDaylightTest.cpp
    common/entity/WaterSplashTest.cpp
    common/SettingsTest.cpp
    server/ServerWorldTest.cpp
    server/ServerWorldBlockUpdateCallbackTest.cpp
    server/test_integrated_server.cpp
    server/BlockDropHandlerTest.cpp
    server/BlockInteractionManagerTest.cpp
    server/interaction/SignCommandHelperTest.cpp
    server/MiningManagerTest.cpp
    server/menu/CraftingMenuTest.cpp
    server/BlockUpdateSyncManagerTest.cpp
    server/test_chunk_worker_pool.cpp
    server/test_generation_chunk_cache.cpp
    server/test_server_chunk_manager.cpp
    server/core/TimeManagerTest.cpp
    server/core/PlayerManagerTest.cpp
    server/core/ConnectionManagerTest.cpp
    server/core/TeleportManagerTest.cpp
    server/core/KeepAliveManagerTest.cpp
    server/core/PositionTrackerTest.cpp
    server/core/WhitelistManagerTest.cpp
    server/core/BannedPlayerListTest.cpp
    server/core/BannedIpListTest.cpp
    server/core/OpListManagerTest.cpp
    # PacketHandler 测试（载具输入、位置验证、实体交互、实体动作）
    server/core/PacketHandlerTest.cpp
    # ServerPlayer 消息发送测试
    server/player/ServerPlayerMessageTest.cpp
    # 维度管理器测试
    server/dimension/ServerDimensionManagerTest.cpp
    server/weather/WeatherManagerTest.cpp
    server/world/spawn/DespawnManagerTest.cpp
    server/command/CommandRegistryTest.cpp
    server/command/SetBlockCommandTest.cpp
    server/command/CloneCommandTest.cpp
    server/command/GiveCommandTest.cpp
    server/command/LootCommandTest.cpp
    server/command/PlayerResolverTest.cpp
    server/command/MessageCommandTest.cpp
    server/command/TellRawCommandTest.cpp
    server/command/ExperienceCommandTest.cpp
    server/command/AttributeCommandTest.cpp
    server/command/EnchantCommandTest.cpp
    server/command/SpawnPointCommandTest.cpp
    server/command/PublishCommandTest.cpp
    server/command/ExecuteCommandTest.cpp
    server/world/player/ServerPlayerEntityManagerTest.cpp
    client/world/player/LocalPlayerIdentityTest.cpp
    client/world/player/ClientPlayerPredictorTest.cpp
    client/world/entity/ClientEntityTest.cpp
    client/entity/ClientEntityAnimationTest.cpp
    client/world/ClientWorldLightUpdateTest.cpp
    # ClientWorld 区块清空测试（维度切换时使用）
    client/world/ClientWorldClearChunksTest.cpp
    # ClientWeather 闪电闪烁测试
    client/world/ClientWeatherLightningFlashTest.cpp
    client/test_mesh_worker_pool.cpp
    client/test_mesh_build_scheduler.cpp
    client/test_sound_handlers.cpp
    client/test_client_dimension_manager.cpp
    client/world/ClientWorldLightUpdateTest.cpp
    client/renderer/test_renderer.cpp
    client/renderer/test_trident_api.cpp
    client/renderer/test_cloud_renderer.cpp
    client/renderer/ChunkRendererStagingLayoutTest.cpp
    client/renderer/entity/AnimalModelTests.cpp
    client/renderer/entity/BipedModelTest.cpp
    # BipedModel::translateHand 测试
    client/renderer/entity/BipedModelTranslateHandTest.cpp
    # PlayerModel 手臂渲染测试
    client/renderer/entity/PlayerModelTest.cpp
    client/renderer/entity/ArmorLayerVisibilityTest.cpp
    client/renderer/entity/LlamaModelTest.cpp
    client/renderer/entity/LlamaRendererTest.cpp
    client/renderer/entity/test_entity_renderer_manager.cpp
    client/renderer/entity/ShadowRendererTest.cpp
    client/renderer/entity/WorldTextRendererTest.cpp
    # HeldBlockLayer 末影人手持方块渲染层测试
    client/renderer/trident/entity/layer/entity/HeldBlockLayerTest.cpp
    # VillagerLayer 村民多层纹理渲染层测试
    client/renderer/trident/entity/layer/entity/VillagerLayerTest.cpp
    # EnergyGlintLayer 附魔光效渲染层测试
    client/renderer/trident/entity/layer/effect/EnergyGlintLayerTest.cpp
    # PlayerRenderer 层渲染器测试（HeadLayer、CapeLayer、ElytraLayer）
    client/renderer/entity/test_player_layers.cpp
    client/renderer/test_gui_item_color.cpp
    client/resource/test_entity_texture_loader.cpp
    client/resource/ItemTextureAtlasTest.cpp
    client/resource/TextureAtlasBuilderTest.cpp
    client/resource/AnimationTextureTest.cpp
    client/resource/test_resource_manager_cloud_texture.cpp
    network/EntityPacketsTest.cpp
    network/ParticlePacketTest.cpp
    network/ExplosionPacketTest.cpp
    network/WorldEventPacketTest.cpp
    network/SpawnPositionPacketTest.cpp
    network/LocalServerConnectionTest.cpp
    network/TitlePacketTest.cpp
    network/BossInfoPacketTest.cpp
    common/entity/loot/LootTest.cpp
    common/entity/loot/LootConditionTest.cpp
    common/entity/loot/LootSerializersTest.cpp
    common/entity/loot/LootTableLoaderTest.cpp
    common/entity/loot/LootJsonIntegrationTest.cpp
    # SetStewEffectFunction 测试
    common/entity/loot/SetStewEffectTest.cpp
    # BlockPredicate 和 StatePropertiesPredicate 测试
    common/advancement/BlockPredicateTest.cpp
    # EffectType 工具函数测试
    common/entity/effect/EffectTypeTest.cpp
    # EffectAttributeModifiers 效果属性修改器测试
    common/entity/effect/EffectAttributeModifiersTest.cpp
    common/world/gen/WorldGenSpawnerTest.cpp
    common/world/gen/OceanFeatureTest.cpp
    common/world/gen/WaterContentRegistryTest.cpp
    common/world/gen/NoiseSurfaceParityTest.cpp
    common/world/gen/NoiseChunkGeneratorCarverParityTest.cpp
    common/world/gen/NetherSurfaceParityTest.cpp
    common/world/gen/feature/NetherFeatureTest.cpp
    common/world/gen/feature/template/test_template.cpp
    common/world/gen/structure/test_ocean_ruin_structure.cpp
    common/world/gen/structure/test_shipwreck_structure.cpp
    common/world/gen/structure/test_new_structures.cpp
    common/world/gen/structure/TemplatedStructuresTest.cpp
    common/world/gen/jigsaw/test_jigsaw.cpp
    common/world/gen/jigsaw/test_jigsaw_matcher.cpp
    common/world/gen/jigsaw/test_template_pool_loader.cpp
    common/entity/EntitySpawnPlacementRegistryTest.cpp
    common/world/gen/ChunkSpawnIntegrationTest.cpp
    # Perfetto 娴嬭瘯锛堥泦鎴愬埌涓绘祴璇曞浠讹級
    common/perfetto/PerfettoTest.cpp
    # Tick 绯荤粺娴嬭瘯
    common/world/tick/ServerTickListTest.cpp
    # 娴佷綋绯荤粺娴嬭瘯
    common/world/fluid/FluidTest.cpp
    common/world/fluid/WaterFluidDropTest.cpp
    # 绾㈢煶绯荤粺娴嬭瘯
    common/world/redstone/RedstoneContextTest.cpp
    common/world/redstone/RedstoneHelperTest.cpp
    common/world/redstone/RedstoneSystemTest.cpp
    common/world/redstone/RedstonePowerTest.cpp
    # 绾㈢煶鏂瑰潡娴嬭瘯
    common/world/block/test_redstone_blocks.cpp
    common/world/block/blocks/redstone/NoteBlockTest.cpp
    common/world/block/blocks/redstone/TNTBlockTest.cpp
    common/world/block/blocks/redstone/PistonBlockTest.cpp
    common/world/block/blocks/redstone/TripWireTest.cpp
    # 红石方块形状测试
    common/world/block/blocks/redstone/RedstoneWireBlockTest.cpp
    common/world/block/blocks/redstone/TripWireBlockTest.cpp
    common/world/block/blocks/redstone/TripwireHookBlockTest.cpp
    common/world/block/blocks/redstone/AbstractPressurePlateBlockTest.cpp
    # Explosion system tests
    common/world/explosion/test_explosion.cpp
    common/world/explosion/ExplosionIntegrationTest.cpp
    # 灏勭嚎妫€娴嬫祴璇?    common/math/RaycastTest.cpp
    # 视锥剔除测试
    common/util/math/FrustumTest.cpp
    # 瀹炰綋鐢熸垚鍥炶皟娴嬭瘯
    server/ServerChunkManagerCallbackTest.cpp
    common/world/EntityManagerSpawnTest.cpp
    # 纰版挒缂撳瓨娴嬭瘯
    physics/CollisionCacheTests.cpp
    # 物理引擎测试
    physics/PhysicsEngineTests.cpp
    # DiscreteVoxelShape 测试
    physics/DiscreteVoxelShapeTest.cpp
    # CollisionShape 测试
    physics/CollisionShapeTest.cpp
    # ServerWorld 纰版挒妫€娴嬫祴璇?    server/ServerWorldCollisionTests.cpp
    # 瀹炰綋杩借釜鍣ㄦ祴璇?    server/world/EntityTrackerTest.cpp
    # EntityTracker UUID 测试
    server/world/EntityTrackerUuidTest.cpp
    # 鐗╁搧鎷惧彇绠＄悊鍣ㄦ祴璇?    server/world/ItemPickupManagerTest.cpp
    # ServerWorld 方块实体测试
    server/world/ServerWorldBlockEntityTest.cpp
    # ServerWorld 命令执行回调测试
    server/world/ServerWorldCommandExecuteTest.cpp
    # 璧勬簮鍖呭吋瀹瑰眰娴嬭瘯
    # 鍐呯疆妯″瀷鐫€鑹查厤缃祴璇?    common/resource/VanillaResourcesTintModelTest.cpp
    # ResourcePackList 自包含测试（不依赖外部资源路径）
    common/resource/ResourcePackListSelfContainedTest.cpp
    # 语言管理器测试
    common/resource/LanguageManagerTest.cpp
    # 缂撳瓨鎬ц兘瀵规瘮娴嬭瘯
    common/util/cache/CacheBenchmark.cpp
    # 妞嶈鐗瑰緛闆嗘垚娴嬭瘯
    common/world/gen/test_vegetation_features.cpp
    # 涓栫晫鐢熸垚纭畾鎬ф祴璇?    common/world/gen/WorldGenDeterminismTest.cpp
    # 瀹㈡埛绔懡浠ゆ爲鍚屾涓庤ˉ鍏ㄦ祴璇?    client/command/ClientCommandManagerTest.cpp
    # 鍏夌収绯荤粺娴嬭瘯
    lighting/LightingTest.cpp
    lighting/LightUpdateTest.cpp
    lighting/BlockLightRegressionTest.cpp
    lighting/BaseLightEngineQueueTest.cpp
    lighting/SkyLightRegressionTest.cpp
    # 鐜鍏夐伄钄芥祴璇?    client/renderer/AmbientOcclusionCalculatorTest.cpp
    client/renderer/trident/blockentity/ChestModelTest.cpp
    client/renderer/trident/blockentity/BeaconBeamModelTest.cpp
    # NaturalSpawner 娴嬭瘯
    server/world/spawn/NaturalSpawnerTest.cpp
    # VillageSiege 测试
    server/world/spawn/VillageSiegeTest.cpp
    # 鍏夌収鍚屾娴嬭瘯
    server/LightSyncTests.cpp
    # Kagero UI妯℃澘绯荤粺娴嬭瘯
    ui/kagero/template/template_test.cpp
    # Kagero UI甯冨眬绯荤粺娴嬭瘯
    client/ui/kagero/layout/MeasureSpecTest.cpp
    client/ui/kagero/layout/LayoutConstraintsTest.cpp
    client/ui/kagero/layout/FlexLayoutTest.cpp
    client/ui/kagero/layout/LayoutEngineTest.cpp
    client/ui/kagero/layout/LayoutEngineIntegrationTest.cpp
    client/ui/kagero/layout/FlexLayoutAlgorithmTest.cpp
    client/ui/kagero/layout/GridLayoutTest.cpp
    client/ui/kagero/layout/AnchorLayoutTest.cpp
    # Kagero UI鐘舵€佺郴缁熸祴璇?    client/ui/kagero/state/StateStoreTest.cpp
    client/ui/kagero/state/ReactiveStateTest.cpp
    client/ui/kagero/state/StateBindingTest.cpp
    client/ui/kagero/state/StateObserverTest.cpp
    # Kagero UI浜嬩欢绯荤粺娴嬭瘯
    ui/kagero/event/event_test.cpp
    # Kagero UI Widget娴嬭瘯
    client/ui/kagero/widget/WidgetTest.cpp
    client/ui/kagero/widget/ButtonWidgetTest.cpp
    client/ui/kagero/widget/ButtonWidgetPaintTest.cpp
    client/ui/kagero/widget/TextWidgetTest.cpp
    client/ui/kagero/widget/RichTextWidgetTest.cpp
    client/ui/kagero/widget/CheckboxWidgetTest.cpp
    client/ui/kagero/widget/SliderWidgetTest.cpp
    client/ui/kagero/widget/TextFieldWidgetTest.cpp
    client/ui/kagero/widget/ContainerWidgetTest.cpp
    client/ui/kagero/widget/ContainerWidgetEventTest.cpp
    client/ui/kagero/widget/ScrollableWidgetTest.cpp
    client/ui/kagero/widget/ListWidgetTest.cpp
    client/ui/kagero/widget/Viewport3DWidgetTest.cpp
    client/ui/kagero/widget/SlotWidgetTest.cpp
    client/ui/minecraft/targetinfo/TargetInfoFormatterTest.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/GuiScale.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/minecraft/targetinfo/TargetInfo.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/Font.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/FontTextureAtlas.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/FontRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/DefaultAsciiFont.cpp
    client/ui/GuiScaleTest.cpp
    # 闄勯瓟绯荤粺娴嬭瘯
    common/item/enchantment/EnchantmentTest.cpp
    # 鑷姩璺宠穬娴嬭瘯
    common/entity/movement/AutoJumpTest.cpp
    common/entity/player/SleepManagerTest.cpp
    # 鐜╁绉诲姩娴嬭瘯
    common/entity/PlayerMovementTest.cpp
    # Assert搴撴祴璇?    common/util/assert/AssertTest.cpp
    # ServerWorkerPool 测试
    common/util/thread/ServerWorkerPoolTest.cpp
    # SHA-256 加密工具测试
    common/util/crypto/Sha256Test.cpp
    # MD5 加密工具测试
    common/util/crypto/Md5Test.cpp
    # UUID 工具测试
    common/util/UuidUtilsTest.cpp
    # 璋冭瘯鍖哄潡鐢熸垚鍣ㄦ祴璇?    common/world/gen/DebugChunkGeneratorTest.cpp
    # 鐢熺墿缇ょ郴棰滆壊娴嬭瘯
    client/world/color/BiomeColorsTest.cpp
    # 鐢熺墿缇ょ郴棰滆壊娣峰悎娴嬭瘯
    client/world/color/blend/BiomeColorBlenderTest.cpp
    # 绮掑瓙绯荤粺娴嬭瘯
    client/renderer/particle/ParticleTest.cpp
    client/renderer/particle/ParticleRegistryTest.cpp
    client/renderer/particle/ParticleManagerTest.cpp
    client/renderer/trident/particle/particles/special/NautilusParticleTest.cpp
    # DiggingParticle 测试
    client/renderer/trident/particle/particles/block/DiggingParticleTest.cpp
    # 绗竴浜虹О鎵嬮儴娓叉煋娴嬭瘯
    client/renderer/firstperson/MatrixStackTest.cpp
    client/renderer/firstperson/FirstPersonRendererTest.cpp
    client/renderer/firstperson/ItemInHandRendererTest.cpp
    # 澹伴煶绯荤粺娴嬭瘯
    common/sound/SoundCategoryTest.cpp
    common/sound/test_sound_events.cpp
    client/sound/SoundTest.cpp
    client/sound/SoundRegistryTest.cpp
    client/sound/TickableSoundTest.cpp
    client/sound/MovingTickableSoundTest.cpp
    # 皮肤系统测试
    common/skin/test_skin_types.cpp
    common/skin/test_skin_cache.cpp
    common/skin/test_game_profile.cpp
    common/skin/test_skin_packets.cpp
    common/skin/test_skin_manager.cpp
    # 富文本系统测试
    common/util/text/TextStyleTest.cpp
    common/util/text/TextComponentTest.cpp
    # 存储系统测试
    common/world/storage/SectionCodecTest.cpp
    common/world/storage/SingleLevelStorageManagerTest.cpp
    common/world/storage/SaveFormatDetectorTest.cpp
    common/world/storage/RegionFileTest.cpp
    common/world/storage/JavaChunkReaderTest.cpp
    common/world/storage/BedrockChunkReaderTest.cpp
    # 村庄系统测试
    common/world/village/VillageTest.cpp
    # 村庄交易系统测试
    common/world/village/trade/MerchantOfferTest.cpp
    # 袭击系统测试
    common/world/village/raid/RaidHeroTrackingTest.cpp
    # 基岩版Addon模组系统测试
    common/mod/bedrock/addon/test_lifecycle.cpp
    common/mod/bedrock/addon/test_block_component_registry.cpp
    common/mod/bedrock/addon/test_item_component_registry.cpp
    # 世界边界测试
    common/world/border/WorldBorderTest.cpp
    # 记分板系统测试
    server/scoreboard/ScoreboardTest.cpp
    server/scoreboard/ScoreCriteriaTest.cpp
    server/scoreboard/TeamTest.cpp
    server/scoreboard/ScoreboardPersistenceTest.cpp
    server/scoreboard/GetTeamTest.cpp
    # Boss 栏系统测试
    server/bossbar/BossBarTest.cpp
    # 统计系统测试
    server/stats/StatisticsTest.cpp
)

# 娣诲姞鏈嶅姟绔簮鏂囦欢
target_sources(mc_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/server/core/PlayerManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/ConnectionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/TimeManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/TeleportManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/KeepAliveManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/PositionTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/PacketHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/GameModeManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/WhitelistManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/BannedPlayerList.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/BannedIpList.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/OpListManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerWorld.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkGenerateTask.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/GenerationChunkCache.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/thread/ServerWorkerPool.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerChunkManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/SingleChunkLifecycleManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityChunkTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/ItemPickupManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/NaturalSpawner.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/DespawnManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/VillageSiege.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/SpawnConditions.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/weather/WeatherManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/drop/BlockDropHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/player/ServerPlayer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/advancement/PlayerAdvancements.cpp
        # 事件系统
        ${CMAKE_SOURCE_DIR}/src/server/event/ServerEventBus.cpp
        # 记分板系统
        ${CMAKE_SOURCE_DIR}/src/server/scoreboard/ServerScoreboard.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/Scoreboard.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScoreObjective.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/Score.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScoreCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScoreCriteriaRenderType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScorePlayerTeam.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/DummyCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/TriggerCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/DeathCountCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/KillCountCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/ReadOnlyCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/TeamKillCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/network/ScoreboardPackets.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/storage/ScoreboardSaveData.cpp
        # Boss 栏系统
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/BossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/ServerBossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfoManager.cpp
        # 统计系统
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatType.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/Stat.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatisticsManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpSession.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpConnection.cpp
        ${CMAKE_SOURCE_DIR}/src/server/application/IntegratedServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/application/MinecraftServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/mod/bedrock/addon/ServerScriptManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/mod/bedrock/addon/bridge/EventBridge.cpp
        ${CMAKE_SOURCE_DIR}/src/server/mod/bedrock/addon/bridge/ServerEventSignals.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/player/ServerPlayerEntityManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/menu/CraftingMenu.cpp
        ${CMAKE_SOURCE_DIR}/src/server/settings/ServerSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/ServerCommandSource.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/CommandRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/PlayerResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/EffectResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/GameModeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DifficultyCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DefaultGameModeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TimeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/KillCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ListCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/HelpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SeedCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TeleportCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/GiveCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/LootCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ClearCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WeatherCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ExperienceCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/KickCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SayCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/StopCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetIdleTimeoutCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SummonCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetBlockCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/FillCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ExecuteCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/OpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DeOpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanIpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/PardonCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/PardonIpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanListCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WhitelistCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SaveAllCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SaveOnCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SaveOffCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SpawnPointCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetWorldSpawnCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/MessageCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TellRawCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TitleCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/PlaySoundCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/StopSoundCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/EffectCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/EnchantCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/GameRuleCommand.cpp
        # P3 命令
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/AdvancementCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BossBarCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DataPackCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ForceLoadCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/LootCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/PublishCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/RecipeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ReloadCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ReplaceItemCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ScheduleCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SpectateCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SpreadPlayersCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TagCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TeamCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TriggerCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WorldBorderCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ScoreboardCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/AttributeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/CloneCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DataCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/data/DataAccessor.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/FunctionCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/LocateCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/LocateBiomeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/MeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ParticleCommand.cpp
        # 交互管理器
        ${CMAKE_SOURCE_DIR}/src/server/interaction/BlockInteractionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/MiningManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/ContainerManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/InventoryManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/SignCommandHelper.cpp
        # 缁村害妯″潡
        # 统计系统
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatType.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/Stat.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatisticsManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/dimension/ServerDimension.cpp
        ${CMAKE_SOURCE_DIR}/src/server/dimension/ServerDimensionManager.cpp
        # 同步管理器
        ${CMAKE_SOURCE_DIR}/src/server/sync/EntitySyncManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/sync/BlockUpdateSyncManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/sync/ChunkSendManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/sync/LightSyncManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/connection/LocalConnection.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/connection/LocalServerConnection.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/sync/ChunkSync.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/packet/PacketSerializer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/packet/PacketDeserializer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/packet/Packet.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/packet/EntityPackets.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/data/ChunkData.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/NibbleArray.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/load/ChunkDistanceGraph.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/load/ChunkLoadTicketManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/gen/ChunkStatus.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/data/ChunkPrimer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/data/IChunk.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/SingleChunkLifecycleManager.cpp
        # 存储系统
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/GlobalStorageManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/SingleLevelStorageManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/core/LevelDatCodec.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/core/WorldStoragePaths.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/core/WorldSessionLock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/list/WorldListEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/list/WorldListService.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/list/WorldNameSanitizer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/request/WorldRequests.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/db/RocksDBDatabase.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/db/SectionCodec.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/section/SectionCache.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/section/SectionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/snapshot/BackupManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/save/DirtyTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/save/AutoSave.cpp
        # 存储后端与读取器
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/core/SaveFormat.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/backend/JavaAnvilBackend.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/backend/BedrockLDBBackend.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/reader/java/RegionFile.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/reader/java/JavaChunkReader.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/reader/java/JavaBlockStateMapper.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/reader/java/JavaBiomeMapper.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/reader/java/JavaLevelDatReader.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/reader/bedrock/BedrockLevelDb.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/reader/bedrock/BedrockChunkReader.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/reader/bedrock/BedrockBiomeMapper.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/reader/bedrock/BedrockLevelDatReader.cpp
        # 村庄系统
        ${CMAKE_SOURCE_DIR}/src/common/world/village/Village.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/VillageManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/VillageGossip.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/poi/PointOfInterest.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/poi/PointOfInterestStorage.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/poi/PointOfInterestType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/raid/Raid.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/raid/RaidManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/raid/RaiderType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/trade/MerchantOffer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/trade/MerchantOffers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/chunk/IChunkGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/chunk/NoiseChunkGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/chunk/DebugChunkGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/CaveCarver.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/CanyonCarver.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/CarvingMask.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/CarverConfiguration.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/WorldCarver.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/Biome.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeEffects.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeGenerationSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeSource.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/source/MultiNoiseBiomeSource.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/source/OverworldBiomeBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/source/NetherBiomeSource.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/source/EndBiomeSource.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/climate/Climate.cpp
        # 生物群系颜色解析器
        ${CMAKE_SOURCE_DIR}/src/client/world/color/BiomeColors.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/BiomeColorCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/BiomeColorBlender.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/ChunkBiomeAccessor.cpp
        # 绮掑瓙绯荤粺
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/Particle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/ParticleManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/ParticleRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/ParticleFactories.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/ParticleTextureAtlas.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/ambient/SporeBlossomParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/special/NautilusParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/ParticleTextureAtlas.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/BasicParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/BlockParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/ItemParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/RedstoneParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/sprite/SimpleSprite.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/sprite/AnimatedSprite.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/RainParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/SnowParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/ambient/BubbleParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/ambient/UnderwaterParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/ambient/CloudParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/block/DiggingParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/FlameParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/SmokeParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/LavaParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/PortalParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/CritParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/ExplosionParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/EmitterParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/PoofParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/SpellParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/DragonBreathParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/SoulParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/RedstoneParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/CampfireParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/mob/HeartParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/mob/VillagerParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/liquid/DripParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/liquid/DripWaterParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/weather/SplashParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/weather/FishingParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/special/NautilusParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/Feature.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/FeatureSpread.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/ConfiguredFeature.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/ore/OreFeature.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/tree/TreeFeature.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/tree/trunk/TrunkPlacer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/tree/trunk/StraightTrunkPlacer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/tree/foliage/FoliagePlacer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/tree/foliage/BlobFoliagePlacer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/placement/Placement.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/spawn/WorldGenSpawner.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/spawn/MobSpawnInfo.cpp
        # ${CMAKE_SOURCE_DIR}/src/common/world/BlockID.cpp  # 宸茬Щ闄?- 浣跨敤鏂扮殑鏂瑰潡绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/entity/entities/player/Player.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/player/SleepManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/core/Entity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/utils/EntityUtils.cpp
        # EndermanEntity 用于 HeldBlockLayer 测试
        ${CMAKE_SOURCE_DIR}/src/common/entity/entities/monster/end/EndermanEntity.cpp
        # NetherEntities 用于恶魂测试
        ${CMAKE_SOURCE_DIR}/src/common/entity/entities/monster/nether/NetherEntities.cpp
        # EndermanGoals 用于注视检测测试
        ${CMAKE_SOURCE_DIR}/src/common/entity/ai/goal/goals/special/EndermanGoals.cpp
        # GhastGoals 用于恶魂 AI 测试
        ${CMAKE_SOURCE_DIR}/src/common/entity/ai/goal/goals/special/GhastGoals.cpp
        # FoxGoals 用于狐狸 AI 测试
        ${CMAKE_SOURCE_DIR}/src/common/entity/ai/goal/goals/special/FoxGoals.cpp
        # PandaGoals 用于熊猫打滚 AI 测试
        ${CMAKE_SOURCE_DIR}/src/common/entity/ai/goal/goals/special/PandaGoals.cpp
        # GhastMovementController 用于恶魂飞行控制器测试
        ${CMAKE_SOURCE_DIR}/src/common/entity/ai/controller/GhastMovementController.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/ai/brain/sensor/Sensors.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/ai/brain/memory/MemoryModuleType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/ai/brain/schedule/Schedule.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/ai/brain/schedule/Activity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/movement/AutoJump.cpp
        ${CMAKE_SOURCE_DIR}/src/common/physics/PhysicsEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/common/physics/CollisionCache.cpp
        # ${CMAKE_SOURCE_DIR}/src/common/physics/collision/BlockCollision.cpp  # 宸茬Щ闄?- 浣跨敤鏂扮殑鏂瑰潡绯荤粺
        # 鏂扮殑鏂瑰潡绯荤粺婧愭枃浠?        ${CMAKE_SOURCE_DIR}/src/common/world/block/Material.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Block.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/BlockRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/registry/VanillaBlocks.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/AirBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/SimpleBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/RotatedPillarBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/FireInfoRegistry.cpp
        # 鏃堕棿绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/world/time/GameTime.cpp
        # 鐗╁搧绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/item/core/Item.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/core/ItemStack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/core/ItemRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/Items.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/block/BlockItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/block/BlockItemRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/context/ItemUseContext.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/context/BlockItemUseContext.cpp
        # 宸ュ叿灞傜骇绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/item/tier/IItemTier.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tier/ItemTiers.cpp
        # 宸ュ叿绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/item/items/tool/ToolType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/tool/TieredItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/tool/ToolItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/tool/PickaxeItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/tool/AxeItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/tool/ShovelItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/tool/HoeItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/tool/SwordItem.cpp
        # 鑳屽寘绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/Slot.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/PlayerInventory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/CraftingInventory.cpp
        # 鍚堟垚绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/Ingredient.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tag/ItemTag.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tag/ItemTags.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/ShapedRecipe.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/ShapelessRecipe.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeSerializers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeBook.cpp
        # 鏂瑰潡瀹炰綋绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/BlockEntityType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/BlockEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/core/LockableBlockEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/core/SimpleInventory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/core/BlockEntityRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/storage/ChestEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/storage/TrappedChestEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/storage/DoubleSidedInventory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/processing/AbstractFurnaceEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/processing/FurnaceEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/processing/BlastFurnaceEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/processing/SmokerEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/processing/FurnaceInventory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/processing/BeaconEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/processing/ConduitEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/transport/HopperEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/transport/IHopper.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/redstone/CommandBlockEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/CraftingTableEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/interactive/EnchantingTableEntity.cpp
        # 鐗╁搧瀹炰綋鍜屾帀钀借〃
        ${CMAKE_SOURCE_DIR}/src/common/entity/entities/item/ItemEntity.cpp
        # 鎺夎惤绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/context/LootContext.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/context/LootContextBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/context/LootParameterSet.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/context/LootParams.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/context/LootParameterSets.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/math/random/RandomRanges.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/entries/LootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/entries/EmptyLootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/entries/ItemLootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/entries/TableLootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/entries/TagLootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/entries/DynamicLootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/entries/AlternativesLootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/entries/SequenceLootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/entries/GroupLootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/entries/LootEntryBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/LootPool.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/LootTable.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/LootTableManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/LootSerializers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/SilkTouchCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/FortuneCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/RandomChanceCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/RandomChanceWithLuckCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/NotCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/AndCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/OrCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/BlockStateCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/ToolTypeCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/SurvivesExplosionCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/KilledByPlayerCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/EntityPropertiesCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/EntityScoresCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/LocationCheckCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/WeatherCheckCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/TimeCheckCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/DamageSourcePropertiesCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/ReferenceCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/FishingOpenWaterCondition.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/conditions/LootConditionBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/StatePropertiesPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/LootTableLoader.cpp
        # 成就系统
        ${CMAKE_SOURCE_DIR}/src/common/advancement/Advancement.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/AdvancementProgress.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/AdvancementDisplay.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/AdvancementRewards.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/AdvancementList.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/AdvancementManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/Criterion.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/CriterionTrigger.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/CriterionTriggers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/ItemPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/EntityPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/EntityFlagsPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/EntityEquipmentPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/NBTPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/LocationPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/BlockPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/MobEffectsPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/impl/TickTrigger.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/impl/InventoryChangedTrigger.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/impl/EntityTriggers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/impl/EffectTriggers.cpp
        ${CMAKE_SOURCE_DIR}/src/server/advancement/PlayerAdvancements.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/drop/BlockDropHandler.cpp
        # 澶╀綋璁＄畻
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/sky/CelestialCalculations.cpp
        # Trident 渲染 API 层
        ${CMAKE_SOURCE_DIR}/src/client/renderer/api/Types.cpp
        # 射线检测
        ${CMAKE_SOURCE_DIR}/src/common/util/math/ray/Raycast.cpp
        # 设置系统
        ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/ClientWorld.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/entity/ClientEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/player/LocalPlayerIdentity.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/player/ClientPlayerPredictor.cpp
        # 维度管理
        ${CMAKE_SOURCE_DIR}/src/client/dimension/ClientDimensionManager.cpp
        # 网格构建线程池
        ${CMAKE_SOURCE_DIR}/src/client/renderer/mesh/MeshWorkerPool.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/mesh/MeshBuildScheduler.cpp
        # 鍖哄潡缃戞牸鐢熸垚
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/ChunkMesher.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/AmbientOcclusionCalculator.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/MeshTypes.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/pipeline/EntityPipeline.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/core/ModelRenderer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/core/EntityModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/core/AgeableModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/base/BipedModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/base/QuadrupedModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/animal/AnimalModels.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/animal/LlamaModel.cpp
        # PlayerModel 手臂渲染测试
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/player/PlayerModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/pipeline/EntityTextureAtlas.cpp
        # HeldBlockLayer 测试只测试 EndermanEntity 的方法，不需要 Vulkan 渲染层代码
        # 方块实体模型
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/blockentity/model/BlockEntityModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/blockentity/model/ChestModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/blockentity/model/BeaconBeamModel.cpp
        # 绗竴浜虹О鎵嬮儴娓叉煋
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/firstperson/MatrixStack.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/firstperson/ItemCameraTransforms.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/firstperson/PlayerModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/firstperson/ItemInHandRenderer.cpp
        # 鍖哄潡妯″瀷缂撳瓨
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ResourceManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockStateLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/TextureAtlasBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemTextureAtlas.cpp

        ${CMAKE_SOURCE_DIR}/src/common/resource/VanillaResources.cpp
        # Kagero UI 模板系统（仅核心解析组件）
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/Ast.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/AstVisitor.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/Lexer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/Parser.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/binder/BindingContext.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/compiler/TemplateCompiler.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/runtime/UpdateScheduler.cpp
        # Kagero UI甯冨眬绯荤粺
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/layout/core/LayoutEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/layout/algorithms/FlexLayout.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/layout/algorithms/GridLayout.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/layout/algorithms/AnchorLayout.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/layout/integration/WidgetLayoutAdaptor.cpp
        # Kagero UI Widget缁勪欢
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/widget/ContainerWidget.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/widget/ListWidget.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/paint/PaintContext.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/paint/Geometry.cpp
        # Assert搴?        ${CMAKE_SOURCE_DIR}/src/common/util/assert/Assert.cpp
        # Crypto utilities
        ${CMAKE_SOURCE_DIR}/src/common/util/crypto/Sha256.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/crypto/Md5.cpp
        # UUID utilities
        ${CMAKE_SOURCE_DIR}/src/common/util/UuidUtils.cpp
        # 澹伴煶绯荤粺 - Common
        ${CMAKE_SOURCE_DIR}/src/common/sound/SoundCategory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sound/SoundEvent.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sound/network/SoundPackets.cpp
        # Skin System
        ${CMAKE_SOURCE_DIR}/src/common/skin/core/SkinTypes.cpp
        ${CMAKE_SOURCE_DIR}/src/common/skin/core/GameProfile.cpp
        ${CMAKE_SOURCE_DIR}/src/common/skin/core/SkinTextures.cpp
        ${CMAKE_SOURCE_DIR}/src/common/skin/manager/SkinCache.cpp
        ${CMAKE_SOURCE_DIR}/src/common/skin/manager/DefaultSkinProvider.cpp
        ${CMAKE_SOURCE_DIR}/src/common/skin/manager/SkinManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/skin/loader/SkinLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/common/skin/loader/FileSkinLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/common/skin/loader/HttpSkinLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/common/skin/parser/SkinMetadataParser.cpp
        ${CMAKE_SOURCE_DIR}/src/common/skin/network/PlayerSkinInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/common/skin/network/SkinPackets.cpp
        ${CMAKE_SOURCE_DIR}/src/client/skin/ClientSkinManager.cpp
        # 澹伴煶绯荤粺 - Client
        ${CMAKE_SOURCE_DIR}/src/client/sound/resource/SoundDefinition.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/resource/SoundRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/SoundHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/SoundLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/StbVorbisImpl.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/SoundPool.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/SoundEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/AudioBufferCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/instance/SoundInstance.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/instance/UnderwaterLoopSound.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/instance/MinecartSound.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/instance/MovingTickableSound.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/backend/AudioBuffer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/backend/OpenALBackend.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/handler/BiomeAmbientHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/handler/UnderwaterAmbientHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/handler/BubbleColumnAmbientHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/handler/EntitySoundHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/handler/WeatherSoundHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/client/command/ClientCommandManager.cpp
        # TemplateInstance.cpp 依赖 Widget 实现，暂时不纳入该测试目标
)

target_link_libraries(mc_tests
    PRIVATE
        mc_common
        mc_bedrock_addon
        GTest::gtest_main
        asio::asio
        nlohmann_json::nlohmann_json
        glm::glm
        Vulkan::Vulkan
        OpenAL::OpenAL
)

if(WIN32)
    target_link_libraries(mc_tests PRIVATE ws2_32)
endif()

target_include_directories(mc_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests
)

target_compile_definitions(mc_tests
    PRIVATE
        MC_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

mc_disable_all_warnings_for_tests(mc_tests)
mc_copy_runtime_dlls(mc_tests)

# 资源测试可执行文件
add_executable(mc_resource_tests
    client/resource/test_resource_location.cpp
    client/resource/test_model_loader.cpp
)

target_link_libraries(mc_resource_tests
    PRIVATE
        mc_common
        mc_bedrock_addon
        GTest::gtest_main
        nlohmann_json::nlohmann_json
        glm::glm
)

# 娣诲姞瀹㈡埛绔祫婧愭簮鏂囦欢
target_sources(mc_resource_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockStateLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/TextureAtlasBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ResourceManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/ResourcePackList.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/ZipResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/FolderResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/IResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/PackMetadata.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/ResourceLocation.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Block.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/BlockRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/registry/VanillaBlocks.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/AirBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/SimpleBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/RotatedPillarBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Material.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/MeshTypes.cpp
)

target_include_directories(mc_resource_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

# stb_image 瀹炵幇鏀惧湪TextureAtlasBuilder.cpp涓?
mc_disable_all_warnings_for_tests(mc_resource_tests)
mc_copy_runtime_dlls(mc_resource_tests)

# 鍙戠幇娴嬭瘯
include(GoogleTest)

function(mc_register_gtests targetName)
    if(WIN32)
        # Windows + clang 当前环境下 applocal.ps1 无法解析 DLL 依赖，
        # 会导致 gtest_discover_tests 在构建阶段直接失败。
        # 改为仅注册到 CTest，不在构建时探测测试列表，先保证整体可编译。
        add_test(NAME ${targetName} COMMAND $<TARGET_FILE:${targetName}>)
    else()
        gtest_discover_tests(${targetName})
    endif()
endfunction()

mc_register_gtests(mc_tests)
mc_register_gtests(mc_resource_tests)

# 输出测试可执行文件位置
set_target_properties(mc_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(mc_resource_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ============================================================================
# Trident 娓叉煋寮曟搸娴嬭瘯
# ============================================================================

if(MC_BUILD_CLIENT)
    # Trident 渲染引擎测试可执行文件
    # 注意：这里只测试核心组件，不测试 TridentEngine 本体
    add_executable(mc_trident_tests
        client/renderer/test_trident_engine.cpp
        client/renderer/PolarBearAnimationTest.cpp
        client/renderer/EntityRendererCoverageTest.cpp
        client/renderer/AnimatedMeshCacheTest.cpp
    )

    # 添加 Trident 渲染引擎核心组件源文件
    target_sources(mc_trident_tests
        PRIVATE
            # Trident 核心（不含 TridentEngine）
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/TridentContext.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/TridentSwapchain.cpp
            # Trident 缓冲区
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/buffer/TridentBuffer.cpp
            # Trident 绾圭悊
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/texture/TridentTexture.cpp
            # Trident 绠＄嚎
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/pipeline/TridentPipeline.cpp
            # Trident 渲染管理器
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/FrameManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/UniformManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/DescriptorManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/RenderPassManager.cpp
            # Trident API 灞?            ${CMAKE_SOURCE_DIR}/src/client/renderer/api/Types.cpp
            # 璧勬簮鍔犺浇
            ${CMAKE_SOURCE_DIR}/src/client/resource/TextureAtlasBuilder.cpp
            ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelCache.cpp
            ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelCache.cpp
            ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelLoader.cpp
            ${CMAKE_SOURCE_DIR}/src/client/resource/ResourceManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelLoader.cpp
            ${CMAKE_SOURCE_DIR}/src/client/resource/BlockStateLoader.cpp
            # 缃戞牸鐩稿叧
            ${CMAKE_SOURCE_DIR}/src/client/renderer/MeshTypes.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/ChunkMesher.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/AmbientOcclusionCalculator.cpp
            # 鐢熺墿缇ょ郴棰滆壊
            ${CMAKE_SOURCE_DIR}/src/client/world/color/BiomeColors.cpp
            ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/BiomeColorCache.cpp
            ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/BiomeColorBlender.cpp
            ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/ChunkBiomeAccessor.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeEffects.cpp
            # 甯哥敤璧勬簮
            ${CMAKE_SOURCE_DIR}/src/common/resource/ResourceLocation.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/ResourcePackList.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/FolderResourcePack.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/ZipResourcePack.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/IResourcePack.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/PackMetadata.cpp
            # 鏂瑰潡绯荤粺
            ${CMAKE_SOURCE_DIR}/src/common/world/block/Material.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/Block.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/BlockRegistry.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/registry/VanillaBlocks.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/AirBlock.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/SimpleBlock.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/RotatedPillarBlock.cpp
            # 璁剧疆
            ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
            # AnimationContext
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/core/AnimationContext.cpp
    )

    target_link_libraries(mc_trident_tests
        PRIVATE
            mc_common
            mc_bedrock_addon
            GTest::gtest_main
            glfw
            Vulkan::Vulkan
            nlohmann_json::nlohmann_json
            glm::glm
    )

    target_include_directories(mc_trident_tests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/include
    )

    mc_disable_all_warnings_for_tests(mc_trident_tests)
    mc_copy_runtime_dlls(mc_trident_tests)

    mc_register_gtests(mc_trident_tests)

    set_target_properties(mc_trident_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # GUI绾圭悊绯荤粺娴嬭瘯锛堢函鏁版嵁缁撴瀯娴嬭瘯锛屼笉闇€瑕乂ulkan锛?    # 杩欎簺娴嬭瘯鏀惧湪涓绘祴璇曞浠朵腑锛岄伩鍏峍ulkan渚濊禆
    target_sources(mc_tests
        PRIVATE
            # GuiSprite 鍜?GuiNinePatch 娴嬭瘯
            ${CMAKE_CURRENT_SOURCE_DIR}/client/renderer/trident/gui/GuiSpriteTest.cpp
            # GuiSpriteManager 娴嬭瘯锛堜笉渚濊禆Vulkan锛?            ${CMAKE_CURRENT_SOURCE_DIR}/client/renderer/trident/gui/GuiSpriteManagerTest.cpp
            # GuiSpriteParser JSON瑙ｆ瀽娴嬭瘯锛堜笉闇€瑕佽祫婧愬寘锛?            ${CMAKE_CURRENT_SOURCE_DIR}/client/renderer/trident/gui/GuiSpriteParserTest.cpp
            # 婧愭枃浠讹紙GuiSprite.hpp 鏄函澶存枃浠讹級
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/gui/GuiSpriteManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/gui/GuiSpriteParser.cpp
    )

    # GUI绾圭悊绯荤粺娴嬭瘯锛堥渶瑕乂ulkan鐨勬祴璇曟殏鏃剁鐢級
    # GuiTextureAtlas 渚濊禆 GuiRenderer 鍜?Vulkan锛岃€屽叾鍔熻兘宸茶 GuiSpriteManager 娴嬭瘯瑕嗙洊
    # 濡傛灉闇€瑕佹祴璇?GuiTextureAtlas 鐨?Vulkan 鍔熻兘锛屽彲浠ユ坊鍔犲洖锛?    # target_sources(mc_trident_tests
    #     PRIVATE
    #         ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/gui/GuiTextureAtlas.cpp
    #         ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/gui/GuiSpriteRegistry.cpp
    #         ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/gui/GuiTextureLoader.cpp
    # )
endif()

add_executable(mc_command_tests
    common/BaseTestServer.cpp
    server/command/CommandRegistryTest.cpp
    server/command/CommandStorageTest.cpp
    server/command/PlayerResolverTest.cpp
    server/command/TeamCommandTest.cpp
    server/command/TriggerCommandTest.cpp
    common/command/arguments/BlockStateArgumentTest.cpp
    common/command/arguments/EntityArgumentTest.cpp
    common/command/arguments/NbtPathTest.cpp
)

target_sources(mc_command_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/server/core/PlayerManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/ConnectionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/TimeManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/TeleportManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/KeepAliveManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/PositionTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/PacketHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/GameModeManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/WhitelistManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/BannedPlayerList.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/BannedIpList.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/OpListManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/player/ServerPlayer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/advancement/PlayerAdvancements.cpp
        # 事件系统
        ${CMAKE_SOURCE_DIR}/src/server/event/ServerEventBus.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerWorld.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkGenerateTask.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/GenerationChunkCache.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/thread/ServerWorkerPool.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerChunkManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/SingleChunkLifecycleManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityChunkTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/ItemPickupManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/player/ServerPlayerEntityManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/drop/BlockDropHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/NaturalSpawner.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/DespawnManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/VillageSiege.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/SpawnConditions.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/weather/WeatherManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/InventoryManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/weather/WeatherManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/sync/EntitySyncManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/sync/BlockUpdateSyncManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/sync/ChunkSendManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/sync/LightSyncManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/ServerCommandSource.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/CommandRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/PlayerResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/EffectResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/GameModeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DifficultyCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DefaultGameModeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TimeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/KickCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/KillCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ListCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/HelpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SeedCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TeleportCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/GiveCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/LootCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ClearCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WeatherCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ExperienceCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SayCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/StopCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetIdleTimeoutCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SummonCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetBlockCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/FillCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ExecuteCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/OpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DeOpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanIpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/PardonCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/PardonIpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanListCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WhitelistCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SaveAllCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SaveOnCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SaveOffCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SpawnPointCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetWorldSpawnCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/MessageCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TellRawCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TitleCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/PlaySoundCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/StopSoundCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/EffectCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/EnchantCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/GameRuleCommand.cpp
        # P3 命令
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/AdvancementCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BossBarCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DataPackCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ForceLoadCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/LootCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/PublishCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/RecipeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ReloadCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ReplaceItemCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ScheduleCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SpectateCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SpreadPlayersCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TagCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TeamCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TriggerCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WorldBorderCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ScoreboardCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/AttributeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/CloneCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DataCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/data/DataAccessor.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/FunctionCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/LocateCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/LocateBiomeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/MeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ParticleCommand.cpp
        # 统计系统
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatType.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/Stat.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatisticsManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/dimension/ServerDimension.cpp
        ${CMAKE_SOURCE_DIR}/src/server/dimension/ServerDimensionManager.cpp
        # 记分板系统
        ${CMAKE_SOURCE_DIR}/src/server/scoreboard/ServerScoreboard.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/Scoreboard.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScoreObjective.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/Score.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScoreCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScoreCriteriaRenderType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScorePlayerTeam.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/DummyCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/TriggerCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/DeathCountCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/KillCountCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/ReadOnlyCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/TeamKillCriteria.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/network/ScoreboardPackets.cpp
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/storage/ScoreboardSaveData.cpp
        # Boss 栏系统
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/BossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/ServerBossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfoManager.cpp
)

target_link_libraries(mc_command_tests
    PRIVATE
        mc_common
        mc_bedrock_addon
        GTest::gtest_main
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

target_include_directories(mc_command_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests
)

mc_disable_all_warnings_for_tests(mc_command_tests)
mc_copy_runtime_dlls(mc_command_tests)

mc_register_gtests(mc_command_tests)

set_target_properties(mc_command_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ============================================================================
# 村庄系统独立测试
# ============================================================================

add_executable(mc_village_tests
    common/world/village/VillageTest.cpp
    common/world/village/trade/MerchantOfferTest.cpp
    common/world/village/raid/RaidHeroTrackingTest.cpp
)

target_sources(mc_village_tests
    PRIVATE
        # 村庄系统源文件
        ${CMAKE_SOURCE_DIR}/src/common/world/village/Village.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/VillageManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/VillageGossip.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/poi/PointOfInterest.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/poi/PointOfInterestStorage.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/poi/PointOfInterestType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/raid/Raid.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/raid/RaidManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/raid/RaiderType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/trade/MerchantOffer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/village/trade/MerchantOffers.cpp
        # 实体系统（用于测试）
        ${CMAKE_SOURCE_DIR}/src/common/entity/core/Entity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/utils/EntityUtils.cpp
        # 方块系统
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Block.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/BlockRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/registry/VanillaBlocks.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/AirBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/SimpleBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/RotatedPillarBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Material.cpp
        # 物品系统
        ${CMAKE_SOURCE_DIR}/src/common/item/core/Item.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/core/ItemStack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/core/ItemRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/Items.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/block/BlockItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/block/BlockItemRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/context/ItemUseContext.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/context/BlockItemUseContext.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tag/ItemTag.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tag/ItemTags.cpp
        # NBT 系统
        ${CMAKE_SOURCE_DIR}/src/common/util/nbt/Nbt.cpp
        # 流体系统
        ${CMAKE_SOURCE_DIR}/src/common/world/fluid/Fluid.cpp
        # 资源定位
        ${CMAKE_SOURCE_DIR}/src/common/resource/ResourceLocation.cpp
)

target_link_libraries(mc_village_tests
    PRIVATE
        mc_common
        mc_bedrock_addon
        GTest::gtest_main
        nlohmann_json::nlohmann_json
        glm::glm
)

target_include_directories(mc_village_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

mc_disable_all_warnings_for_tests(mc_village_tests)
mc_copy_runtime_dlls(mc_village_tests)

mc_register_gtests(mc_village_tests)

set_target_properties(mc_village_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
