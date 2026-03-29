# tests/CMakeLists.txt
# 测试配置

find_package(GTest CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)

# 通用测试可执行文件
add_executable(mc_tests
    common/test_math.cpp
    common/test_network.cpp
    common/test_world.cpp
    common/test_core.cpp
    # common/test_blockid.cpp  # 已移除 - 使用新的方块系统
    # common/test_noise.cpp     # 已移除 - Noise 类已删除
    common/test_entity.cpp
    common/test_chunksync.cpp
    common/test_localconnection.cpp
    common/test_chunkloadticket.cpp
    # common/test_collision.cpp  # 暂时禁用 - 依赖旧的碰撞系统
    common/test_property.cpp
    common/test_block.cpp
    common/test_chunk_generation.cpp
    common/test_biome.cpp
    common/test_surface.cpp
    common/test_ore_feature.cpp
    common/test_carver.cpp
    common/test_tree_feature.cpp
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
    common/entity/inventory/CraftingInventoryTest.cpp
    common/entity/inventory/container/ChestContainerTest.cpp
    common/entity/inventory/container/HopperContainerTest.cpp
    common/entity/inventory/container/FurnaceContainerTest.cpp
    common/world/blockentity/BlockEntityTest.cpp
    common/world/blockentity/ChestEntityTest.cpp
    common/world/blockentity/FurnaceEntityTest.cpp
    common/world/blockentity/HopperEntityTest.cpp
    common/world/block/blocks/DoorBlockTest.cpp
    common/world/block/blocks/CauldronBlockTest.cpp
    common/world/block/blocks/EnchantingTableBlockTest.cpp
    common/world/block/blocks/ChestBlockTest.cpp
    common/world/block/blocks/HopperBlockTest.cpp
    common/command/test_command_dispatcher.cpp
    entity/EntityCoreTests.cpp
    entity/AttributeTests.cpp
    entity/LivingEntityTests.cpp
    entity/GoalTests.cpp
    entity/PathfindingTests.cpp
    entity/RandomWalkingGoalTest.cpp
    common/SettingsTest.cpp
    server/ServerWorldTest.cpp
    server/test_integrated_server.cpp
    server/BlockDropHandlerTest.cpp
    server/test_chunk_worker_pool.cpp
    server/test_server_chunk_manager.cpp
    server/core/TimeManagerTest.cpp
    server/core/PlayerManagerTest.cpp
    server/core/ConnectionManagerTest.cpp
    server/core/TeleportManagerTest.cpp
    server/core/KeepAliveManagerTest.cpp
    server/core/PositionTrackerTest.cpp
    server/weather/WeatherManagerTest.cpp
    client/test_mesh_worker_pool.cpp
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
    common/entity/EntitySpawnPlacementRegistryTest.cpp
    common/world/gen/ChunkSpawnIntegrationTest.cpp
    # Perfetto 测试（集成到主测试套件）
    common/perfetto/PerfettoTest.cpp
    # Tick 系统测试
    common/world/tick/ServerTickListTest.cpp
    # 流体系统测试
    common/world/fluid/FluidTest.cpp
    # 红石系统测试
    common/world/redstone/RedstoneContextTest.cpp
    common/world/redstone/RedstoneHelperTest.cpp
    common/world/redstone/RedstoneSystemTest.cpp
    common/world/redstone/RedstonePowerTest.cpp
    # 红石方块测试
    common/world/block/test_redstone_blocks.cpp
    # 射线检测测试
    common/math/RaycastTest.cpp
    # 实体生成回调测试
    server/ServerChunkManagerCallbackTest.cpp
    common/world/EntityManagerSpawnTest.cpp
    # 碰撞缓存测试
    physics/CollisionCacheTests.cpp
    # ServerWorld 碰撞检测测试
    server/ServerWorldCollisionTests.cpp
    # 实体追踪器测试
    server/world/EntityTrackerTest.cpp
    # 物品拾取管理器测试
    server/world/ItemPickupManagerTest.cpp
    # 资源包兼容层测试
    common/resource/compat/CompatLayerTest.cpp
    # 内置模型着色配置测试
    common/resource/VanillaResourcesTintModelTest.cpp
    # 缓存性能对比测试
    common/util/cache/CacheBenchmark.cpp
    # 生物群系层测试
    common/world/biome/layer/BiomeLayerTest.cpp
    common/world/biome/layer/MergeLayersTest.cpp
    # 植被特征集成测试
    common/world/gen/test_vegetation_features.cpp
    # 世界生成确定性测试
    common/world/gen/WorldGenDeterminismTest.cpp
    # 光照系统测试
    lighting/LightingTest.cpp
    lighting/LightUpdateTest.cpp
    # 环境光遮蔽测试
    client/renderer/AmbientOcclusionCalculatorTest.cpp
    # NaturalSpawner 测试
    server/world/spawn/NaturalSpawnerTest.cpp
    # 光照同步测试
    server/LightSyncTests.cpp
    # Kagero UI模板系统测试
    ui/kagero/template/template_test.cpp
    # Kagero UI布局系统测试
    client/ui/kagero/layout/MeasureSpecTest.cpp
    client/ui/kagero/layout/LayoutConstraintsTest.cpp
    client/ui/kagero/layout/FlexLayoutTest.cpp
    client/ui/kagero/layout/LayoutEngineTest.cpp
    client/ui/kagero/layout/FlexLayoutAlgorithmTest.cpp
    client/ui/kagero/layout/GridLayoutTest.cpp
    client/ui/kagero/layout/AnchorLayoutTest.cpp
    # Kagero UI状态系统测试
    client/ui/kagero/state/StateStoreTest.cpp
    client/ui/kagero/state/ReactiveStateTest.cpp
    client/ui/kagero/state/StateBindingTest.cpp
    client/ui/kagero/state/StateObserverTest.cpp
    # Kagero UI事件系统测试
    ui/kagero/event/event_test.cpp
    # Kagero UI Widget测试
    client/ui/kagero/widget/WidgetTest.cpp
    client/ui/kagero/widget/ButtonWidgetTest.cpp
    client/ui/kagero/widget/TextWidgetTest.cpp
    client/ui/kagero/widget/CheckboxWidgetTest.cpp
    client/ui/kagero/widget/SliderWidgetTest.cpp
    client/ui/kagero/widget/TextFieldWidgetTest.cpp
    client/ui/kagero/widget/ContainerWidgetTest.cpp
    client/ui/kagero/widget/ScrollableWidgetTest.cpp
    client/ui/kagero/widget/ListWidgetTest.cpp
    client/ui/kagero/widget/Viewport3DWidgetTest.cpp
    client/ui/kagero/widget/SlotWidgetTest.cpp
    # 附魔系统测试
    common/item/enchantment/EnchantmentTest.cpp
    # 自动跳跃测试
    common/entity/movement/AutoJumpTest.cpp
    # 玩家移动测试
    common/entity/PlayerMovementTest.cpp
    # Assert库测试
    common/util/assert/AssertTest.cpp
    # 调试区块生成器测试
    common/world/gen/DebugChunkGeneratorTest.cpp
    # 生物群系颜色测试
    client/world/color/BiomeColorsTest.cpp
    # 生物群系颜色混合测试
    client/world/color/blend/BiomeColorBlenderTest.cpp
    # 声音系统测试
    common/sound/SoundCategoryTest.cpp
    client/sound/SoundTest.cpp
)

# 添加服务端源文件
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
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/GameModeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TimeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/KillCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ListCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/HelpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SeedCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TeleportCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/GiveCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ClearCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WeatherCommand.cpp
        # 交互管理器
        ${CMAKE_SOURCE_DIR}/src/server/interaction/BlockInteractionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/MiningManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/ContainerManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/InventoryManager.cpp
        # 同步管理器
        ${CMAKE_SOURCE_DIR}/src/server/sync/EntitySyncManager.cpp
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
        # ${CMAKE_SOURCE_DIR}/src/common/world/BlockID.cpp  # 已移除 - 使用新的方块系统
        ${CMAKE_SOURCE_DIR}/src/common/entity/Player.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/Entity.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/movement/AutoJump.cpp
        ${CMAKE_SOURCE_DIR}/src/common/physics/PhysicsEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/common/physics/CollisionCache.cpp
        # ${CMAKE_SOURCE_DIR}/src/common/physics/collision/BlockCollision.cpp  # 已移除 - 使用新的方块系统
        # 新的方块系统源文件
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Material.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/Block.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/BlockRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/VanillaBlocks.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/AirBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/SimpleBlock.cpp
        ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/RotatedPillarBlock.cpp
        # 时间系统
        ${CMAKE_SOURCE_DIR}/src/common/world/time/GameTime.cpp
        # 物品系统
        ${CMAKE_SOURCE_DIR}/src/common/item/Item.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/ItemStack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/ItemRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/Items.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/BlockItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/BlockItemRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/ItemUseContext.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/BlockItemUseContext.cpp
        # 工具层级系统
        ${CMAKE_SOURCE_DIR}/src/common/item/tier/IItemTier.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tier/ItemTiers.cpp
        # 工具系统
        ${CMAKE_SOURCE_DIR}/src/common/item/tool/ToolType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tool/TieredItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tool/ToolItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tool/PickaxeItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tool/AxeItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tool/ShovelItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tool/HoeItem.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/tool/SwordItem.cpp
        # 背包系统
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/Slot.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/PlayerInventory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/Container.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/inventory/CraftingInventory.cpp
        # 合成系统
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/Ingredient.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeType.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/ShapedRecipe.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/ShapelessRecipe.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeSerializers.cpp
        ${CMAKE_SOURCE_DIR}/src/common/item/crafting/RecipeLoader.cpp
        # 方块实体系统
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
        # 物品实体和掉落表
        ${CMAKE_SOURCE_DIR}/src/common/entity/ItemEntity.cpp
        # 掉落系统
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootContext.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/RandomRanges.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootEntry.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootPool.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootTable.cpp
        ${CMAKE_SOURCE_DIR}/src/common/entity/loot/LootConditions.cpp
        # 方块掉落处理
        ${CMAKE_SOURCE_DIR}/src/server/world/drop/BlockDropHandler.cpp
        # 天体计算
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/sky/CelestialCalculations.cpp
        # Trident 渲染 API 层
        ${CMAKE_SOURCE_DIR}/src/client/renderer/api/Types.cpp
        # 射线检测
        ${CMAKE_SOURCE_DIR}/src/common/util/math/ray/Raycast.cpp
        # 设置系统
        ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
        # 网格构建线程池
        ${CMAKE_SOURCE_DIR}/src/client/renderer/mesh/MeshWorkerPool.cpp
        # 区块网格生成
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/ChunkMesher.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/AmbientOcclusionCalculator.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/MeshTypes.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/ModelRenderer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/EntityModel.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/model/AnimalModels.cpp
        # 区块模型缓存
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ResourceManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockStateLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/TextureAtlasBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemTextureAtlas.cpp
        # 资源包兼容层
        ${CMAKE_SOURCE_DIR}/src/common/resource/compat/PackFormat.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/compat/ResourceMapper.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/compat/TextureMapper.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/compat/v1_12/ResourceMapperV112.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/compat/v1_13/ResourceMapperV113.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/loader/ResourceLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/common/resource/VanillaResources.cpp
        # Kagero UI模板系统（仅核心解析组件）
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/Ast.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/AstVisitor.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/Lexer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/parser/Parser.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/binder/BindingContext.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/compiler/TemplateCompiler.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/template/runtime/UpdateScheduler.cpp
        # Kagero UI布局系统
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/layout/core/LayoutEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/layout/algorithms/FlexLayout.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/layout/algorithms/GridLayout.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/layout/algorithms/AnchorLayout.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/layout/integration/WidgetLayoutAdaptor.cpp
        # Kagero UI Widget组件
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/widget/ContainerWidget.cpp
        ${CMAKE_SOURCE_DIR}/src/client/ui/kagero/paint/PaintContext.cpp
        # Assert库
        ${CMAKE_SOURCE_DIR}/src/common/util/assert/Assert.cpp
        # 声音系统 - Common
        ${CMAKE_SOURCE_DIR}/src/common/sound/SoundCategory.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sound/SoundEvent.cpp
        ${CMAKE_SOURCE_DIR}/src/common/sound/network/SoundPackets.cpp
        # 声音系统 - Client
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
        # TemplateInstance.cpp 依赖 Widget 实现，在测试中暂时排除
)

target_link_libraries(mc_tests
    PRIVATE
        mc_common
        GTest::gtest
        GTest::gtest_main
        asio::asio
        nlohmann_json::nlohmann_json
        glm::glm
        spdlog::spdlog
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
        GTest::gtest
        GTest::gtest_main
        nlohmann_json::nlohmann_json
        glm::glm
        spdlog::spdlog
)

# 添加客户端资源源文件
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

# stb_image 实现放在TextureAtlasBuilder.cpp中

mc_set_compiler_warnings(mc_resource_tests)

# 发现测试
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
# Trident 渲染引擎测试
# ============================================================================

if(MC_BUILD_CLIENT)
    # Trident 渲染引擎测试可执行文件
    # 注意：只测试核心组件，不测试 TridentEngine（它依赖太多其他组件）
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
            # Trident 纹理
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/texture/TridentTexture.cpp
            # Trident 管线
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/pipeline/TridentPipeline.cpp
            # Trident 渲染管理器
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/FrameManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/UniformManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/DescriptorManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/RenderPassManager.cpp
            # Trident API 层
            ${CMAKE_SOURCE_DIR}/src/client/renderer/api/Types.cpp
            # 资源加载
            ${CMAKE_SOURCE_DIR}/src/client/resource/TextureAtlasBuilder.cpp
            ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelCache.cpp
            ${CMAKE_SOURCE_DIR}/src/client/resource/ResourceManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelLoader.cpp
            ${CMAKE_SOURCE_DIR}/src/client/resource/BlockStateLoader.cpp
            # 网格相关
            ${CMAKE_SOURCE_DIR}/src/client/renderer/MeshTypes.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/ChunkMesher.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/AmbientOcclusionCalculator.cpp
            # 生物群系颜色
            ${CMAKE_SOURCE_DIR}/src/client/world/color/BiomeColors.cpp
            ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/BiomeColorCache.cpp
            ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/BiomeColorBlender.cpp
            ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/ChunkBiomeAccessor.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/biome/BiomeEffects.cpp
            # 常用资源
            ${CMAKE_SOURCE_DIR}/src/common/resource/ResourceLocation.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/ResourcePackList.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/FolderResourcePack.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/ZipResourcePack.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/IResourcePack.cpp
            ${CMAKE_SOURCE_DIR}/src/common/resource/PackMetadata.cpp
            # 方块系统
            ${CMAKE_SOURCE_DIR}/src/common/world/block/Material.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/Block.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/BlockRegistry.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/VanillaBlocks.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/AirBlock.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/SimpleBlock.cpp
            ${CMAKE_SOURCE_DIR}/src/common/world/block/blocks/RotatedPillarBlock.cpp
            # 设置
            ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
    )

    target_link_libraries(mc_trident_tests
        PRIVATE
            mc_common
            GTest::gtest
            GTest::gtest_main
            glfw
            Vulkan::Vulkan
            nlohmann_json::nlohmann_json
            glm::glm
            spdlog::spdlog
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

    # GUI纹理系统测试（纯数据结构测试，不需要Vulkan）
    # 这些测试放在主测试套件中，避免Vulkan依赖
    target_sources(mc_tests
        PRIVATE
            # GuiSprite 和 GuiNinePatch 测试
            ${CMAKE_CURRENT_SOURCE_DIR}/client/renderer/trident/gui/GuiSpriteTest.cpp
            # GuiSpriteManager 测试（不依赖Vulkan）
            ${CMAKE_CURRENT_SOURCE_DIR}/client/renderer/trident/gui/GuiSpriteManagerTest.cpp
            # GuiSpriteParser JSON解析测试（不需要资源包）
            ${CMAKE_CURRENT_SOURCE_DIR}/client/renderer/trident/gui/GuiSpriteParserTest.cpp
            # 源文件（GuiSprite.hpp 是纯头文件）
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/gui/GuiSpriteManager.cpp
            ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/gui/GuiSpriteParser.cpp
    )

    # GUI纹理系统测试（需要Vulkan的测试暂时禁用）
    # GuiTextureAtlas 依赖 GuiRenderer 和 Vulkan，而其功能已被 GuiSpriteManager 测试覆盖
    # 如果需要测试 GuiTextureAtlas 的 Vulkan 功能，可以添加回：
    # target_sources(mc_trident_tests
    #     PRIVATE
    #         ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/gui/GuiTextureAtlas.cpp
    #         ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/gui/GuiSpriteRegistry.cpp
    #         ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/gui/GuiTextureLoader.cpp
    # )
endif()
