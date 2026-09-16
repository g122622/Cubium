# src/server/CMakeLists.txt
# 服务端可执行文件

# ===========================================================================
# mc_gen - world/gen 实现 OBJECT 库
#
# 架构规则：src/common 不得依赖 src/server。world/gen 实现物理迁移到
# src/server/world/gen/ 后，既被 minecraft-server（生产服务端）编译，
# 也被 minecraft-client（内置服）/ mc_tests / mc_benchmarks /
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
    world/gen/feature/template/TemplateLoader.cpp
    world/gen/feature/template/RuleTest.cpp
    world/gen/feature/template/CappedStructureProcessor.cpp
    world/gen/feature/template/ProtectedBlocksProcessor.cpp
    world/gen/structure/Structure.cpp
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

# mc_gen 的 .cpp 重度依赖 common 头（Result.hpp/IWorld/各 world 类），并传递引入
# spdlog/nlohmann_json/simdjson/asmjit/fmt 等第三方头（均由 mc_common PUBLIC 链接）。
# 故 mc_gen 须 PRIVATE 链接 mc_common：编译 mc_gen 自身 .cpp 时需要 common 符号与头路径，
# 但 mc_gen 是纯实现 OBJECT 库，不对外暴露头接口，故用 PRIVATE 避免向使用方传递
# mc_common 链接（minecraft-server / minecraft-client / mc_tests / mc_benchmarks /
# mc_trident_tests 本就各自直接链接 mc_common，传递会造成 "duplicate libraries" 警告）。
target_link_libraries(mc_gen PRIVATE mc_common)

target_include_directories(mc_gen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

mc_set_compiler_warnings(mc_gen)
add_executable(minecraft-server
    main.cpp
    application/StandaloneServer.cpp
    application/IntegratedServer.cpp
    application/MinecraftServer.cpp
    application/ServerApplicationEntry.cpp
    settings/ServerSettings.cpp
    # 新网络层（1.21.11 IR）：服务端门面 + 握手状态机 + 配置注册表 + Play 路由
    network/ServerNetwork.cpp
    network/ServerHandshake.cpp
    network/RegistryDataBuilder.cpp
    network/EnchantmentNbtBuilder.cpp
    network/ServerPlayRouter.cpp
    # 远程 TCP 会话管理门面（远程会话四件套下沉自 StandaloneServer/IntegratedServer）
    network/RemoteSessionManager.cpp
    # 玩家广播门面（broadcast*/send* 转发型网络广播下沉自 MinecraftServer）
    network/PlayerBroadcaster.cpp
    # 登录流程门面（玩家创建 + 初始游戏状态推送整簇下沉自 MinecraftServer）
    network/LoginFlow.cpp
    # Play 包处理门面（routeInboundPlayPacket + 13 个非纯虚 handle*Packet +
    # updateEntityTrackingForPlayer 整簇下沉自 MinecraftServer）
    network/ServerPlayHandler.cpp
    # Play/Configuration 单包 IR 构造器（buildPlaySoundIr/buildPermissionLevelChangeIr/
    # buildCommandsIr/buildLevelParticlesIr，下沉自 IServer 单包业务方法）
    network/PacketBuilders.cpp
    # 地图数据周期推送构造器（pushDirtyMaps，遍历玩家背包脏 MapData 构造 MapItemData 下发，
    # 下沉自 ServerWorld::_pushMapDataToHolders）
    network/MapPacketBuilder.cpp
    # 注册表装配门面（vanilla 注册表初始化下沉自 MinecraftServer::initializeRegistries）
    registry/RegistryBootstrap.cpp
    # 核心模块
    core/PlayerManager.cpp
    core/ConnectionManager.cpp
    core/TimeManager.cpp
    core/TeleportManager.cpp
    core/KeepAliveManager.cpp
    core/PositionTracker.cpp
    core/GameModeManager.cpp
    core/WhitelistManager.cpp
    core/BannedPlayerList.cpp
    core/BannedIpList.cpp
    core/OpListManager.cpp
    # 维度模块
    dimension/ServerDimension.cpp
    dimension/ServerDimensionManager.cpp
    # 世界模块
    world/ServerWorld.cpp
    world/ChunkTaskScheduler.cpp
    world/ChunkProgressionTask.cpp
    world/ChunkLightingProvider.cpp
    world/ServerChunkManager.cpp
    world/SingleChunkLifecycleManager.cpp
    world/ServerLightQueue.cpp
    world/RuntimeLightingProvider.cpp
    world/RuntimeLightTask.cpp
    world/ChunkLoadLightTask.cpp
    # 世界模块 - 光照引擎与管理器（自 common/world/lighting 迁入）
    world/lighting/engine/BaseLightEngine.cpp
    world/lighting/engine/BlockLightEngine.cpp
    world/lighting/engine/SkyLightEngine.cpp
    world/lighting/manager/WorldLightManager.cpp
    world/drop/BlockDropHandler.cpp
    world/spawn/NaturalSpawner.cpp
    world/spawn/SpawnConditions.cpp
    world/spawn/DespawnManager.cpp
    world/spawn/VillageSiege.cpp
    world/entity/EntityTracker.cpp
    world/entity/ItemPickupManager.cpp
    world/player/ServerPlayerEntityManager.cpp
    world/weather/WeatherManager.cpp
    # 结构定位算法门面（findNearestStructure/findNearestMapStructure 下沉自 ServerWorld）
    world/structure/StructureLocator.cpp
    # 世界模块 - 游戏事件系统
    world/gameevent/PositionSource.cpp
    world/gameevent/GameEventListenerRegistry.cpp
    world/gameevent/GameEventDispatcher.cpp
    world/gameevent/DynamicGameEventListener.cpp
    world/gameevent/VibrationSystemServer.cpp

    # 世界模块 - 幽匿方块实体（服务端方法）
    world/blockentity/sculk/SculkVibrationSystem.cpp
    world/blockentity/sculk/SculkShriekerHelper.cpp

    player/ServerPlayer.cpp
    # 成就系统
    advancement/PlayerAdvancements.cpp
    # 事件系统
    event/ServerEventBus.cpp
    # 记分板系统
    scoreboard/ServerScoreboard.cpp
    # 统计系统
    stats/StatType.cpp
    stats/Stat.cpp
    stats/StatRegistry.cpp
    stats/StatisticsManager.cpp
    # 交互管理器
    interaction/BlockInteractionManager.cpp
    interaction/MiningManager.cpp
    interaction/ContainerManager.cpp
    interaction/InventoryManager.cpp
    interaction/SignCommandHelper.cpp
    # 同步管理器
    sync/BlockUpdateSyncManager.cpp
    sync/ChunkSendManager.cpp
    sync/WeatherSyncService.cpp
    # 命令系统
    command/ServerCommandSource.cpp
    command/CommandRegistry.cpp
    command/support/PlayerResolver.cpp
    command/support/EntityResolver.cpp
    command/support/EffectResolver.cpp
    command/support/FunctionSuggestionProvider.cpp
    command/support/SpreadAlgorithm.cpp
    command/data/DataAccessor.cpp
    command/commands/GameModeCommand.cpp
    command/commands/DifficultyCommand.cpp
    command/commands/DefaultGameModeCommand.cpp
    command/commands/TimeCommand.cpp
    command/commands/KickCommand.cpp
    command/commands/KillCommand.cpp
    command/commands/ListCommand.cpp
    command/commands/HelpCommand.cpp
    command/commands/SeedCommand.cpp
    command/commands/TeleportCommand.cpp
    command/commands/GiveCommand.cpp
    command/commands/ClearCommand.cpp
    command/commands/WeatherCommand.cpp
    command/commands/ExperienceCommand.cpp
    command/commands/SayCommand.cpp
    command/commands/StopCommand.cpp
    command/commands/SetIdleTimeoutCommand.cpp
    command/commands/SummonCommand.cpp
    command/commands/SetBlockCommand.cpp
    command/commands/FillCommand.cpp
    command/commands/ExecuteCommand.cpp
    command/commands/OpCommand.cpp
    command/commands/DeOpCommand.cpp
    command/commands/BanCommand.cpp
    command/commands/BanIpCommand.cpp
    command/commands/BanListCommand.cpp
    command/commands/PardonCommand.cpp
    command/commands/PardonIpCommand.cpp
    command/commands/WhitelistCommand.cpp
    command/commands/SaveAllCommand.cpp
    command/commands/SaveOnCommand.cpp
    command/commands/SaveOffCommand.cpp
    command/commands/SpawnPointCommand.cpp
    command/commands/SetWorldSpawnCommand.cpp
    command/commands/MessageCommand.cpp
    command/commands/TellRawCommand.cpp
    command/commands/TitleCommand.cpp
    command/commands/PlaySoundCommand.cpp
    command/commands/StopSoundCommand.cpp
    command/commands/EffectCommand.cpp
    command/commands/EnchantCommand.cpp
    command/commands/MeCommand.cpp
    command/commands/ParticleCommand.cpp
    command/commands/LocateCommand.cpp
    command/commands/LocateBiomeCommand.cpp
    command/commands/AttributeCommand.cpp
    command/commands/CloneCommand.cpp
    command/commands/DataCommand.cpp
    command/commands/FunctionCommand.cpp
    command/commands/ScheduleCommand.cpp
    command/commands/SpreadPlayersCommand.cpp
    command/commands/WorldBorderCommand.cpp
    command/commands/ScoreboardCommand.cpp
    command/commands/BossBarCommand.cpp
    command/commands/TagCommand.cpp
    command/commands/TeamCommand.cpp
    command/commands/AdvancementCommand.cpp
    command/commands/DataPackCommand.cpp
    command/commands/ForceLoadCommand.cpp
    command/commands/GameRuleCommand.cpp
    command/commands/LootCommand.cpp
    command/commands/PublishCommand.cpp
    command/commands/RecipeCommand.cpp
    command/commands/ReloadCommand.cpp
    command/commands/ReplaceItemCommand.cpp
    command/commands/SpectateCommand.cpp
    command/commands/TriggerCommand.cpp
    command/commands/WardenSpawnTrackerCommand.cpp
    # 菜单系统
    menu/CraftingMenu.cpp
    # Boss栏系统
    bossbar/BossInfo.cpp
    bossbar/ServerBossInfo.cpp
    bossbar/CustomServerBossInfo.cpp
    bossbar/CustomServerBossInfoManager.cpp
    bossbar/ServerDragonBossBar.cpp
    # 函数系统
    function/CommandFunction.cpp
    function/FunctionLoader.cpp
    function/FunctionManager.cpp
    function/MacroFunction.cpp
    function/StringTemplate.cpp
    function/TimerQueue.cpp
    # 脚本系统集成
    mod/bedrock/addon/ServerScriptManager.cpp
    mod/bedrock/addon/bridge/EventBridge.cpp
    mod/bedrock/addon/bridge/ServerEventSignals.cpp

    # -------------------------------------------------------------------
    # gen 源文件已提取为 mc_gen OBJECT 库（见本文件末尾），供所有需要
    # world/gen 实现的 target（minecraft-server / minecraft-client /
    # mc_tests / mc_benchmarks / mc_trident_tests）统一链接，避免 common
    # 静态库中违规引用 server gen 头的 .o（TreeGenerators/FungusBlock/
    # BigMushroomGenerators/SpreadableSnowyDirtBlock）在非 server target
    # 上找不到 gen 符号实现。
    # -------------------------------------------------------------------
)

target_link_libraries(minecraft-server
    PRIVATE
        mc_gen
        mc_common
        mc_bedrock_addon
        mc_test
        nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(minecraft-server PRIVATE ws2_32)
endif()

mc_set_compiler_warnings(minecraft-server)
mc_copy_runtime_dlls(minecraft-server)

# ============================================================================
# GameTest 集成测试框架 - server 专属源（facade/minecraft/runner/simulated/script/native-builtin）
#
# 架构约束：facade/+minecraft/+runner/+simulated/+script/ 依赖 server 类型（MinecraftServer/
# ServerWorld/CommandRegistry）+ mc_bedrock_addon（script/ JS 绑定基建），不能入引擎无关的
# mc_test 库。经 target_sources 注入 minecraft-server（生产路径：GameTestServer 门面 +
# /gametest 命令 + @minecraft/server-gametest JS 模块）。
# ServerScriptManager.cpp 与 IntegratedServer.cpp 同时编入 client exe（client/CMakeLists.txt），
# 它们不 include server/test/ 头（GameTestModuleBinding 注册在 GameTestServer.cpp，server 专属），
# 故 client build 不受此 target_sources 影响。
# ============================================================================
target_sources(minecraft-server
    PRIVATE
        # facade/ - 对外门面层
        ${CMAKE_CURRENT_SOURCE_DIR}/test/facade/GameTestServer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/facade/GameTestCommand.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/facade/GameTestHelper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/facade/GameTestRegistrar.cpp
        # minecraft/ - ServerWorld 具体绑定
        ${CMAKE_CURRENT_SOURCE_DIR}/test/minecraft/instance/MinecraftGameTestInstance.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/minecraft/batch/MinecraftGameTestBatchRunner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/minecraft/helper/MinecraftGameTestHelperProvider.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/minecraft/structure/MinecraftStructurePlacer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/minecraft/structure/StructureBounds.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/minecraft/structure/GameTestStructureBootstrap.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/minecraft/structure/BehaviorPackStructureSource.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/minecraft/listener/WorldVisualizationListener.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/minecraft/environment/MinecraftEnvironmentApplier.cpp
        # runner/ - 编排
        ${CMAKE_CURRENT_SOURCE_DIR}/test/runner/GameTestRunner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/runner/GameTestRunnerBuilder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/runner/spawner/StructureGridSpawner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/runner/reporter/GlobalTestReporter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/runner/reporter/LogTestReporter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/runner/reporter/JUnitTestReporter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/runner/reporter/FailedTestCollector.cpp
        # native/ - 内置样例测试（保链接期保留 TU）
        ${CMAKE_CURRENT_SOURCE_DIR}/test/native/builtin/BuiltinNativeTests.cpp
        # simulated/ - SimulatedPlayer
        ${CMAKE_CURRENT_SOURCE_DIR}/test/simulated/SimulatedPlayer.cpp
        # script/ - @minecraft/server-gametest JS 绑定骨架
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/GameTestModuleBinding.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/context/ScriptGameTestAccessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/context/ScriptBindingRegistry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/binding/ScriptRegister.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/binding/ScriptTestHelper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/binding/ScriptGameTestError.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/binding/ScriptGameTestTypes.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/binding/ScriptRegistrationBuilder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/binding/ScriptRegistrationBuilderBinding.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/binding/ScriptSequence.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/binding/ScriptSimulatedPlayer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/binding/ScriptGameTestFunction.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/script/binding/ScriptAsyncGameTestRunResult.cpp
)

# ============================================================================
# 集成测试行为包 TS 编译（tests/integrated）：构建期把 .ts 编译成 .js 供 QuickJS 运行。
# 行为包源码在仓库内 tests/integrated/（TS 源 + .mcstructure 结构），由 --gametest 门面
# 经 --gametest-packs 扫描加载。node_modules 不入 git，build.mjs 自管依赖（缺失则 npm install）。
# 仿 src/common/CMakeLists.txt 的 baking add_custom_command 模式：OUTPUT 用 stamp 文件
# （build.mjs 一次产多包多文件无单一输出），target_sources 把 stamp 拉入 server 源触发构建。
# NODE_EXECUTABLE 是 src/common/CMakeLists.txt 的全局 cache 变量，common 先于 server 解析，可直接复用。
# ============================================================================
set(_integrated_root "${CMAKE_SOURCE_DIR}/tests/integrated")
set(_integrated_build_script "${_integrated_root}/build.mjs")
set(_integrated_build_stamp "${CMAKE_BINARY_DIR}/tests/integrated/.build.stamp")
add_custom_command(
    OUTPUT ${_integrated_build_stamp}
    COMMAND ${NODE_EXECUTABLE} ${_integrated_build_script}
    COMMAND ${CMAKE_COMMAND} -E touch ${_integrated_build_stamp}
    DEPENDS ${_integrated_build_script}
            ${_integrated_root}/package.json
            ${_integrated_root}/tsconfig.base.json
            ${_integrated_root}/utils/block/build.ts
            ${_integrated_root}/utils/entity/assert.ts
            ${_integrated_root}/challenge/src/main.ts
            ${_integrated_root}/challenge/src/ChallengeTests.ts
            ${_integrated_root}/command/src/main.ts
            ${_integrated_root}/command/src/CommandTests.ts
            ${_integrated_root}/mob_behavior/src/main.ts
            ${_integrated_root}/mob_behavior/src/tests/monster/basic/PhantomTests.ts
            ${_integrated_root}/mob_behavior/src/tests/monster/nether/ZoglinTests.ts
            ${_integrated_root}/mob_behavior/src/tests/monster/undead/ZombieTests.ts
            ${_integrated_root}/mob_behavior/src/tests/passive/basic/CowTests.ts
            ${_integrated_root}/mob_behavior/src/tests/passive/basic/PigTests.ts
            ${_integrated_root}/mob_behavior/src/tests/passive/basic/SheepTests.ts
            ${_integrated_root}/mob_behavior/src/tests/passive/basic/ChickenTests.ts
            ${_integrated_root}/mob_behavior/src/tests/passive/golem/IronGolemTests.ts
            # 注意：各行为包 src/utils/ 下的 build.ts/assert.ts 是 build.mjs 的 stageUtils()
            # 在构建期从顶层 tests/integrated/utils/ 复制过去的生成产物，不是源文件，
            # 不能列入 DEPENDS（否则 ninja 报 "missing and no known rule to make it"）。
            # 顶层 utils/ 源文件已在上面第 291-292 行声明依赖。
            ${_integrated_root}/starter/src/StarterTests.ts
    WORKING_DIRECTORY ${_integrated_root}
    COMMENT "Compiling integrated behavior packs (TypeScript -> JavaScript)"
    VERBATIM
)
set_source_files_properties(${_integrated_build_stamp} PROPERTIES GENERATED TRUE)
target_sources(minecraft-server PRIVATE ${_integrated_build_stamp})

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
