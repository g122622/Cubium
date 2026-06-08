# src/common/CMakeLists.txt
# 公共库 - 客户端和服务端共享

find_package(LibArchive REQUIRED)

# Perfetto 性能追踪子模块
add_subdirectory(perfetto)

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
    network/packet/DimensionPackets.cpp
    network/packet/ExperiencePackets.cpp
    network/packet/ParticlePacket.cpp
    network/packet/ExplosionPacket.cpp
    network/packet/SleepPacket.cpp
    network/packet/SpawnPositionPacket.cpp
    network/packet/MapDataPacket.cpp
    network/packet/SetPassengersPacket.cpp
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
    entity/core/EntityTypeIdNumber.cpp
    entity/core/EntityClassification.cpp
    entity/core/EntitySpawnPlacementRegistry.cpp
    entity/core/LivingEntity.cpp
    entity/core/MobEntity.cpp
    entity/core/CreatureEntity.cpp
    entity/core/AgeableEntity.cpp
    entity/core/FlyingEntity.cpp
    # Entity - Serialization
    entity/serialization/EntityDeserializer.cpp
    entity/serialization/NbtHelper.cpp
    # Entity - Effect
    entity/effect/EffectType.cpp
    entity/effect/EffectInstance.cpp
    entity/effect/EffectManager.cpp
    entity/effect/EffectAttributeModifiers.cpp
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
    # Entity - Passive Animals (water)
    entity/entities/passive/water/WaterMobEntity.cpp
    entity/entities/passive/water/SquidEntity.cpp
    entity/entities/passive/water/DolphinEntity.cpp
    entity/entities/passive/water/AxolotlEntity.cpp
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

    # Entity - Passive - Horse
    entity/entities/passive/horse/AbstractHorseEntity.cpp
    entity/entities/passive/horse/HorseEntity.cpp
    entity/entities/passive/horse/DonkeyEntity.cpp
    entity/entities/passive/horse/MuleEntity.cpp
    entity/entities/passive/horse/SkeletonHorseEntity.cpp
    entity/entities/passive/horse/ZombieHorseEntity.cpp
    entity/entities/passive/horse/LlamaEntity.cpp
    # Entity - Monster
    entity/interfaces/IFlinging.cpp
    entity/interfaces/IRideable.cpp
    entity/entities/monster/MonsterEntity.cpp
    # Entity - Monster (undead)
    entity/entities/monster/undead/ZombieEntity.cpp
    entity/entities/monster/undead/AbstractSkeletonEntity.cpp
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
    # Entity - Boss (EnderDragon is incomplete, Wither is complete)
    # entity/entities/boss/EnderDragonEntity.cpp
    entity/entities/boss/WitherEntity.cpp
    # Entity - Villager
    entity/entities/villager/AbstractVillagerEntity.cpp
    entity/entities/villager/VillagerEntity.cpp
    # Entity - Projectile (entity files have deep API dependencies - need systematic refactoring)
    entity/entities/projectile/ProjectileHelper.cpp
    entity/entities/projectile/ProjectileEntity.cpp
    entity/entities/projectile/DamagingProjectileEntity.cpp
    entity/entities/projectile/ThrowableEntity.cpp
    entity/entities/projectile/AbstractArrowEntity.cpp
    entity/entities/projectile/AbstractFireballEntity.cpp
    entity/entities/projectile/ProjectileItemEntity.cpp
    entity/entities/projectile/TridentEntity.cpp
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
    item/loot/conditions/SurvivesExplosionCondition.cpp
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
    entity/combat/CombatRules.cpp
    entity/damage/CombatEntry.cpp
    entity/damage/CombatTracker.cpp
    entity/ai/EntitySenses.cpp
    entity/ai/controller/LookController.cpp
    entity/ai/controller/MovementController.cpp
    entity/ai/controller/JumpController.cpp
    entity/ai/controller/VexMovementController.cpp
    entity/ai/controller/GhastMovementController.cpp
    entity/ai/goal/goals/SwimGoal.cpp
    entity/ai/goal/goals/RandomWalkingGoal.cpp
    entity/ai/goal/goals/LookAtGoal.cpp
    entity/ai/goal/goals/PanicGoal.cpp
    entity/ai/goal/goals/BreedGoal.cpp
    entity/ai/goal/goals/TemptGoal.cpp
    entity/ai/goal/goals/FollowParentGoal.cpp
    entity/ai/goal/goals/MeleeAttackGoal.cpp
    entity/ai/goal/goals/AvoidEntityGoal.cpp
    entity/ai/goal/goals/RandomSwimmingGoal.cpp
    entity/ai/goal/goals/FishSwimGoal.cpp
    entity/ai/goal/goals/FindWaterGoal.cpp
    entity/ai/goal/goals/SwimUpGoal.cpp
    entity/ai/goal/goals/EatGrassGoal.cpp
    entity/ai/goal/goals/interact/TameableGoals.cpp
    entity/ai/goal/goals/interact/LandOnOwnersShoulderGoal.cpp
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
    entity/ai/goal/goals/special/PhantomGoals.cpp
    entity/ai/goal/goals/special/IronGolemGoals.cpp
    entity/ai/goal/goals/special/ShulkerGoals.cpp
    entity/ai/goal/goals/special/EvokerGoals.cpp
    entity/ai/goal/goals/special/IllusionerGoals.cpp
    entity/ai/goal/goals/special/VexGoals.cpp
    entity/ai/goal/goals/special/EndermanGoals.cpp
    entity/ai/goal/goals/special/TurtleGoals.cpp
    entity/ai/goal/goals/special/WanderingTraderGoals.cpp
    entity/ai/goal/goals/special/BeeGoals.cpp
    entity/ai/goal/goals/special/FoxGoals.cpp
    entity/ai/goal/goals/special/PandaGoals.cpp
    entity/ai/goal/goals/special/MoveToLavaGoal.cpp
    entity/ai/goal/goals/special/PatrolGoals.cpp
    entity/ai/goal/goals/special/RavagerGoals.cpp
    entity/ai/goal/goals/special/GhastGoals.cpp
    entity/ai/goal/goals/special/SilverfishGoals.cpp
    entity/ai/goal/goals/AdditionalGoals.cpp
    # Entity - AI - Villager Goals
    entity/ai/goal/goals/villager/VillagerGoals.cpp
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
    entity/ai/util/RandomPositionGenerator.cpp
    entity/inventory/IInventory.cpp
    entity/inventory/Slot.cpp
    entity/inventory/PlayerInventory.cpp
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
    entity/inventory/container/AnvilContainer.cpp
    entity/inventory/container/LoomContainer.cpp

    # Item System - Core
    item/core/Item.cpp
    item/core/ItemStack.cpp
    item/core/ItemRegistry.cpp
    item/core/ItemGroup.cpp
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

    # Item System - Attributes
    item/attribute/ItemAttributeModifiers.cpp

    # Item System - Tags
    item/tag/ItemTag.cpp
    item/tag/ItemTags.cpp

    # Item System - Block Items
    item/items/block/BlockItem.cpp
    item/items/block/BlockItemRegistry.cpp
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
    item/items/weapon/CrossbowItem.cpp
    item/items/weapon/TridentItem.cpp
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
    item/items/special/BucketItem.cpp
    item/items/special/FishBucketItem.cpp
    item/items/special/FlintAndSteelItem.cpp
    item/items/special/MilkBucketItem.cpp
    item/items/special/EnchantedBookItem.cpp
    item/items/special/NameTagItem.cpp
    item/items/special/SaddleItem.cpp
    item/items/special/OnAStickItem.cpp
    item/items/special/StickItems.cpp

    # Item - Trial Items
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
    item/crafting/RecipeNetworkSerializer.cpp
    item/crafting/special/RepairItemRecipe.cpp
    item/crafting/special/ArmorDyeRecipe.cpp
    item/crafting/special/BookCloningRecipe.cpp
    item/crafting/special/MapCloningRecipe.cpp
    item/crafting/special/MapExtendingRecipe.cpp
    item/crafting/special/TippedArrowRecipe.cpp
    item/crafting/special/BannerDuplicateRecipe.cpp
    item/crafting/special/ShieldDecorationRecipe.cpp

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
    item/enchantment/enchantments/weapon/DamageEnchantment.cpp
    item/enchantment/enchantments/weapon/BaneOfArthropodsEnchantment.cpp
    item/enchantment/enchantments/tool/UnbreakingEnchantment.cpp

    # World - Entity
    world/entity/EntityManager.cpp

    # World - IWorld implementation
    world/IWorld.cpp

    # World - Game Rules
    world/gamerule/GameRule.cpp
    world/gamerule/GameRules.cpp

    # World - Spawn
    world/spawn/MobSpawnInfo.cpp

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
    command/arguments/NbtPath.cpp
    command/arguments/NbtPathArgumentType.cpp

    # World - Block System
    world/block/Material.cpp
    world/block/Block.cpp
    world/block/BlockState.cpp
    world/block/BlockRegistry.cpp
    world/block/BlockSoundType.cpp
    world/block/BlockTags.cpp
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
    world/block/IWaterLoggable.cpp
    world/block/blocks/AirBlock.cpp
    world/block/blocks/SimpleBlock.cpp
    world/block/blocks/FallingBlock.cpp
    world/block/blocks/RotatedPillarBlock.cpp
    world/block/blocks/DirectionalBlock.cpp
    world/block/blocks/HorizontalBlock.cpp
    world/block/blocks/LiquidBlock.cpp
    world/block/blocks/CauldronBlock.cpp
    world/block/blocks/EnchantingTableBlock.cpp
    world/block/blocks/AbstractFurnaceBlock.cpp
    world/block/blocks/FurnaceBlock.cpp
    world/block/blocks/BlastFurnaceBlock.cpp
    world/block/blocks/SmokerBlock.cpp
    world/block/blocks/ChestBlock.cpp
    world/block/blocks/TrappedChestBlock.cpp
    world/block/blocks/ShulkerBoxBlock.cpp
    world/block/blocks/HopperBlock.cpp

    # World - Block - Dirt
    world/block/blocks/dirt/SpreadableSnowyDirtBlock.cpp
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

    # World - Block - Functional
    world/block/blocks/functional/BedBlock.cpp
    world/block/blocks/functional/BrewingStandBlock.cpp
    world/block/blocks/functional/ComposterBlock.cpp
    world/block/blocks/functional/CompostableItems.cpp
    world/block/blocks/functional/CakeBlock.cpp
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
    world/block/blocks/functional/FletchingTableBlock.cpp
    world/block/blocks/functional/SmithingTableBlock.cpp
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
    world/block/blocks/decorative/StainedGlassBlock.cpp
    world/block/blocks/decorative/BannerBlock.cpp

    # World - Block - Vegetation
    world/block/blocks/vegetation/DoublePlantBlock.cpp
    world/block/blocks/vegetation/TallGrassBlock.cpp
    world/block/blocks/vegetation/FlowerBlock.cpp
    world/block/blocks/vegetation/SaplingBlock.cpp
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

    # World - Block - Pale Garden
    world/block/blocks/pale_garden/CreakingHeartBlock.cpp
    world/block/blocks/pale_garden/EyeblossomBlock.cpp
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
    world/block/blocks/copper/WeatheringCopperBlock.cpp
    world/block/blocks/copper/WeatheringCopperChainBlock.cpp
    world/block/blocks/copper/WeatheringCopperDoorBlock.cpp
    world/block/blocks/copper/WeatheringCopperGrateBlock.cpp
    world/block/blocks/copper/WeatheringCopperLanternBlock.cpp
    world/block/blocks/copper/WeatheringCopperSlabBlock.cpp
    world/block/blocks/copper/WeatheringCopperStairBlock.cpp
    world/block/blocks/copper/WeatheringCopperTrapDoorBlock.cpp

    # World - Block - Nether
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
    world/block/blocks/special/SpecialBlocks.cpp

    # World - Block - Mob
    world/block/blocks/mob/BeehiveBlock.cpp
    world/block/blocks/mob/TurtleEggBlock.cpp
    world/block/blocks/mob/InfestedBlock.cpp
    world/block/blocks/mob/SpawnerBlock.cpp
    world/block/blocks/mob/DragonBreathBlock.cpp

    # World - Block - Sculk
    world/block/blocks/sculk/SculkBlocks.cpp

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
    world/blockentity/interactive/JukeboxEntity.cpp
    world/blockentity/interactive/LecternEntity.cpp
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

    # World - Chunk
    world/chunk/ChunkData.cpp
    world/chunk/ChunkDistanceGraph.cpp
    world/chunk/ChunkLoadTicketManager.cpp
    world/chunk/ChunkStatus.cpp
    world/chunk/ChunkPrimer.cpp
    world/chunk/IChunk.cpp
    world/chunk/SingleChunkLifecycleManager.cpp

    # World - Generation - Noise
    world/gen/noise/PerlinNoise.cpp
    world/gen/noise/NormalNoise.cpp
    world/gen/noise/SimplexNoise.cpp

    # World - Generation - Density
    world/gen/density/BlendedNoise.cpp
    world/gen/density/DensityFunctions.cpp
    world/gen/density/NoiseChunk.cpp
    world/gen/density/NoiseRouter.cpp
    world/gen/density/NoiseRouterData.cpp
    world/gen/density/Beardifier.cpp
    world/gen/RandomState.cpp

    # World - Generation - Chunk Generators
    world/gen/chunk/IChunkGenerator.cpp
    world/gen/chunk/NoiseChunkGenerator.cpp
    world/gen/chunk/DebugChunkGenerator.cpp

    # World - Dimension
    world/dimension/DimensionType.cpp
    world/dimension/Dimension.cpp
    world/dimension/DimensionManager.cpp
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

    # World - Generation - Carvers
    world/gen/carver/WorldCarver.cpp
    world/gen/carver/CaveCarver.cpp
    world/gen/carver/CanyonCarver.cpp
    world/gen/carver/CarverConfiguration.cpp
    world/gen/carver/NetherCaveCarver.cpp

    # World - Biome
    world/biome/Biome.cpp
    world/biome/BiomeSource.cpp
    world/biome/BiomeRegistry.cpp
    world/biome/BiomeGenerationSettings.cpp

    # World - Biome - Climate
    world/biome/climate/Climate.cpp

    # World - Biome - Source
    world/biome/source/MultiNoiseBiomeSource.cpp
    world/biome/source/OverworldBiomeBuilder.cpp
    world/biome/source/NetherBiomeSource.cpp
    world/biome/source/EndBiomeSource.cpp

    # World - Generation - Surface
    world/gen/surface/SurfaceRules.cpp

    # World - Generation - Aquifer
    world/gen/aquifer/Aquifer.cpp

    # World - Generation - Feature
    world/gen/feature/Feature.cpp
    world/gen/feature/FeatureSpread.cpp
    world/gen/feature/ConfiguredFeature.cpp
    world/gen/feature/ore/OreFeature.cpp
    world/gen/feature/tree/TreeFeature.cpp
    world/gen/feature/tree/trunk/TrunkPlacer.cpp
    world/gen/feature/tree/trunk/StraightTrunkPlacer.cpp
    world/gen/feature/tree/trunk/TrunkPlacers.cpp
    world/gen/feature/tree/trunk/CherryTrunkPlacer.cpp
    world/gen/feature/tree/foliage/FoliagePlacer.cpp
    world/gen/feature/tree/foliage/BlobFoliagePlacer.cpp
    world/gen/feature/tree/foliage/FoliagePlacers.cpp
    world/gen/feature/tree/foliage/CherryFoliagePlacer.cpp
    # World - Generation - Feature - Vegetation
    world/gen/feature/vegetation/FlowerFeature.cpp
    world/gen/feature/vegetation/GrassFeature.cpp
    world/gen/feature/vegetation/BigMushroomFeature.cpp
    world/gen/feature/vegetation/CactusFeature.cpp
    world/gen/feature/vegetation/SugarCaneFeature.cpp
    world/gen/feature/vegetation/IceSpikeFeature.cpp

    # World - Generation - Feature - Cave
    world/gen/feature/cave/CaveFeatures.cpp
    world/gen/feature/cave/LushCavesFeatures.cpp

    # World - Generation - Feature - Ocean
    world/gen/feature/ocean/KelpFeature.cpp
    world/gen/feature/ocean/SeagrassFeature.cpp
    world/gen/feature/ocean/SeaPickleFeature.cpp
    world/gen/feature/ocean/CoralFeature.cpp
    world/gen/feature/ocean/OceanDecorationFeature.cpp
    world/gen/feature/ocean/BlueIceFeature.cpp

    # World - Generation - Feature - Spike
    world/gen/feature/spike/EndSpikeFeature.cpp

    # World - Generation - Feature - Gateway
    world/gen/feature/gateway/EndGatewayFeature.cpp

    # World - Generation - Feature - Fungus
    world/gen/feature/fungus/HugeFungusFeature.cpp

    # World - Generation - Feature - Nether
    world/gen/feature/nether/GlowstoneFeature.cpp
    world/gen/feature/nether/BasaltFeature.cpp
    world/gen/feature/nether/MagmaPatchFeature.cpp
    world/gen/feature/nether/NetherFeatures.cpp

    # World - Generation - Feature - Lake
    world/gen/feature/lake/LakeFeature.cpp

    # World - Generation - Feature - Predicate
    world/gen/feature/predicate/BlockPredicate.cpp

    # World - Generation - Feature - Template
    world/gen/feature/template/Template.cpp
    world/gen/feature/template/TemplateManager.cpp
    world/gen/feature/template/TemplateLoader.cpp
    world/gen/feature/template/RuleTest.cpp
    world/gen/feature/template/CopperBulbDegradationProcessor.cpp

    # World - Generation - Structure
    world/gen/structure/Structure.cpp
    world/gen/structure/StructureBoundingBox.cpp
    world/gen/structure/StructureManager.cpp
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
    world/gen/jigsaw/JigsawPiece.cpp
    world/gen/jigsaw/JigsawPattern.cpp
    world/gen/jigsaw/JigsawManager.cpp
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
    world/gen/placement/Placement.cpp
    world/gen/placement/PlacementUtils.cpp
    world/gen/placement/Placements.cpp
    world/gen/placement/PlacementRegistry.cpp

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
    # physics/collision/BlockCollision.cpp  # 已移除 - 使用新的方块系统
    physics/shape/DiscreteVoxelShape.cpp
    physics/shape/VoxelShape.cpp
    physics/shape/Shapes.cpp

    # Resource - Core
    resource/ResourceLocation.cpp
    resource/PackMetadata.cpp
    resource/IResourcePack.cpp
    resource/FolderResourcePack.cpp
    resource/ZipResourcePack.cpp
    resource/ResourcePackList.cpp
    resource/DataPackList.cpp
    resource/InMemoryResourcePack.cpp
    resource/VanillaResources.cpp

    # Resource - Metadata
    resource/metadata/AnimationMetadata.cpp

    # Resource - Language
    resource/LanguageManager.cpp

    # NBT (Named Binary Tag) Library
    util/nbt/Nbt.cpp

    # Text System (Rich Text)
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
    util/crypto/Md5.cpp

    # UUID utilities
    util/UuidUtils.cpp

    # Platform utilities
    util/PlatformInfo.cpp

    # Compression utilities
    util/CompressionUtils.cpp

    # Thread utilities
    util/thread/ServerWorkerPool.cpp

    # Assert library
    util/assert/Assert.cpp

    # Sound system - Core
    sound/SoundCategory.cpp
    sound/SoundEvent.cpp
    sound/SoundEvents.cpp

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
    advancement/Criterion.cpp
    advancement/trigger/CriterionTrigger.cpp
    advancement/trigger/CriterionTriggers.cpp
    advancement/trigger/conditions/ItemPredicate.cpp
    advancement/trigger/conditions/EntityPredicate.cpp
    advancement/trigger/conditions/EntityFlagsPredicate.cpp
    advancement/trigger/conditions/EntityEquipmentPredicate.cpp
    advancement/trigger/conditions/NBTPredicate.cpp
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
        mc::perfetto
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
