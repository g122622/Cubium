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
    main.cpp
    common/test_math.cpp
    common/test_network.cpp
    common/test_world.cpp
    common/test_core.cpp
    common/GameDirectoryTest.cpp
    common/DefaultValuesTest.cpp
    common/PackTypeTest.cpp
    common/DataPackRepositoryTest.cpp
    # common/test_blockid.cpp  # 宸茬Щ闄?- 浣跨敤鏂扮殑鏂瑰潡绯荤粺
    # common/test_noise.cpp     # 宸茬Щ闄?- Noise 绫诲凡鍒犻櫎
    common/test_entity.cpp
    common/test_entity_physics.cpp
    common/test_chunksync.cpp
    common/test_localconnection.cpp
    common/test_chunkloadticket.cpp
    # common/test_collision.cpp  # 鏆傛椂绂佺敤 - 渚濊禆鏃х殑纰版挒绯荤粺
    common/test_property.cpp
    common/util/StateHolderTest.cpp
    common/util/text/Utf8Test.cpp
    common/TestWorldHelper.cpp
    common/BaseTestServer.cpp
    common/test_block.cpp
    common/test_pointed_dripstone.cpp
    common/test_chunk_generation.cpp
    common/test_chunk_level.cpp
    common/test_biome.cpp
    # Biome 扩展单元测试（温度缓存、SpawnFinder、FixedBiomeSource、BiomeTags、TheVoid）
    common/world/biome/BiomeContainerTest.cpp
    common/world/biome/BiomeExtendedTest.cpp
    common/world/biome/BiomeFactoryClimateTest.cpp
    common/world/biome/ClimateTest.cpp
    common/world/biome/RTreeTest.cpp
    common/world/biome/OverworldBiomeBuilderTest.cpp
    # BiomeLoader 数据驱动生物生成配置解析测试（creature_spawn_probability / spawners 校验）
    common/world/biome/BiomeLoaderSpawnTest.cpp
    common/test_biome_freeze.cpp
    common/world/biome/BiomeGetPrecipitationAtTest.cpp
    # BiomeGenerationSettings 花卉列表填充与 GrassBlock::grow 解析路径测试
    common/world/biome/BiomeFlowerFeatureTest.cpp
    common/test_ore_feature.cpp
    common/test_carver.cpp
    common/test_tree_feature.cpp
    common/test_new_tree_features.cpp
    common/world/gen/feature/state/test_block_state_provider.cpp
    common/test_feature_size.cpp
    common/test_decoration_stage.cpp
    common/test_time.cpp
    common/test_item.cpp
    common/test_block_item.cpp
    common/test_inventory.cpp
    common/test_equipment_serialization.cpp
    common/test_item_entity.cpp
    common/test_container.cpp
    common/item/crafting/IngredientTest.cpp
    common/item/crafting/RecipeBookTest.cpp
    common/item/crafting/RecipeManagerTest.cpp
    common/item/crafting/ShapedRecipeTest.cpp
    common/item/crafting/ShapelessRecipeTest.cpp
    common/item/crafting/RecipeLoaderTest.cpp
    common/item/crafting/RecipeFormatTest.cpp
    common/item/crafting/RecipeSerializersTest.cpp
    common/item/crafting/BannerDuplicateRecipeTest.cpp
    common/item/crafting/ShieldDecorationRecipeTest.cpp
    # DecoratedPotRecipe 饰纹陶罐合成配方测试
    common/item/crafting/DecoratedPotRecipeTest.cpp
    common/item/tool/ToolTests.cpp
    common/item/tool/ShearsItemTest.cpp
    # ItemTags 花朵标签测试
    common/item/tag/ItemTagsTest.cpp
    # BedItem 床物品放置测试
    common/item/BedItemTest.cpp
    # BlockItemUseContext 上下文测试（pitch 透传、getNearestLookingDirections 方向排序）
    common/item/BlockItemUseContextTest.cpp
    # ItemTagLoader 数据包标签加载测试
    common/item/tag/ItemTagLoaderTest.cpp
    common/item/potion/GlassBottleItemTest.cpp
    common/item/potion/PotionUtilsTest.cpp
    common/item/weapon/FireChargeItemTest.cpp
    common/item/weapon/ThrowableItemTest.cpp
    common/item/weapon/WeaponItemTest.cpp
    common/item/weapon/SpearItemTest.cpp
    # WindChargeItem 风弹物品测试（投掷、冷却、消耗）
    common/item/WindChargeItemTest.cpp
    # WolfArmorItem / NautilusArmorItem / ArmadilloScuteArmorMaterial 测试
    common/item/WolfAndNautilusArmorTest.cpp
    # ArmorSlot::Body / ArmorMaterial / ArmorItem::isBodyArmor 测试
    common/item/ArmorMaterialTest.cpp
    # HarnessItem 欢乐诡鬼装备测试（16色注册、颜色映射、HARNESSES 标签集成）
    common/item/HarnessItemTest.cpp
    # BundleItem 收纳袋测试（17色变体注册、BundleContents 权重系统、槽位覆写协议、染色转化）
    common/item/special/BundleContentsTest.cpp
    common/item/special/BundleItemTest.cpp
    common/item/special/TransmuteRecipeTest.cpp
    common/item/special/BucketItemTest.cpp
    common/item/special/FishBucketItemTest.cpp
    common/item/special/BoneMealItemTest.cpp
    common/item/special/HoneycombItemTest.cpp
    common/item/special/WaxIntegrationTest.cpp
    common/item/special/OnAStickItemTest.cpp
    # PowderSnowBucketItem 细雪桶测试
    common/item/special/PowderSnowBucketItemTest.cpp
    # MusicDiscItem 音乐唱片测试（比较器信号、isMusicDisc、声音事件ID）
    common/item/special/MusicDiscItemTest.cpp
    # PotterySherdItem / SmithingTemplateItem 测试（陶片图案映射、锻造模板类型、注册完整性）
    common/item/special/PotterySherdAndSmithingTemplateTest.cpp
    # 物品 addInformation tooltip 翻译测试（锻造模板格式、旗帜图案翻译键、LanguageManager回退）
    common/item/special/ItemTooltipTest.cpp
    # BrushItem 刷子物品测试（属性、onUseTick触发时机、非玩家实体停止使用、粒子生成）
    common/item/special/BrushItemTest.cpp
    common/world/map/MaterialColorTest.cpp
    common/world/map/MapDataTest.cpp
    common/item/food/ChorusFruitItemTest.cpp
    common/item/food/FoodItemTest.cpp
    common/item/NewItemTest.cpp
    common/item/ItemCraftedByTest.cpp
    common/item/BlockItemNbtTest.cpp
    common/item/GameMasterBlockItemTest.cpp
    common/item/WoodVariantBlockItemTest.cpp
    common/item/DeepslateItemRegistrationTest.cpp
    common/item/PolishedBlackstoneItemRegistrationTest.cpp
    common/item/BlackstoneVariantItemRegistrationTest.cpp
    common/item/RawOreItemRegistrationTest.cpp
    common/item/AnvilLightningRodItemRegistrationTest.cpp
    common/item/CaveAndNetherItemRegistrationTest.cpp
    common/item/ChainItemRegistrationTest.cpp
    common/item/CopperDoorTrapdoorItemRegistrationTest.cpp
    common/item/MiscBlockItemRegistrationTest.cpp
    common/item/MudCaveMangroveItemRegistrationTest.cpp
    common/item/ShelfItemRegistrationTest.cpp
    common/item/TrailsItemsRegistrationTest.cpp
    common/item/core/AdventureModePredicateTest.cpp
    common/item/core/AdventureModeIntegrationTest.cpp
    common/item/core/ProjectileItemTest.cpp
    common/item/enchantment/EnchantmentTest.cpp
    common/item/enchantment/EnchantmentCallbackTest.cpp
    common/item/enchantment/AttemptDamageItemTest.cpp
    common/item/enchantment/MaceEnchantmentTest.cpp
    common/item/enchantment/LocationEnchantmentTest.cpp
    common/entity/inventory/CraftingInventoryTest.cpp
    common/entity/inventory/PlayerInventoryNbtTest.cpp
    common/entity/inventory/PlayerEnderChestInventoryTest.cpp
    common/entity/inventory/IRecipeHolderTest.cpp
    common/entity/inventory/InventoryRefTest.cpp
    common/entity/inventory/container/ChestContainerTest.cpp
    common/entity/inventory/container/HopperContainerTest.cpp
    common/entity/inventory/container/FurnaceContainerTest.cpp
    client/ui/screen/FurnaceScreenTextureTest.cpp
    common/entity/inventory/container/EnchantmentContainerTest.cpp
    common/entity/inventory/container/BrewingStandContainerTest.cpp
    common/entity/inventory/container/AnvilContainerTest.cpp
    common/entity/inventory/container/LoomContainerTest.cpp
    common/entity/inventory/container/MerchantContainerTest.cpp
    common/entity/inventory/container/CrafterContainerTest.cpp
    common/entity/entities/passive/basic/ChickenEntityTest.cpp
    common/entity/entities/passive/basic/RabbitEntityTest.cpp
    common/entity/entities/passive/basic/MooshroomEntityTest.cpp
    common/entity/entities/passive/basic/AnimalEntityTest.cpp
    common/entity/entities/passive/tamable/WolfEntityTest.cpp
    common/entity/entities/passive/tamable/ParrotEntityTest.cpp
    common/entity/entities/passive/tamable/OcelotEntityTest.cpp
    # Crackiness 裂纹程度追踪器测试（狼铠裂纹渲染）
    common/entity/entities/passive/tamable/CrackinessTest.cpp
    # BegGoal 驯服物品乞求测试
    entity/BegGoalTest.cpp
    # CatEntity 猫实体测试（驯服物品、繁殖物品、CatTemptGoal、CatAvoidPlayerGoal）
    entity/CatEntityTest.cpp
    # CatGoals 猫AI目标测试
    entity/CatGoalsTest.cpp
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
    # CopperGolemEntity 铜傀儡测试
    common/entity/entities/passive/golem/CopperGolemEntityTest.cpp
    # CopperGolemEntity turnToStatue 拴绳掉落与雕像转化测试
    common/entity/entities/passive/golem/CopperGolemTurnToStatueTest.cpp
    # TransportItemsBetweenContainersGoal 铜傀儡物品运输目标测试
    entity/TransportItemsBetweenContainersGoalTest.cpp
    common/entity/DolphinEntityTest.cpp
    # 鱿鱼 AI 目标测试
    common/entity/SquidGoalsTest.cpp
    # 发光鱿鱼实体测试（同步数据、NBT 往返、hurt 暗化、喷墨粒子、registerData 唯一性）
    entity/GlowSquidEntityTest.cpp
    # 村民 AI 目标测试
    common/entity/VillagerGoalsTest.cpp
    # 职业映射测试
    common/entity/ProfessionMappingTest.cpp
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
    # 铁傀儡实体测试（属性、攻击、持花状态等）
    common/entity/IronGolemEntityTest.cpp
    # canAttackType 攻击类型判断测试（Mob基类排除恶魂、Phantom覆盖、Breeze白名单、IronGolem排除苦力怕）
    entity/CanAttackTypeTest.cpp
    # 属性映射表测试（resetBaseValue、hasModifier、getModifierValue）
    common/entity/attribute/AttributeMapTest.cpp
    # 属性注册表测试（isKnown、getRange、normalizeName、getDefaultValue）
    common/entity/attribute/AttributeRegistryTest.cpp
    # EntityTypeTag 实体类型标签测试
    common/entity/tag/EntityTypeTagsTest.cpp
    # DamageTypeTag 伤害类型标签测试（含 DamageSource::is() 和加载器测试）
    common/entity/damage/DamageTypeTagsTest.cpp
    # damageTilt 完整链路对齐测试（DamageSource.sourcePosition / LivingEntity.hurtDir / 网络包 hurtDir 往返）
    common/entity/damage/DamageTiltAlignmentTest.cpp
    common/entity/entities/monster/EndermiteEntityTest.cpp
    # SpiderEntity AI 目标、光照敏感、中毒攻击测试
    entity/SpiderEntityTest.cpp
    # RavagerEntity 劫掠兽测试
    entity/RavagerEntityTest.cpp
    common/entity/entities/monster/ZombieVillagerEntityTest.cpp
    # VillagerEntity die/remove/releaseAllPois 测试
    common/entity/entities/villager/VillagerEntityTest.cpp
    # ZombieVillagerEntity 床和铁栏杆加速治愈测试
    entity/ZombieVillagerConversionTest.cpp
    # VillagerData 升级逻辑和交易经验球测试
    entity/VillagerTradeXpTest.cpp
    # ZombieEntity 僵尸转溺尸测试
    common/entity/entities/monster/undead/ZombieEntityTest.cpp
    # SkeletonEntity 骷髅射箭测试
    common/entity/entities/monster/undead/SkeletonEntityTest.cpp
    # SkeletonEntity 骷髅战斗目标切换测试（getWeaponHoldingHand, canUseNonMeleeWeapon, setCombatTask, setEquipment）
    common/entity/entities/monster/undead/SkeletonCombatTaskTest.cpp
    # AbstractSkeletonEntity 拉弓状态（DATA_CHARGING_BOW_PARAM）注册与读写测试
    common/entity/entities/monster/undead/SkeletonChargingBowTest.cpp
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
    # IllusionerEntity 镜像分身偏移、施法状态和远程攻击测试
    common/entity/entities/monster/illager/IllusionerEntityTest.cpp
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
    # OminousItemSpawnerEntity 不祥物品生成器测试
    common/entity/entities/misc/OminousItemSpawnerEntityTest.cpp
    common/entity/entities/HangingEntityTest.cpp
    # EnderCrystalEntity 治愈末影龙测试
    common/entity/entities/effect/EnderCrystalHealTest.cpp
    # MonsterEntity 生成位置检查测试
    common/entity/entities/monster/MonsterSpawnTest.cpp
    # LivingEntity 箭矢计数和发光效果测试
    common/entity/core/LivingEntityArrowCountTest.cpp
    # LivingEntity 吸收值 getter/setter 和边界 clamping 测试
    common/entity/core/LivingEntityAbsorptionTest.cpp
    # LivingEntity 装备破损回调（onEquippedItemBroken、broadcastBreakEvent）测试
    common/entity/core/LivingEntityBreakEventTest.cpp
    # LivingEntity 装备更新检测（detectEquipmentUpdates、stopLocationBasedEffects、equipmentHasChanged）测试
    common/entity/core/EquipmentUpdateTest.cpp
    # StepHeight 步高系统测试（Entity、LivingEntity、特定实体步高设置）
    common/entity/core/StepHeightTest.cpp
    # Entity::isInRain() 双位置检测和 EndermanEntity::isInWaterOrRain() 测试
    common/entity/core/EntityIsInRainTest.cpp
    # 方块碰撞检测测试（doBlockCollisions 在 LivingEntity::aiStep 中的调用）
    common/entity/core/BlockCollisionTest.cpp
    # 步声系统测试（Entity::playStepSound 组合步声、内部步声、紫水晶共振）
    common/entity/core/StepSoundTest.cpp
    # 拴绳系统测试
    common/entity/core/LeashSystemTest.cpp
    # MutableEquipment 可变装备槽测试
    common/entity/core/MutableEquipmentTest.cpp
    common/entity/EntitySerializationTest.cpp
    # Entity::isOnSameTeam() 队伍系统测试
    entity/EntityTeamTest.cpp
    # Entity::isAlliedTo() 盟友系统测试
    entity/AlliedAndTargetTest.cpp
    # Player::canHarmPlayer() PvP 保护测试
    entity/PlayerCanHarmPlayerTest.cpp
    # Player::attack() 横扫攻击过滤、荆棘附魔反伤、武器损耗测试
    entity/PlayerAttackTest.cpp
    # IRideable 速度设置逻辑测试
    common/entity/interfaces/IRideableTest.cpp
    # BoostHelper NBT 序列化测试（可骑乘实体加速辅助）
    common/entity/interfaces/BoostHelperTest.cpp
    # DamageSource isExplosion() 和 isFire() 测试
    common/entity/damage/DamageSourceExplosionTest.cpp
    # 冰冻系统测试（DamageType::Freeze、冰冻状态计算、冰冻标签、冰冻游戏规则）
    common/entity/core/EntityFreezeTest.cpp
    # 冰冻系统集成测试（tickFreeze、clearFreeze、canFreeze、Player::isInvulnerableTo）
    common/entity/core/EntityFreezeIntegrationTest.cpp
    # 矿车 dropItem() 伤害源检测测试
    common/entity/entities/vehicle/MinecartDropItemTest.cpp
    # 矿车 _checkRailState() 铁轨形状提取测试
    common/entity/entities/vehicle/MinecartCheckRailStateTest.cpp
    # 船实体 getBoatItem() 和 dropItem() 测试
    common/entity/entities/vehicle/BoatEntityDropItemTest.cpp
    # 船实体 getBoatGlide() 滑度采样测试
    common/entity/entities/vehicle/BoatGlideTest.cpp
    # 船实体陆地摩擦力减半条件测试
    common/entity/entities/vehicle/BoatLandFrictionTest.cpp
    # ChestBoatEntity getBoatItem() 虚函数测试
    common/entity/entities/vehicle/ChestBoatEntityTest.cpp
    # 实体掉落 DO_ENTITY_DROPS 游戏规则测试
    common/entity/entities/EntityDropGameRuleTest.cpp
    # 实体 dropItem 受 DO_ENTITY_DROPS 控制的集成测试
    common/entity/entities/EntityDropItemGameRuleTest.cpp
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
    # EnchantmentPredicate 附魔谓词测试
    advancement/EnchantmentPredicateTest.cpp
    # NBTPredicate NBT谓词测试（Mojangson/JSON解析、匹配、序列化）
    advancement/NBTPredicateTest.cpp
    # BredAnimalsTrigger 动物繁殖触发器测试
    advancement/BredAnimalsTriggerTest.cpp
    # AvoidVibrationTrigger 避免振动触发器测试
    advancement/AvoidVibrationTriggerTest.cpp
    # AdvancementEventHandler 测试（getServerPlayer 架构验证）
    server/advancement/AdvancementEventHandlerTest.cpp
    # PlayerAdvancements 监听器注册/注销、进度管理、可见性测试
    server/advancement/PlayerAdvancementsTest.cpp
    # PlayerDestroyItemEvent 物品销毁事件测试
    server/event/PlayerDestroyItemEventTest.cpp
    # TameAnimalEvent 驯服动物事件测试
    server/event/TameAnimalEventTest.cpp
    # SummonedEntityEvent 召唤实体事件测试
    server/event/SummonedEntityEventTest.cpp
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
    # ArrowEntity 修复验证测试（拾取状态、伤害公式、isPlayer判断）
    common/entity/ArrowEntityFixTest.cpp
    # ItemEntity 伤害处理测试（hurt、isImmuneToFire、canBeHurtBy、FIRE_RESISTANT标签）
    common/entity/ItemEntityTest.cpp
    # Entity 岩浆伤害/火焰处理测试（lavaIgnite、lavaHurt、shouldPlayLavaHurtSound、clearFire、baseTick火焰逻辑）
    common/entity/core/EntityLavaFireTest.cpp
    # 难度系统测试
    common/entity/combat/DifficultyHelperTest.cpp
    # DifficultyInstance 区域难度测试
    common/entity/combat/DifficultyInstanceTest.cpp
    # MobEntity 装备生成和finalizeSpawn测试
    common/entity/combat/MobEntityEquipmentTest.cpp
    # MobEntity 交互测试（processInitialInteract、canBeLeashed、_spawnOffspringFromSpawnEgg）
    common/entity/core/MobEntityInteractTest.cpp
    # MobEntity 寻路惩罚值测试（setPathfindingMalus/getPathfindingMalus/shouldPassengersInheritMalus）
    common/entity/core/PathfindingMalusTest.cpp
    # Entity 持久化随机数生成器测试（getRandom() 返回引用、独立 RNG、状态连续性、const 可调用性）
    common/entity/core/EntityRandomTest.cpp
    # MobEntity 序列化测试（掉落概率、死亡掉落表、拴绳数据 NBT 序列化）
    entity/MobEntitySerializationTest.cpp
    # MobEntity DATA_MOB_FLAGS_PARAM 位 2 (MOB_FLAG_AGGRESSIVE) 同步与位隔离测试
    entity/MobEntityAggressiveFlagTest.cpp
    # MobEntity dropPreservedEquipment 测试（装备掉落保留逻辑、绑定诅咒、保留/非保留装备处理）
    common/entity/core/DropPreservedEquipmentTest.cpp
    # Entity::getLootTableId() 及其子类覆写测试
    common/entity/core/GetLootTableIdTest.cpp
    # 战斗追踪器测试
    common/entity/combat/CombatTrackerTest.cpp
    # CombatRules EPF 测试
    common/entity/combat/CombatRulesEPFTest.cpp
    # 横扫攻击静止检测条件测试
    common/entity/combat/SweepAttackConditionTest.cpp
    # 疾跑击退速度修正测试
    common/entity/combat/CauseExtraKnockbackTest.cpp
    # 物品切换缩放（getItemSwapScale / itemSwapTicker）测试
    common/entity/combat/PlayerItemSwapScaleTest.cpp
    # 击退计算测试
    common/entity/combat/GetKnockbackTest.cpp
    # GameRules 游戏规则测试
    common/world/gamerule/GameRulesTest.cpp
    # PVP 游戏规则测试
    common/world/gamerule/PvpGameRuleTest.cpp
    # GameEvent 游戏事件测试
    common/world/gameevent/GameEventTest.cpp
    common/world/gameevent/GameEventSystemTest.cpp
    # GameEvent 服务端集成测试
    server/world/gameevent/GameEventServerTest.cpp
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
    # LootableContainerBlockEntity 测试（观察者模式检查、canOpen 虚函数重写、战利品上下文完善）
    common/world/blockentity/LootableContainerBlockEntityTest.cpp
    # SimpleInventory 战利品感知回调测试（验证 BarrelEntity/ChestEntity/DispenserBlockEntity 的延迟填充机制）
    common/world/blockentity/LootUnpackCallbackTest.cpp
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
    # ShulkerBoxBlock 16色潜影盒方块测试（注册、颜色、isShulkerBox、标签）
    common/world/block/blocks/ShulkerBoxBlockTest.cpp
    # CommandBlockEntity 命令方块实体测试（命令执行、红石触发、连锁执行、条件检测）
    common/world/blockentity/CommandBlockEntityTest.cpp
    # BannerEntity 旗帜方块实体测试（图案存储、底色、自定义名称、NBT序列化）
    common/world/blockentity/BannerEntityTest.cpp
    # JukeboxEntity 唱片机方块实体测试（播放/停止、比较器信号、序列化、tick）
    common/world/blockentity/JukeboxEntityTest.cpp
    # BellBlockEntity 钟方块实体测试（敲击触发、摇晃/共振状态机、方向、序列化、克隆）
    common/world/blockentity/BellBlockEntityTest.cpp
    # CopperGolemStatueBlockEntity 铜傀儡雕像方块实体测试（removeStatue、自定义名称、序列化、克隆）
    common/world/blockentity/CopperGolemStatueBlockEntityTest.cpp
    # BannerPattern 旗帜图案工具类测试（hashname、文件名、图案物品判断）
    common/world/blockentity/BannerPatternTest.cpp
    # VaultBlockEntity 宝库方块实体测试（状态机、钥匙插入、战利品弹出、玩家检测）
    common/world/blockentity/VaultBlockEntityTest.cpp
    # TrialSpawnerBlockEntity 试炼刷怪笼方块实体测试（状态机、怪物生成、奖励弹出、不祥变体）
    common/world/blockentity/TrialSpawnerBlockEntityTest.cpp
    # CrafterBlockEntity 自动合成器方块实体测试（槽位锁定、合成输入、红石信号、序列化、tick动画）
    common/world/blockentity/CrafterBlockEntityTest.cpp
    # ShelfBlockEntity 书架方块实体测试（比较器信号、物品交换、序列化、克隆）
    common/world/blockentity/ShelfBlockEntityTest.cpp
    # MobSpawnerBlockEntity 刷怪笼方块实体测试（生成逻辑、玩家检测、延迟重置、序列化、克隆）
    common/world/blockentity/MobSpawnerBlockEntityTest.cpp
    # SpawnerBlock + SpawnEggItem 刷怪笼与刷怪蛋交互测试
    common/world/block/blocks/mob/SpawnerBlockSpawnEggTest.cpp
    # DecoratedPotBlockEntity 饰纹陶罐方块实体测试
    common/world/blockentity/DecoratedPotBlockEntityTest.cpp
    # BrushableBlockEntity 可刷方块实体测试（刷扫/冷却/重置/战利品/序列化/克隆）
    common/world/blockentity/BrushableBlockEntityTest.cpp
    # SculkBlockEntity 幽匿方块实体测试
    common/world/blockentity/sculk/SculkBlockEntityTest.cpp
    common/world/block/blocks/DoorBlockTest.cpp
    common/world/block/blocks/TrapDoorBlockTest.cpp
    common/world/block/blocks/BlockSoundTest.cpp
    common/world/block/blocks/CauldronBlockTest.cpp
    common/world/block/blocks/LavaCauldronBlockTest.cpp
    common/world/block/blocks/LightningRodBlockTest.cpp
    common/world/block/blocks/LecternBlockTest.cpp
    common/world/block/blocks/MobBlocksTest.cpp
    common/world/block/DispenseItemBehaviorTest.cpp
    common/world/storage/StorageTaskTest.cpp
    common/world/storage/PlayerStorageTest.cpp
    common/world/storage/RocksDBDatabaseTest.cpp
    server/world/ServerWorldPersistenceTest.cpp
    common/world/storage/WorldNameSanitizerTest.cpp
    common/world/storage/JavaChunkReaderTest.cpp
    common/world/storage/JavaColumnReaderTest.cpp
    common/world/block/blocks/EnchantingTableBlockTest.cpp
    common/world/block/blocks/ChestBlockTest.cpp
    # ShelfBlock 书架方块测试（侧链连接、红石充能、比较器信号、旋转镜像）
    common/world/block/blocks/ShelfBlockTest.cpp
    # ShelfBlock 书架方块交互测试（onBlockActivated、heldItemTransformedTo）
    common/world/block/blocks/ShelfBlockInteractionTest.cpp
    common/world/block/blocks/decorative/PaneBlockTest.cpp
    common/world/block/blocks/decorative/DecorativeBlockTest.cpp
    common/world/block/blocks/decorative/StainedGlassBlockTest.cpp
    common/world/block/blocks/decorative/ScaffoldingBlockTest.cpp
    common/world/block/blocks/decorative/TorchBlockTest.cpp
    # CampfireBlock 营火方块测试
    common/world/block/blocks/decorative/CampfireBlockTest.cpp
    # CandleBlock 蜡烛方块测试
    common/world/block/blocks/decorative/CandleBlockTest.cpp
    common/world/block/blocks/functional/BellBlockTest.cpp
    common/world/block/blocks/functional/BedBlockTest.cpp
    common/world/block/blocks/functional/GrindstoneBlockTest.cpp
    common/world/block/blocks/functional/ComposterBlockTest.cpp
    # CandleCakeBlock 蜡烛蛋糕方块测试
    common/world/block/blocks/functional/CandleCakeBlockTest.cpp
    common/world/block/blocks/functional/AnvilBlockTest.cpp
    common/world/block/blocks/functional/CraftingTableBlockTest.cpp
    common/world/block/blocks/functional/StonecutterBlockTest.cpp
    common/world/block/blocks/functional/SmithingTableBlockTest.cpp
    common/world/block/blocks/functional/BeaconBlockTest.cpp
    # BrewingStandBlock 酿造台方块测试（onBlockPlacedBy 自定义名称传递）
    common/world/block/blocks/functional/BrewingStandBlockTest.cpp
    # DecoratedPotBlock 饰纹陶罐方块测试
    common/world/block/blocks/functional/DecoratedPotBlockTest.cpp
    # SnifferEggBlock 嗅探兽蛋方块测试（孵化逻辑、嗅探兽生成）
    common/world/block/blocks/functional/SnifferEggBlockTest.cpp
    # LayeredCauldronBlock 水炼药锅方块测试
    common/world/block/blocks/LayeredCauldronBlockTest.cpp
    common/world/block/blocks/nether/EnderChestBlockTest.cpp
    common/world/block/WaterLoggableTest.cpp
    common/world/block/blocks/HopperBlockTest.cpp
    # SignBlock 告示牌方块测试（onBlockActivated、状态属性）
    common/world/block/blocks/SignBlockTest.cpp
    # SignBlock 告示牌编辑流程核心逻辑测试（编辑者校验/涂蜡校验/文本更新）
    common/world/block/blocks/SignBlockEditTest.cpp
    common/world/block/blocks/IceBlockTest.cpp
    common/world/block/blocks/SnowBlockTest.cpp
    common/world/block/blocks/SpreadableSnowyDirtBlockTest.cpp
    common/world/block/blocks/nether/NyliumBlockTest.cpp
    common/world/block/blocks/nether/NetherWoodBlocksTest.cpp
    common/world/block/blocks/SoulFireBlockTest.cpp
    common/world/block/blocks/FireBlockCollisionTest.cpp
    common/world/block/blocks/FireBlockTest.cpp
    common/world/block/blocks/NetherPortalBlockTest.cpp
    common/world/block/blocks/BubbleColumnBlockTest.cpp
    common/world/block/blocks/AnimateTickTest.cpp
    common/world/block/blocks/FallingBlockTest.cpp
    common/world/block/blocks/ConcretePowderBlockTest.cpp
    common/world/block/blocks/VegetationBlockTest.cpp
    common/world/block/blocks/SweetBerryBushBlockTest.cpp
    common/world/block/blocks/CaveVinesBlockTest.cpp
    common/world/block/blocks/SeagrassBlockTest.cpp
    common/world/block/blocks/DoubleBlockTest.cpp
    common/world/block/UpdateFromNeighbourShapesTest.cpp
    common/world/block/blocks/PlantTypeTest.cpp
    common/world/block/blocks/MushroomBlockTest.cpp
    common/world/block/blocks/dirt/MudBlockTest.cpp
    common/world/block/blocks/dirt/DirtPathBlockTest.cpp
    common/world/block/blocks/cave/GlowLichenBlockTest.cpp
    common/world/block/blocks/cave/BigDripleafBlockTest.cpp
    common/world/block/blocks/cave/BigDripleafStemBlockTest.cpp
    common/world/block/blocks/cave/PointedDripstoneBlockTest.cpp
    common/world/block/blocks/pale_garden/PaleHangingMossBlockTest.cpp
    common/world/block/blocks/pale_garden/EyeblossomBlockTest.cpp
    common/world/block/blocks/garden/GardenFlowerTest.cpp
    common/world/block/blocks/garden/FlowerBedBlockTest.cpp
    common/world/block/blocks/end/ChorusPlantBlockTest.cpp
    common/world/block/blocks/end/DragonEggBlockTest.cpp
    common/world/block/BlockTagsTest.cpp
    common/world/block/SupportTypeTest.cpp
    common/world/block/IGrowableTest.cpp
    common/world/gameevent/VibrationSystemTest.cpp
    # VibrationSystem 序列化测试
    common/world/gameevent/VibrationSystemSerializationTest.cpp
    # BlockEvent 方块事件测试
    common/world/blockevent/BlockEventTest.cpp
    # SculkSensorBlock 幽匿感测体方块测试
    common/world/block/blocks/sculk/SculkSensorBlockTest.cpp
    common/entity/EntityMotionMultiplierTest.cpp
    common/world/block/blocks/CropBlocksTest.cpp
    common/world/block/blocks/TorchflowerCropTest.cpp
    common/world/block/blocks/PitcherCropBlockTest.cpp
    common/world/block/blocks/StemBlocksTest.cpp
    common/world/block/blocks/StemGrowthLogicTest.cpp
    common/world/block/blocks/TryGrowFruitTest.cpp
    common/world/block/blocks/GolemBlockTest.cpp
    common/world/block/blocks/PumpkinCarveTest.cpp
    common/world/block/blocks/RespawnAnchorBlockTest.cpp
    common/world/block/blocks/SpongeBlockTest.cpp
    common/world/block/blocks/OxidizableBlockTest.cpp
    common/world/block/blocks/copper/CopperBarsBlockTest.cpp
    common/world/block/blocks/copper/CopperGolemStatueBlockTest.cpp
    common/world/block/blocks/copper/CopperChestBlockTest.cpp
    common/world/block/CompostableItemsTest.cpp
    common/world/block/BlockStateMutableTest.cpp
    common/world/block/BuildingVariantRegistrationTest.cpp
    common/world/block/BlockPushEntitiesUpTest.cpp
    common/world/block/state/pattern/BlockPatternTest.cpp
    common/world/CanSeeSkyTest.cpp
    common/world/IWorldContainsAnyLiquidTest.cpp
    common/world/EndTeleporterTest.cpp
    common/world/dimension/EndDragonFightTest.cpp
    common/world/dimension/EndDragonFightBossBarTest.cpp
    common/world/dimension/EndDragonFightSpawnTest.cpp
    common/world/dimension/EndDragonFightRespawnTest.cpp
    common/world/dimension/EndDragonFightFindPortalTest.cpp
    common/command/test_command_dispatcher.cpp
    entity/EntityCoreTests.cpp
    entity/EntityTagsTests.cpp
    entity/FlyingEntityTests.cpp
    entity/EntitySensesTest.cpp
    entity/RidingSystemTests.cpp
    # 骑乘循环检测测试
    entity/RidingCycleDetectionTests.cpp
    # 骑乘循环集成测试
    entity/RidingCycleIntegrationTests.cpp
    entity/PassengerSystemTests.cpp
    entity/StriderEntityTest.cpp
    entity/PigEntityTest.cpp
    # 鹦鹉螺 NBT 序列化/气泡粒子/装备查询测试
    entity/NautilusEntityTest.cpp
    entity/FlingingSupportTypesTest.cpp
    entity/AttributeTests.cpp
    # Player LUCK 属性测试
    entity/PlayerLuckAttributeTest.cpp
    entity/LivingEntityTests.cpp
    # LivingEntity::swing(Hand) 服务端广播 EntityAnimationPacket 测试
    entity/LivingEntitySwingBroadcastTest.cpp
    entity/HurtMarkedTest.cpp
    entity/AnimalSoundTests.cpp
    entity/GoalTests.cpp
    entity/EatGrassGoalTest.cpp
    entity/CreeperSwellGoalTest.cpp
    entity/AreaEffectCloudEntityTest.cpp
    entity/BlazeFireballAttackGoalTest.cpp
    entity/BlazeHoverBehaviorTest.cpp
    # 旋风人动画状态机测试（Pose 转换、resetTask 守卫、AnimationState 访问器）
    entity/BreezeEntityAnimationTest.cpp
    # BreezeEntity::shootWindCharge 目标 Y 坐标计算测试（partialY 0.3/0.8、发射点 Y、冷却与边界）
    entity/BreezeShootWindChargeTest.cpp
    # 火球类投射物测试（伤害、爆炸、区域效果云）
    entity/FireballEntityTest.cpp
    # 风弹实体风爆效果测试（玩家击退收集、旁观/创造飞行过滤、broadcastExplosion 调用）
    entity/WindChargeEntityBurstTest.cpp
    # DamagingProjectileEntity 粒子效果测试（水下气泡、拖尾粒子）
    entity/DamagingProjectileParticleTest.cpp
    # LookAtGoal 类型过滤测试
    common/entity/ai/goal/LookAtGoalTest.cpp
    # FishSwimGoal 鱼类游泳目标测试
    common/entity/ai/goal/FishSwimGoalTest.cpp
    # FleeSunGoal/RestrictSunGoal/FindShelterGoal/FlyGoal 阳光和飞行目标测试
    common/entity/ai/goal/SunAndFlyGoalsTest.cpp
    # AvoidBlockGoal 方块回避目标测试
    common/entity/ai/goal/AvoidBlockGoalTest.cpp
    # MoveTowardsRestrictionGoal 向家范围移动目标测试
    common/entity/ai/goal/MoveTowardsRestrictionGoalTest.cpp
    entity/GhastEntityTest.cpp
    # 恶魂 AI 目标测试
    common/entity/GhastGoalsTest.cpp
    entity/GuardianAttackGoalTest.cpp
    # 猪灵类实体测试（金装备检测、AI 目标）
    entity/PlayerGoldArmorTest.cpp
    entity/NetherEntitiesTest.cpp
    # PatrolGoal 测试
    common/entity/ai/goal/PatrolGoalTest.cpp
    common/entity/entities/boss/WitherEntityTest.cpp
    # WitherEntity 侧头朝向计算测试（_updateSideHeadRotations/rotlerp 限速/无目标回退/bodyRot 插值）
    common/entity/entities/boss/WitherEntitySideHeadTest.cpp
    common/entity/entities/boss/WardenEntityTest.cpp
    # EnderDragonEntity 末影龙测试
    entity/EnderDragonEntityTest.cpp
    # EndermanEntity 受伤后瞬移逻辑测试
    entity/EndermanHurtTeleportTest.cpp
    # EndermanEntity AI 目标测试（ResetAngerGoal, WaterAvoidingRandomWalkingGoal）
    entity/EndermanAIGoalsTest.cpp
    entity/EvokerGoalsTest.cpp
    # EvokerFangsEntity 队伍伤害检查测试
    entity/EvokerFangsTest.cpp
    entity/VexGoalsTest.cpp
    # IllusionerGoals 幻术师法术常量和 AI 优先级测试
    entity/IllusionerGoalsTest.cpp
    entity/BrainScheduleTests.cpp
    entity/BrainMovementTaskTests.cpp
    entity/BrainInteractTaskTests.cpp
    # BrainActionTask 行为任务测试
    entity/BrainActionTaskTests.cpp
    # BrainActionTask 新行为任务测试
    entity/BrainActionTaskNewTests.cpp
    entity/SensorTests.cpp
    entity/DamagingProjectileSupportTypesTest.cpp
    entity/FishSupportTypesTest.cpp
    entity/HorseAppearanceSupportTypesTest.cpp
    entity/HorseSupportTypesTest.cpp
    entity/HorseTamingTest.cpp
    entity/HorseBreedingTest.cpp
    entity/HorseJumpBoostTest.cpp
    entity/HorseAiGoalsTest.cpp
    # AbstractHorse 动画计数器、扬蹄条件、Owner UUID、NBT 序列化测试
    entity/AbstractHorseEntityTest.cpp
    common/entity/entities/passive/horse/HorseCanEquipTest.cpp
    # 马类交互逻辑测试（interactMob、equipArmor、equipChest、doPlayerRide）
    common/entity/entities/passive/horse/HorseInteractTest.cpp
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
    common/entity/player/PlayerPermissionTest.cpp
    # Player 建造权限测试（mayBuild/mayUseItemAt/blockActionRestricted）
    common/entity/player/PlayerBuildPermissionTest.cpp
    common/entity/player/PlayerSleepTest.cpp
    # Player 旁观者摄像机跟踪测试
    common/entity/player/PlayerSpectatorTest.cpp
    # Player 冲量坠落伤害免疫测试
    common/entity/player/ImpulseContextTest.cpp
    # Player NBT 序列化测试
    common/entity/player/PlayerSerializationTest.cpp
    # Player 交互距离属性测试（方块/实体交互范围、创造模式修饰符）
    common/entity/player/PlayerInteractionRangeTest.cpp
    entity/ProjectileHelperTest.cpp
    # ThrowableEntity 传送门处理测试（下界传送门、末地折跃门）
    entity/ThrowablePortalTest.cpp
    # FishingBobberEntity 钩住实体测试
    common/entity/entities/projectile/FishingBobberHookEntityTest.cpp
    # FishingBobberEntity 开放水域检测测试
    common/entity/entities/projectile/OpenWaterCheckTest.cpp
    # ProjectileEntity canHitEntity 骑乘同载具测试
    common/entity/entities/projectile/ProjectileCanHitEntityTest.cpp
    # ProjectileDeflection 弹射物偏转系统测试
    common/entity/entities/projectile/ProjectileDeflectionTest.cpp
    # ProjectileEntity::shoot 负 inaccuracy 行为测试（旋风人风弹散布修复）
    common/entity/entities/projectile/ProjectileShootInaccuracyTest.cpp
    # FireworkRocketEntity 爆炸伤害测试
    common/entity/entities/projectile/FireworkRocketExplosionTest.cpp
    # DragonFireballEntity 龙息云和粒子类型测试
    common/entity/entities/projectile/DragonFireballCloudTest.cpp
    # ShulkerBulletEntity 爆炸粒子效果测试
    common/entity/entities/projectile/ShulkerBulletParticleTest.cpp
    # TridentEntity 忠诚附魔逻辑测试
    common/entity/entities/projectile/TridentEntityTest.cpp
    entity/SkeletonSupportTypesTest.cpp
    entity/SpellcastingIllagerSupportTypesTest.cpp
    entity/SpawnLocationHelperTest.cpp
    entity/ShoulderRidingSupportTypesTest.cpp
    # 鹦鹉 AI 目标测试（LandOnOwnersShoulderGoal, WaterAvoidingRandomFlyingGoal, FollowMobGoal）
    entity/ParrotGoalsTest.cpp
    entity/TraderLlamaSupportTypesTest.cpp
    # 商队羊驼集成测试（拴绳/流浪商人/消失/乘客交互）
    entity/TraderLlamaIntegrationTest.cpp
    # LlamaFollowCaravanGoal 商队跟随目标测试
    entity/LlamaFollowCaravanGoalTest.cpp
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
    # AxolotlGoals 美西螈 AI 目标测试
    common/entity/ai/goal/goals/special/AxolotlGoalsTest.cpp
    # DolphinGoals 海豚 AI 目标测试
    common/entity/ai/goal/goals/special/DolphinGoalsTest.cpp
    # DrownedGoals 溺尸 AI 目标测试
    common/entity/ai/goal/goals/special/DrownedGoalsTest.cpp
    # WanderingTraderGoals 流浪商人 AI 目标测试
    common/entity/ai/goal/goals/special/WanderingTraderGoalsTest.cpp
    # OfferFlowerGift 铁傀儡赠花核心逻辑测试
    common/entity/ai/goal/goals/special/OfferFlowerGiftTest.cpp
    # Sensors 传感器测试（VillagerHostilesSensor、AvoidEntitySensor）
    common/entity/ai/brain/sensor/SensorsTest.cpp
    # RandomPositionGenerator 随机位置生成器测试
    common/entity/ai/util/RandomPositionGeneratorTest.cpp
    # PiglinAi 猪灵AI工具类测试
    common/entity/ai/util/PiglinAiTest.cpp
    # PiglinAi::isWearingGold 金盔甲判定测试
    common/entity/ai/util/PiglinAiIsWearingGoldTest.cpp
    # BreakDoorGoal 破门目标测试
    common/entity/ai/goal/goals/interact/BreakDoorGoalTest.cpp
    # OpenDoorGoal 开门目标测试
    common/entity/ai/goal/goals/interact/OpenDoorGoalTest.cpp
    # HurtByTargetGoal 被攻击反击目标测试
    common/entity/ai/goal/goals/target/HurtByTargetGoalTest.cpp
    # TargetGoal 视线记忆（setUnseenMemoryTicks）测试
    common/entity/ai/goal/goals/target/TargetGoalUnseenMemoryTest.cpp
    # DrownedEntity registerGoals 目标替换测试
    common/entity/ai/goal/goals/target/DrownedRegisterGoalsTest.cpp
    # GoalSelector 目标选择器测试（removeGoalsOfType 等）
    common/entity/ai/goal/GoalSelectorTest.cpp
    entity/AiGoalRegressionTest.cpp
    entity/PathWeightTest.cpp
    entity/PathWeightOverrideTest.cpp
    entity/MinecartTests.cpp
    entity/ElytraAndSpinAttackTest.cpp
    common/entity/entities/EntityDaylightTest.cpp
    common/entity/WaterSplashTest.cpp
    common/SettingsTest.cpp
    server/ServerWorldTest.cpp
    server/ServerWorldBlockUpdateCallbackTest.cpp
    server/test_integrated_server.cpp
    server/test_save_player_runtime_state.cpp
    server/BlockDropHandlerTest.cpp
    server/BlockInteractionManagerTest.cpp
    server/interaction/SignCommandHelperTest.cpp
    server/interaction/MiningManagerCallbackTest.cpp
    server/MiningManagerTest.cpp
    server/menu/CraftingMenuTest.cpp
    server/BlockUpdateSyncManagerTest.cpp
    # test_chunk_worker_pool 已删除（ChunkGenerateTask 被 ChunkProgressionTask + ChunkTaskScheduler 取代）
    # GenerationChunkCache 已删除（被 StaticChunkCache2D + ChunkTaskScheduler 取代）
    # ChunkSnapshot 已删除（可变 ChunkPrimer 模型取代不可变快照）
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
    server/core/PermissionResolverTest.cpp
    # PacketHandler 测试（载具输入、位置验证、实体交互、实体动作）
    server/core/PacketHandlerTest.cpp
    # PacketHandler 载具校正辅助函数单元测试
    server/core/PacketHandlerMoveVehicleHelpersTest.cpp
    # PacketHandler handleMoveVehicle 集成测试（moved wrongly / 回退 / 正常接受）
    server/core/PacketHandlerMoveVehicleIntegrationTest.cpp
    # ServerPlayer 消息发送测试
    server/player/ServerPlayerMessageTest.cpp
    # ServerPlayer PvP 保护机制测试
    server/player/ServerPlayerPvpTest.cpp
    # 维度管理器测试
    server/dimension/ServerDimensionManagerTest.cpp
    server/weather/WeatherManagerTest.cpp
    server/world/spawn/DespawnManagerTest.cpp
    # NaturalSpawner 实体分类与容量上限对齐测试（修复实体无限累积）
    server/world/spawn/NaturalSpawnerClassificationTest.cpp
    server/world/spawn/NaturalSpawnerCapTest.cpp
    server/command/CommandRegistryTest.cpp
    server/command/LocateCommandTest.cpp
    server/command/SetBlockCommandTest.cpp
    server/command/CloneCommandTest.cpp
    server/command/GiveCommandTest.cpp
    server/command/ReplaceItemCommandTest.cpp
    server/command/LootCommandTest.cpp
    server/command/WhitelistBanCommandTest.cpp
    server/command/PlayerResolverTest.cpp
    server/command/EntityResolverTest.cpp
    server/command/MessageCommandTest.cpp
    server/command/TellRawCommandTest.cpp
    server/command/ExperienceCommandTest.cpp
    server/command/AttributeCommandTest.cpp
    server/command/EnchantCommandTest.cpp
    server/command/SpawnPointCommandTest.cpp
    server/command/SetWorldSpawnCommandTest.cpp
    server/command/PublishCommandTest.cpp
    server/command/StopSoundCommandTest.cpp
    server/command/SpreadPlayersCommandTest.cpp
    server/command/SpreadAlgorithmTest.cpp
    server/command/ExecuteCommandTest.cpp
    server/world/player/ServerPlayerEntityManagerTest.cpp
    # SculkShriekerHelper 幽匿尖啸体辅助函数测试
    server/world/blockentity/sculk/SculkShriekerHelperTest.cpp
    client/world/player/LocalPlayerIdentityTest.cpp
    client/world/player/PlayerIdentityRegistryTest.cpp
    client/world/player/ClientPlayerPredictorTest.cpp
    client/world/entity/ClientEntityTest.cpp
    # ClientEntity 骷髅拉弓状态元数据同步测试（skeleton/stray/bogged/wither_skeleton）
    client/world/entity/ClientEntitySkeletonSyncTest.cpp
    # ClientEntity Mob 激怒状态元数据同步测试（zombie/husk/drowned/zombie_villager/skeleton/creeper）
    client/world/entity/ClientEntityAggressiveSyncTest.cpp
    # ClientEntity 凋灵侧头朝向客户端镜像计算测试（tickWitherSideHeads/rotlerp 限速/插值/元数据同步）
    client/world/entity/ClientEntityWitherSideHeadTest.cpp
    # ClientEntity 钓鱼浮标元数据同步端到端测试（DATA_HOOKED_ENTITY/DATA_BITING 序列化→反序列化→镜像）
    client/world/entity/ClientEntityFishingBobberSyncTest.cpp
    client/entity/ClientEntityAnimationTest.cpp
    client/world/ClientWorldLightUpdateTest.cpp
    # ClientWorld 区块清空测试（维度切换时使用）
    client/world/ClientWorldClearChunksTest.cpp
    # ClientWorld 方块实体客户端存储测试（onBlockEntityData / getBlockEntity / removeBlockEntity / clearBlockEntities）
    client/world/ClientWorldBlockEntityTest.cpp
    # ClientWorld 天空可见性和高度图查询测试
    client/world/ClientWorldSkyAndHeightmapTest.cpp
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
    # BipedModel 弩装填/持握动画测试
    client/renderer/entity/BipedModelCrossbowTest.cpp
    # BipedModel Spyglass/Brush 手臂姿态角度测试
    client/renderer/entity/BipedModelArmPoseTest.cpp
    # SkeletonModel 手臂姿态委托基类 BipedModel 测试（BowAndArrow/CrossbowCharge/CrossbowHold/isAggressive）
    client/renderer/entity/SkeletonModelArmPoseTest.cpp
    # ZombieModel::setAngles 激怒状态手臂动画测试（aggressive=true/false、swingProgress 攻击因子、bobArms、变体继承）
    client/renderer/entity/ZombieModelArmPoseTest.cpp
    # DrownedModel 游泳动画覆盖测试（swimAmount 渐入/手臂/腿部/头部覆盖、ThrowSpear 重应用）
    client/renderer/entity/DrownedModelSwimAnimationTest.cpp
    # WitherModel 侧头朝向注入测试（setSideHeadRotations/setAngles 度→弧度/主头不受影响/两侧头独立）
    client/renderer/entity/WitherModelSideHeadTest.cpp
    # EntityRendererManager::_applySkeletonArmPose 契约测试（isChargingBow → BowAndArrow/Empty）
    client/renderer/entity/ApplySkeletonArmPoseTest.cpp
    # EntityRendererManager::_applyDrownedTridentPose 契约测试（isAggressive → ThrowSpear/Empty）
    client/renderer/entity/ApplyDrownedTridentPoseTest.cpp
    # BipedModel 鞘翅飞行速度因子测试
    client/renderer/entity/BipedModelSpeedValueTest.cpp
    # elytra::computeSpeedValue 纯逻辑公式测试（_applyBipedElytraState 单元测试）
    client/renderer/entity/ElytraSpeedValueTest.cpp
    # dynamic_cast<BipedModel*> 安全性测试（验证非 BipedModel 派生模型不会误触发）
    client/renderer/entity/BipedModelDynamicCastTest.cpp
    # PlayerModel 手臂渲染测试
    client/renderer/entity/PlayerModelTest.cpp
    # PlayerArmPoseResolver 手臂姿态解析器测试
    client/renderer/entity/PlayerArmPoseResolverTest.cpp
    client/renderer/entity/ArmorLayerVisibilityTest.cpp
    client/renderer/entity/LlamaModelTest.cpp
    client/renderer/entity/LlamaRendererTest.cpp
    # RabbitModel 兔子模型跳跃角度测试（setJumpRotation 对 thigh/foot/arm 旋转角度的影响）
    client/renderer/entity/RabbitModelTest.cpp
    # DolphinModel 海豚模型运动状态注入测试（setMotionMagnitude + 摆尾动画 cos 公式）
    client/renderer/entity/DolphinModelTest.cpp
    # Fullbright 渲染器 isFullbright() 测试
    client/renderer/entity/FullbrightRendererTest.cpp
    # WitchModel 女巫模型测试
    client/renderer/entity/WitchModelTest.cpp
    client/renderer/entity/test_entity_renderer_manager.cpp
    client/renderer/entity/ShadowRendererTest.cpp
    client/renderer/entity/WorldTextRendererTest.cpp
    # HeldBlockLayer 末影人手持方块渲染层测试
    client/renderer/trident/entity/layer/entity/HeldBlockLayerTest.cpp
    # VillagerLayer 村民多层纹理渲染层测试
    client/renderer/trident/entity/layer/entity/VillagerLayerTest.cpp
    # WolfCollarLayer 狼项圈渲染层测试（shouldRender + 颜色映射）
    client/renderer/trident/entity/layer/entity/WolfCollarLayerTest.cpp
    # EnergyGlintLayer 附魔光效渲染层测试
    client/renderer/trident/entity/layer/effect/EnergyGlintLayerTest.cpp
    # EntityPipeline BlendMode 枚举契约测试（Multiply/None 专用管线）
    client/renderer/trident/entity/pipeline/EntityPipelineBlendModeTest.cpp
    # 载具渲染器模型矩阵构建与 TNT 闪烁计算测试
    client/renderer/trident/entity/renderer/vehicle/VehicleRendererMatrixTest.cpp
    # 特殊实体渲染器（TNT/FallingBlock）模型矩阵与闪烁测试
    client/renderer/trident/entity/renderer/special/SpecialRendererMatrixTest.cpp
    # BlockMeshBuilder 方块网格构建工具测试
    client/renderer/trident/entity/util/BlockMeshBuilderTest.cpp
    # PlayerRenderer 层渲染器测试（HeadLayer、CapeLayer、ElytraLayer）
    client/renderer/entity/test_player_layers.cpp
    # ItemMeshBuilder 回退网格、法线变换、纹理坐标解析测试
    client/renderer/trident/item/ItemMeshBuilderTest.cpp
    client/renderer/test_gui_item_color.cpp
    client/resource/test_entity_texture_loader.cpp
    client/resource/ItemTextureAtlasTest.cpp
    # 统一图集数据驱动（AtlasSource 体系 + Loader）
    client/resource/atlas/AtlasSourceTest.cpp
    client/resource/atlas/AtlasConfigLoaderTest.cpp
    client/resource/TextureAtlasBuilderTest.cpp
    client/resource/AnimationTextureTest.cpp
    client/resource/test_resource_manager_cloud_texture.cpp
    network/EntityPacketsTest.cpp
    network/ParticlePacketTest.cpp
    network/ParticlePacketVibrationTest.cpp
    # DustParticleData 粒子数据包测试
    network/ParticlePacketDustTest.cpp
    # TrailParticleData 粒子数据包测试
    network/ParticlePacketTrailTest.cpp
    # EntityEffect 粒子数据包测试
    network/ParticlePacketEntityEffectTest.cpp
    # Block 方块粒子数据包测试（携带方块状态 ID）
    network/ParticlePacketBlockTest.cpp
    # Item 物品粒子数据包测试（携带物品堆）
    network/ParticlePacketItemTest.cpp
    network/ExplosionPacketTest.cpp
    network/WorldEventPacketTest.cpp
    network/SpawnPositionPacketTest.cpp
    network/DimensionPacketsTest.cpp
    network/LocalServerConnectionTest.cpp
    network/TitlePacketTest.cpp
    network/BossInfoPacketTest.cpp
    # 旁观者摄像机同步包测试
    network/SetCameraPacketTest.cpp
    # 告示牌网络包序列化/反序列化测试
    network/SignPacketsTest.cpp
    # 方块实体数据同步包序列化/反序列化测试（含 NBT 往返、错误路径）
    network/BlockEntityDataPacketTest.cpp
    common/entity/loot/LootTest.cpp
    common/entity/loot/LootConditionTest.cpp
    common/entity/loot/MatchToolConditionTest.cpp
    common/entity/loot/LootSerializersTest.cpp
    common/entity/loot/LootTableLoaderTest.cpp
    common/entity/loot/LootJsonIntegrationTest.cpp
    # SetStewEffectFunction 测试
    common/entity/loot/SetStewEffectTest.cpp
    # LootPredicateManager 和 LootPredicateLoader 测试
    common/entity/loot/LootPredicateManagerTest.cpp
    common/entity/loot/LootPredicateLoaderTest.cpp
    # ReferenceCondition 谓词引用条件测试
    common/entity/loot/ReferenceConditionTest.cpp
    # NBT选择器过滤和谓词条件过滤测试
    common/entity/loot/NBTSelectorFilterTest.cpp
    # LootParameterSet 参数验证测试
    common/entity/loot/LootParameterSetValidationTest.cpp
    # BlockPredicate 和 StatePropertiesPredicate 测试
    common/advancement/BlockPredicateTest.cpp
    # AdvancementVisibilityEvaluator 成就可见性评估器测试
    common/advancement/AdvancementVisibilityEvaluatorTest.cpp
    # AdvancementLoader 成就加载器测试
    common/advancement/AdvancementLoaderTest.cpp
    # EffectType 工具函数测试
    common/entity/effect/EffectTypeTest.cpp
    # AnimationState 动画状态机工具类测试（start/stop/startIfStopped/animateWhen/copyFrom）
    common/entity/AnimationStateTest.cpp
    # EffectAttributeModifiers 效果属性修改器测试
    common/entity/effect/EffectAttributeModifiersTest.cpp
    # EffectManager 效果合并行为测试
    common/entity/effect/EffectManagerMergeTest.cpp
    # EffectInstance endsWithin 到期查询测试
    common/entity/effect/EffectInstanceEndsWithinTest.cpp
    # 饱和效果测试
    common/entity/effect/SaturationEffectTest.cpp
    common/world/gen/WorldGenSpawnerTest.cpp
    common/world/gen/OceanFeatureTest.cpp
    common/world/gen/WaterContentRegistryTest.cpp
    common/world/gen/NoiseSurfaceParityTest.cpp
    common/world/gen/NoiseChunkGeneratorCarverParityTest.cpp
    common/world/gen/NetherSurfaceParityTest.cpp
    # 噪声生成器单元测试
    common/world/gen/PerlinNoiseTest.cpp
    common/world/gen/NormalNoiseTest.cpp
    common/world/gen/SimplexNoiseTest.cpp
    common/world/gen/PerlinSimplexNoiseTest.cpp
    # Noises 注册表单元测试
    common/world/gen/NoisesTest.cpp
    # NoiseLoader 数据驱动测试（加载原版数据包验证注入路径）
    common/world/gen/NoiseLoaderDatadrivenTest.cpp
    # DensityFunctionLoader 数据驱动测试（加载原版数据包验证 35 DF Holder 解析）
    common/world/gen/DensityFunctionLoaderDatadrivenTest.cpp
    # JavaLegacyRandom 单元测试
    common/world/gen/JavaLegacyRandomTest.cpp
    # Xoroshiro128ppRandom 单元测试
    common/world/gen/Xoroshiro128ppRandomTest.cpp
    # PositionalRandomFactory 单元测试
    common/world/gen/PositionalRandomFactoryTest.cpp
    # BiomeManager 和 LinearCongruentialGenerator 单元测试
    common/world/gen/BiomeManagerTest.cpp
    # Beardifier (BEARD_KERNEL + affectedBox) 和 CarvingContext 扩展单元测试
    common/world/gen/BeardifierCarvingContextTest.cpp
    # DensityFunctions (Invert, Mul 边界值) 噪声叶子/样条/缓存单元测试
    common/world/gen/density/DensityFunctionsTest.cpp
    # FindTopSurface 密度函数单元测试（preliminarySurfaceLevel 核心）
    common/world/gen/density/FindTopSurfaceTest.cpp
    common/world/gen/feature/FeatureSorterTest.cpp
    common/world/gen/feature/MonsterRoomFeatureTest.cpp
    common/world/gen/feature/RandomSelectorFeatureTest.cpp
    common/world/gen/feature/RandomPatchFeatureTest.cpp
    common/world/gen/feature/SimpleBlockFeatureTest.cpp
    common/world/gen/feature/SimpleRandomSelectorFeatureTest.cpp
    common/world/gen/feature/DiskAndPileFeatureTest.cpp
    common/world/gen/feature/ScatteredOreAndReplaceBlobsTest.cpp
    common/world/gen/feature/EnvironmentScanAndRuleBasedProviderTest.cpp
    common/world/gen/placement/RandomOffsetAndWaterDepthTest.cpp
    common/world/gen/placement/CountOnEveryLayerAndNoisePlacementTest.cpp
    common/world/gen/feature/predicate/MatchingPredicatesTest.cpp
    common/world/gen/feature/SnowAndFreezeFeatureTest.cpp
    common/world/gen/feature/end/EndFeatureTest.cpp
    common/world/gen/feature/template/test_template.cpp
    common/world/gen/feature/template/TemplateEntityPlacementTest.cpp
    common/world/gen/feature/template/BlockAgeProcessorTest.cpp
    common/world/gen/feature/template/test_capped_processor.cpp
    common/world/gen/feature/template/ProtectedBlocksProcessorTest.cpp
    common/world/gen/FeaturePlacerTest.cpp
    common/world/gen/valueprovider/test_int_provider_parser.cpp
    common/world/gen/structure/test_ocean_ruin_structure.cpp
    common/world/gen/structure/test_shipwreck_structure.cpp
    common/world/gen/structure/test_new_structures.cpp
    common/world/gen/structure/test_mansion_grid.cpp
    common/world/gen/structure/test_structure_type_mapping.cpp
    common/world/gen/structure/test_structure_definition_loader.cpp
    common/world/gen/structure/test_structure_type_registry.cpp
    common/world/gen/structure/test_structure_set_loader.cpp
    common/world/gen/structure/StructureCheckTest.cpp
    common/world/gen/structure/StructurePieceChestDispenserTest.cpp
    common/world/gen/structure/PlaceEndPortalFramesTest.cpp
    common/world/gen/structure/StrongholdPiecesTest.cpp
    common/world/gen/structure/TemplatedStructuresTest.cpp
    common/world/gen/structure/RuinedPortalStructureTest.cpp
    common/world/gen/structure/StructureTagsTest.cpp
    common/world/gen/jigsaw/test_jigsaw.cpp
    common/world/gen/jigsaw/test_jigsaw_matcher.cpp
    common/world/gen/jigsaw/test_pos_rule_test.cpp
    common/world/gen/jigsaw/test_template_pool_loader.cpp
    common/entity/EntitySpawnPlacementRegistryTest.cpp
    common/world/gen/ChunkSpawnIntegrationTest.cpp
    # Perfetto 娴嬭瘯锛堥泦鎴愬埌涓绘祴璇曞浠讹級
    common/profiler/PerfettoTest.cpp
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
    common/world/block/InfestedBlockTest.cpp
    common/world/block/blocks/building/StairsBlockTest.cpp
    common/world/block/blocks/building/FenceBlockTest.cpp
    common/world/block/blocks/building/WallBlockTest.cpp
    common/world/block/blocks/redstone/NoteBlockTest.cpp
    common/world/block/blocks/redstone/TNTBlockTest.cpp
    common/world/block/blocks/redstone/PistonBlockTest.cpp
    common/world/block/blocks/redstone/PistonHeadBlockTest.cpp
    common/world/block/blocks/redstone/TripWireTest.cpp
    # 海绵方块和 Block::dropResources 测试
    common/world/block/blocks/special/SpongeBlockTest.cpp
    # 红石方块形状测试
    common/world/block/blocks/redstone/RedstoneWireBlockTest.cpp
    common/world/block/blocks/redstone/TripWireBlockTest.cpp
    common/world/block/blocks/redstone/TripwireHookBlockTest.cpp
    common/world/block/blocks/redstone/AbstractPressurePlateBlockTest.cpp
    common/world/block/blocks/redstone/DetectorRailBlockTest.cpp
    common/world/block/blocks/redstone/RailStateTest.cpp
    common/world/block/blocks/redstone/RailBlockWaterlogTest.cpp
    # Explosion system tests
    common/world/explosion/test_explosion.cpp
    common/world/explosion/ExplosionIntegrationTest.cpp
    common/world/explosion/ExplosionPlayerBranchTest.cpp
    common/world/explosion/WitherSkullExplosionContextTest.cpp
    # 灏勭嚎妫€娴嬫祴璇?    common/math/RaycastTest.cpp
    # 视锥剔除测试
    common/util/math/FrustumTest.cpp
    # 瀹炰綋鐢熸垚鍥炶皟娴嬭瘯
    server/ServerChunkManagerCallbackTest.cpp
    # ServerChunkManager 后处理去重测试（m_postProcessedChunks / isPostProcessingDone）
    server/ServerChunkManagerPostProcessTest.cpp
    # StructureReferences 并发竞争测试
    server/StructureReferencesRaceTest.cpp
    common/world/EntityManagerSpawnTest.cpp
    # EntityManager UUID 索引测试
    common/world/EntityManagerUuidTest.cpp
    # 纰版挒缂撳瓨娴嬭瘯
    physics/CollisionCacheTests.cpp
    # 物理引擎测试
    physics/PhysicsEngineTests.cpp
    # DiscreteVoxelShape 测试
    physics/DiscreteVoxelShapeTest.cpp
    # CollisionShape 测试
    physics/CollisionShapeTest.cpp
    # VoxelShape 布尔运算和 IndexMerger 测试
    physics/VoxelShapeJoinTest.cpp
    # ServerWorld 纰版挒妫€娴嬫祴璇?    server/ServerWorldCollisionTests.cpp
    # 瀹炰綋杩借釜鍣ㄦ祴璇?    server/world/EntityTrackerTest.cpp
    # EntityTracker UUID 测试
    server/world/EntityTrackerUuidTest.cpp
    # 鐗╁搧鎷惧彇绠＄悊鍣ㄦ祴璇?    server/world/ItemPickupManagerTest.cpp
    # ServerWorld 方块实体测试
    server/world/ServerWorldBlockEntityTest.cpp
    # ServerWorld::broadcastBlockEntity 广播回调测试
    server/world/ServerWorldBroadcastBlockEntityTest.cpp
    # ServerWorld 出生点朝向测试
    server/world/ServerWorldSpawnAngleTest.cpp
    # ServerWorld initializeWorldSpawn 气候搜索集成测试
    server/world/ServerWorldInitializeWorldSpawnTest.cpp
    # ServerWorld 命令执行回调测试
    server/world/ServerWorldCommandExecuteTest.cpp
    # ServerWorld 爆炸广播回调测试（broadcastExplosion 委托 m_onBroadcastExplosion）
    server/world/ServerWorldExplosionCallbackTest.cpp
    # ServerWorld tickPrecipitation 测试
    server/world/TickPrecipitationTest.cpp
    # isBlockInLine 方块射线遍历测试
    server/world/IsBlockInLineTest.cpp
    # StaticChunkCache2D 预分配二维区块缓存测试
    server/world/StaticChunkCache2DTest.cpp
    # 璧勬簮鍖呭吋瀹瑰眰娴嬭瘯
    # 鍐呯疆妯″瀷鐫€鑹查厤缃祴璇?    common/resource/VanillaResourcesTintModelTest.cpp
    # ResourcePackList 自包含测试（不依赖外部资源路径）
    common/resource/PackRepositorySelfContainedTest.cpp
    # 语言管理器测试
    common/resource/LanguageManagerTest.cpp
    # 缂撳瓨鎬ц兘瀵规瘮娴嬭瘯
    common/util/cache/CacheBenchmark.cpp
    # 妞嶈鐗瑰緛闆嗘垚娴嬭瘯
    # 涓栫晫鐢熸垚纭畾鎬ф祴璇?    common/world/gen/WorldGenDeterminismTest.cpp
    # 瀹㈡埛绔懡浠ゆ爲鍚屾涓庤ˉ鍏ㄦ祴璇?    client/command/ClientCommandManagerTest.cpp
    # 鍏夌収绯荤粺娴嬭瘯
    lighting/LightingTest.cpp
    lighting/LightUpdateTest.cpp
    lighting/BlockLightRegressionTest.cpp
    lighting/BaseLightEngineQueueTest.cpp
    lighting/SkyLightRegressionTest.cpp
    lighting/WorldLightManagerTest.cpp
    # 主线程读路径 visible 侧回归测试（③-2a）
    lighting/MainThreadVisibleReadTest.cpp
    # 运行时方块变更光照延迟队列批处理测试
    lighting/ServerLightQueueTest.cpp
    # 区块加载光照 worker 任务测试（③-2b：搬 worker + send 续延）
    lighting/ChunkLoadLightTaskTest.cpp
    # 运行时光照 worker 传播 + 主线程 flush 并发测试（③-1）
    lighting/RuntimeLightConcurrencyTest.cpp
    # 鐜鍏夐伄钄芥祴璇?    client/renderer/AmbientOcclusionCalculatorTest.cpp
    client/renderer/trident/blockentity/ChestModelTest.cpp
    client/renderer/trident/blockentity/BeaconBeamModelTest.cpp
    client/renderer/trident/blockentity/BlockEntityRendererGameTimeTest.cpp
    # NaturalSpawner 娴嬭瘯
    server/world/spawn/NaturalSpawnerTest.cpp
    # VillageSiege 测试
    server/world/spawn/VillageSiegeTest.cpp
    # 鍏夌収鍚屾娴嬭瘯
    server/LightSyncTests.cpp
    # Kagero UI模板系统测试
    ui/kagero/template/template_test.cpp
    # Kagero UI Value 类原生 i64/u32/f64 支持测试
    ui/kagero/template/ValueTypesTest.cpp
    # Kagero UI内置事件测试
    ui/kagero/template/BuiltinEventsTest.cpp
    # Kagero UI内置 Widget 工厂测试（grid/slot 等属性解析与生效）
    client/ui/kagero/template/BuiltinWidgetsTest.cpp
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
    client/ui/kagero/widget/ImageWidgetTest.cpp
    # Tooltip 工具提示组件测试
    client/ui/kagero/widget/TooltipTest.cpp
    client/ui/kagero/KageroEngineTest.cpp
    # Kagero Paint 绘制层测试（TextureImage 构造参数、ImageFormat 传播等）
    client/ui/kagero/paint/TextureImageTest.cpp
    client/ui/FontObfuscatedTest.cpp
    client/ui/minecraft/targetinfo/TargetInfoFormatterTest.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/GuiScale.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/minecraft/targetinfo/TargetInfo.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/Font.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/FontTextureAtlas.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/FontRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/DefaultAsciiFont.cpp
    client/ui/GuiScaleTest.cpp
    # 屏幕工具提示命中检测与位置计算测试
    client/ui/screen/ScreenTooltipTest.cpp
    # 容器屏幕交互系统测试（Shift+click/数字键/Q键/中键/双击/拖拽/点击外部）
    client/ui/screen/ContainerScreenInteractionTest.cpp
    # 收纳袋 tooltip 渲染器布局算法测试
    client/ui/screen/tooltip/BundleTooltipRendererTest.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ui/screen/tooltip/BundleTooltipRenderer.cpp
    # ScreenStackWidget 屏幕栈回调与事件通知测试
    client/ui/minecraft/widgets/ScreenStackWidgetTest.cpp
    # CreateWorldScreen 创建世界界面配置选项与 buildRequest 测试
    client/ui/minecraft/screens/CreateWorldScreenTest.cpp
    # MessageScreen 通用通知对话框回调与 ESC 关闭测试
    client/ui/minecraft/screens/MessageScreenTest.cpp
    # SignEditScreen 告示牌编辑屏幕按键交互测试（Tab切换/Enter提交/ESC取消）
    client/ui/minecraft/screens/SignEditScreenTest.cpp
    # HUD 饥饿条渲染逻辑测试（图标状态映射、饥饿效果变体、饱和度抖动动画）
    client/ui/minecraft/widgets/HudHungerRenderingTest.cpp
    # 闄勯瓟绯荤粺娴嬭瘯
    common/item/enchantment/EnchantmentTest.cpp
    common/item/enchantment/MaceEnchantmentTest.cpp
    common/item/enchantment/LocationEnchantmentTest.cpp
    # 鑷姩璺宠穬娴嬭瘯
    common/entity/movement/AutoJumpTest.cpp
    common/entity/player/SleepManagerTest.cpp
    # 鐜╁绉诲姩娴嬭瘯
    common/entity/PlayerMovementTest.cpp
    # Assert搴撴祴璇?    common/util/assert/AssertTest.cpp
    # ServerWorkerPool 测试
    common/util/thread/ServerWorkerPoolTest.cpp
    # ServerWorkerPool 区域互斥测试（对齐 Moonrise 区域锁执行器）
    common/util/thread/ServerWorkerPoolAreaMutexTest.cpp
    # ReentrantAreaLock 区域锁测试
    common/util/concurrent/ReentrantAreaLockTest.cpp
    # LockSupport park/unpark 原语测试
    common/util/concurrent/LockSupportTest.cpp
    # MultiThreadedQueue 无锁队列测试
    common/util/concurrent/MultiThreadedQueueTest.cpp
    # ConcurrentLong2ObjectHashTable 无锁哈希表测试
    common/util/concurrent/ConcurrentLong2ObjectHashTableTest.cpp
    # SHA-256 加密工具测试
    common/util/crypto/Sha256Test.cpp
    # SHA-1 加密工具测试
    common/util/crypto/Sha1Test.cpp
    # MD5 加密工具测试
    common/util/crypto/Md5Test.cpp
    # UUID 工具测试
    common/util/UuidUtilsTest.cpp
    # 特殊日期工具测试
    common/util/SpecialDatesTest.cpp
    # LinkedHashSet 有序集合测试
    common/util/LinkedHashSetTest.cpp
    # DateTimeUtils 日期时间工具测试
    common/util/DateTimeUtilsTest.cpp
    # NbtJsonUtils NBT-JSON转换工具测试
    util/NbtJsonUtilsTest.cpp
    # NBT标签深度比较测试
    util/NbtEqualsTest.cpp
    # 璋冭瘯鍖哄潡鐢熸垚鍣ㄦ祴璇?    common/world/gen/DebugChunkGeneratorTest.cpp
    # 区块生成器MC对齐测试
    common/world/gen/chunk/ChunkGeneratorAlignmentTest.cpp
    # WorldGenRegion测试
    common/world/gen/chunk/WorldGenRegionTest.cpp
    # WorldGenRegion详细单元测试
    common/world/gen/chunk/WorldGenRegionDetailedTest.cpp
    # WorldGenRegion区块访问窗口验证测试
    common/world/gen/chunk/WorldGenRegionAccessTest.cpp
    # ChunkStep byRadius[] 查找表单元测试（对齐 Moonrise ChunkStepMixin）
    common/world/chunk/gen/ChunkStepByRadiusTest.cpp
    # NoiseColumn单元测试
    common/world/gen/chunk/NoiseColumnTest.cpp
    # FlatChunkGenerator详细单元测试
    common/world/gen/chunk/FlatChunkGeneratorDetailedTest.cpp
    # ChunkGenerator接口与BaseChunkGenerator单元测试
    common/world/gen/chunk/ChunkGeneratorInterfaceTest.cpp
    # NoiseChunkGenerator单元测试
    common/world/gen/chunk/NoiseChunkGeneratorTest.cpp
    # IChunkGenerator::clearStructureCache() 单元测试
    common/world/gen/chunk/ClearStructureCacheTest.cpp
    # 结构生成生物群系过滤测试
    common/world/gen/chunk/BiomeFilterTest.cpp
    # 地形常识测试
    common/world/gen/chunk/TerrainCommonSenseTest.cpp
    # 海洋水域生成复现测试
    common/world/gen/chunk/OceanWaterReproTest.cpp
    # 表面规则一致性测试
    common/world/gen/chunk/SurfaceRuleParityTest.cpp
    # 区块内X轴高度不对称性测试
    common/world/gen/chunk/ChunkXAxisHeightTest.cpp
    # 生成设置单元测试（NoiseSettings, DimensionSettings, FlatLayerInfo, FlatLevelGeneratorSettings等）
    common/world/gen/settings/SettingsTest.cpp
    # spawnTarget 数据流集成测试（NoiseChunk/RandomState/ServerWorld）
    common/world/gen/density/SpawnTargetIntegrationTest.cpp
    # 鐢熺墿缇ょ郴棰滆壊娴嬭瘯
    client/world/color/BiomeColorsTest.cpp
    # 鐢熺墿缇ょ郴棰滆壊娣峰悎娴嬭瘯
    client/world/color/blend/BiomeColorBlenderTest.cpp
    # 绮掑瓙绯荤粺娴嬭瘯
    client/renderer/particle/ParticleTest.cpp
    client/renderer/particle/ParticleRegistryTest.cpp
    client/renderer/particle/WhiteSmokeParticleTest.cpp
    client/renderer/particle/ParticleManagerTest.cpp
    client/renderer/trident/particle/particles/special/NautilusParticleTest.cpp
    client/renderer/trident/particle/particles/special/SpecialParticleTest.cpp
    client/renderer/trident/particle/particles/special/VibrationSignalParticleTest.cpp
    client/renderer/trident/particle/data/VibrationParticleDataTest.cpp
    # DustParticleData 粒子数据测试
    client/renderer/trident/particle/data/DustParticleDataTest.cpp
    # TrailParticleData 粒子数据测试
    client/renderer/trident/particle/data/TrailParticleDataTest.cpp
    # EntityEffectParticleData 粒子数据测试
    client/renderer/trident/particle/data/EntityEffectParticleDataTest.cpp
    # ItemParticleData 粒子数据测试
    client/renderer/trident/particle/data/ItemParticleDataTest.cpp
    # 粒子缩放测试
    client/renderer/trident/particle/particles/effect/ParticleScalingTest.cpp
    # SpellParticle 颜色提取测试
    client/renderer/trident/particle/particles/effect/SpellParticleColorTest.cpp
    # DiggingParticle 测试
    client/renderer/trident/particle/particles/block/DiggingParticleTest.cpp
    # DustPillarParticle 测试
    client/renderer/trident/particle/particles/block/DustPillarParticleTest.cpp
    # 新增粒子类测试
    client/renderer/trident/particle/particles/NewParticleTest.cpp
    # Effect particle tests
    client/renderer/trident/particle/particles/effect/EffectParticleTest.cpp
    # Block particle tests
    client/renderer/trident/particle/particles/block/BlockParticleTest.cpp
    # Ambient particle tests
    client/renderer/trident/particle/particles/ambient/AmbientParticleTest.cpp
    # Mob particle tests
    client/renderer/trident/particle/particles/mob/MobParticleTest.cpp
    # Liquid particle tests
    client/renderer/trident/particle/particles/liquid/LiquidParticleTest.cpp
    # Particle tick behavior tests
    client/renderer/trident/particle/particles/TickBehaviorTest.cpp
    # 绗竴浜虹О鎵嬮儴娓叉煋娴嬭瘯
    client/renderer/firstperson/MatrixStackTest.cpp
    client/renderer/firstperson/FirstPersonRendererTest.cpp
    client/renderer/firstperson/ItemInHandRendererTest.cpp
    client/renderer/firstperson/FirstPersonTransformsTest.cpp
    # D7/D9 evaluateWhichHandsToRender + isScoping 测试
    client/renderer/firstperson/EvaluateWhichHandsToRenderTest.cpp
    # D12 第一人称手持物品 3D 升级测试
    client/renderer/firstperson/HeldItemMesh3DTest.cpp
    # 澹伴煶绯荤粺娴嬭瘯
    common/sound/SoundCategoryTest.cpp
    common/sound/test_sound_events.cpp
    # JukeboxSong/JukeboxSongs/JukeboxSongPlayer 唱片机歌曲注册表与播放器测试
    common/sound/jukebox/JukeboxSongTest.cpp
    client/sound/SoundTest.cpp
    client/sound/SoundRegistryTest.cpp
    client/sound/TickableSoundTest.cpp
    client/sound/MovingTickableSoundTest.cpp
    # OpenALBackend 音频后端源计数追踪与功能测试
    client/sound/OpenALBackendTest.cpp
    # 聊天历史与消息类型测试
    client/chat/ChatHistoryTest.cpp
    # 皮肤系统测试
    common/skin/test_skin_types.cpp
    common/skin/test_skin_cache.cpp
    common/skin/test_game_profile.cpp
    common/skin/test_skin_metadata_parser.cpp
    common/skin/test_skin_packets.cpp
    common/skin/test_skin_manager.cpp
    common/skin/test_default_skin_provider.cpp
    common/skin/test_skin_loader_async.cpp
    # 富文本系统测试
    common/util/text/TextStyleTest.cpp
    common/util/text/TextComponentTest.cpp
    # 存储系统测试
    common/world/storage/SectionCodecTest.cpp
    # 调色板容器单元测试
    common/world/chunk/data/PalettedContainerTest.cpp
    common/world/storage/SingleLevelStorageManagerTest.cpp
    common/world/storage/GlobalStorageManagerTest.cpp
    common/world/storage/SaveFormatDetectorTest.cpp
    common/world/storage/RegionFileTest.cpp
    common/world/storage/JavaChunkReaderTest.cpp
    common/world/storage/JavaColumnReaderTest.cpp
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
    server/scoreboard/ScoreboardDataManagerTest.cpp
    server/scoreboard/GetTeamTest.cpp
    # Boss 栏系统测试
    server/bossbar/BossBarTest.cpp
    # 统计系统测试
    server/stats/StatisticsTest.cpp
    server/stats/AwardCustomStatTest.cpp
    # 函数系统测试
    server/function/FunctionLoaderTest.cpp
    server/function/FunctionManagerTest.cpp
    server/function/MacroFunctionTest.cpp
    server/function/StringTemplateTest.cpp
    server/function/TimerQueueTest.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkTaskScheduler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkProgressionTask.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkLightingProvider.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/thread/ServerWorkerPool.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/concurrent/ReentrantAreaLock.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerChunkManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/SingleChunkLifecycleManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerLightQueue.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/RuntimeLightingProvider.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/RuntimeLightTask.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkLoadLightTask.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityChunkTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/ItemPickupManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/NaturalSpawner.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/DespawnManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/VillageSiege.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/SpawnConditions.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/weather/WeatherManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/drop/BlockDropHandler.cpp
        # 游戏事件系统（服务端实现）
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/PositionSource.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/GameEventListenerRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/GameEventDispatcher.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/DynamicGameEventListener.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/VibrationSystemServer.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/storage/ScoreboardDataManager.cpp
        # Boss 栏系统
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/BossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/ServerBossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfoManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/ServerDragonBossBar.cpp
        # 函数系统
        ${CMAKE_SOURCE_DIR}/src/server/function/CommandFunction.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/FunctionLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/FunctionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/MacroFunction.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/StringTemplate.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/TimerQueue.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/world/blockentity/sculk/SculkShriekerHelper.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/blockentity/sculk/SculkVibrationSystem.cpp
        ${CMAKE_SOURCE_DIR}/src/server/menu/CraftingMenu.cpp
        ${CMAKE_SOURCE_DIR}/src/server/settings/ServerSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/ServerCommandSource.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/CommandRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/PlayerResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/EntityResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/EffectResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/FunctionSuggestionProvider.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/command/support/SpreadAlgorithm.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WardenSpawnTrackerCommand.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/data/Heightmap.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/world/storage/reader/java/JavaColumnReader.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/WorldCarver.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/Biome.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeGenerationSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeSource.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeTag.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeTags.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeFactoryOverworld.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeFactoryNether.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeFactoryEnd.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/source/MultiNoiseBiomeSource.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/source/OverworldBiomeBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/source/NetherBiomeBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/source/EndBiomeSource.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/climate/Sampler.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/climate/SpawnFinder.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/weather/WeatherUtils.cpp
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
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/special/TrailParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/special/VibrationSignalParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/VibrationParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/ParticleTextureAtlas.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/BasicParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/DustParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/EntityEffectParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/TrailParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/BlockParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/ItemParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/RedstoneParticleData.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/sprite/SimpleSprite.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/sprite/AnimatedSprite.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/RainParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/SnowParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/ambient/BubbleParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/ambient/BubblePopParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/ambient/UnderwaterParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/ambient/CloudParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/ambient/SuspendedTownParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/block/DiggingParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/block/DustPillarParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/FlameParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/SmokeParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/WhiteSmokeParticle.cpp
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
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/liquid/DripstoneDripParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/weather/SplashParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/weather/FishingParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/special/NautilusParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/special/TrailParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/special/VibrationSignalParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/data/VibrationParticleData.cpp
        # 粒子实现 - 环境（新增）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/ambient/NetherSporeParticle.cpp
        # 粒子实现 - 效果（新增）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/AshParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/CopperFireFlameParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/DamageIndicatorParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/DustParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/DustPlumeParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/EggCrackParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/ElderGuardianParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/ElectricSparkParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/FireflyParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/FireworkParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/FlashParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/GlowParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/GustParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/GustEmitterParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/InfestedParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/ItemPickupParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/LightParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/NoteParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/OmenParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/SculkChargeParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/SculkSoulParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/ShriekParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/SmallFlameParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/SonicBoomParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/TrialSpawnerParticle.cpp
        # 粒子实现 - 生物（新增）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/mob/SpitParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/mob/SquidInkParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/mob/TotemParticle.cpp
        # 粒子实现 - 特殊（新增）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/special/TrailParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/special/VaultConnectionParticle.cpp
        # 粒子实现 - 方块（新增）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/block/ComposterParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/block/ItemParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/block/LeavesParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/block/ScrapeParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/block/WaxParticle.cpp
        # 粒子实现 - 液体（新增）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/liquid/CherryLeavesDripParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/liquid/FallingNectarParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/Feature.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/feature/FeatureSpread.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/item/core/AdventureModePredicate.cpp
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
        # 收纳袋
        ${CMAKE_SOURCE_DIR}/src/common/item/items/special/bundle/BundleContents.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/items/special/bundle/BundleItem.cpp
        # 背包系统
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/Slot.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/PlayerInventory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/PlayerEnderChestInventory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/CraftingInventory.cpp
        # 鍚堟垚绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/Ingredient.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tag/ItemTag.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tag/ItemTagLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tag/ItemTags.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/ShapedRecipe.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/ShapelessRecipe.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeSerializers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeBook.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/TransmuteRecipe.cpp
        # 鏂瑰潡瀹炰綋绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/BlockEntityType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/BlockEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/core/LockableBlockEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/core/SimpleInventory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/core/BlockEntityRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/storage/ChestEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/storage/TrappedChestEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/storage/EnderChestEntity.cpp
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
        # 试炼类方块实体
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/trial/VaultBlockEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/trial/TrialSpawnerBlockEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/trial/CrafterBlockEntity.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/LootPredicateManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/loot/LootPredicateLoader.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/EnchantmentPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/LocationPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/BlockPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/conditions/MobEffectsPredicate.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/impl/TickTrigger.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/impl/InventoryChangedTrigger.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/impl/EntityTriggers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/impl/EffectTriggers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/advancement/trigger/impl/AvoidVibrationTrigger.cpp
        ${CMAKE_SOURCE_DIR}/src/server/advancement/PlayerAdvancements.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/drop/BlockDropHandler.cpp
        # 澶╀綋璁＄畻
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/sky/CelestialCalculations.cpp
        # 射线检测
        ${CMAKE_SOURCE_DIR}/src/common/util/math/ray/Raycast.cpp
        # 设置系统
        ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/ClientWorld.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/entity/ClientEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/entity/ClientEntityManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/player/LocalPlayerIdentity.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/player/ClientPlayerPredictor.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/player/PlayerIdentityRegistry.cpp
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
        # ItemMeshBuilder 物品网格构建测试
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/item/ItemMeshBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/base/BipedModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/base/ElytraSpeedValue.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/base/QuadrupedModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/animal/ChickenModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/animal/CowModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/animal/PigModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/animal/SheepModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/animal/LlamaModel.cpp
    # RabbitModel 兔子模型跳跃角度测试依赖
    ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/animal/RabbitModel.cpp
        # DolphinModel 海豚模型运动状态注入测试依赖（setMotionMagnitude + 摆尾动画）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/aquatic/AquaticModels.cpp
        # VillagerModel + WitchModel 女巫模型测试依赖
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/animal/VillagerModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/monster/WitchModel.cpp
        # CreeperModel + SpiderModel 非 BipedModel 派生模型（dynamic_cast 安全性测试依赖）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/monster/CreeperModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/monster/SpiderModel.cpp
        # SkeletonModel 手臂姿态委托基类 BipedModel 测试依赖
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/monster/SkeletonModel.cpp
        # ZombieModel 手臂姿态激怒状态测试依赖 + 变体模型继承测试依赖
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/monster/ZombieModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/monster/MonsterVariantModels.cpp
        # WitherModel 侧头朝向注入测试依赖（SpecialMonsterModels 包含 WitherModel）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/monster/SpecialMonsterModels.cpp
        # PlayerModel 手臂渲染测试
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/player/PlayerModel.cpp
        # PlayerArmPoseResolver 手臂姿态解析器（测试依赖）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/renderer/player/PlayerArmPoseResolver.cpp
        # 载具渲染器（BoatRenderer / MinecartRenderer 模型矩阵构建与 TNT 闪烁）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/core/EntityRenderer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/renderer/vehicle/VehicleRenderers.cpp
        # 特殊实体渲染器（TNTRenderer / FallingBlockRenderer 模型矩阵与闪烁）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/renderer/special/SpecialEntityRenderers.cpp
        # 投射物模型（SpecialEntityRenderers 中 EnderCrystalRenderer 等构造函数依赖）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/projectile/ProjectileModels.cpp
        # BlockMeshBuilder 方块网格构建工具
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/util/BlockMeshBuilder.cpp
        # EntityRenderer 依赖的辅助渲染器
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/util/ShadowRenderer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/util/NameTagRenderer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/util/WorldTextRenderer.cpp
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
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/firstperson/FirstPersonTransforms.cpp
        # 鍖哄潡妯″瀷缂撳瓨
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ResourceManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockStateLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/TextureAtlasBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemTextureAtlas.cpp
        # 统一图集数据驱动（替换硬编码图集系统的纹理收集层）
        ${CMAKE_SOURCE_DIR}/src/client/resource/atlas/AtlasSource.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/atlas/AtlasSourceParser.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/atlas/AtlasConfigLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/atlas/IdentifierPattern.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/atlas/MissingNo.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/atlas/Sources.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/atlas/SpriteLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/atlas/TexturePathVariant.cpp

        ${CMAKE_SOURCE_DIR}/src/common/resource/VanillaResources.cpp
        # Kagero UI 模板系统（仅核心解析组件）
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/Ast.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/AstVisitor.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/Lexer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/Parser.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/binder/BindingContext.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/bindings/BuiltinEvents.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/bindings/BuiltinWidgets.cpp
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
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/widget/Widget.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/KageroEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/paint/PaintContext.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/paint/Geometry.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/paint/TextureImage.cpp
        # ScreenStackWidget 及 Screen 依赖
        ${CMAKE_SOURCE_DIR}/src/client/ui/minecraft/screens/Screen.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/minecraft/widgets/ScreenStackWidget.cpp
        # CreateWorldScreen 测试依赖：模板屏幕基类 + 模板运行时实例 + 创建世界屏幕
        ${CMAKE_SOURCE_DIR}/src/client/ui/minecraft/screens/TemplateScreen.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/runtime/TemplateInstance.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/minecraft/screens/CreateWorldScreen.cpp
        # MessageScreen 测试依赖：通用通知对话框实现
        ${CMAKE_SOURCE_DIR}/src/client/ui/minecraft/screens/MessageScreen.cpp
        # SignEditScreen 测试依赖：告示牌编辑屏幕实现（Tab切换/Enter提交/ESC取消）
        ${CMAKE_SOURCE_DIR}/src/client/ui/minecraft/screens/SignEditScreen.cpp
        # Assert搴?        ${CMAKE_SOURCE_DIR}/src/common/util/assert/Assert.cpp
        # Crypto utilities
        ${CMAKE_SOURCE_DIR}/src/common/util/crypto/Sha256.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/crypto/Sha1.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/crypto/Md5.cpp
        # UUID utilities
        ${CMAKE_SOURCE_DIR}/src/common/util/UuidUtils.cpp
        # 澹伴煶绯荤粺 - Common
        ${CMAKE_SOURCE_DIR}/src/common/sound/SoundCategory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sound/SoundEvent.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sound/SoundEvents.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sound/network/SoundPackets.cpp
        # Jukebox
        ${CMAKE_SOURCE_DIR}/src/common/sound/jukebox/JukeboxSong.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sound/jukebox/JukeboxSongs.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sound/jukebox/JukeboxSongPlayer.cpp
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
        ${CMAKE_SOURCE_DIR}/src/client/skin/SkinTextureUploader.cpp
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
        ${CMAKE_SOURCE_DIR}/src/client/chat/ChatHistory.cpp
        # TemplateInstance.cpp 依赖 Widget 实现，暂时不纳入该测试目标
)

target_link_libraries(mc_tests
    PRIVATE
        mc_common
        mc_bedrock_addon
        mc_offset_allocator
        GTest::gtest
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
    client/resource/TexturePathVariantTest.cpp
    client/resource/test_fire_texture_loader.cpp
    # FireAnimationState 动画状态机单元测试（纯逻辑，不依赖 Vulkan）
    client/renderer/trident/entity/effect/fire/test_fire_animation_state.cpp
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
        ${CMAKE_SOURCE_DIR}/src/client/resource/atlas/TexturePathVariant.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/repository/PackRepository.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/repository/PackListBase.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/pack/ZipResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/pack/FolderResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/pack/IResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/pack/PackMetadata.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/ResourceLocation.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Block.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/BlockRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/registry/VanillaBlocks.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/AirBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/SimpleBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/RotatedPillarBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Material.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/effect/fire/FireTextureLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/effect/fire/FireAnimationState.cpp
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

# 单个测试用例执行超时（秒）。ctest 跑到该时长未结束即判失败，
# 便于及早暴露区块生成/光照等长耗时用例的 hang/flake。
set(MC_TEST_TIMEOUT 300 CACHE STRING "单个 gtest 用例的 ctest 超时秒数")

function(mc_register_gtests targetName)
    if(WIN32)
        # Windows + clang + 多配置生成器下，gtest_discover_tests 默认在构建期以 Release/Debug
        # 等首个配置探测测试列表，但 applocal.ps1 未在该阶段部署 DLL → 探测二进制启动失败。
        # 改用 DISCOVERY_MODE PRE_TEST：ctest 运行期才执行探测（此时 DLL 已随目标部署到
        # $<TARGET_FILE_DIR>），并用 DL_PATHS 把该目录加入 DLL 搜索路径。每个用例单独注册到
        # CTest 并附 TIMEOUT，实现"单测试 5 分钟超时"。
        gtest_discover_tests(${targetName}
            DISCOVERY_MODE PRE_TEST
            DL_PATHS "$<TARGET_FILE_DIR:${targetName}>"
            PROPERTIES TIMEOUT ${MC_TEST_TIMEOUT}
        )
    else()
        gtest_discover_tests(${targetName}
            PROPERTIES TIMEOUT ${MC_TEST_TIMEOUT}
        )
    endif()
endfunction()

mc_register_gtests(mc_tests)
mc_register_gtests(mc_resource_tests)

# 输出测试可执行文件位置
set_target_properties(mc_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# QuickJS（quickjs-ng）的栈溢出检查用 uintptr_t 算术：stack_limit = stack_top - stack_size，
# 与当前栈指针 sp 比较。当 JS 栈限制（DEFAULT_CONTEXT_STACK_SIZE_BYTES=4MB）大于宿主 C 栈
# （Windows 默认 1MB）时，stack_top - 4MB 会发生无符号下溢，回绕成接近 UINTPTR_MAX 的巨大值，
# 于是 sp < stack_limit 恒为真——任何 evaluate（哪怕 Promise.resolve().then(...)）都会误报
# "Maximum call stack size exceeded"。quickjs-ng 自身的 CMake 也因此把可执行文件栈设为 8MB
# （见 _deps/quickjs-ng-src/CMakeLists.txt 的 /STACK:8388608）。这里为 mc_tests 对齐设置。
# 注意：本项目用 clang + lld-link（非 MSVC link.exe），不能用 if(MSVC) 判断（clang 下 MSVC 为
# FALSE）；通过 -Xlinker 把 /STACK 传给 lld-link（lld-link 与 link.exe 一样接受 /STACK）。
if(WIN32)
    target_link_options(mc_tests PRIVATE "-Xlinker" "/STACK:8388608")
endif()

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
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/buffer/TridentStagingBufferPool.cpp
            # Trident 绾圭悊
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/texture/TridentTexture.cpp
            # Trident 绠＄嚎
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/pipeline/TridentPipeline.cpp
            # Trident 渲染管理器
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/FrameManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/UniformManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/DescriptorManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/RenderPassManager.cpp
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
            # 甯哥敤璧勬簮
            ${CMAKE_SOURCE_DIR}/src/common/resource/ResourceLocation.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/repository/PackRepository.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/repository/PackListBase.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/pack/FolderResourcePack.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/pack/ZipResourcePack.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/pack/IResourcePack.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/pack/PackMetadata.cpp
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
            mc_offset_allocator
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
            # GuiSpriteRegistry data correctness test (slider/handle/attack indicator coords and nine-patch; no Vulkan)
            ${CMAKE_CURRENT_SOURCE_DIR}/client/renderer/trident/gui/GuiSpriteRegistryTest.cpp
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
    server/command/LocateCommandTest.cpp
    server/command/CommandStorageTest.cpp
    server/command/EntityDataAccessorEffectTest.cpp
    server/command/PlayerResolverTest.cpp
    server/command/EntityResolverTest.cpp
    server/command/ServerCommandSourceTest.cpp
    server/command/StopSoundCommandTest.cpp
    server/command/SpreadPlayersCommandTest.cpp
    server/command/SpreadAlgorithmTest.cpp
    server/command/TeamCommandTest.cpp
    server/command/TriggerCommandTest.cpp
    server/command/ReplaceItemCommandTest.cpp
    server/command/BlockDataAccessorTest.cpp
    server/command/WardenSpawnTrackerCommandTest.cpp
    server/command/TeleportPermissionParseTest.cpp
    common/command/arguments/BlockStateArgumentTest.cpp
    common/command/arguments/EntityArgumentTest.cpp
    common/command/arguments/FunctionArgumentTest.cpp
    common/command/arguments/ItemArgumentTest.cpp
    common/command/arguments/ItemSlotArgumentTest.cpp
    common/command/arguments/NbtPathTest.cpp
    common/command/arguments/TimeArgumentTest.cpp
    common/command/arguments/Vec2ArgumentTest.cpp
    command_main.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkTaskScheduler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkProgressionTask.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkLightingProvider.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/thread/ServerWorkerPool.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/concurrent/ReentrantAreaLock.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerChunkManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/SingleChunkLifecycleManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerLightQueue.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/RuntimeLightingProvider.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/RuntimeLightTask.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkLoadLightTask.cpp
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
        # 游戏事件系统（服务端实现）
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/PositionSource.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/GameEventListenerRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/GameEventDispatcher.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/DynamicGameEventListener.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/VibrationSystemServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/blockentity/sculk/SculkVibrationSystem.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/blockentity/sculk/SculkShriekerHelper.cpp
        ${CMAKE_SOURCE_DIR}/src/server/sync/EntitySyncManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/sync/BlockUpdateSyncManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/sync/ChunkSendManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/ServerCommandSource.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/CommandRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/PlayerResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/EntityResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/EffectResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/FunctionSuggestionProvider.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/command/support/SpreadAlgorithm.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WardenSpawnTrackerCommand.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/scoreboard/storage/ScoreboardDataManager.cpp
        # Boss 栏系统
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/BossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/ServerBossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfoManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/ServerDragonBossBar.cpp
        # 函数系统
        ${CMAKE_SOURCE_DIR}/src/server/function/CommandFunction.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/FunctionLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/FunctionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/MacroFunction.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/StringTemplate.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/TimerQueue.cpp
)

target_link_libraries(mc_command_tests
    PRIVATE
        mc_common
        mc_bedrock_addon
        GTest::gtest
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
    common/world/village/raid/RaidManagerTest.cpp
    common/world/village/raid/RaidBossBarProgressTest.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/item/core/AdventureModePredicate.cpp
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
