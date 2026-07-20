# src/common/CMakeLists.txt
# 公共库 - 客户端和服务端共享

find_package(LibArchive REQUIRED)

# Perfetto 性能追踪子模块
add_subdirectory(profiler)

add_library(mc_common STATIC
    # Core
    core/Result.cpp
    core/GameDirectory.cpp
    core/settings/SettingsBase.cpp

    # Input
    input/KeyBinding.cpp

    # Math
    util/math/MathUtils.cpp
    util/math/ray/Raycast.cpp
    util/math/frustum/Frustum.cpp

    # Math - Random
    util/math/random/IRandom.cpp
    util/math/random/Mt19937Random.cpp
    util/math/random/PositionalRandomFactory.cpp
    util/math/random/Xoroshiro128ppRandom.cpp
    util/math/random/Xoshiro256ppRandom.cpp
    util/math/random/LcgRandom.cpp
    util/math/random/JavaLegacyRandom.cpp
    util/math/random/UniformIntDistribution.cpp
    util/math/random/UniformRealDistribution.cpp
    util/math/random/RandomRanges.cpp

    # Network - Connection
    network/connection/LocalConnection.cpp
    network/connection/LocalServerConnection.cpp

    # Network - Packet
    network/packet/Packet.cpp
    network/packet/PacketSerializer.cpp
    network/packet/PacketDeserializer.cpp
    network/packet/CommandTreePacket.cpp
    network/packet/AdvancementPackets.cpp
    network/packet/InventoryPackets.cpp
    network/packet/ContainerPacketHandler.cpp
    network/packet/EntityPackets.cpp
    network/packet/EntityMetadataSerializer.cpp
    network/packet/GameStateChangePacket.cpp
    network/packet/PlayerAbilitiesPacket.cpp
    network/packet/ServerDifficultyPacket.cpp
    network/packet/BlockBreakAnimPacket.cpp
    network/packet/BlockEventPacket.cpp
    network/packet/DimensionPackets.cpp
    network/packet/ExperiencePackets.cpp
    network/packet/ParticlePacket.cpp
    network/packet/ExplosionPacket.cpp
    network/packet/SleepPacket.cpp
    network/packet/SpawnPositionPacket.cpp
    network/packet/MapDataPacket.cpp
    network/packet/SetCameraPacket.cpp
    network/packet/SetPassengersPacket.cpp
    network/packet/SetEntityLinkPacket.cpp
    network/packet/TitlePacket.cpp
    network/packet/WorldBorderPacket.cpp
    network/packet/BossInfoPacket.cpp

    # Network - Sync
    network/sync/ChunkSync.cpp

    # Entity Core
    entity/core/Entity.cpp
    entity/utils/EntityUtils.cpp
    entity/utils/ItemDropHelper.cpp
    entity/core/EntityType.cpp
    entity/core/EntityClassification.cpp
    entity/core/LivingEntity.cpp
    entity/core/MobEntity.cpp
    entity/core/CreatureEntity.cpp
    entity/core/AgeableEntity.cpp
    entity/core/FlyingEntity.cpp
    # Entity Registry - 原版实体类型批量注册
    entity/registry/VanillaEntities.cpp
    entity/registry/VanillaEntityTypeKeys.cpp
    entity/tag/EntityTypeTag.cpp
    entity/tag/EntityTypeTags.cpp
    entity/tag/EntityTypeTagLoader.cpp
    entity/damage/tag/DamageTypeTag.cpp
    entity/damage/tag/DamageTypeTags.cpp
    entity/damage/tag/DamageTypeTagLoader.cpp
    # Entity - Serialization
    entity/serialization/EntityDeserializer.cpp
    entity/serialization/EquipmentSlotNames.cpp
    entity/serialization/NbtHelper.cpp
    # Entity - Effect
    entity/effect/EffectType.cpp
    entity/effect/EffectInstance.cpp
    entity/effect/EffectManager.cpp
    entity/effect/EffectAttributeModifiers.cpp
    entity/enchantment/LocationEnchantmentTracker.cpp
    # Entity - Food
    entity/food/FoodStats.cpp
    # Entity - Player
    entity/entities/player/Player.cpp
    entity/entities/player/GameModeUtils.cpp
    entity/player/SleepManager.cpp
    entity/player/SpawnPointValidator.cpp
    entity/player/CooldownTracker.cpp
    # Entity - Item
    entity/entities/item/ItemEntity.cpp
    # Entity - Passive Animals (basic)
    entity/entities/passive/basic/AnimalEntity.cpp
    entity/entities/passive/basic/PigEntity.cpp
    entity/entities/passive/basic/CowEntity.cpp
    entity/entities/passive/basic/SheepEntity.cpp
    entity/entities/passive/basic/ChickenEntity.cpp
    entity/entities/passive/basic/RabbitEntity.cpp
    entity/entities/passive/basic/MooshroomEntity.cpp
    # Entity - Passive Animals (tamable)
    entity/entities/passive/tamable/TameableEntity.cpp
    entity/entities/passive/tamable/WolfEntity.cpp
    entity/entities/passive/tamable/Crackiness.cpp
    entity/entities/passive/tamable/CatEntity.cpp
    entity/entities/passive/tamable/OcelotEntity.cpp
    entity/entities/passive/tamable/ParrotEntity.cpp
    # Entity - Passive Animals (special)
    entity/entities/passive/special/FoxEntity.cpp
    entity/entities/passive/special/PandaEntity.cpp
    entity/entities/passive/special/PolarBearEntity.cpp
    entity/entities/passive/special/TurtleEntity.cpp
    entity/entities/passive/special/BeeEntity.cpp
    entity/entities/passive/special/StriderEntity.cpp
    entity/entities/passive/special/SnifferEntity.cpp
    # Entity - Passive Animals (water)
    entity/entities/passive/water/WaterMobEntity.cpp
    entity/entities/passive/water/SquidEntity.cpp
    entity/entities/passive/water/GlowSquidEntity.cpp
    entity/entities/passive/water/DolphinEntity.cpp
    entity/entities/passive/water/AxolotlEntity.cpp
    # Entity - Passive Animals (nautilus)
    entity/entities/passive/nautilus/AbstractNautilusEntity.cpp
    entity/entities/passive/nautilus/NautilusEntity.cpp
    entity/entities/passive/nautilus/ZombieNautilusEntity.cpp
    # Entity - Passive Animals (fish)
    entity/entities/passive/fish/AbstractFishEntity.cpp
    entity/entities/passive/fish/AbstractGroupFishEntity.cpp
    entity/entities/passive/fish/CodEntity.cpp
    entity/entities/passive/fish/SalmonEntity.cpp
    entity/entities/passive/fish/PufferfishEntity.cpp
    entity/entities/passive/fish/TropicalFishEntity.cpp
    # Entity - Passive Animals (ambient)
    entity/entities/passive/ambient/AmbientEntity.cpp
    entity/entities/passive/ambient/BatEntity.cpp
    # Entity - Passive Animals (golem)
    entity/entities/passive/golem/GolemEntity.cpp
    entity/entities/passive/golem/IronGolemEntity.cpp
    entity/entities/passive/golem/SnowGolemEntity.cpp
    entity/entities/passive/golem/CopperGolemEntity.cpp

    # Entity - Passive - Horse
    entity/entities/passive/horse/AbstractHorseEntity.cpp
    entity/entities/passive/horse/AbstractChestedHorseEntity.cpp
    entity/entities/passive/horse/HorseEntity.cpp
    entity/entities/passive/horse/DonkeyEntity.cpp
    entity/entities/passive/horse/MuleEntity.cpp
    entity/entities/passive/horse/SkeletonHorseEntity.cpp
    entity/entities/passive/horse/ZombieHorseEntity.cpp
    entity/entities/passive/horse/LlamaEntity.cpp
    entity/entities/passive/horse/TraderLlamaEntity.cpp
    # Entity - Monster
    entity/interfaces/IFlinging.cpp
    entity/interfaces/IRideable.cpp
    entity/entities/monster/MonsterEntity.cpp
    # Entity - Monster (undead)
    entity/entities/monster/undead/ZombieEntity.cpp
    entity/entities/monster/undead/AbstractSkeletonEntity.cpp
    entity/entities/monster/undead/BoggedEntity.cpp
    entity/entities/monster/undead/SkeletonEntity.cpp
    entity/entities/monster/undead/HuskEntity.cpp
    entity/entities/monster/undead/DrownedEntity.cpp
    entity/entities/monster/undead/StrayEntity.cpp
    entity/entities/monster/undead/WitherSkeletonEntity.cpp
    entity/entities/monster/undead/ZombieVillagerEntity.cpp
    # Entity - Monster (arthropod)
    entity/entities/monster/arthropod/SpiderEntity.cpp
    entity/entities/monster/arthropod/CaveSpiderEntity.cpp
    entity/entities/monster/arthropod/EndermiteEntity.cpp
    # Entity - Monster (basic)
    entity/entities/monster/basic/CreeperEntity.cpp
    entity/entities/monster/basic/SlimeEntity.cpp
    entity/entities/monster/basic/PhantomEntity.cpp
    entity/entities/monster/basic/GiantEntity.cpp
    # Entity - Monster (end)
    entity/entities/monster/end/EndermanEntity.cpp
    entity/entities/monster/end/ShulkerEntity.cpp
    # Entity - Monster (nether)
    entity/entities/monster/nether/BlazeEntity.cpp
    entity/entities/monster/nether/NetherEntities.cpp
    # Entity - Monster (ocean)
    entity/entities/monster/ocean/GuardianEntity.cpp
    entity/entities/monster/ocean/ElderGuardianEntity.cpp
    # Entity - Monster (illager)
    entity/entities/monster/illager/AbstractIllagerEntity.cpp
    entity/entities/monster/illager/SpellcastingIllagerEntity.cpp
    entity/entities/monster/illager/PatrollerEntity.cpp
    entity/entities/monster/illager/AbstractRaiderEntity.cpp
    entity/entities/monster/illager/WitchEntity.cpp
    entity/entities/monster/illager/IllagerEntities.cpp
    entity/entities/monster/illager/EvokerEntity.cpp
    entity/entities/monster/illager/IllusionerEntity.cpp
    entity/entities/monster/illager/RavagerEntity.cpp
    entity/entities/monster/illager/VexEntity.cpp
    # Entity - Monster (breeze)
    entity/entities/monster/breeze/BreezeEntity.cpp
    # Entity - Vehicle
    entity/entities/vehicle/BoatEntity.cpp
    entity/entities/vehicle/ChestBoatEntity.cpp
    entity/entities/vehicle/MinecartEntity.cpp
    # Entity - Hanging
    entity/entities/hanging/HangingEntity.cpp
    # Entity - Effect
    entity/entities/effect/EffectEntities.cpp
    # Entity - Orb (Experience Orb)
    entity/entities/orb/ExperienceOrbEntity.cpp
    # Entity - Experience System
    entity/experience/ExperienceManager.cpp
    entity/experience/ExperienceDropHandler.cpp
    # Entity - Misc
    entity/entities/misc/MiscEntities.cpp
    entity/entities/misc/OminousItemSpawnerEntity.cpp
    # Entity - Boss
    entity/entities/boss/EnderDragonEntity.cpp
    entity/entities/boss/WardenEntity.cpp
    entity/entities/boss/WitherEntity.cpp
    # Entity - Villager
    entity/entities/villager/AbstractVillagerEntity.cpp
    entity/entities/villager/VillagerEntity.cpp
    # Entity - Projectile (entity files have deep API dependencies - need systematic refactoring)
    entity/entities/projectile/ProjectileHelper.cpp
    entity/entities/projectile/ProjectileEntity.cpp
    entity/entities/projectile/ProjectileDeflection.cpp
    entity/entities/projectile/DamagingProjectileEntity.cpp
    entity/entities/projectile/ThrowableEntity.cpp
    entity/entities/projectile/AbstractArrowEntity.cpp
    entity/entities/projectile/AbstractFireballEntity.cpp
    entity/entities/projectile/ProjectileItemEntity.cpp
    entity/entities/projectile/TridentEntity.cpp
    entity/entities/projectile/SpearEntity.cpp
    entity/entities/projectile/OtherProjectiles.cpp
    entity/entities/projectile/WindChargeEntity.cpp
    # Entity - Movement
    entity/movement/AutoJump.cpp
    # Item - Loot System
    item/loot/conditions/SilkTouchCondition.cpp
    item/loot/conditions/FortuneCondition.cpp
    item/loot/conditions/RandomChanceCondition.cpp
    item/loot/conditions/RandomChanceWithLuckCondition.cpp
    item/loot/conditions/NotCondition.cpp
    item/loot/conditions/AndCondition.cpp
    item/loot/conditions/OrCondition.cpp
    item/loot/conditions/BlockStateCondition.cpp
    item/loot/conditions/ToolTypeCondition.cpp
    item/loot/conditions/MatchToolCondition.cpp
    item/loot/conditions/SurvivesExplosionCondition.cpp
    item/loot/conditions/TableBonusCondition.cpp
    item/loot/conditions/KilledByPlayerCondition.cpp
    item/loot/conditions/EntityPropertiesCondition.cpp
    item/loot/conditions/EntityScoresCondition.cpp
    item/loot/conditions/LocationCheckCondition.cpp
    item/loot/conditions/WeatherCheckCondition.cpp
    item/loot/conditions/TimeCheckCondition.cpp
    item/loot/conditions/DamageSourcePropertiesCondition.cpp
    item/loot/conditions/ReferenceCondition.cpp
    item/loot/conditions/FishingOpenWaterCondition.cpp
    item/loot/conditions/LootConditionBuilder.cpp
    item/loot/context/LootParameterSet.cpp
    item/loot/context/LootParams.cpp
    item/loot/context/LootParameterSets.cpp
    item/loot/context/LootContext.cpp
    item/loot/context/LootContextBuilder.cpp
    item/loot/entries/LootEntry.cpp
    item/loot/entries/EmptyLootEntry.cpp
    item/loot/entries/ItemLootEntry.cpp
    item/loot/entries/TableLootEntry.cpp
    item/loot/entries/TagLootEntry.cpp
    item/loot/entries/DynamicLootEntry.cpp
    item/loot/entries/AlternativesLootEntry.cpp
    item/loot/entries/SequenceLootEntry.cpp
    item/loot/entries/GroupLootEntry.cpp
    item/loot/entries/LootEntryBuilder.cpp
    item/loot/StatePropertiesPredicate.cpp
    item/loot/LootPool.cpp
    item/loot/LootTable.cpp
    item/loot/LootTableManager.cpp
    item/loot/LootTableLoader.cpp
    item/loot/LootPredicateManager.cpp
    item/loot/LootPredicateLoader.cpp
    item/loot/LootSerializers.cpp
    item/loot/functions/LootFunction.cpp
    item/loot/functions/SetCountFunction.cpp
    item/loot/functions/ApplyBonusFunction.cpp
    item/loot/functions/LootingEnchantBonusFunction.cpp
    item/loot/functions/SetDamageFunction.cpp
    item/loot/functions/SetNameFunction.cpp
    item/loot/functions/SetLoreFunction.cpp
    item/loot/functions/LimitCountFunction.cpp
    item/loot/functions/FurnaceSmeltFunction.cpp
    item/loot/functions/EnchantWithLevelsFunction.cpp
    item/loot/functions/EnchantRandomlyFunction.cpp
    item/loot/functions/ExplosionDecayFunction.cpp
    item/loot/functions/SetNbtFunction.cpp
    item/loot/functions/CopyNameFunction.cpp
    item/loot/functions/CopyBlockStateFunction.cpp
    item/loot/functions/CopyNbtFunction.cpp
    item/loot/functions/FillPlayerHeadFunction.cpp
    item/loot/functions/SetAttributesFunction.cpp
    item/loot/functions/SetContentsFunction.cpp
    item/loot/functions/SetLootTableFunction.cpp
    item/loot/functions/ExplorationMapFunction.cpp
    item/loot/functions/SetStewEffectFunction.cpp
    item/loot/functions/LootFunctionBuilder.cpp
    entity/combat/AttackContext.cpp
    entity/combat/PlayerAttackHelper.cpp
    entity/combat/DifficultyHelper.cpp
    entity/combat/DifficultyInstance.cpp
    entity/combat/CombatRules.cpp
    entity/damage/CombatEntry.cpp
    entity/damage/CombatTracker.cpp
    entity/damage/DamageSource.cpp
    entity/ai/EntitySenses.cpp
    entity/ai/controller/LookController.cpp
    entity/ai/controller/MovementController.cpp
    entity/ai/controller/JumpController.cpp
    entity/ai/controller/FlyingMovementController.cpp
    entity/ai/controller/VexMovementController.cpp
    entity/ai/controller/GhastMovementController.cpp
    entity/ai/controller/PhantomMovementController.cpp
    entity/ai/controller/DrownedMoveControl.cpp
    entity/ai/controller/RabbitJumpControl.cpp
    entity/ai/controller/RabbitMoveControl.cpp
    entity/ai/goal/goals/SwimGoal.cpp
    entity/ai/goal/goals/RandomWalkingGoal.cpp
    entity/ai/goal/goals/LookAtGoal.cpp
    entity/ai/goal/goals/PanicGoal.cpp
    entity/ai/goal/goals/BreedGoal.cpp
    entity/ai/goal/goals/TemptGoal.cpp
    entity/ai/goal/goals/FollowParentGoal.cpp
    entity/ai/goal/goals/MeleeAttackGoal.cpp
    entity/ai/goal/goals/AvoidEntityGoal.cpp
    entity/ai/goal/goals/AvoidBlockGoal.cpp
    entity/ai/goal/goals/RandomSwimmingGoal.cpp
    entity/ai/goal/goals/FishSwimGoal.cpp
    entity/ai/goal/goals/FindWaterGoal.cpp
    entity/ai/goal/goals/SwimUpGoal.cpp
    entity/ai/goal/goals/EatGrassGoal.cpp
    entity/ai/goal/goals/FleeSunGoal.cpp
    entity/ai/goal/goals/RestrictSunGoal.cpp
    entity/ai/goal/goals/FindShelterGoal.cpp
    entity/ai/goal/goals/FlyGoal.cpp
    entity/ai/goal/goals/interact/TameableGoals.cpp
    entity/ai/goal/goals/interact/LandOnOwnersShoulderGoal.cpp
    entity/ai/goal/goals/interact/DoorInteractGoal.cpp
    entity/ai/goal/goals/interact/BreakDoorGoal.cpp
    entity/ai/goal/goals/interact/OpenDoorGoal.cpp
    entity/ai/goal/goals/interact/RaiderOpenDoorGoal.cpp
    entity/ai/goal/goals/movement/MovementGoals.cpp
    entity/ai/goal/goals/movement/FollowSchoolLeaderGoal.cpp
    entity/ai/goal/goals/movement/WaterAvoidingRandomFlyingGoal.cpp
    entity/ai/goal/goals/movement/FollowMobGoal.cpp
    entity/ai/goal/goals/attack/RangedAttackGoals.cpp
    entity/ai/goal/goals/target/TargetGoals.cpp
    entity/ai/goal/goals/special/SpecialGoals.cpp
    entity/ai/goal/goals/special/AxolotlGoals.cpp
    entity/ai/goal/goals/special/SquidGoals.cpp
    entity/ai/goal/goals/special/SlimeGoals.cpp
    entity/ai/goal/goals/special/BatGoals.cpp
    entity/ai/goal/goals/special/DolphinGoals.cpp
    entity/ai/goal/goals/special/GuardianAttackGoal.cpp
    entity/ai/goal/goals/special/BlazeFireballAttackGoal.cpp
    entity/ai/goal/goals/special/BreezeGoals.cpp
    entity/ai/goal/goals/special/PhantomGoals.cpp
    entity/ai/goal/goals/special/IronGolemGoals.cpp
    entity/ai/goal/goals/special/CopperGolemGoals.cpp
    entity/ai/goal/goals/special/ShulkerGoals.cpp
    entity/ai/goal/goals/special/EvokerGoals.cpp
    entity/ai/goal/goals/special/IllusionerGoals.cpp
    entity/ai/goal/goals/special/VexGoals.cpp
    entity/ai/goal/goals/special/EndermanGoals.cpp
    entity/ai/goal/goals/special/TurtleGoals.cpp
    entity/ai/goal/goals/special/WanderingTraderGoals.cpp
    entity/ai/goal/goals/special/BeeGoals.cpp
    entity/ai/goal/goals/special/CatGoals.cpp
    entity/ai/goal/goals/special/FoxGoals.cpp
    entity/ai/goal/goals/special/PandaGoals.cpp
    entity/ai/goal/goals/special/MoveToLavaGoal.cpp
    entity/ai/goal/goals/special/RaidGardenGoal.cpp
    entity/ai/goal/goals/special/PatrolGoals.cpp
    entity/ai/goal/goals/special/RavagerGoals.cpp
    entity/ai/goal/goals/special/GhastGoals.cpp
    entity/ai/goal/goals/special/SilverfishGoals.cpp
    entity/ai/goal/goals/special/DrownedGoals.cpp
    entity/ai/goal/goals/AdditionalGoals.cpp
    # Entity - AI - Villager Goals
    entity/ai/goal/goals/villager/AvoidHostileGoal.cpp
    entity/ai/goal/goals/villager/CongregateGoal.cpp
    entity/ai/goal/goals/villager/FarmerWorkGoal.cpp
    entity/ai/goal/goals/villager/GatherItemsGoal.cpp
    entity/ai/goal/goals/villager/GoToBedGoal.cpp
    entity/ai/goal/goals/villager/LookAtEntitiesGoal.cpp
    entity/ai/goal/goals/villager/LookForJobSiteGoal.cpp
    entity/ai/goal/goals/villager/ShareItemsGoal.cpp
    entity/ai/goal/goals/villager/SleepAtNightGoal.cpp
    entity/ai/goal/goals/villager/VillagerBreedGoal.cpp
    entity/ai/goal/goals/villager/VillagerGoalUtils.cpp
    entity/ai/goal/goals/villager/WorkAtJobSiteGoal.cpp
    # Entity - AI - Brain System
    entity/ai/brain/memory/MemoryModuleType.cpp
    entity/ai/brain/sensor/Sensors.cpp
    # Entity - Villager
    entity/entities/villager/ProfessionMapping.cpp
    entity/ai/brain/schedule/Activity.cpp
    entity/ai/brain/schedule/Schedule.cpp
    entity/ai/pathfinding/PathPoint.cpp
    entity/ai/pathfinding/Path.cpp
    entity/ai/pathfinding/Region.cpp
    entity/ai/pathfinding/WalkNodeProcessor.cpp
    entity/ai/pathfinding/RavagerNodeProcessor.cpp
    entity/ai/pathfinding/PathFinder.cpp
    entity/ai/pathfinding/PathNavigator.cpp
    entity/ai/util/PiglinAi.cpp
    entity/ai/util/RandomPositionGenerator.cpp
    entity/inventory/IInventory.cpp
    entity/inventory/InventoryRef.cpp
    entity/inventory/Slot.cpp
    entity/inventory/PlayerInventory.cpp
    entity/inventory/PlayerEnderChestInventory.cpp
    entity/inventory/CreativeInventory.cpp
    entity/inventory/CraftingInventory.cpp
    entity/inventory/AbstractContainerMenu.cpp
    entity/inventory/IRecipeHolder.cpp
    entity/inventory/container/ChestContainer.cpp
    entity/inventory/container/HopperContainer.cpp
    entity/inventory/container/FurnaceContainer.cpp
    entity/inventory/container/EnchantmentContainer.cpp
    entity/inventory/container/CartographyContainer.cpp
    entity/inventory/container/BrewingStandContainer.cpp
    entity/inventory/container/CrafterContainer.cpp
    entity/inventory/container/AnvilContainer.cpp
    entity/inventory/container/LoomContainer.cpp
    entity/inventory/container/MerchantContainer.cpp
    entity/inventory/container/MerchantContainerMenu.cpp
    entity/inventory/container/MerchantResultSlot.cpp

    # Item System - Core
    item/core/AdventureModePredicate.cpp
    item/core/Item.cpp
    item/core/ItemStack.cpp
    item/core/ItemRegistry.cpp
    item/core/ItemGroup.cpp
    item/core/ProjectileItem.cpp
    item/Items.cpp

    # Item System - Context
    item/context/ItemUseContext.cpp
    item/context/BlockItemUseContext.cpp

    # Item System - Food
    item/food/Food.cpp
    item/food/Foods.cpp
    item/items/food/FoodItem.cpp
    item/items/food/HoneyBottleItem.cpp
    item/items/food/ChorusFruitItem.cpp
    item/items/food/GoldenAppleItem.cpp

    # Item System - Armor
    item/armor/ArmorMaterial.cpp
    item/items/armor/ArmorItem.cpp
    item/items/armor/DyeableArmorItem.cpp
    item/items/armor/ElytraItem.cpp
    item/items/armor/HorseArmorItem.cpp
    item/items/armor/NautilusArmorItem.cpp
    item/items/armor/WolfArmorItem.cpp

    # Item System - Attributes
    item/attribute/ItemAttributeModifiers.cpp

    # Item System - Tags
    item/tag/ItemTag.cpp
    item/tag/ItemTagLoader.cpp
    item/tag/ItemTags.cpp

    # Item System - Block Items
    item/items/block/BedItem.cpp
    item/items/block/BlockItem.cpp
    item/items/block/BlockItemRegistry.cpp
    item/items/block/GameMasterBlockItem.cpp
    item/items/block/SeedsItem.cpp
    item/items/block/WallOrFloorItem.cpp
    item/items/block/BannerItem.cpp

    # Item - Banner Pattern
    item/items/BannerPatternItem.cpp

    # Item - Tier System
    item/tier/IItemTier.cpp
    item/tier/ItemTiers.cpp

    # Item - Tool System (moved to items/tool)
    item/items/tool/ToolType.cpp
    item/items/tool/TieredItem.cpp
    item/items/tool/ToolItem.cpp
    item/items/tool/PickaxeItem.cpp
    item/items/tool/AxeItem.cpp
    item/items/tool/ShovelItem.cpp
    item/items/tool/HoeItem.cpp
    item/items/tool/SwordItem.cpp
    item/items/tool/ShearsItem.cpp

    # Item - Weapons
    item/items/weapon/BowItem.cpp
    item/items/weapon/ArrowItem.cpp
    item/items/weapon/TippedArrowItem.cpp
    item/items/weapon/SpectralArrowItem.cpp
    item/items/weapon/FireChargeItem.cpp
    item/items/weapon/FireworkRocketItem.cpp
    item/items/weapon/CrossbowItem.cpp
    item/items/weapon/TridentItem.cpp
    item/items/weapon/SpearItem.cpp
    item/items/weapon/ShieldItem.cpp
    item/items/weapon/ThrowableItem.cpp
    item/items/weapon/ThrowableItems.cpp
    item/items/weapon/FishingRodItem.cpp

    # Item - Potion System
    item/potion/Potion.cpp
    item/potion/PotionRegistry.cpp
    item/potion/Potions.cpp
    item/potion/PotionUtils.cpp
    item/potion/PotionBrewing.cpp

    # Item - Potion Items
    item/items/potion/PotionItem.cpp
    item/items/potion/ThrowablePotionItem.cpp
    item/items/potion/SplashPotionItem.cpp
    item/items/potion/LingeringPotionItem.cpp
    item/items/potion/GlassBottleItem.cpp

    # Item - Special Items
    item/items/special/BoneMealItem.cpp
    item/items/special/bundle/BundleContents.cpp
    item/items/special/bundle/BundleItem.cpp
    item/items/special/BucketItem.cpp
    item/items/special/FishBucketItem.cpp
    item/items/special/FlintAndSteelItem.cpp
    item/items/special/HarnessItem.cpp
    item/items/special/HoneycombItem.cpp
    item/items/special/KnowledgeBookItem.cpp
    item/items/special/MilkBucketItem.cpp
    item/items/special/MusicDiscItem.cpp
    item/items/special/PotterySherdItem.cpp
    item/items/special/PowderSnowBucketItem.cpp
    item/items/special/SmithingTemplateItem.cpp
    item/items/special/EnchantedBookItem.cpp
    item/items/special/NameTagItem.cpp
    item/items/special/LeadItem.cpp
    item/items/special/SaddleItem.cpp
    item/items/special/OnAStickItem.cpp
    item/items/special/StickItems.cpp
    item/items/special/SpawnEggItem.cpp
    item/items/special/BrushItem.cpp

    # Item - Trial Items
    item/items/trial/BreezeRodItem.cpp
    item/items/trial/MaceItem.cpp
    item/items/trial/OminousBottleItem.cpp
    item/items/trial/OminousTrialKeyItem.cpp
    item/items/trial/TrialKeyItem.cpp
    item/items/trial/WindChargeItem.cpp

    # Item - Map Items
    item/items/map/AbstractMapItem.cpp
    item/items/map/EmptyMapItem.cpp
    item/items/map/FilledMapItem.cpp

    # Item - Vehicle Items
    item/items/vehicle/MinecartItem.cpp
    item/items/vehicle/BoatItem.cpp

    # Item - Crafting
    item/crafting/Ingredient.cpp
    item/crafting/RecipeBook.cpp
    item/crafting/RecipeType.cpp
    item/crafting/RecipeManager.cpp
    item/crafting/ShapedRecipe.cpp
    item/crafting/ShapelessRecipe.cpp
    item/crafting/RecipeSerializers.cpp
    item/crafting/RecipeLoader.cpp
    item/crafting/SmeltingRecipe.cpp
    item/crafting/StonecuttingRecipe.cpp
    item/crafting/SmithingRecipe.cpp
    item/crafting/TransmuteRecipe.cpp
    item/crafting/RecipeNetworkSerializer.cpp
    item/crafting/special/RepairItemRecipe.cpp
    item/crafting/special/ArmorDyeRecipe.cpp
    item/crafting/special/BookCloningRecipe.cpp
    item/crafting/special/MapCloningRecipe.cpp
    item/crafting/special/MapExtendingRecipe.cpp
    item/crafting/special/TippedArrowRecipe.cpp
    item/crafting/special/BannerDuplicateRecipe.cpp
    item/crafting/special/ShieldDecorationRecipe.cpp
    item/crafting/special/DecoratedPotRecipe.cpp

    # Item - Enchantment System
    item/enchantment/Enchantment.cpp
    item/enchantment/EnchantmentRegistry.cpp
    item/enchantment/EnchantmentContainer.cpp
    item/enchantment/EnchantmentHelper.cpp
    item/enchantment/enchantments/FortuneEnchantment.cpp
    item/enchantment/enchantments/SilkTouchEnchantment.cpp
    item/enchantment/enchantments/AllEnchantments.cpp
    item/enchantment/enchantments/protection/ProtectionEnchantment.cpp
    item/enchantment/enchantments/protection/ThornsEnchantment.cpp
    item/enchantment/enchantments/protection/DepthStriderEnchantment.cpp
    item/enchantment/enchantments/protection/FrostWalkerEnchantment.cpp
    item/enchantment/enchantments/special/SoulSpeedEnchantment.cpp
    item/enchantment/enchantments/weapon/DamageEnchantment.cpp
    item/enchantment/enchantments/weapon/BaneOfArthropodsEnchantment.cpp
    item/enchantment/enchantments/mace/DensityEnchantment.cpp
    item/enchantment/enchantments/mace/BreachEnchantment.cpp
    item/enchantment/enchantments/mace/WindBurstEnchantment.cpp
    item/enchantment/enchantments/tool/UnbreakingEnchantment.cpp

    # World - Entity
    world/entity/EntityManager.cpp

    # World - IWorld implementation
    world/IWorld.cpp

    # World - Game Rules
    world/gamerule/GameRule.cpp
    world/gamerule/GameRules.cpp

    # World - Spawn
    world/spawn/EntitySpawnPlacementRegistry.cpp
    world/spawn/MobSpawnInfo.cpp
    world/spawn/SlimeChunkChecker.cpp

    # World - Block Entity
    world/blockentity/BlockEntityType.cpp
    world/blockentity/BlockEntity.cpp
    world/blockentity/ContainerBlockEntity.cpp
    world/blockentity/CraftingTableEntity.cpp

    # Screen
    screen/ScreenType.cpp

    # Command
    command/CommandTreeSnapshot.cpp
    command/arguments/EntityArgument.cpp
    command/arguments/FunctionArgument.cpp
    command/arguments/ItemArgument.cpp
    command/arguments/NbtPath.cpp
    command/arguments/NbtPathArgumentType.cpp

    # World - Block System
    world/block/Material.cpp
    world/block/Block.cpp
    world/block/BlockState.cpp
    world/block/BlockRegistry.cpp
    world/block/FireInfoRegistry.cpp
    world/block/BlockSoundType.cpp
    world/block/BlockTags.cpp
    world/block/SupportType.cpp
    world/block/state/pattern/BlockInWorld.cpp
    world/block/state/pattern/BlockPattern.cpp
    world/block/state/pattern/BlockPatternBuilder.cpp
    world/block/registry/AgriculturalBlocks.cpp
    world/block/registry/BaseBlocks.cpp
    world/block/registry/BuildingBlocks.cpp
    world/block/registry/BuildingVariantBlocks.cpp
    world/block/registry/ColoredBlocks.cpp
    world/block/registry/NaturalBlocks.cpp
    world/block/registry/NetherBlocks.cpp
    world/block/registry/RedstoneBlocks.cpp
    world/block/registry/SignBannerBlocks.cpp
    world/block/registry/VanillaBlocks.cpp
    world/block/registry/VegetationBlocks.cpp
    world/block/registry/DeepslateBlocks.cpp
    world/block/registry/CopperBlocks.cpp
    world/block/registry/CaveBlocks.cpp
    world/block/registry/SculkBlocks.cpp
    world/block/registry/MangroveBlocks.cpp
    world/block/registry/MudBlocks.cpp
    world/block/registry/WildBlocks.cpp
    world/block/registry/CherryBlocks.cpp
    world/block/registry/BambooBlocks.cpp
    world/block/registry/TrailsBlocks.cpp
    world/block/registry/TuffBlocks.cpp
    world/block/registry/TrialBlocks.cpp
    world/block/registry/PaleGardenBlocks.cpp
    world/block/registry/GardenBlocks.cpp
    world/block/registry/ShelfBlocks.cpp
    world/block/registry/CandleBlocks.cpp
    world/block/registry/FlowerPotBlocks.cpp
    world/block/IWaterLoggable.cpp
    world/block/blocks/AirBlock.cpp
    world/block/blocks/SimpleBlock.cpp
    world/block/blocks/FallingBlock.cpp
    world/block/blocks/ConcretePowderBlock.cpp
    world/block/blocks/RotatedPillarBlock.cpp
    world/block/blocks/DirectionalBlock.cpp
    world/block/blocks/HorizontalBlock.cpp
    world/block/blocks/MultifaceBlock.cpp
    world/block/blocks/MultifaceSpreader.cpp
    world/block/blocks/LiquidBlock.cpp
    world/block/blocks/CauldronBlock.cpp
    world/block/blocks/LayeredCauldronBlock.cpp
    world/block/blocks/LavaCauldronBlock.cpp
    world/block/blocks/EnchantingTableBlock.cpp
    world/block/blocks/BookshelfBlock.cpp
    world/block/blocks/ShelfBlock.cpp
    world/block/blocks/AbstractFurnaceBlock.cpp
    world/block/blocks/FurnaceBlock.cpp
    world/block/blocks/BlastFurnaceBlock.cpp
    world/block/blocks/SmokerBlock.cpp
    world/block/blocks/ChestBlock.cpp
    world/block/blocks/TrappedChestBlock.cpp
    world/block/blocks/ShulkerBoxBlock.cpp
    world/block/blocks/HopperBlock.cpp

    # World - Block - Dirt
    world/block/blocks/dirt/SnowyDirtBlock.cpp
    world/block/blocks/dirt/SpreadableSnowyDirtBlock.cpp
    world/block/blocks/dirt/MudBlock.cpp
    world/block/blocks/DoorBlock.cpp
    world/block/blocks/FenceGateBlock.cpp

    # World - Block - Building
    world/block/blocks/building/StairsBlock.cpp
    world/block/blocks/building/SlabBlock.cpp
    world/block/blocks/building/WallBlock.cpp
    world/block/blocks/building/FenceBlock.cpp
    world/block/blocks/building/TrapDoorBlock.cpp

    # World - Block - Agricultural
    world/block/blocks/agricultural/BushBlock.cpp
    world/block/blocks/agricultural/CocoaBlock.cpp
    world/block/blocks/agricultural/CropBlock.cpp
    world/block/blocks/agricultural/WheatBlock.cpp
    world/block/blocks/agricultural/CarrotBlock.cpp
    world/block/blocks/agricultural/PotatoBlock.cpp
    world/block/blocks/agricultural/BeetrootBlock.cpp
    world/block/blocks/agricultural/FarmlandBlock.cpp
    world/block/blocks/agricultural/MelonPumpkinBlocks.cpp
    world/block/blocks/agricultural/StemBlock.cpp
    world/block/blocks/agricultural/TorchflowerCropBlock.cpp
    world/block/blocks/agricultural/PitcherCropBlock.cpp

    # World - Block - Functional
    world/block/blocks/functional/BedBlock.cpp
    world/block/blocks/functional/BrewingStandBlock.cpp
    world/block/blocks/functional/ComposterBlock.cpp
    world/block/blocks/functional/CompostableItems.cpp
    world/block/blocks/functional/CakeBlock.cpp
    world/block/blocks/functional/CandleCakeBlock.cpp
    world/block/blocks/functional/BeaconBlock.cpp
    world/block/blocks/functional/BarrelBlock.cpp
    world/block/blocks/functional/LecternBlock.cpp
    world/block/blocks/functional/GrindstoneBlock.cpp
    world/block/blocks/functional/StonecutterBlock.cpp
    world/block/blocks/functional/LoomBlock.cpp
    world/block/blocks/functional/BellBlock.cpp
    world/block/blocks/functional/JukeboxBlock.cpp
    world/block/blocks/functional/RespawnAnchorBlock.cpp
    world/block/blocks/functional/LodestoneBlock.cpp
    world/block/blocks/functional/CartographyTableBlock.cpp
    world/block/blocks/functional/CraftingTableBlock.cpp
    world/block/blocks/functional/FletchingTableBlock.cpp
    world/block/blocks/functional/SmithingTableBlock.cpp
    world/block/blocks/functional/AnvilBlock.cpp
    world/block/blocks/functional/TrailsBlocks.cpp

    # World - Block - Decorative
    world/block/blocks/decorative/PaneBlock.cpp
    world/block/blocks/decorative/CarpetBlock.cpp
    world/block/blocks/decorative/GlazedTerracottaBlock.cpp
    world/block/blocks/decorative/LanternBlock.cpp
    world/block/blocks/decorative/ChainBlock.cpp
    world/block/blocks/decorative/LadderBlock.cpp
    world/block/blocks/decorative/FlowerPotBlock.cpp
    world/block/blocks/decorative/ScaffoldingBlock.cpp
    world/block/blocks/decorative/CampfireBlock.cpp
    world/block/blocks/decorative/AbstractCandleBlock.cpp
    world/block/blocks/decorative/CandleBlock.cpp
    world/block/blocks/decorative/StainedGlassBlock.cpp
    world/block/blocks/decorative/BannerBlock.cpp
    world/block/blocks/decorative/TorchBlock.cpp
    world/block/blocks/decorative/WallTorchBlock.cpp

    # World - Block - Vegetation
    world/block/blocks/vegetation/DoublePlantBlock.cpp
    world/block/blocks/vegetation/TallGrassBlock.cpp
    world/block/blocks/vegetation/FlowerBlock.cpp
    world/block/blocks/vegetation/SaplingBlock.cpp
    world/block/blocks/vegetation/TreeGenerators.cpp
    world/block/blocks/vegetation/MushroomBlock.cpp
    world/block/blocks/vegetation/CactusBlock.cpp
    world/block/blocks/vegetation/SugarCaneBlock.cpp
    world/block/blocks/vegetation/VineBlock.cpp
    world/block/blocks/vegetation/LilyPadBlock.cpp
    world/block/blocks/vegetation/SweetBerryBushBlock.cpp
    world/block/blocks/vegetation/BambooBlock.cpp
    world/block/blocks/vegetation/LeavesBlock.cpp

    # World - Block - Mangrove
    world/block/blocks/mangrove/MangrovePropaguleBlock.cpp
    world/block/blocks/mangrove/MangroveRootsBlock.cpp

    # World - Block - Garden Awakening
    world/block/blocks/garden/CactusFlowerBlock.cpp
    world/block/blocks/garden/FlowerBedBlock.cpp
    world/block/blocks/garden/LeafLitterBlock.cpp

    # World - Block - Pale Garden
    world/block/blocks/pale_garden/CreakingHeartBlock.cpp
    world/block/blocks/pale_garden/EyeblossomBlock.cpp
    world/block/blocks/pale_garden/MossyCarpetBlock.cpp
    world/block/blocks/pale_garden/PaleHangingMossBlock.cpp

    # World - Block - Coral
    world/block/blocks/coral/CoralBlock.cpp

    # World - Block - Ice
    world/block/blocks/ice/IceBlock.cpp
    world/block/blocks/ice/SnowBlock.cpp

    # World - Block - Ocean
    world/block/blocks/ocean/SeaPickleBlock.cpp
    world/block/blocks/ocean/KelpBlock.cpp
    world/block/blocks/ocean/SeagrassBlock.cpp
    world/block/blocks/ocean/TallSeagrassBlock.cpp
    world/block/blocks/ocean/BubbleColumnBlock.cpp
    world/block/blocks/ocean/DriedKelpBlock.cpp
    world/block/blocks/ocean/ConduitBlock.cpp
    world/block/blocks/SignBlock.cpp
    world/block/blocks/HangingSignBlock.cpp
    world/block/blocks/LightningRodBlock.cpp

    # World - Block - Cave
    world/block/blocks/cave/AmethystBlock.cpp
    world/block/blocks/cave/AmethystClusterBlock.cpp
    world/block/blocks/cave/AzaleaBlock.cpp
    world/block/blocks/cave/BigDripleafBlock.cpp
    world/block/blocks/cave/BigDripleafStemBlock.cpp
    world/block/blocks/cave/BuddingAmethystBlock.cpp
    world/block/blocks/cave/CaveVinesBlock.cpp
    world/block/blocks/cave/CaveVinesPlantBlock.cpp
    world/block/blocks/cave/FrogspawnBlock.cpp
    world/block/blocks/growing_plant/GrowingPlantBlock.cpp
    world/block/blocks/growing_plant/GrowingPlantHeadBlock.cpp
    world/block/blocks/growing_plant/GrowingPlantBodyBlock.cpp
    world/block/blocks/cave/GlowLichenBlock.cpp
    world/block/blocks/cave/HangingRootsBlock.cpp
    world/block/blocks/cave/MossBlock.cpp
    world/block/blocks/cave/PointedDripstoneBlock.cpp
    world/block/blocks/cave/PowderSnowBlock.cpp
    world/block/blocks/cave/RootedDirtBlock.cpp
    world/block/blocks/cave/SmallDripleafBlock.cpp
    world/block/blocks/cave/SporeBlossomBlock.cpp

    # World - Block - Copper
    world/block/blocks/copper/CopperBulbBlock.cpp
    world/block/blocks/copper/CopperChestBlock.cpp
    world/block/blocks/copper/CopperGolemStatueBlock.cpp
    world/block/blocks/copper/IOxidizableBlock.cpp
    world/block/blocks/copper/WeatheringCopperBlock.cpp
    world/block/blocks/copper/WeatheringCopperBarsBlock.cpp
    world/block/blocks/copper/WeatheringCopperChainBlock.cpp
    world/block/blocks/copper/WeatheringCopperDoorBlock.cpp
    world/block/blocks/copper/WeatheringCopperGrateBlock.cpp
    world/block/blocks/copper/WeatheringCopperLanternBlock.cpp
    world/block/blocks/copper/WeatheringCopperSlabBlock.cpp
    world/block/blocks/copper/WeatheringCopperStairBlock.cpp
    world/block/blocks/copper/WeatheringCopperTrapDoorBlock.cpp
    world/block/blocks/copper/WeatheringLightningRodBlock.cpp

    # World - Block - Nether
    world/block/blocks/nether/EnderChestBlock.cpp
    world/block/blocks/nether/FireBlock.cpp
    world/block/blocks/nether/SoulFireBlock.cpp
    world/block/blocks/nether/NetherPortalBlock.cpp
    world/block/blocks/nether/NetherWartBlock.cpp
    world/block/blocks/nether/NetherRootsBlock.cpp
    world/block/blocks/nether/NetherSproutsBlock.cpp
    world/block/blocks/nether/MagmaBlock.cpp
    world/block/blocks/nether/NyliumBlock.cpp

    # World - Block - End
    world/block/blocks/end/EndPortalBlock.cpp
    world/block/blocks/end/EndPortalFrameBlock.cpp
    world/block/blocks/end/EndGatewayBlock.cpp
    world/block/blocks/end/ChorusPlantBlock.cpp
    world/block/blocks/end/ChorusFlowerBlock.cpp
    world/block/blocks/end/DragonEggBlock.cpp

    # World - Block - Special
    world/block/blocks/special/BarrierBlock.cpp
    world/block/blocks/special/StructureVoidBlock.cpp
    world/block/blocks/special/StructureBlock.cpp
    world/block/blocks/special/JigsawBlock.cpp
    world/block/blocks/special/CommandBlock.cpp
    world/block/blocks/special/RepeatingCommandBlock.cpp
    world/block/blocks/special/ChainCommandBlock.cpp
    world/block/blocks/special/SlimeBlock.cpp
    world/block/blocks/special/HoneyBlock.cpp
    world/block/blocks/special/SpongeBlock.cpp
    world/block/blocks/special/WetSpongeBlock.cpp
    world/block/blocks/special/WebBlock.cpp

    # World - Block - Mob
    world/block/blocks/mob/BeehiveBlock.cpp
    world/block/blocks/mob/TurtleEggBlock.cpp
    world/block/blocks/mob/InfestedBlock.cpp
    world/block/blocks/mob/InfestedRotatedPillarBlock.cpp
    world/block/blocks/mob/SpawnerBlock.cpp
    world/block/blocks/mob/DragonBreathBlock.cpp

    # World - Block - Sculk
    world/block/blocks/sculk/SculkBlocks.cpp
    world/block/blocks/sculk/SculkSpreader.cpp

    # World - Block - Trial
    world/block/blocks/trial/HeavyCoreBlock.cpp
    world/block/blocks/trial/TrialBlocks.cpp

    # World - Block - Redstone
    world/block/blocks/redstone/RedstoneBlock.cpp
    world/block/blocks/redstone/RedstoneTorchBlock.cpp
    world/block/blocks/redstone/RedstoneWallTorchBlock.cpp
    world/block/blocks/redstone/RedstoneWireBlock.cpp
    world/block/blocks/redstone/RedstoneDiodeBlock.cpp
    world/block/blocks/redstone/RedstoneRepeaterBlock.cpp
    world/block/blocks/redstone/RedstoneComparatorBlock.cpp
    world/block/blocks/redstone/ObserverBlock.cpp
    world/block/blocks/redstone/AbstractButtonBlock.cpp
    world/block/blocks/redstone/StoneButtonBlock.cpp
    world/block/blocks/redstone/WoodButtonBlock.cpp
    world/block/blocks/redstone/LeverBlock.cpp
    world/block/blocks/redstone/AbstractPressurePlateBlock.cpp
    world/block/blocks/redstone/StonePressurePlateBlock.cpp
    world/block/blocks/redstone/WoodPressurePlateBlock.cpp
    world/block/blocks/redstone/WeightedPressurePlateBlock.cpp
    world/block/blocks/redstone/DaylightDetectorBlock.cpp
    world/block/blocks/redstone/PistonBlock.cpp
    world/block/blocks/redstone/PistonStructureHelper.cpp
    world/block/blocks/redstone/PistonHeadBlock.cpp
    world/block/blocks/redstone/MovingPistonBlock.cpp
    world/block/blocks/redstone/DispenserBlock.cpp
    world/block/blocks/redstone/DropperBlock.cpp
    world/block/blocks/redstone/RedstoneLampBlock.cpp
    world/block/blocks/redstone/TripWireBlock.cpp
    world/block/blocks/redstone/TripWireHookBlock.cpp
    world/block/blocks/redstone/NoteBlock.cpp
    world/block/blocks/redstone/TNTBlock.cpp
    world/block/blocks/redstone/TargetBlock.cpp
    world/block/blocks/redstone/RedstoneOreBlock.cpp
    # World - Block - Redstone - Rails
    world/block/blocks/redstone/RailState.cpp
    world/block/blocks/redstone/AbstractRailBlock.cpp
    world/block/blocks/redstone/RailBlock.cpp
    world/block/blocks/redstone/PoweredRailBlock.cpp
    world/block/blocks/redstone/DetectorRailBlock.cpp
    world/block/blocks/redstone/ActivatorRailBlock.cpp

    # World - Block - Dispense
    world/block/dispense/IDispenseItemBehavior.cpp
    world/block/dispense/DispenseItemBehaviorRegistry.cpp

    # World - Block Entity
    world/blockentity/core/SimpleInventory.cpp
    world/blockentity/core/LockableBlockEntity.cpp
    world/blockentity/core/LootableContainerBlockEntity.cpp
    world/blockentity/core/BlockEntityRegistry.cpp
    world/blockentity/core/BlockEntityDeserializer.cpp
    world/blockentity/interactive/EnchantingTableEntity.cpp
    world/blockentity/interactive/EndGatewayEntity.cpp
    world/blockentity/interactive/DispenserBlockEntity.cpp
    world/blockentity/interactive/DropperBlockEntity.cpp
    world/blockentity/interactive/PistonBlockEntity.cpp
    world/blockentity/interactive/SignEntity.cpp
    world/blockentity/interactive/BannerEntity.cpp
    world/blockentity/interactive/BannerPattern.cpp
    world/blockentity/interactive/BrushableBlockEntity.cpp
    world/blockentity/interactive/DecoratedPotPattern.cpp
    world/blockentity/interactive/DecoratedPotBlockEntity.cpp
    world/blockentity/interactive/BeehiveBlockEntity.cpp
    world/blockentity/interactive/BellBlockEntity.cpp
    world/blockentity/interactive/BrushableBlockEntity.cpp
    world/blockentity/interactive/JukeboxEntity.cpp
    world/blockentity/interactive/LecternEntity.cpp
    world/blockentity/interactive/ShelfBlockEntity.cpp
    world/blockentity/interactive/CopperGolemStatueBlockEntity.cpp
    world/blockentity/storage/ChestEntity.cpp
    world/blockentity/storage/TrappedChestEntity.cpp
    world/blockentity/storage/DoubleSidedInventory.cpp
    world/blockentity/storage/EnderChestEntity.cpp
    world/blockentity/storage/ShulkerBoxEntity.cpp
    world/blockentity/storage/BarrelEntity.cpp
    world/blockentity/transport/HopperEntity.cpp
    world/blockentity/transport/IHopper.cpp
    world/blockentity/processing/AbstractFurnaceEntity.cpp
    world/blockentity/processing/FurnaceEntity.cpp
    world/blockentity/processing/BlastFurnaceEntity.cpp
    world/blockentity/processing/SmokerEntity.cpp
    world/blockentity/processing/FurnaceInventory.cpp
    world/blockentity/processing/BrewingStandEntity.cpp
    world/blockentity/processing/BeaconEntity.cpp
    world/blockentity/processing/ConduitEntity.cpp
    world/blockentity/processing/CampfireBlockEntity.cpp
    world/blockentity/redstone/ComparatorEntity.cpp
    world/blockentity/redstone/CommandBlockEntity.cpp
    world/blockentity/redstone/DaylightDetectorEntity.cpp

    # World - Block Entity - Trial
    world/blockentity/trial/TrialSpawnerBlockEntity.cpp
    world/blockentity/trial/VaultBlockEntity.cpp
    world/blockentity/trial/CrafterBlockEntity.cpp

    # World - Block Entity - Sculk
    world/blockentity/sculk/SculkSensorBlockEntity.cpp
    world/blockentity/sculk/SculkShriekerBlockEntity.cpp

    world/blockentity/spawner/MobSpawnerBlockEntity.cpp
    world/blockentity/spawner/SpawnerLogic.cpp

    # World - Chunk
    world/chunk/data/BiomeContainer.cpp
    world/chunk/data/ChunkData.cpp
    world/chunk/data/ChunkPrimer.cpp
    world/chunk/data/ChunkSection.cpp
    world/chunk/data/PalettedContainer.cpp
    world/chunk/data/Heightmap.cpp
    world/chunk/gen/ChunkDependencies.cpp
    world/chunk/gen/ChunkPyramid.cpp
    world/chunk/gen/ChunkStatus.cpp
    world/chunk/load/ChunkDistanceGraph.cpp
    world/chunk/load/ChunkLoadTicketManager.cpp

    # World - Game Event System
    # Note: .cpp files that depend on server headers are in src/server/world/gameevent/
    world/gameevent/GameEvents.cpp
    world/gameevent/VibrationSystem.cpp

    # World - Generation - Noise
    world/gen/noise/PerlinNoise.cpp
    world/gen/noise/PerlinSimplexNoise.cpp
    world/gen/noise/NormalNoise.cpp
    world/gen/noise/Noises.cpp
    world/gen/noise/NoiseLoader.cpp
    world/gen/noise/SimplexNoise.cpp

    # World - Generation - Density
    world/gen/density/BlendedNoise.cpp
    world/gen/density/DensityFunctions.cpp
    world/gen/density/DensityFunctionTypeRegistry.cpp
    world/gen/density/DensityFunctionLoader.cpp
    world/gen/density/DensityFunctionRegistry.cpp
    world/gen/density/NoiseBindingVisitor.cpp
    world/gen/density/NoiseChunk.cpp
    world/gen/density/NoiseRouter.cpp
    world/gen/density/TerrainProvider.cpp
    world/gen/density/Beardifier.cpp
    world/gen/density/OreVeinifier.cpp
    world/gen/RandomState.cpp

    # World - Generation - Chunk Generators
    world/gen/chunk/IChunkGenerator.cpp
    world/gen/chunk/NoiseChunkGenerator.cpp
    world/gen/chunk/DebugChunkGenerator.cpp
    world/gen/chunk/FlatChunkGenerator.cpp

    # World - Generation - Settings
    world/dimension/DimensionType.cpp
    world/dimension/Dimension.cpp
    world/dimension/DimensionManager.cpp
    world/dimension/end/EndDragonFight.cpp
    world/dimension/end/DragonRespawnAnimation.cpp
    world/dimension/teleport/PortalSize.cpp
    world/dimension/teleport/Teleporter.cpp

    # World - Map
    world/map/MaterialColor.cpp
    world/map/MapDecoration.cpp
    world/map/MapBanner.cpp
    world/map/MapFrame.cpp
    world/map/MapData.cpp
    world/map/MapIdTracker.cpp
    world/map/MapDataManager.cpp

    # World - Generation - Settings
    world/gen/settings/DimensionSettings.cpp
    world/gen/settings/FlatLevelGeneratorPresetLoader.cpp
    world/gen/settings/FlatLevelGeneratorPresetRegistry.cpp
    world/gen/settings/FlatLevelGeneratorSettings.cpp
    world/gen/settings/NoiseSettingsLoader.cpp
    world/gen/settings/NoiseSettingsRegistry.cpp
    world/gen/settings/WorldPreset.cpp
    world/gen/settings/WorldPresetLoader.cpp
    world/gen/settings/WorldPresetRegistry.cpp

    # World - Generation - Carvers
    world/gen/carver/WorldCarver.cpp
    world/gen/carver/CarvingContext.cpp
    world/gen/carver/CarvingMask.cpp
    world/gen/carver/CaveCarver.cpp
    world/gen/carver/CanyonCarver.cpp
    world/gen/carver/NetherWorldCarver.cpp
    world/gen/carver/ConfiguredCarverRegistry.cpp
    world/gen/carver/ConfiguredCarverLoader.cpp

    # World - Biome
    world/biome/Biome.cpp
    world/biome/BiomeClimate.cpp
    world/biome/BiomeManager.cpp
    world/biome/BiomeSource.cpp
    world/biome/BiomeRegistry.cpp
    world/biome/BiomeLoader.cpp
    world/biome/BiomeFactoryOverworld.cpp
    world/biome/BiomeFactoryNether.cpp
    world/biome/BiomeFactoryEnd.cpp
    world/biome/BiomeGenerationSettings.cpp
    world/biome/BiomeTag.cpp
    world/biome/BiomeTagLoader.cpp
    world/biome/BiomeTags.cpp

    # World - Biome - Climate
    world/biome/climate/ParameterPointCodec.cpp
    world/biome/climate/Sampler.cpp
    world/biome/climate/SpawnFinder.cpp

    # World - Biome - Source
    world/biome/source/MultiNoiseBiomeSource.cpp
    world/biome/source/OverworldBiomeBuilder.cpp
    world/biome/source/NetherBiomeBuilder.cpp
    world/biome/source/EndBiomeSource.cpp

    # World - Generation - Surface
    world/gen/surface/VerticalAnchor.cpp
    world/gen/surface/SurfaceCondition.cpp
    world/gen/surface/SurfaceRule.cpp
    world/gen/surface/SurfaceRuleContext.cpp
    world/gen/surface/SurfaceRuleDeserializer.cpp
    world/gen/surface/SurfaceSystem.cpp
    world/gen/surface/SurfaceRulesFactory.cpp

    # World - Generation - Aquifer
    world/gen/aquifer/Aquifer.cpp
    world/gen/aquifer/DisabledAquifer.cpp
    world/gen/aquifer/FluidPickerFactory.cpp
    world/gen/aquifer/FluidStatus.cpp
    world/gen/aquifer/NoiseBasedAquifer.cpp

    # World - Generation - Feature
    world/gen/feature/Feature.cpp
    world/gen/FeaturePlacer.cpp
    world/gen/feature/FeatureSpread.cpp
    world/gen/feature/FeatureSorter.cpp
    world/gen/feature/ConfiguredFeatureRegistry.cpp
    world/gen/feature/ConfiguredFeatureLoader.cpp
    world/gen/feature/FeatureTypeRegistry.cpp
    world/gen/feature/MonsterRoomFeature.cpp
    world/gen/feature/BlockColumnFeature.cpp
    world/gen/feature/SimpleBlockFeature.cpp
    world/gen/feature/RandomBooleanSelectorFeature.cpp
    world/gen/feature/SimpleRandomSelectorFeature.cpp
    world/gen/feature/RandomSelectorFeature.cpp
    world/gen/feature/EndPlatformFeature.cpp
    world/gen/feature/BonusChestFeature.cpp
    world/gen/feature/VoidStartPlatformFeature.cpp
    world/gen/feature/BlockBlobFeature.cpp
    world/gen/feature/BasaltPillarFeature.cpp
    world/gen/feature/RandomPatchFeature.cpp
    world/gen/feature/BlockPileFeature.cpp
    world/gen/feature/DiskFeature.cpp
    world/gen/feature/NetherForestVegetationFeature.cpp
    world/gen/feature/VinesFeature.cpp
    world/gen/feature/WeepingVinesFeature.cpp
    world/gen/feature/TwistingVinesFeature.cpp
    world/gen/feature/SpringFeature.cpp
    world/gen/feature/ScatteredOreFeature.cpp
    world/gen/feature/ReplaceBlobsFeature.cpp
    world/gen/feature/DripstoneUtils.cpp
    world/gen/feature/Column.cpp

    # World - Generation - Feature - Parser
    world/gen/feature/parser/BlockStateParser.cpp
    world/gen/feature/parser/BlockStateProviderParser.cpp
    world/gen/feature/parser/FluidStateParser.cpp
    world/gen/feature/parser/RuleTestParser.cpp
    world/gen/feature/parser/TrunkPlacerParser.cpp
    world/gen/feature/parser/FoliagePlacerParser.cpp
    world/gen/feature/parser/FeatureSizeParser.cpp
    world/gen/feature/parser/BlockPredicateParser.cpp

    # World - Generation - Feature - State
    world/gen/feature/state/BlockStateProvider.cpp
    world/gen/feature/state/RuleBasedBlockStateProvider.cpp
    world/gen/feature/state/RotatedBlockStateProvider.cpp
    world/gen/feature/state/NoiseThresholdBlockStateProvider.cpp
    world/gen/feature/state/NoiseBlockStateProvider.cpp
    world/gen/feature/state/DualNoiseBlockStateProvider.cpp
    world/gen/feature/state/RandomizedIntBlockStateProvider.cpp

    world/gen/feature/ore/OreFeature.cpp
    world/gen/feature/tree/TreeFeature.cpp
    world/gen/feature/tree/FallenTreeFeature.cpp
    world/gen/feature/tree/decorator/TreeDecorator.cpp
    world/gen/feature/tree/decorator/AttachToLogsDecorator.cpp
    world/gen/feature/tree/decorator/TrunkVineDecorator.cpp
    world/gen/feature/tree/trunk/TrunkPlacer.cpp
    world/gen/feature/tree/trunk/StraightTrunkPlacer.cpp
    world/gen/feature/tree/trunk/TrunkPlacers.cpp
    world/gen/feature/tree/trunk/CherryTrunkPlacer.cpp
    world/gen/feature/tree/trunk/UpwardsBranchingTrunkPlacer.cpp
    world/gen/feature/tree/foliage/FoliagePlacer.cpp
    world/gen/feature/tree/foliage/BlobFoliagePlacer.cpp
    world/gen/feature/tree/foliage/FoliagePlacers.cpp
    world/gen/feature/tree/foliage/CherryFoliagePlacer.cpp
    world/gen/feature/tree/foliage/RandomSpreadFoliagePlacer.cpp
    # World - Generation - Feature - Vegetation
    world/gen/feature/vegetation/FlowerFeature.cpp
    world/gen/feature/vegetation/BigMushroomFeature.cpp
    world/gen/feature/vegetation/BambooFeature.cpp

    # World - Generation - Feature - Cave
    world/gen/feature/cave/RootSystemFeature.cpp
    world/gen/feature/cave/VegetationPatchFeature.cpp
    world/gen/feature/cave/PointedDripstoneFeature.cpp
    world/gen/feature/cave/LargeDripstoneFeature.cpp
    world/gen/feature/cave/DripstoneClusterFeature.cpp
    world/gen/feature/cave/DesertWellFeature.cpp
    world/gen/feature/cave/IcebergFeature.cpp
    world/gen/feature/cave/FossilFeature.cpp
    world/gen/feature/cave/GeodeFeature.cpp
    world/gen/feature/cave/MultifaceGrowthFeature.cpp
    world/gen/feature/cave/SculkPatchFeature.cpp

    # World - Generation - Feature - Ocean
    world/gen/feature/ocean/KelpFeature.cpp
    world/gen/feature/ocean/SeagrassFeature.cpp
    world/gen/feature/ocean/SeaPickleFeature.cpp
    world/gen/feature/ocean/CoralFeature.cpp
    world/gen/feature/ocean/CoralTreeFeature.cpp
    world/gen/feature/ocean/CoralMushroomFeature.cpp
    world/gen/feature/ocean/CoralClawFeature.cpp
    world/gen/feature/ocean/BlueIceFeature.cpp

    # World - Generation - Feature - End
    world/gen/feature/end/EndSpikeFeature.cpp
    world/gen/feature/end/EndGatewayFeature.cpp
    world/gen/feature/end/IceSpikeFeature.cpp
    world/gen/feature/end/ChorusPlantFeature.cpp
    world/gen/feature/end/EndIslandFeature.cpp

    # World - Generation - Feature - Nether
    world/gen/feature/nether/HugeFungusFeature.cpp

    # World - Generation - Feature - Nether
    world/gen/feature/nether/GlowstoneFeature.cpp
    world/gen/feature/nether/BasaltFeature.cpp
    world/gen/feature/nether/DeltaFeature.cpp
    world/gen/feature/nether/UnderwaterMagmaFeature.cpp

    # World - Generation - Feature - Lake
    world/gen/feature/LakeFeature.cpp
    world/gen/feature/SnowAndFreezeFeature.cpp

    # World - Generation - Feature - Predicate
    world/gen/feature/predicate/BlockPredicate.cpp
    world/gen/feature/predicate/MatchingFluidsPredicate.cpp

    # World - Generation - Feature - Template
    world/gen/feature/template/Template.cpp
    world/gen/feature/template/TemplateManager.cpp
    world/gen/feature/template/TemplateLoader.cpp
    world/gen/feature/template/RuleTest.cpp
    world/gen/feature/template/CappedStructureProcessor.cpp
    world/gen/feature/template/ProtectedBlocksProcessor.cpp

    # World - Generation - Structure
    world/gen/structure/Structure.cpp
    world/gen/structure/StructureBoundingBox.cpp
    world/gen/structure/StructureDefinitionLoader.cpp
    world/gen/structure/StructureCheck.cpp
    world/gen/structure/StructureManager.cpp
    world/gen/structure/StructureSet.cpp
    world/gen/structure/StructureSetLoader.cpp
    world/gen/structure/StructureTag.cpp
    world/gen/structure/StructureTags.cpp
    world/gen/structure/StructureTagLoader.cpp
    world/gen/structure/StructureTypeRegistry.cpp
    world/gen/structure/placement/StructurePlacement.cpp
    world/gen/structure/placement/RandomSpreadStructurePlacement.cpp
    world/gen/structure/placement/ConcentricRingsStructurePlacement.cpp
    world/gen/structure/structures/RuinedPortalStructure.cpp
    world/gen/structure/structures/BuriedTreasureStructure.cpp
    world/gen/structure/structures/MineshaftStructure.cpp
    world/gen/structure/structures/VillageStructure.cpp
    world/gen/structure/structures/StrongholdStructure.cpp
    world/gen/structure/structures/StrongholdPieces.cpp
    world/gen/structure/structures/DesertPyramidStructure.cpp
    world/gen/structure/structures/JungleTempleStructure.cpp
    world/gen/structure/structures/OceanMonumentPieces.cpp
    world/gen/structure/structures/OceanMonumentStructure.cpp
    world/gen/structure/structures/ShipwreckStructure.cpp
    world/gen/structure/structures/OceanRuinStructure.cpp
    world/gen/structure/structures/FortressStructure.cpp
    world/gen/structure/structures/IglooStructure.cpp
    world/gen/structure/structures/SwampHutStructure.cpp
    world/gen/structure/structures/NetherFossilStructure.cpp
    world/gen/structure/structures/PillagerOutpostStructure.cpp
    world/gen/structure/structures/EndCityStructure.cpp
    world/gen/structure/structures/WoodlandMansionStructure.cpp
    world/gen/structure/structures/BastionRemnantStructure.cpp
    world/gen/structure/structures/TrialChambersStructure.cpp
    world/gen/structure/JigsawStructure.cpp

    # World - Generation - Jigsaw
    world/gen/jigsaw/JigsawAssembler.cpp
    world/gen/jigsaw/JigsawPiece.cpp
    world/gen/jigsaw/JigsawPlacer.cpp
    world/gen/jigsaw/EmptyJigsawPiece.cpp
    world/gen/jigsaw/SingleJigsawPiece.cpp
    world/gen/jigsaw/ListJigsawPiece.cpp
    world/gen/jigsaw/FeatureJigsawPiece.cpp
    world/gen/jigsaw/TemplatePool.cpp
    world/gen/jigsaw/TemplatePoolRegistry.cpp
    world/gen/jigsaw/JigsawTransform.cpp
    world/gen/jigsaw/ProcessorListLoader.cpp
    world/gen/jigsaw/TemplatePoolLoader.cpp
    world/gen/jigsaw/ProcessorListRegistry.cpp
    world/gen/jigsaw/PoolAliasBinding.cpp

    # World - Generation - Structure - Pools
    world/gen/structure/pools/Pools.cpp
    world/gen/structure/pools/ProcessorLists.cpp
    world/gen/structure/pools/village/VillagePools.cpp
    world/gen/structure/pools/bastion/BastionPools.cpp
    world/gen/structure/pools/pillager_outpost/PillagerOutpostPools.cpp
    world/gen/structure/pools/trial_chambers/TrialChambersPools.cpp

    # World - Generation - Placement
    world/gen/placement/BiomeFilterPlacement.cpp
    world/gen/placement/BlockPredicateFilterPlacement.cpp
    world/gen/placement/EnvironmentScanPlacement.cpp
    world/gen/placement/Placement.cpp
    world/gen/placement/PlacementUtils.cpp
    world/gen/placement/Placements.cpp
    world/gen/placement/PlacementRegistry.cpp
    world/gen/placement/PlacedFeature.cpp
    world/gen/placement/PlacedFeatureRegistry.cpp
    world/gen/placement/PlacedFeatureLoader.cpp

    # World - Generation - Value Provider
    world/gen/valueprovider/FloatProviderParser.cpp
    world/gen/valueprovider/HeightProviderParser.cpp
    world/gen/valueprovider/IntProviderParser.cpp

    # World - Generation - Spawn
    world/gen/spawn/WorldGenSpawner.cpp

    # World - Time
    world/time/GameTime.cpp

    # World - Weather
    world/weather/WeatherUtils.cpp

    # World - Border
    world/border/WorldBorder.cpp

    # World - Storage
    world/storage/GlobalStorageManager.cpp
    world/storage/SingleLevelStorageManager.cpp
    world/storage/entity/EntityStorageManager.cpp
    world/storage/blockentity/BlockEntityStorageManager.cpp
    world/storage/core/WorldStoragePaths.cpp
    world/storage/core/WorldSessionLock.cpp
    world/storage/core/LevelDatCodec.cpp
    world/storage/list/WorldListEntry.cpp
    world/storage/list/WorldListService.cpp
    world/storage/list/WorldNameSanitizer.cpp
    world/storage/request/WorldRequests.cpp
    world/storage/db/RocksDBDatabase.cpp
    world/storage/db/SectionCodec.cpp
    world/storage/section/SectionCache.cpp
    world/storage/section/SectionManager.cpp
    world/storage/snapshot/BackupManager.cpp
    world/storage/save/DirtyTracker.cpp
    world/storage/save/AutoSave.cpp
    world/storage/task/StorageTask.cpp
    world/storage/task/StorageTaskManager.cpp
    world/storage/player/PlayerSaveData.cpp
    world/storage/player/PlayerDataManager.cpp
    world/storage/backend/IStorageBackend.hpp
    world/storage/backend/JavaAnvilBackend.cpp
    world/storage/backend/BedrockLDBBackend.cpp
    world/storage/core/SaveFormat.cpp
    world/storage/reader/java/RegionFile.cpp
    world/storage/reader/java/JavaWorldReader.cpp
    world/storage/reader/java/JavaColumnReader.cpp
    world/storage/reader/java/JavaChunkReader.cpp
    world/storage/reader/java/JavaBlockStateMapper.cpp
    world/storage/reader/java/JavaBiomeMapper.cpp
    world/storage/reader/java/JavaLevelDatReader.cpp
    world/storage/reader/bedrock/BedrockLevelDb.cpp
    world/storage/reader/bedrock/LevelDBKey.cpp
    world/storage/reader/bedrock/PaletteUtil.cpp
    world/storage/reader/bedrock/BedrockWorldReader.cpp
    world/storage/reader/bedrock/BedrockColumnReader.cpp
    world/storage/reader/bedrock/BedrockChunkReader.cpp
    world/storage/reader/bedrock/BedrockBiomeMapper.cpp
    world/storage/reader/bedrock/BedrockLevelDatReader.cpp

    # World - Tick System
    world/tick/manager/TickManager.cpp
    # (header-only templates: TickPriority.hpp, ScheduledTick.hpp, ITickList.hpp, EmptyTickList.hpp, ServerTickList.hpp)

    # World - Redstone System
    world/redstone/RedstoneContext.cpp
    world/redstone/RedstonePower.cpp
    world/redstone/RedstoneSystem.cpp
    world/redstone/RedstoneHelper.cpp

    # World - Lighting System
    world/lighting/InternalLightUtils.cpp
    world/lighting/engine/BaseLightEngine.cpp
    world/lighting/engine/BlockLightEngine.cpp
    world/lighting/engine/SkyLightEngine.cpp
    world/lighting/engine/LightEngineUtils.cpp
    world/lighting/storage/EmptinessMap.cpp
    world/lighting/storage/SWMRNibbleArray.cpp
    world/lighting/manager/WorldLightManager.cpp
    # (header-only templates: LightType.hpp, IChunkLightProvider.hpp)

    # World - Fluid System
    world/fluid/Fluid.cpp
    world/fluid/FluidRegistry.cpp
    world/fluid/Fluids.cpp
    world/fluid/FluidTags.cpp
    world/fluid/FlowingFluid.cpp
    world/fluid/fluids/EmptyFluid.cpp
    world/fluid/fluids/WaterFluid.cpp
    world/fluid/fluids/LavaFluid.cpp

    # World - Explosion System
    world/explosion/ExplosionContext.cpp
    world/explosion/Explosion.cpp

    # World - Village System
    world/village/Village.cpp
    world/village/VillageManager.cpp
    world/village/VillageGossip.cpp
    world/village/VillageGossipType.cpp
    # World - Village - POI
    world/village/poi/PointOfInterest.cpp
    world/village/poi/PointOfInterestStorage.cpp
    world/village/poi/PointOfInterestType.cpp
    # World - Village - Trade
    world/village/trade/MerchantOffer.cpp
    world/village/trade/MerchantOffers.cpp
    world/village/trade/VillagerTrades.cpp
    world/village/trade/WanderingTraderTrades.cpp
    # World - Village - Raid
    world/village/raid/Raid.cpp
    world/village/raid/RaidManager.cpp
    world/village/raid/RaiderType.cpp

    # Physics
    physics/PhysicsEngine.cpp
    physics/CollisionCache.cpp
    physics/collision/CollisionShape.cpp
    # physics/collision/BlockCollision.cpp  # 已移除 - 使用新的方块系统
    physics/shape/DiscreteVoxelShape.cpp
    physics/shape/VoxelShape.cpp
    physics/shape/Shapes.cpp

    # Resource - Core
    resource/ResourceLocation.cpp
    resource/pack/PackMetadata.cpp
    resource/pack/IResourcePack.cpp
    resource/pack/FolderResourcePack.cpp
    resource/pack/ZipResourcePack.cpp
    resource/repository/PackListBase.cpp
    resource/repository/PackRepository.cpp
    resource/pack/InMemoryResourcePack.cpp
    resource/VanillaResources.cpp

    # Resource - Metadata
    resource/metadata/AnimationMetadata.cpp

    # Resource - Language
    resource/LanguageManager.cpp

    # NBT (Named Binary Tag) Library
    util/nbt/Nbt.cpp
    util/nbt/NbtJsonUtils.cpp

    # Text System (Rich Text)
    util/text/Utf8.cpp
    util/text/TextStyle.cpp
    util/text/ITextComponent.cpp
    util/text/TextParser.cpp
    util/text/TranslationTextComponent.cpp

    # NibbleArray (4-bit compact storage for lighting)
    util/NibbleArray.cpp

    # Property system
    util/property/EnumProperty.cpp

    # Cache utilities
    util/cache/Long2IntLRUCache.cpp
    util/cache/OpenAddressingLRUCache.cpp

    # Crypto utilities
    util/crypto/Sha256.cpp
    util/crypto/Sha1.cpp
    util/crypto/Md5.cpp

    # UUID utilities
    util/UuidUtils.cpp

    # Platform utilities
    util/PlatformInfo.cpp

    # Compression utilities
    util/CompressionUtils.cpp

    # Thread utilities
    util/thread/ServerWorkerPool.cpp

    # Concurrency utilities
    util/concurrent/ReentrantAreaLock.cpp

    # Assert library
    util/assert/Assert.cpp
    util/assert/CrashHandler.cpp

    # Sound system - Core
    sound/SoundCategory.cpp
    sound/SoundEvent.cpp
    sound/SoundEvents.cpp

    # Sound system - Jukebox
    sound/jukebox/JukeboxSong.cpp
    sound/jukebox/JukeboxSongs.cpp
    sound/jukebox/JukeboxSongPlayer.cpp

    # Sound system - Network
    sound/network/SoundPackets.cpp

    # Skin System - Core
    skin/core/SkinTypes.cpp
    skin/core/GameProfile.cpp
    skin/core/SkinTextures.cpp
    # Skin System - Manager
    skin/manager/SkinCache.cpp
    skin/manager/DefaultSkinProvider.cpp
    skin/manager/SkinManager.cpp
    # Skin System - Loader
    skin/loader/SkinLoader.cpp
    skin/loader/FileSkinLoader.cpp
    skin/loader/HttpSkinLoader.cpp
    # Skin System - Parser
    skin/parser/SkinMetadataParser.cpp
    # Skin System - Network
    skin/network/PlayerSkinInfo.cpp
    skin/network/SkinPackets.cpp

    # Advancement System - Core
    advancement/Advancement.cpp
    advancement/AdvancementDisplay.cpp
    advancement/AdvancementRewards.cpp
    advancement/AdvancementProgress.cpp
    advancement/AdvancementList.cpp
    advancement/AdvancementManager.cpp
    advancement/AdvancementLoader.cpp
    advancement/AdvancementVisibilityEvaluator.cpp
    advancement/Criterion.cpp
    advancement/trigger/CriterionTrigger.cpp
    advancement/trigger/CriterionTriggers.cpp
    advancement/trigger/conditions/ItemPredicate.cpp
    advancement/trigger/conditions/EntityPredicate.cpp
    advancement/trigger/conditions/EntityFlagsPredicate.cpp
    advancement/trigger/conditions/EntityEquipmentPredicate.cpp
    advancement/trigger/conditions/NBTPredicate.cpp
    advancement/trigger/conditions/EnchantmentPredicate.cpp
    advancement/trigger/conditions/LocationPredicate.cpp
    advancement/trigger/conditions/BlockPredicate.cpp
    advancement/trigger/conditions/MobEffectsPredicate.cpp
    # Trigger implementations
    advancement/trigger/impl/InventoryChangedTrigger.cpp
    advancement/trigger/impl/TickTrigger.cpp
    advancement/trigger/impl/EffectTriggers.cpp
    advancement/trigger/impl/LocationTrigger.cpp
    advancement/trigger/impl/PlayerKilledEntityTrigger.cpp
    advancement/trigger/impl/BlockTriggers.cpp
    advancement/trigger/impl/ItemTriggers.cpp
    advancement/trigger/impl/EntityTriggers.cpp
    advancement/trigger/impl/ChanneledLightningTrigger.cpp
    advancement/trigger/impl/AvoidVibrationTrigger.cpp

    # Scoreboard System - Core
    scoreboard/core/Scoreboard.cpp
    scoreboard/core/ScoreObjective.cpp
    scoreboard/core/Score.cpp
    scoreboard/core/ScoreCriteria.cpp
    scoreboard/core/ScoreCriteriaRenderType.cpp
    scoreboard/core/ScorePlayerTeam.cpp
    # Scoreboard System - Criteria
    scoreboard/criteria/DummyCriteria.cpp
    scoreboard/criteria/TriggerCriteria.cpp
    scoreboard/criteria/DeathCountCriteria.cpp
    scoreboard/criteria/KillCountCriteria.cpp
    scoreboard/criteria/ReadOnlyCriteria.cpp
    scoreboard/criteria/TeamKillCriteria.cpp
    # Scoreboard System - Network
    scoreboard/network/ScoreboardPackets.cpp
    # Scoreboard System - Storage
    scoreboard/storage/ScoreboardSaveData.cpp
    scoreboard/storage/ScoreboardDataManager.cpp
)

target_include_directories(mc_common
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(mc_common
    PUBLIC
        glm::glm
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        mc::profiler
    PRIVATE
        LibArchive::LibArchive
        ZLIB::ZLIB
        RocksDB::rocksdb
        zstd::libzstd
        leveldb::leveldb
)

mc_set_compiler_warnings(mc_common)

target_compile_features(mc_common PUBLIC cxx_std_20)

# 导出头文件
target_include_directories(mc_common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# 基岩版Addon兼容模组系统
add_subdirectory(mod/bedrock/addon)
