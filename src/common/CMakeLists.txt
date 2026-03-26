# src/common/CMakeLists.txt
# 公共库 - 客户端和服务端共享

find_package(LibArchive REQUIRED)

# Perfetto 性能追踪子模块
add_subdirectory(perfetto)

add_library(mc_common STATIC
    # Core
    core/Result.cpp
    core/settings/SettingsBase.cpp

    # Input
    input/KeyBinding.cpp

    # Math
    util/math/MathUtils.cpp
    util/math/ray/Raycast.cpp

    # Math - Random
    util/math/random/IRandom.cpp
    util/math/random/Mt19937Random.cpp
    util/math/random/Xoroshiro128ppRandom.cpp
    util/math/random/Xoshiro256ppRandom.cpp
    util/math/random/LcgRandom.cpp
    util/math/random/UniformIntDistribution.cpp
    util/math/random/UniformRealDistribution.cpp

    # Network - Connection
    network/connection/LocalConnection.cpp
    network/connection/LocalServerConnection.cpp

    # Network - Packet
    network/packet/Packet.cpp
    network/packet/PacketSerializer.cpp
    network/packet/InventoryPackets.cpp
    network/packet/ContainerPacketHandler.cpp
    network/packet/EntityPackets.cpp
    network/packet/EntityMetadataSerializer.cpp
    network/packet/GameStateChangePacket.cpp
    network/packet/PlayerAbilitiesPacket.cpp
    network/packet/BlockBreakAnimPacket.cpp

    # Network - Sync
    network/sync/ChunkSync.cpp

    # Entity
    entity/Entity.cpp
    entity/EntityType.cpp
    entity/EntityClassification.cpp
    entity/EntitySpawnPlacementRegistry.cpp
    entity/Player.cpp
    entity/PlayerManager.cpp
    entity/ItemEntity.cpp
    entity/GameModeUtils.cpp
    entity/living/LivingEntity.cpp
    # Entity - Movement
    entity/movement/AutoJump.cpp
    entity/mob/MobEntity.cpp
    entity/mob/CreatureEntity.cpp
    entity/mob/AgeableEntity.cpp
    entity/animal/AnimalEntity.cpp
    entity/animal/PigEntity.cpp
    entity/animal/CowEntity.cpp
    entity/animal/SheepEntity.cpp
    entity/animal/ChickenEntity.cpp
    # Entity - Loot System
    entity/loot/LootContext.cpp
    entity/loot/RandomRanges.cpp
    entity/loot/LootEntry.cpp
    entity/loot/LootPool.cpp
    entity/loot/LootTable.cpp
    entity/loot/LootConditions.cpp
    entity/combat/AttackContext.cpp
    entity/combat/PlayerAttackHelper.cpp
    entity/damage/CombatEntry.cpp
    entity/damage/CombatTracker.cpp
    entity/ai/controller/LookController.cpp
    entity/ai/controller/MovementController.cpp
    entity/ai/controller/JumpController.cpp
    entity/ai/goal/goals/SwimGoal.cpp
    entity/ai/goal/goals/RandomWalkingGoal.cpp
    entity/ai/goal/goals/LookAtGoal.cpp
    entity/ai/goal/goals/PanicGoal.cpp
    entity/ai/goal/goals/BreedGoal.cpp
    entity/ai/goal/goals/TemptGoal.cpp
    entity/ai/goal/goals/FollowParentGoal.cpp
    entity/ai/goal/goals/MeleeAttackGoal.cpp
    entity/ai/goal/goals/AvoidEntityGoal.cpp
    entity/ai/pathfinding/PathPoint.cpp
    entity/ai/pathfinding/WalkNodeProcessor.cpp
    entity/ai/pathfinding/PathFinder.cpp
    entity/ai/pathfinding/PathNavigator.cpp
    entity/inventory/Slot.cpp
    entity/inventory/PlayerInventory.cpp
    entity/inventory/Container.cpp
    entity/inventory/CraftingInventory.cpp
    entity/inventory/AbstractContainerMenu.cpp

    # Item System
    item/Item.cpp
    item/ItemStack.cpp
    item/ItemRegistry.cpp
    item/Items.cpp
    item/BlockItem.cpp
    item/BlockItemRegistry.cpp
    item/ItemUseContext.cpp
    item/BlockItemUseContext.cpp

    # Item - Tier System
    item/tier/IItemTier.cpp
    item/tier/ItemTiers.cpp

    # Item - Tool System
    item/tool/ToolType.cpp
    item/tool/TieredItem.cpp
    item/tool/ToolItem.cpp
    item/tool/PickaxeItem.cpp
    item/tool/AxeItem.cpp
    item/tool/ShovelItem.cpp
    item/tool/HoeItem.cpp
    item/tool/SwordItem.cpp

    # Item - Crafting
    item/crafting/Ingredient.cpp
    item/crafting/RecipeType.cpp
    item/crafting/RecipeManager.cpp
    item/crafting/ShapedRecipe.cpp
    item/crafting/ShapelessRecipe.cpp
    item/crafting/RecipeSerializers.cpp
    item/crafting/RecipeLoader.cpp

    # Item - Enchantment System
    item/enchantment/Enchantment.cpp
    item/enchantment/EnchantmentRegistry.cpp
    item/enchantment/EnchantmentContainer.cpp
    item/enchantment/EnchantmentHelper.cpp
    item/enchantment/enchantments/FortuneEnchantment.cpp
    item/enchantment/enchantments/SilkTouchEnchantment.cpp

    # World - Entity
    world/entity/EntityManager.cpp

    # World - IWorld implementation
    world/IWorld.cpp

    # World - Spawn
    world/spawn/MobSpawnInfo.cpp

    # World - Block Entity
    world/blockentity/BlockEntityType.cpp
    world/blockentity/BlockEntity.cpp
    world/blockentity/CraftingTableEntity.cpp

    # Screen
    screen/ScreenType.cpp

    # World - Block System
    world/block/Material.cpp
    world/block/Block.cpp
    world/block/BlockRegistry.cpp
    world/block/VanillaBlocks.cpp
    world/block/blocks/AirBlock.cpp
    world/block/blocks/SimpleBlock.cpp
    world/block/blocks/RotatedPillarBlock.cpp
    world/block/blocks/LiquidBlock.cpp

    # World - Chunk
    world/chunk/ChunkData.cpp
    world/chunk/ChunkDistanceGraph.cpp
    world/chunk/ChunkLoadTicketManager.cpp
    world/chunk/ChunkStatus.cpp
    world/chunk/ChunkPrimer.cpp
    world/chunk/IChunk.cpp
    world/chunk/SingleChunkLifecycleManager.cpp

    # World - Generation - Noise
    world/gen/noise/ImprovedNoiseGenerator.cpp
    world/gen/noise/OctavesNoiseGenerator.cpp

    # World - Generation - Chunk Generators
    world/gen/chunk/IChunkGenerator.cpp
    world/gen/chunk/NoiseChunkGenerator.cpp

    # World - Generation - Settings
    world/gen/settings/DimensionSettings.cpp

    # World - Generation - Carvers
    world/gen/carver/WorldCarver.cpp
    world/gen/carver/CaveCarver.cpp
    world/gen/carver/CanyonCarver.cpp
    world/gen/carver/UnderwaterCarver.cpp

    # World - Biome
    world/biome/Biome.cpp
    world/biome/BiomeProvider.cpp
    world/biome/BiomeRegistry.cpp
    world/biome/BiomeGenerationSettings.cpp

    # World - Biome - Layer
    world/biome/layer/LayerContext.cpp
    world/biome/layer/BiomeValues.cpp
    world/biome/layer/LayerUtil.cpp
    world/biome/layer/transformers/TransformerTraits.cpp
    world/biome/layer/transformers/SourceLayers.cpp
    world/biome/layer/transformers/ClimateLayers.cpp
    world/biome/layer/transformers/EdgeLayers.cpp
    world/biome/layer/transformers/ZoomLayers.cpp
    world/biome/layer/transformers/BiomeLayers.cpp
    world/biome/layer/transformers/MergeLayers.cpp

    # World - Generation - Surface
    world/gen/surface/SurfaceBuilders.cpp

    # World - Generation - Feature
    world/gen/feature/Feature.cpp
    world/gen/feature/FeatureSpread.cpp
    world/gen/feature/ConfiguredFeature.cpp
    world/gen/feature/ore/OreFeature.cpp
    world/gen/feature/tree/TreeFeature.cpp
    world/gen/feature/tree/trunk/TrunkPlacer.cpp
    world/gen/feature/tree/trunk/StraightTrunkPlacer.cpp
    world/gen/feature/tree/trunk/TrunkPlacers.cpp
    world/gen/feature/tree/foliage/FoliagePlacer.cpp
    world/gen/feature/tree/foliage/BlobFoliagePlacer.cpp
    world/gen/feature/tree/foliage/FoliagePlacers.cpp
    # World - Generation - Feature - Vegetation
    world/gen/feature/vegetation/FlowerFeature.cpp
    world/gen/feature/vegetation/GrassFeature.cpp
    world/gen/feature/vegetation/BigMushroomFeature.cpp
    world/gen/feature/vegetation/CactusFeature.cpp
    world/gen/feature/vegetation/SugarCaneFeature.cpp
    world/gen/feature/vegetation/IceSpikeFeature.cpp

    # World - Generation - Feature - Lake
    world/gen/feature/lake/LakeFeature.cpp

    # World - Generation - Structure
    world/gen/structure/Structure.cpp
    world/gen/structure/StructureManager.cpp
    world/gen/structure/structures/RuinedPortalStructure.cpp
    world/gen/structure/structures/BuriedTreasureStructure.cpp
    world/gen/structure/structures/MineshaftStructure.cpp
    world/gen/structure/structures/VillageStructure.cpp
    world/gen/structure/structures/StrongholdStructure.cpp
    world/gen/structure/structures/DesertPyramidStructure.cpp
    world/gen/structure/structures/JungleTempleStructure.cpp
    world/gen/structure/structures/OceanMonumentStructure.cpp
    world/gen/structure/JigsawStructure.cpp

    # World - Generation - Jigsaw
    world/gen/jigsaw/JigsawPiece.cpp
    world/gen/jigsaw/JigsawPattern.cpp
    world/gen/jigsaw/JigsawManager.cpp

    # World - Generation - Template
    world/gen/feature/template/Template.cpp
    world/gen/feature/template/TemplateManager.cpp
    world/gen/feature/template/TemplateLoader.cpp

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

    # World - Tick System
    world/tick/manager/TickManager.cpp
    # (header-only templates: TickPriority.hpp, ScheduledTick.hpp, ITickList.hpp, EmptyTickList.hpp, ServerTickList.hpp)

    # World - Lighting System
    world/lighting/InternalLight.cpp
    world/lighting/engine/LevelBasedGraph.cpp
    world/lighting/engine/BlockLightEngine.cpp
    world/lighting/engine/SkyLightEngine.cpp
    world/lighting/engine/LightEngineUtils.cpp
    world/lighting/engine/LightEngineCache.cpp
    world/lighting/storage/BlockLightStorage.cpp
    world/lighting/storage/SkyLightStorage.cpp
    world/lighting/storage/EmptinessMap.cpp
    world/lighting/storage/SWMRNibbleArray.cpp
    world/lighting/manager/WorldLightManager.cpp
    # (header-only templates: LightType.hpp, IChunkLightProvider.hpp, SWMRLightDataMap.hpp, SectionLightStorage.hpp)

    # World - Fluid System
    world/fluid/Fluid.cpp
    world/fluid/FluidRegistry.cpp
    world/fluid/FluidTags.cpp
    world/fluid/FlowingFluid.cpp
    world/fluid/fluids/EmptyFluid.cpp
    world/fluid/fluids/WaterFluid.cpp
    world/fluid/fluids/LavaFluid.cpp

    # Physics
    physics/PhysicsEngine.cpp
    physics/CollisionCache.cpp
    # physics/collision/BlockCollision.cpp  # 已移除 - 使用新的方块系统

    # Resource - Core
    resource/ResourceLocation.cpp
    resource/PackMetadata.cpp
    resource/IResourcePack.cpp
    resource/FolderResourcePack.cpp
    resource/ZipResourcePack.cpp
    resource/ResourcePackList.cpp
    resource/InMemoryResourcePack.cpp
    resource/VanillaResources.cpp

    # Resource - Compatibility Layer
    resource/compat/PackFormat.cpp
    resource/compat/ResourceMapper.cpp
    resource/compat/TextureMapper.cpp
    resource/compat/v1_12/ResourceMapperV112.cpp
    resource/compat/v1_13/ResourceMapperV113.cpp

    # Resource - Loader
    resource/loader/ResourceLoader.cpp

    # NBT (Named Binary Tag) Library
    util/nbt/Nbt.cpp

    # NibbleArray (4-bit compact storage for lighting)
    util/NibbleArray.cpp

    # Cache utilities
    util/cache/Long2IntLRUCache.cpp
    util/cache/OpenAddressingLRUCache.cpp

    # Platform utilities
    util/PlatformInfo.cpp

    # Assert library
    util/assert/Assert.cpp

    # Sound system - Core
    sound/SoundCategory.cpp
    sound/SoundEvent.cpp

    # Sound system - Network
    sound/network/SoundPackets.cpp
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
)

mc_set_compiler_warnings(mc_common)

target_compile_features(mc_common PUBLIC cxx_std_17)

# 导出头文件
target_include_directories(mc_common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
