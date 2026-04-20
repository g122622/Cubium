# tests/CMakeLists.txt
# 娴嬭瘯閰嶇疆

find_package(GTest CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)

# 通用测试可执行文件
add_executable(mc_tests
    common/test_math.cpp
    common/test_network.cpp
    common/test_world.cpp
    common/test_core.cpp
    # common/test_blockid.cpp  # 宸茬Щ闄?- 浣跨敤鏂扮殑鏂瑰潡绯荤粺
    # common/test_noise.cpp     # 宸茬Щ闄?- Noise 绫诲凡鍒犻櫎
    common/test_entity.cpp
    common/test_chunksync.cpp
    common/test_localconnection.cpp
    common/test_chunkloadticket.cpp
    # common/test_collision.cpp  # 鏆傛椂绂佺敤 - 渚濊禆鏃х殑纰版挒绯荤粺
    common/test_property.cpp
    common/test_block.cpp
    common/test_chunk_generation.cpp
    common/test_biome.cpp
    common/test_surface.cpp
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
    common/item/crafting/RecipeManagerTest.cpp
    common/item/crafting/ShapedRecipeTest.cpp
    common/item/crafting/ShapelessRecipeTest.cpp
    common/item/crafting/RecipeLoaderTest.cpp
    common/item/tool/ToolTests.cpp
    common/item/potion/GlassBottleItemTest.cpp
    common/entity/inventory/CraftingInventoryTest.cpp
    common/entity/inventory/container/ChestContainerTest.cpp
    common/entity/inventory/container/HopperContainerTest.cpp
    common/entity/inventory/container/FurnaceContainerTest.cpp
    common/entity/entities/passive/basic/ChickenEntityTest.cpp
    # 缁忛獙绯荤粺娴嬭瘯
    common/entity/experience/ExperienceTest.cpp
    common/entity/experience/ExperienceManagerTest.cpp
    common/entity/experience/ExperienceOrbEntityTest.cpp
    common/world/blockentity/BlockEntityTest.cpp
    common/world/blockentity/ChestEntityTest.cpp
    common/world/blockentity/FurnaceEntityTest.cpp
    common/world/blockentity/HopperEntityTest.cpp
    common/world/blockentity/BlockEntityTodoTest.cpp
    common/world/block/blocks/DoorBlockTest.cpp
    common/world/block/blocks/CauldronBlockTest.cpp
    common/world/block/blocks/EnchantingTableBlockTest.cpp
    common/world/block/blocks/ChestBlockTest.cpp
    common/world/block/blocks/decorative/PaneBlockTest.cpp
    common/world/block/blocks/HopperBlockTest.cpp
    common/world/block/blocks/IceBlockTest.cpp
    common/world/block/blocks/VegetationBlockTest.cpp
    common/command/test_command_dispatcher.cpp
    entity/EntityCoreTests.cpp
    entity/EntitySensesTest.cpp
    entity/FlingingSupportTypesTest.cpp
    entity/AttributeTests.cpp
    entity/LivingEntityTests.cpp
    entity/AnimalSoundTests.cpp
    entity/GoalTests.cpp
    entity/BrainScheduleTests.cpp
    entity/DamagingProjectileSupportTypesTest.cpp
    entity/FishSupportTypesTest.cpp
    entity/HorseAppearanceSupportTypesTest.cpp
    entity/HorseSupportTypesTest.cpp
    entity/PatrollerSupportTypesTest.cpp
    entity/PlayerSupportTypesTest.cpp
    entity/PlayerPoseCollisionTest.cpp
    entity/ProjectileHelperTest.cpp
    entity/SkeletonSupportTypesTest.cpp
    entity/SpellcastingIllagerSupportTypesTest.cpp
    entity/SpawnLocationHelperTest.cpp
    entity/ShoulderRidingSupportTypesTest.cpp
    entity/TraderLlamaSupportTypesTest.cpp
    entity/PathfindingTests.cpp
    entity/RandomWalkingGoalTest.cpp
    entity/AiGoalRegressionTest.cpp
    common/SettingsTest.cpp
    server/ServerWorldTest.cpp
    server/ServerWorldBlockUpdateCallbackTest.cpp
    server/test_integrated_server.cpp
    server/BlockDropHandlerTest.cpp
    server/BlockInteractionManagerTest.cpp
    server/menu/CraftingMenuTest.cpp
    server/BlockUpdateSyncManagerTest.cpp
    server/test_chunk_worker_pool.cpp
    server/test_server_chunk_manager.cpp
    server/core/TimeManagerTest.cpp
    server/core/PlayerManagerTest.cpp
    server/core/ConnectionManagerTest.cpp
    server/core/TeleportManagerTest.cpp
    server/core/KeepAliveManagerTest.cpp
    server/core/PositionTrackerTest.cpp
    server/weather/WeatherManagerTest.cpp
    server/command/CommandRegistryTest.cpp
    client/test_mesh_worker_pool.cpp
    client/test_mesh_build_scheduler.cpp
    client/world/ClientWorldLightUpdateTest.cpp
    client/renderer/test_renderer.cpp
    client/renderer/test_trident_api.cpp
    client/renderer/test_cloud_renderer.cpp
    client/renderer/ChunkRendererStagingLayoutTest.cpp
    client/renderer/entity/AnimalModelTests.cpp
    client/renderer/entity/test_entity_renderer_manager.cpp
    client/renderer/test_gui_item_color.cpp
    client/resource/test_entity_texture_loader.cpp
    client/resource/ItemTextureAtlasTest.cpp
    client/resource/TextureAtlasBuilderTest.cpp
    client/resource/test_resource_manager_cloud_texture.cpp
    network/EntityPacketsTest.cpp
    network/LocalServerConnectionTest.cpp
    common/entity/loot/LootTest.cpp
    common/entity/loot/LootConditionTest.cpp
    common/world/gen/WorldGenSpawnerTest.cpp
    common/world/gen/OceanFeatureTest.cpp
    common/world/gen/WaterContentRegistryTest.cpp
    common/world/gen/NoiseSurfaceParityTest.cpp
    common/world/gen/NoiseChunkGeneratorCarverParityTest.cpp
    common/world/gen/NetherSurfaceParityTest.cpp
    common/world/gen/feature/NetherFeatureTest.cpp
    common/entity/EntitySpawnPlacementRegistryTest.cpp
    common/world/gen/ChunkSpawnIntegrationTest.cpp
    # Perfetto 娴嬭瘯锛堥泦鎴愬埌涓绘祴璇曞浠讹級
    common/perfetto/PerfettoTest.cpp
    # Tick 绯荤粺娴嬭瘯
    common/world/tick/ServerTickListTest.cpp
    # 娴佷綋绯荤粺娴嬭瘯
    common/world/fluid/FluidTest.cpp
    # 绾㈢煶绯荤粺娴嬭瘯
    common/world/redstone/RedstoneContextTest.cpp
    common/world/redstone/RedstoneHelperTest.cpp
    common/world/redstone/RedstoneSystemTest.cpp
    common/world/redstone/RedstonePowerTest.cpp
    # 绾㈢煶鏂瑰潡娴嬭瘯
    common/world/block/test_redstone_blocks.cpp
    # 灏勭嚎妫€娴嬫祴璇?    common/math/RaycastTest.cpp
    # 瀹炰綋鐢熸垚鍥炶皟娴嬭瘯
    server/ServerChunkManagerCallbackTest.cpp
    common/world/EntityManagerSpawnTest.cpp
    # 纰版挒缂撳瓨娴嬭瘯
    physics/CollisionCacheTests.cpp
    # ServerWorld 纰版挒妫€娴嬫祴璇?    server/ServerWorldCollisionTests.cpp
    # 瀹炰綋杩借釜鍣ㄦ祴璇?    server/world/EntityTrackerTest.cpp
    # 鐗╁搧鎷惧彇绠＄悊鍣ㄦ祴璇?    server/world/ItemPickupManagerTest.cpp
    # 璧勬簮鍖呭吋瀹瑰眰娴嬭瘯
    common/resource/compat/CompatLayerTest.cpp
    # 鍐呯疆妯″瀷鐫€鑹查厤缃祴璇?    common/resource/VanillaResourcesTintModelTest.cpp
    # ResourcePackList 自包含测试（不依赖外部资源路径）
    common/resource/ResourcePackListSelfContainedTest.cpp
    # 缂撳瓨鎬ц兘瀵规瘮娴嬭瘯
    common/util/cache/CacheBenchmark.cpp
    # 鐢熺墿缇ょ郴灞傛祴璇?    common/world/biome/layer/BiomeLayerTest.cpp
    common/world/biome/layer/MergeLayersTest.cpp
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
    # NaturalSpawner 娴嬭瘯
    server/world/spawn/NaturalSpawnerTest.cpp
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
    client/ui/kagero/widget/CheckboxWidgetTest.cpp
    client/ui/kagero/widget/SliderWidgetTest.cpp
    client/ui/kagero/widget/TextFieldWidgetTest.cpp
    client/ui/kagero/widget/ContainerWidgetTest.cpp
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
    # 鐜╁绉诲姩娴嬭瘯
    common/entity/PlayerMovementTest.cpp
    # Assert搴撴祴璇?    common/util/assert/AssertTest.cpp
    # 璋冭瘯鍖哄潡鐢熸垚鍣ㄦ祴璇?    common/world/gen/DebugChunkGeneratorTest.cpp
    # 鐢熺墿缇ょ郴棰滆壊娴嬭瘯
    client/world/color/BiomeColorsTest.cpp
    # 鐢熺墿缇ょ郴棰滆壊娣峰悎娴嬭瘯
    client/world/color/blend/BiomeColorBlenderTest.cpp
    # 绮掑瓙绯荤粺娴嬭瘯
    client/renderer/particle/ParticleTest.cpp
    client/renderer/particle/ParticleRegistryTest.cpp
    # 绗竴浜虹О鎵嬮儴娓叉煋娴嬭瘯
    client/renderer/firstperson/MatrixStackTest.cpp
    client/renderer/firstperson/FirstPersonRendererTest.cpp
    # 澹伴煶绯荤粺娴嬭瘯
    common/sound/SoundCategoryTest.cpp
    client/sound/SoundTest.cpp
    client/sound/SoundRegistryTest.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerWorld.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkWorkerPool.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerChunkManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/ItemPickupManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/NaturalSpawner.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/SpawnConditions.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/weather/WeatherManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/drop/BlockDropHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/player/ServerPlayer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpSession.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/TcpConnection.cpp
        ${CMAKE_SOURCE_DIR}/src/server/application/IntegratedServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/application/MinecraftServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/menu/CraftingMenu.cpp
        ${CMAKE_SOURCE_DIR}/src/server/settings/ServerSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/ServerCommandSource.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/CommandRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/PlayerResolver.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ClearCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WeatherCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ExperienceCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/KickCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SayCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/StopCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetIdleTimeoutCommand.cpp
        # 交互管理器
        ${CMAKE_SOURCE_DIR}/src/server/interaction/BlockInteractionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/MiningManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/ContainerManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/InventoryManager.cpp
        # 缁村害妯″潡
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
        ${CMAKE_SOURCE_DIR}/src/common/network/packet/Packet.cpp
        ${CMAKE_SOURCE_DIR}/src/common/network/packet/EntityPackets.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkData.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/NibbleArray.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkDistanceGraph.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkLoadTicketManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkStatus.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/ChunkPrimer.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/IChunk.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/chunk/SingleChunkLifecycleManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/noise/ImprovedNoiseGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/noise/OctavesNoiseGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/chunk/IChunkGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/chunk/NoiseChunkGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/chunk/DebugChunkGenerator.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/CaveCarver.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/CanyonCarver.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/carver/WorldCarver.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/Biome.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeProvider.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeEffects.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeGenerationSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/LayerContext.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/BiomeValues.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/LayerUtil.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/transformers/SourceLayers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/transformers/ClimateLayers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/transformers/EdgeLayers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/transformers/ZoomLayers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/transformers/BiomeLayers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/biome/layer/transformers/MergeLayers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/gen/surface/SurfaceBuilders.cpp
        # 生物群系颜色解析器
        ${CMAKE_SOURCE_DIR}/src/client/world/color/BiomeColors.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/BiomeColorCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/BiomeColorBlender.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/ChunkBiomeAccessor.cpp
        # 绮掑瓙绯荤粺
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/Particle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/ParticleRegistry.cpp
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
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/block/DiggingParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/FlameParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/SmokeParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/LavaParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/PortalParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/effect/CritParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/mob/HeartParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/liquid/DripParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/liquid/DripWaterParticle.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/particle/particles/weather/SplashParticle.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/entity/core/Entity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/movement/AutoJump.cpp
        ${CMAKE_SOURCE_DIR}/src/common/physics/PhysicsEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/common/physics/CollisionCache.cpp
        # ${CMAKE_SOURCE_DIR}/src/common/physics/collision/BlockCollision.cpp  # 宸茬Щ闄?- 浣跨敤鏂扮殑鏂瑰潡绯荤粺
        # 鏂扮殑鏂瑰潡绯荤粺婧愭枃浠?        ${CMAKE_SOURCE_DIR}/src/common/world/block/Material.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Block.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/BlockRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/VanillaBlocks.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/AirBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/SimpleBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/RotatedPillarBlock.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/Container.cpp
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
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/transport/HopperEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/transport/IHopper.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/CraftingTableEntity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/blockentity/interactive/EnchantingTableEntity.cpp
        # 鐗╁搧瀹炰綋鍜屾帀钀借〃
        ${CMAKE_SOURCE_DIR}/src/common/entity/entities/item/ItemEntity.cpp
        # 鎺夎惤绯荤粺
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootContext.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/RandomRanges.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootPool.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootTable.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootConditions.cpp
        # 鏂瑰潡鎺夎惤澶勭悊
        ${CMAKE_SOURCE_DIR}/src/server/world/drop/BlockDropHandler.cpp
        # 澶╀綋璁＄畻
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/sky/CelestialCalculations.cpp
        # Trident 渲染 API 层
        ${CMAKE_SOURCE_DIR}/src/client/renderer/api/Types.cpp
        # 射线检测
        ${CMAKE_SOURCE_DIR}/src/common/util/math/ray/Raycast.cpp
        # 璁剧疆绯荤粺
        ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/ClientWorld.cpp
        # 网格构建线程池
        ${CMAKE_SOURCE_DIR}/src/client/renderer/mesh/MeshWorkerPool.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/mesh/MeshBuildScheduler.cpp
        # 鍖哄潡缃戞牸鐢熸垚
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/ChunkMesher.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/AmbientOcclusionCalculator.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/MeshTypes.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/ModelRenderer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/EntityModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/AnimalModels.cpp
        # 绗竴浜虹О鎵嬮儴娓叉煋
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/firstperson/MatrixStack.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/firstperson/ItemCameraTransforms.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/firstperson/PlayerModel.cpp
        # 鍖哄潡妯″瀷缂撳瓨
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ResourceManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockStateLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/TextureAtlasBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemTextureAtlas.cpp
        # 璧勬簮鍖呭吋瀹瑰眰
        ${CMAKE_SOURCE_DIR}/src/common/resource/compat/PackFormat.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/compat/ResourceMapper.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/compat/TextureMapper.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/compat/v1_12/ResourceMapperV112.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/compat/v1_13/ResourceMapperV113.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/loader/ResourceLoader.cpp
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
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/paint/PaintContext.cpp
        # Assert搴?        ${CMAKE_SOURCE_DIR}/src/common/util/assert/Assert.cpp
        # 澹伴煶绯荤粺 - Common
        ${CMAKE_SOURCE_DIR}/src/common/sound/SoundCategory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sound/SoundEvent.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sound/network/SoundPackets.cpp
        # 澹伴煶绯荤粺 - Client
        ${CMAKE_SOURCE_DIR}/src/client/sound/resource/SoundDefinition.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/resource/SoundRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/SoundHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/SoundLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/StbVorbisImpl.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/SoundPool.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/SoundEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/instance/SoundInstance.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/backend/AudioBuffer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/backend/OpenALBackend.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/handler/BiomeAmbientHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/client/sound/handler/UnderwaterAmbientHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/client/command/ClientCommandManager.cpp
        # TemplateInstance.cpp 依赖 Widget 实现，暂时不纳入该测试目标
)

target_link_libraries(mc_tests
    PRIVATE
        mc_common
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
)

target_compile_definitions(mc_tests
    PRIVATE
        MC_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

mc_set_compiler_warnings(mc_tests)

# 资源测试可执行文件
add_executable(mc_resource_tests
    client/resource/test_resource_location.cpp
    client/resource/test_model_loader.cpp
)

target_link_libraries(mc_resource_tests
    PRIVATE
        mc_common
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
        ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/ResourcePackList.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/ZipResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/FolderResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/IResourcePack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/PackMetadata.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/ResourceLocation.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Block.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/BlockRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/VanillaBlocks.cpp
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
mc_set_compiler_warnings(mc_resource_tests)

# 鍙戠幇娴嬭瘯
include(GoogleTest)
gtest_discover_tests(mc_tests)
gtest_discover_tests(mc_resource_tests)

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
            ${CMAKE_SOURCE_DIR}/src/common/world/block/VanillaBlocks.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/AirBlock.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/SimpleBlock.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/RotatedPillarBlock.cpp
            # 璁剧疆
            ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
    )

    target_link_libraries(mc_trident_tests
        PRIVATE
            mc_common
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

    mc_set_compiler_warnings(mc_trident_tests)

    gtest_discover_tests(mc_trident_tests)

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
    server/command/CommandRegistryTest.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/player/ServerPlayer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/InventoryManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/weather/WeatherManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/ServerCommandSource.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/CommandRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/PlayerResolver.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ClearCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WeatherCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ExperienceCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SayCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/StopCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetIdleTimeoutCommand.cpp
)

target_link_libraries(mc_command_tests
    PRIVATE
        mc_common
        GTest::gtest_main
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

target_include_directories(mc_command_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

mc_set_compiler_warnings(mc_command_tests)

gtest_discover_tests(mc_command_tests)

set_target_properties(mc_command_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)




