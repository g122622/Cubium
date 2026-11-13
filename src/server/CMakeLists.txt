# src/server/CMakeLists.txt
# 服务端可执行文件

# 命令实现源清单与 mc_benchmark 共用，避免两处清单漂移导致链接期 undefined symbol。
include(${CMAKE_SOURCE_DIR}/cmake/ServerCommandSources.cmake)

# ===========================================================================
# mc_gen - world/gen 实现 OBJECT 库
#
# 架构规则：src/common 不得依赖 src/server。world/gen 实现物理迁移到
# src/server/world/gen/ 后，既被 minecraft-server（生产服务端）编译，
# 也被 minecraft-client（内置服）/ mc_tests / mc_benchmark /
# mc_trident_tests 编译。这些 target 的 .o（含 mc_common 静态库中
# 违规引用 server gen 头的 TreeGenerators/FungusBlock/
# BigMushroomGenerators/SpreadableSnowyDirtBlock）需要 gen 符号实现。
# 抽成 OBJECT 库后，同一批 .o 被各 target 复用，避免重复编译与重复符号。
# ===========================================================================
add_library(mc_gen OBJECT
    world/gen/biome/BiomeLoader.cpp
    world/gen/biome/BiomeTagLoader.cpp
    world/gen/biome/source/EndBiomeSource.cpp
    world/gen/biome/source/MultiNoiseBiomeSource.cpp
    world/gen/noise/PerlinNoise.cpp
    world/gen/noise/NormalNoise.cpp
    world/gen/noise/Noises.cpp
    world/gen/noise/NoiseLoader.cpp
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
    world/gen/density/ast/AstNodes.cpp
    world/gen/density/ast/McToAst.cpp
    world/gen/density/ast/OptoPasses.cpp
    world/gen/density/ast/CompiledDensityFunction.cpp
    world/gen/density/ast/BytecodeGen.cpp
    world/gen/density/ast/DensityEvalHelpers.cpp
    world/gen/density/ast/DensityJitTrampolines.cpp
    world/gen/density/ast/DensityJitCompiler.cpp
    world/gen/RandomState.cpp
    world/gen/chunk/IChunkGenerator.cpp
    world/gen/chunk/NoiseChunkGenerator.cpp
    world/gen/chunk/DebugChunkGenerator.cpp
    world/gen/chunk/FlatChunkGenerator.cpp
    world/gen/chunk/ChunkPrimer.cpp
    world/gen/settings/DimensionSettings.cpp
    world/gen/settings/FlatLevelGeneratorPresetLoader.cpp
    world/gen/settings/FlatLevelGeneratorPresetRegistry.cpp
    world/gen/settings/FlatLevelGeneratorSettings.cpp
    world/gen/settings/NoiseSettingsLoader.cpp
    world/gen/settings/NoiseSettingsRegistry.cpp
    world/gen/settings/WorldPreset.cpp
    world/gen/settings/WorldPresetLoader.cpp
    world/gen/settings/WorldPresetRegistry.cpp
    world/gen/carver/WorldCarver.cpp
    world/gen/carver/CarvingContext.cpp
    world/gen/carver/CarvingMask.cpp
    world/gen/carver/CaveCarver.cpp
    world/gen/carver/CanyonCarver.cpp
    world/gen/carver/NetherWorldCarver.cpp
    world/gen/carver/ConfiguredCarverRegistry.cpp
    world/gen/carver/ConfiguredCarverLoader.cpp
    world/gen/surface/VerticalAnchor.cpp
    world/gen/surface/SurfaceCondition.cpp
    world/gen/surface/SurfaceRule.cpp
    world/gen/surface/SurfaceRuleContext.cpp
    world/gen/surface/SurfaceRuleDeserializer.cpp
    world/gen/surface/SurfaceSystem.cpp
    world/gen/surface/SurfaceRulesFactory.cpp
    world/gen/aquifer/Aquifer.cpp
    world/gen/aquifer/DisabledAquifer.cpp
    world/gen/aquifer/FluidPickerFactory.cpp
    world/gen/aquifer/FluidStatus.cpp
    world/gen/aquifer/NoiseBasedAquifer.cpp
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
    world/gen/feature/parser/BlockStateParser.cpp
    world/gen/feature/parser/BlockStateProviderParser.cpp
    world/gen/feature/parser/FluidStateParser.cpp
    world/gen/feature/parser/RuleTestParser.cpp
    world/gen/feature/parser/TrunkPlacerParser.cpp
    world/gen/feature/parser/FoliagePlacerParser.cpp
    world/gen/feature/parser/FeatureSizeParser.cpp
    world/gen/feature/parser/BlockPredicateParser.cpp
    world/gen/feature/state/BlockStateProvider.cpp
    world/gen/feature/state/RuleBasedBlockStateProvider.cpp
    world/gen/feature/state/RotatedBlockStateProvider.cpp
    world/gen/feature/state/NoiseThresholdBlockStateProvider.cpp
    world/gen/feature/state/NoiseBlockStateProvider.cpp
    world/gen/feature/state/DualNoiseBlockStateProvider.cpp
    world/gen/feature/state/RandomizedIntBlockStateProvider.cpp
    world/gen/feature/ore/OreFeature.cpp
    world/gen/feature/tree/TreeFeature.cpp
    world/gen/feature/tree/ServerTreeGenerators.cpp
    world/gen/feature/tree/FallenTreeFeature.cpp
    world/gen/feature/tree/decorator/TreeDecorator.cpp
    world/gen/feature/tree/decorator/AttachToLogsDecorator.cpp
    world/gen/feature/tree/decorator/PlaceOnGroundDecorator.cpp
    world/gen/feature/tree/decorator/SimpleTreeDecorators.cpp
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
    world/gen/feature/vegetation/FlowerFeature.cpp
    world/gen/feature/vegetation/BigMushroomFeature.cpp
    world/gen/feature/vegetation/ServerBigMushroomGenerators.cpp
    world/gen/feature/vegetation/BambooFeature.cpp
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
    world/gen/feature/ocean/KelpFeature.cpp
    world/gen/feature/ocean/SeagrassFeature.cpp
    world/gen/feature/ocean/SeaPickleFeature.cpp
    world/gen/feature/ocean/CoralFeature.cpp
    world/gen/feature/ocean/CoralTreeFeature.cpp
    world/gen/feature/ocean/CoralMushroomFeature.cpp
    world/gen/feature/ocean/CoralClawFeature.cpp
    world/gen/feature/ocean/BlueIceFeature.cpp
    world/gen/feature/end/EndSpikeFeature.cpp
    world/gen/feature/end/EndGatewayFeature.cpp
    world/gen/feature/end/IceSpikeFeature.cpp
    world/gen/feature/end/ChorusPlantFeature.cpp
    world/gen/feature/end/EndIslandFeature.cpp
    world/gen/feature/nether/HugeFungusFeature.cpp
    world/gen/feature/nether/GlowstoneFeature.cpp
    world/gen/feature/nether/BasaltFeature.cpp
    world/gen/feature/nether/DeltaFeature.cpp
    world/gen/feature/nether/UnderwaterMagmaFeature.cpp
    world/gen/feature/LakeFeature.cpp
    world/gen/feature/SnowAndFreezeFeature.cpp
    world/gen/feature/predicate/BlockPredicate.cpp
    world/gen/feature/predicate/MatchingFluidsPredicate.cpp
    world/gen/feature/template/Template.cpp
    world/gen/feature/template/TemplateManager.cpp
    world/gen/feature/template/TemplateManagerHostBinding.cpp
    world/gen/feature/template/TemplateLoader.cpp
    world/gen/feature/template/RuleTest.cpp
    world/gen/feature/template/CappedStructureProcessor.cpp
    world/gen/feature/template/ProtectedBlocksProcessor.cpp
    world/gen/structure/Structure.cpp
    world/gen/structure/StructureDefinitionLoader.cpp
    world/gen/structure/StructureCheck.cpp
    world/gen/structure/StructureManager.cpp
    world/gen/structure/StructurePlacementState.cpp
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
    world/gen/structure/structures/StrongholdStructure.cpp
    world/gen/structure/structures/StrongholdPieces.cpp
    world/gen/structure/structures/DesertPyramidStructure.cpp
    world/gen/structure/structures/JungleTempleStructure.cpp
    world/gen/structure/structures/OceanMonumentPieces.cpp
    world/gen/structure/structures/OceanMonumentStructure.cpp
    world/gen/structure/structures/ShipwreckStructure.cpp
    world/gen/structure/structures/OceanRuinStructure.cpp
    world/gen/structure/structures/FortressStructure.cpp
    world/gen/structure/structures/NetherFortressPieces.cpp
    world/gen/structure/structures/IglooStructure.cpp
    world/gen/structure/structures/SwampHutStructure.cpp
    world/gen/structure/structures/NetherFossilStructure.cpp
    world/gen/structure/structures/EndCityStructure.cpp
    world/gen/structure/structures/WoodlandMansionStructure.cpp
    world/gen/structure/JigsawStructure.cpp
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
    world/gen/jigsaw/TemplatePoolLoader.cpp
    world/gen/jigsaw/ProcessorListLoader.cpp
    world/gen/jigsaw/ProcessorListRegistry.cpp
    world/gen/jigsaw/PoolAliasBinding.cpp
    world/gen/structure/pools/Pools.cpp
    world/gen/structure/pools/ProcessorLists.cpp
    world/gen/structure/pools/village/VillagePools.cpp
    world/gen/structure/pools/bastion/BastionPools.cpp
    world/gen/structure/pools/pillager_outpost/PillagerOutpostPools.cpp
    world/gen/structure/pools/trial_chambers/TrialChambersPools.cpp
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
    world/gen/valueprovider/FloatProviderParser.cpp
    world/gen/valueprovider/HeightProviderParser.cpp
    world/gen/valueprovider/IntProviderParser.cpp
    world/gen/spawn/WorldGenSpawner.cpp
)

# 噪声与密度函数的求值必须逐位对齐原版。全局 -ffast-math 允许编译器重排 `a + b*c`、
# 把同一表达式在释放点重新求值，实测会让 perlin_noise / normal_noise / noise_density
# 的末位与原版差 1~37 ulp（137 处）。这些文件关掉 fast-math，其余文件不受影响。
set_source_files_properties(
    world/gen/noise/PerlinNoise.cpp
    world/gen/noise/NormalNoise.cpp
    world/gen/density/BlendedNoise.cpp
    PROPERTIES COMPILE_OPTIONS "-fno-fast-math"
)

# mc_gen 的 .cpp 重度依赖 common 头（Result.hpp/IWorld/各 world 类），并传递引入
# spdlog/nlohmann_json/simdjson/asmjit/fmt 等第三方头（均由 mc_common PUBLIC 链接）。
# 故 mc_gen 须 PRIVATE 链接 mc_common：编译 mc_gen 自身 .cpp 时需要 common 符号与头路径，
# 但 mc_gen 是纯实现 OBJECT 库，不对外暴露头接口，故用 PRIVATE 避免向使用方传递
# mc_common 链接（minecraft-server / minecraft-client / mc_tests / mc_benchmark /
# mc_trident_tests 本就各自直接链接 mc_common，传递会造成 "duplicate libraries" 警告）。
target_link_libraries(mc_gen PRIVATE mc_common PUBLIC absl::flat_hash_map)

target_include_directories(mc_gen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

mc_set_compiler_warnings(mc_gen)

# mc_server_storage - world/storage 实现 OBJECT 库（含 RocksDB 运行时层与纯文件系统层）。
# 详见 src/server/world/storage/CMakeLists.txt。在 minecraft-server 定义之前引入，确保
# minecraft-server 能链接 mc_server_storage。
add_subdirectory(world/storage)

# 交互式终端（--interactive）依赖 FTXUI。vcpkg 导出的是组件目标 ftxui::{screen,dom,component}，
# 不含 umbrella 别名 ftxui::ftxui，故使用组件目标 ftxui::component（它公开依赖 ftxui::dom）。
#
# 注意：FTXUI 的导出配置在 Windows 上带 INTERFACE_COMPILE_OPTIONS "/utf-8"（MSVC 语法），
# 而本项目用 clang++（GNU 风格参数），直接 link 会把 "/utf-8" 当成文件参数而报
# "clang++: error: no such file or directory: '/utf-8'"。源码是纯 ASCII、项目其它目标
# 均不依赖该选项，故清除它；其余 FTXUI 用法不受影响。
find_package(ftxui CONFIG REQUIRED)
foreach(_ftxui_target ftxui::screen ftxui::dom ftxui::component)
    if(TARGET ${_ftxui_target})
        set_property(TARGET ${_ftxui_target} PROPERTY INTERFACE_COMPILE_OPTIONS "")
    endif()
endforeach()

# ===========================================================================
# mc_server_shared - minecraft-server 与 mc_tests 共用的服务端实现 OBJECT 库
#
# 这些 .cpp 原先在可执行文件与 mc_tests 里各编译一遍（mc_tests 侧共 182 个文件），
# 同一份实现产生两份 .o、两份调试信息与两倍编译时间。抽成 OBJECT 库后只编译一次。
#
# 用 OBJECT 而不是 STATIC：服务端存在依赖静态初始化顺序的注册表装配，静态库下
# 未被引用的 .o 会被链接器整段丢弃，行为会静默改变。
#
# 只收「无进程入口、无 FTXUI 依赖」的文件：main.cpp / ServerApplicationEntry.cpp /
# StandaloneServer.cpp 与 application/console/ui/** 留在可执行文件内部，因为
# mc_tests 不链接 FTXUI。
# ===========================================================================
add_library(mc_server_shared OBJECT
    ${MC_SERVER_COMMAND_SOURCES}
    access/AccessManager.cpp
    access/admission/BannedIpList.cpp
    access/admission/BannedPlayerList.cpp
    access/admission/WhitelistManager.cpp
    access/permission/OpListManager.cpp
    advancement/PlayerAdvancements.cpp
    application/IntegratedServer.cpp
    application/MinecraftServer.cpp
    application/console/bridge/ConsoleCommandQueue.cpp
    application/console/buffer/EventRingBuffer.cpp
    application/console/buffer/LogRingBuffer.cpp
    application/console/input/CommandCompletion.cpp
    application/console/render/LineWrapper.cpp
    application/console/render/PanelStats.cpp
    application/console/render/PanelStatsCollector.cpp
    bossbar/BossInfo.cpp
    bossbar/CustomServerBossInfo.cpp
    bossbar/CustomServerBossInfoManager.cpp
    bossbar/ServerBossInfo.cpp
    bossbar/ServerDragonBossBar.cpp
    command/CommandRegistry.cpp
    command/ServerCommandSource.cpp
    command/data/DataAccessor.cpp
    command/support/ChatBroadcast.cpp
    command/support/EffectResolver.cpp
    command/support/EntityResolver.cpp
    command/support/FunctionSuggestionProvider.cpp
    command/support/PlayerResolver.cpp
    command/support/SpreadAlgorithm.cpp
    dimension/ServerDimension.cpp
    dimension/ServerDimensionManager.cpp
    event/ServerEventBus.cpp
    function/CommandFunction.cpp
    function/FunctionLoader.cpp
    function/FunctionManager.cpp
    function/MacroFunction.cpp
    function/StringTemplate.cpp
    function/TimerQueue.cpp
    interaction/BlockInteractionManager.cpp
    interaction/ContainerManager.cpp
    interaction/DigSpeed.cpp
    interaction/InventoryManager.cpp
    interaction/MiningManager.cpp
    interaction/SignCommandHelper.cpp
    item/instrument/InstrumentLoader.cpp
    item/instrument/InstrumentTagLoader.cpp
    menu/CraftingMenu.cpp
    mod/bedrock/addon/ServerScriptManager.cpp
    mod/bedrock/addon/bridge/EventBridge.cpp
    mod/bedrock/addon/bridge/ServerEventSignals.cpp
    network/base/ServerClientConnection.cpp
    network/handshake/EnchantmentNbtBuilder.cpp
    network/handshake/LoginFlow.cpp
    network/handshake/RegistryDataBuilder.cpp
    network/handshake/ServerHandshake.cpp
    network/outbound/DialogPacketBuilder.cpp
    network/outbound/MapPacketBuilder.cpp
    network/outbound/PacketBuilders.cpp
    network/outbound/PacketSender.cpp
    network/outbound/PlayerBroadcaster.cpp
    network/play/BlockActionHandler.cpp
    network/play/ChatHandler.cpp
    network/play/ContainerActionHandler.cpp
    network/play/EntityActionHandler.cpp
    network/play/MovementHandler.cpp
    network/play/PlayerStateHandler.cpp
    network/play/ServerPlayHandler.cpp
    network/play/SessionSignalHandler.cpp
    network/session/ClientSession.cpp
    network/session/ClientSessionManager.cpp
    network/session/PlayerConnectionManager.cpp
    network/session/ServerNetwork.cpp
    network/session/keepalive/KeepAliveManager.cpp
    network/sync/BlockUpdateSyncManager.cpp
    network/sync/ChunkSendManager.cpp
    network/sync/WeatherSyncService.cpp
    network/sync/chunk/ChunkSyncManager.cpp
    network/sync/chunk/PlayerChunkTracker.cpp
    network/sync/player/TeleportManager.cpp
    player/PlayerRespawnManager.cpp
    player/ServerPlayer.cpp
    player/gamemode/GameModeManager.cpp
    player/lifecycle/PlayerManager.cpp
    player/movement/PlayerMovementManager.cpp
    registry/RegistryBootstrap.cpp
    scoreboard/ServerScoreboard.cpp
    scoreboard/storage/ScoreboardDataManager.cpp
    scoreboard/storage/ScoreboardSaveData.cpp
    settings/ServerSettings.cpp
    stats/Stat.cpp
    stats/StatRegistry.cpp
    stats/StatType.cpp
    stats/StatisticsManager.cpp
    test/base/coords/TestTransform.cpp
    test/base/data/TestData.cpp
    test/base/error/GameTestError.cpp
    test/facade/GameTestCommand.cpp
    test/facade/GameTestHelper.cpp
    test/facade/GameTestRegistrar.cpp
    test/facade/GameTestServer.cpp
    test/framework/batch/BaseGameTestBatchRunner.cpp
    test/framework/batch/GameTestBatch.cpp
    test/framework/environment/AllOfEnvironment.cpp
    test/framework/environment/EnvironmentRegistry.cpp
    test/framework/environment/FunctionsEnvironment.cpp
    test/framework/environment/SetGameRulesEnvironment.cpp
    test/framework/environment/TimeOfDayEnvironment.cpp
    test/framework/environment/WeatherEnvironment.cpp
    test/framework/function/BaseGameTestFunction.cpp
    test/framework/helper/NullGameTestHelper.cpp
    test/framework/instance/BaseGameTestInstance.cpp
    test/framework/registry/GameTestRegistry.cpp
    test/framework/sequence/GameTestSequence.cpp
    test/framework/ticker/GameTestTicker.cpp
    test/minecraft/batch/MinecraftGameTestBatchRunner.cpp
    test/minecraft/environment/MinecraftEnvironmentApplier.cpp
    test/minecraft/helper/MinecraftGameTestHelperProvider.cpp
    test/minecraft/instance/MinecraftGameTestInstance.cpp
    test/minecraft/listener/WorldVisualizationListener.cpp
    test/minecraft/structure/BehaviorPackStructureSource.cpp
    test/minecraft/structure/GameTestStructureBootstrap.cpp
    test/minecraft/structure/MinecraftStructurePlacer.cpp
    test/minecraft/structure/StructureBounds.cpp
    test/native/NativeGameTestFunction.cpp
    test/native/NativeTestRegistrationBuilder.cpp
    test/native/builtin/BuiltinNativeTests.cpp
    test/runner/GameTestRunner.cpp
    test/runner/GameTestRunnerBuilder.cpp
    test/runner/reporter/FailedTestCollector.cpp
    test/runner/reporter/GlobalTestReporter.cpp
    test/runner/reporter/JUnitTestReporter.cpp
    test/runner/reporter/LogTestReporter.cpp
    test/runner/spawner/StructureGridSpawner.cpp
    test/script/GameTestModuleBinding.cpp
    test/script/binding/ScriptAsyncGameTestRunResult.cpp
    test/script/binding/ScriptGameTestError.cpp
    test/script/binding/ScriptGameTestFunction.cpp
    test/script/binding/ScriptGameTestTypes.cpp
    test/script/binding/ScriptRegister.cpp
    test/script/binding/ScriptRegistrationBuilder.cpp
    test/script/binding/ScriptRegistrationBuilderBinding.cpp
    test/script/binding/ScriptSequence.cpp
    test/script/binding/ScriptSimulatedPlayer.cpp
    test/script/binding/ScriptTestHelper.cpp
    test/script/context/ScriptBindingRegistry.cpp
    test/script/context/ScriptGameTestAccessor.cpp
    test/simulated/SimulatedPlayer.cpp
    world/chunk/lifecycle/generation/ChunkLightingProvider.cpp
    world/lighting/runtime/ChunkLoadLightTask.cpp
    world/chunk/lifecycle/generation/ChunkProgressionTask.cpp
    world/chunk/lifecycle/generation/ChunkGenerationScheduler.cpp
    world/lighting/runtime/RuntimeLightTask.cpp
    world/lighting/runtime/RuntimeLightingProvider.cpp
    world/chunk/ServerChunkManager.cpp
    world/chunk/lifecycle/ChunkLifecycleController.cpp
    world/chunk/lifecycle/state/ChunkRegistry.cpp
    world/chunk/lifecycle/loading/ChunkSourceLoader.cpp
    world/chunk/lifecycle/unloading/ChunkUnloadController.cpp
    world/chunk/lifecycle/generation/ChunkStepExecutor.cpp
    world/chunk/integration/WorldChunkLifecycle.cpp
    world/chunk/diagnostics/ChunkDiagnostics.cpp
    world/chunk/lifecycle/demand/ChunkDemandController.cpp
    world/chunk/lifecycle/demand/base/ChunkDistanceGraph.cpp
    world/lighting/runtime/ServerLightQueue.cpp
    world/ServerWorld.cpp
    world/chunk/lifecycle/state/ChunkHolder.cpp
    world/blockentity/sculk/SculkShriekerHelper.cpp
    world/blockentity/sculk/SculkVibrationSystem.cpp
    world/drop/BlockDropHandler.cpp
    world/entity/EntityTracker.cpp
    world/entity/ItemPickupManager.cpp
    world/entity/waypoint/WaypointManager.cpp
    world/gameevent/DynamicGameEventListener.cpp
    world/gameevent/GameEventDispatcher.cpp
    world/gameevent/GameEventListenerRegistry.cpp
    world/gameevent/PositionSource.cpp
    world/gameevent/VibrationSystemServer.cpp
    world/lighting/engine/BaseLightEngine.cpp
    world/lighting/engine/BlockLightEngine.cpp
    world/lighting/engine/SkyLightEngine.cpp
    world/lighting/manager/WorldLightManager.cpp
    world/player/PlayerSpawnFinder.cpp
    world/player/ServerPlayerEntityManager.cpp
    world/scheduling/AreaTaskScheduler.cpp
    world/spawn/DespawnManager.cpp
    world/spawn/NaturalSpawner.cpp
    world/spawn/VillageSiege.cpp
    world/spawn/WanderingTraderSpawner.cpp
    world/structure/StructureLocator.cpp
    world/time/GameTime.cpp
    world/time/TimeManager.cpp
    world/weather/WeatherManager.cpp
)

target_link_libraries(mc_server_shared
    PRIVATE
        mc_common
        mc_gen
        mc_server_storage
        nlohmann_json::nlohmann_json
)

# mc_tests 侧的编译定义（MC_SOURCE_DIR/MC_SOURCE_ROOT）只有 ServerApplicationEntry.cpp 使用，
# 该文件不在本库内；这里仍注入 MC_SOURCE_ROOT，保证任何后续引用它的实现行为一致。
target_compile_definitions(mc_server_shared PRIVATE MC_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")

# 复用 mc_common 的 PCH：本库与 minecraft-server 编译同一批实现，PCH 头内容通用。
target_precompile_headers(mc_server_shared REUSE_FROM mc_common)
mc_set_compiler_warnings(mc_server_shared)

add_executable(minecraft-server
    main.cpp
    # 数据包驱动的物品定义加载器（乐器；注册表与标签容器位于 common）
    application/StandaloneServer.cpp
    application/ServerApplicationEntry.cpp
    # 交互式终端（--interactive）：日志区 + 状态面板 + 命令输入框（FTXUI）
    application/console/ServerConsole.cpp
    application/console/ui/TerminalUi.cpp
    application/console/ui/ConsoleLayout.cpp
    application/console/ui/LogViewComponent.cpp
    application/console/ui/StatusPanelComponent.cpp
    application/console/ui/CommandLineComponent.cpp
    application/console/ui/CompletionPopupComponent.cpp
    application/console/bridge/ConsoleLogConsumer.cpp
    application/console/bridge/GameEventFeed.cpp
    application/console/bridge/TerminalCommandSource.cpp
    application/console/bridge/CompletionRequestQueue.cpp
    application/console/metrics/ServerMetricsSnapshot.cpp
    # 新网络层（1.21.11 IR）：连接基座 + 会话门面 + 握手/配置 + Play 入站 + 出站构造
    # 连接基座（IServerClientConnection + Local/Wire 双模单连接实现）
    # 会话生命周期门面（accept + 连接表 + tick 泵）
    # 单客户端会话（协议状态 + 入站派发链，本地与远程共用）
    # 远程 TCP 会话管理门面（远程会话四件套下沉自 StandaloneServer/IntegratedServer）
    # 握手/登录/配置阶段（四阶段状态机 + Configuration 载荷构造 + Play 入场序列）
    # Play 阶段入站处理（ServerPlayHandler::route + 13 个非纯虚 handle*Packet +
    # updateEntityTrackingForPlayer 整簇下沉自 MinecraftServer）
    # Play 阶段各包族处理器（按包族拆分自 ServerPlayHandler.cpp）
    # 出站构造与广播（Play/Configuration 单包 IR 构造 + 距离过滤广播 + 地图周期推送）
    # 注册表装配门面（vanilla 注册表初始化下沉自 MinecraftServer::initializeRegistries）
    # 核心模块
    # 维度模块
    # 世界模块
    # 世界模块 - 光照引擎与管理器（自 common/world/lighting 迁入）
    # 世界模块 - 游戏时间（自 common/world/time 迁入）
    # 结构定位算法门面（findNearestStructure/findNearestMapStructure 下沉自 ServerWorld）
    # 世界模块 - 游戏事件系统

    # 世界模块 - 幽匿方块实体（服务端方法）

    # 成就系统
    # 事件系统
    # 记分板系统
    # 统计系统
    # 交互管理器
    physics/TravelTick.cpp
    # 同步管理器
    # 区块推送记账（ChunkView/PlayerChunkTracker/ChunkSyncManager，
    # 自 common/network/sync/ChunkSync.hpp 迁入；双向共用的 ChunkSerializer 留在 common）
    # 命令系统
    # 命令实现全量（清单见 cmake/ServerCommandSources.cmake，与 mc_benchmark 共用）
    # 菜单系统
    # Boss栏系统
    # 函数系统
    # 脚本系统集成

    # -------------------------------------------------------------------
    # gen 源文件已提取为 mc_gen OBJECT 库（见本文件末尾），供所有需要
    # world/gen 实现的 target（minecraft-server / minecraft-client /
    # mc_tests / mc_benchmark / mc_trident_tests）统一链接，避免 common
    # 静态库中违规引用 server gen 头的 .o（TreeGenerators/FungusBlock/
    # BigMushroomGenerators/SpreadableSnowyDirtBlock）在非 server target
    # 上找不到 gen 符号实现。
    # -------------------------------------------------------------------
)

target_link_libraries(minecraft-server
    PRIVATE
        mc_server_shared
        mc_gen
        mc_server_storage
        mc_common
        mc_bedrock_addon
        nlohmann_json::nlohmann_json
        # 交互式终端（--interactive）。仅服务端 exe 使用：mc_common/mc_tests 等不链接 FTXUI。
        ftxui::component
)

if(WIN32)
    target_link_libraries(minecraft-server PRIVATE ws2_32)
endif()

mc_set_compiler_warnings(minecraft-server)
mc_copy_runtime_dlls(minecraft-server)

# ============================================================================
# GameTest 集成测试框架 - 整体作为 server 一部分（原 mc_test 库已取消）
#
# base/+framework/+native/（引擎无关核心层）+ facade/+minecraft/+runner/+simulated/+script/
# +native/builtin/（服务端专属层）合并注入 minecraft-server，命名空间统一 mc::test。
# 历史上 base/framework/native 曾独立为 mc_test 库，但实际只有服务端使用 gametest
# （MC 原版即如此），且 client 完全不引用 mc_test，故将其物理迁入 server/test/ 下，
# 消除"common 库被 server exe 链接"的跨层依赖，目录与逻辑一致。
# ServerScriptManager.cpp 与 IntegratedServer.cpp 同时编入 client exe（client/CMakeLists.txt），
# 它们不 include server/test/ 头（GameTestModuleBinding 注册在 GameTestServer.cpp，server 专属），
# 故 client build 不受此 target_sources 影响。
# ============================================================================
target_sources(minecraft-server
    PRIVATE
        # base/ - 末梢基础类型（依赖链最底，被所有层依赖）
        # framework/ - 引擎无关抽象核心
        # native/ - 原生测试函数族注册机制（被 GameTestRegistrar 封装）
        # facade/ - 对外门面层
        # minecraft/ - ServerWorld 具体绑定
        # runner/ - 编排
        # native/ - 内置样例测试（保链接期保留 TU）
        # simulated/ - SimulatedPlayer
        # script/ - @minecraft/server-gametest JS 绑定骨架
)

# 注入源码根路径宏，使 --gametest 门面能定位仓库内 tests/integrated（无论运行时 CWD 如何）。
# ServerApplicationEntry.cpp 用 #ifdef MC_SOURCE_ROOT 拼接默认 --gametest-packs 路径。
target_compile_definitions(minecraft-server PRIVATE MC_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")

target_compile_features(minecraft-server PRIVATE cxx_std_20)

# 复用 mc_common 的预编译头（PCH），避免为 server target 重复创建一份 .pch。
# REUSE_FROM 要求两 target 的 PCH 相关编译选项一致；mc_common 与本 target 均经
# mc_set_compiler_warnings 设置相同警告基线，且 PCH 头内容通用，可安全复用。
target_precompile_headers(minecraft-server REUSE_FROM mc_common)

set_target_properties(minecraft-server PROPERTIES
    OUTPUT_NAME "minecraft-server"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Windows 默认主线程栈仅 1MB，而 QuickJS 栈溢出检查按 stack_top - stack_size 计算 stack_limit，
# stack_size 默认 4MB。1MB 栈下 stack_top（sp）远小于 4MB，stack_top - stack_size 无符号下溢成
# 巨大值，致使 js_check_stack_overflow 的 sp < stack_limit 恒真，模块编译/链接阶段即误报
# "Maximum call stack size exceeded"。故 Windows 下把主线程栈保留空间提升到 16MB，
# 让 4MB 递归预算不再下溢且有充足余量（quickjs-ng 自身对其 qjsc 工具亦设 8MB，同理）。
# 编译器为 clang.exe（非 clang-cl），MSVC 变量为 false，须按 Clang+lld-link 分支用 -Xlinker。
# macOS/Linux 默认栈 8MB 无此问题。
if(WIN32)
    if(MSVC)
        target_link_options(minecraft-server PRIVATE /STACK:16777216)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MINGW)
        target_link_options(minecraft-server PRIVATE -Xlinker /STACK:16777216)
    else()
        target_link_options(minecraft-server PRIVATE -Wl,--stack,16777216)
    endif()
endif()

target_sources(mc_gen PRIVATE
    world/chunk/base/pipeline/ChunkDependencies.cpp
    world/chunk/base/pipeline/ChunkPyramid.cpp
    world/chunk/base/pipeline/ChunkStatus.cpp
    world/gen/structure/base/StructureBoundingBox.cpp
)
