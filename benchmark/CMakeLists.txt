add_executable(mc_benchmark
    main.cpp
    MemoryProfiler.hpp
    MemoryProfiler.cpp
    PerfettoProfilerAdapter.hpp
    PerfettoProfilerAdapter.cpp
    cases/ChunkGenerationBenchmark.cpp
    cases/LightingBenchmark.cpp
    cases/PalettedContainerBenchmark.cpp
    cases/ServerInitializeBenchmark.cpp
)

# 光照引擎与管理器（自 common/world/lighting 迁入 server，LightingBenchmark 引用其符号；
# 区块生成调度链路符号由 mc_gen + 下列 server 源文件提供，与 minecraft-server 的
# 源登记方式一致，不直接链接 minecraft-server 可执行目标）。
# 源清单参照 tests/unit/CMakeLists.txt 对 mc_tests 的同链路登记（符号闭包）：
# - ServerChunkManager 链：ServerChunkManager/ChunkTaskScheduler/ChunkProgressionTask/
#   SingleChunkLifecycleManager/ServerWorld/ChunkSendManager(onChunkPreUnload)/
#   ScoreboardDataManager(存储层构造引用)
# - 光照链：4 个 lighting 引擎文件 + ServerLightQueue/RuntimeLightingProvider/
#   RuntimeLightTask/ChunkLoadLightTask + ReentrantAreaLock
target_sources(mc_benchmark
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerChunkManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkTaskScheduler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkProgressionTask.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkLightingProvider.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/SingleChunkLifecycleManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerWorld.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ServerLightQueue.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/RuntimeLightingProvider.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/RuntimeLightTask.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/ChunkLoadLightTask.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/sync/ChunkSendManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/scoreboard/storage/ScoreboardDataManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/scoreboard/storage/ScoreboardSaveData.cpp
        # ServerWorld 传递依赖（构造/回调链引用）：
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/entity/ItemPickupManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/player/ServerPlayerEntityManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/weather/WeatherManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/time/GameTime.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/structure/StructureLocator.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/GameEventDispatcher.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/GameEventListenerRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/PositionSource.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/DynamicGameEventListener.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/VibrationSystemServer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/blockentity/sculk/SculkVibrationSystem.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/blockentity/sculk/SculkShriekerHelper.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/TimeManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/PlayerManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/ConnectionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/advancement/PlayerAdvancements.cpp
        ${CMAKE_SOURCE_DIR}/src/server/event/ServerEventBus.cpp
        # ServerPlayer/命令源/函数管理器（ServerWorld 回调链引用）
        ${CMAKE_SOURCE_DIR}/src/server/player/ServerPlayer.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/ServerCommandSource.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/CommandRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/FunctionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/CommandFunction.cpp
        # 维度管理（ServerWorld/传送链引用）
        ${CMAKE_SOURCE_DIR}/src/server/dimension/ServerDimension.cpp
        ${CMAKE_SOURCE_DIR}/src/server/dimension/ServerDimensionManager.cpp
        # 统计系统（ServerPlayer 引用）
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatType.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/Stat.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/server/stats/StatisticsManager.cpp
        # 命令实现全量（CommandRegistry::registerBuiltinCommands 引用全部 registerTo）
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
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DebugConfigCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SummonCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetBlockCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/FillCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ExecuteCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/OpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DeOpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanIpCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanListCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/PardonCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/PardonIpCommand.cpp
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
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/MeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ParticleCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/LocateCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/LocateBiomeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/AttributeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/CloneCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DataCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/FunctionCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ScheduleCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SpreadPlayersCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WorldBorderCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ScoreboardCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/BossBarCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TagCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TeamCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/AdvancementCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/DataPackCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ForceLoadCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/GameRuleCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/LootCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/PublishCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/RecipeCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ReloadCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/ReplaceItemCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/SpectateCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/TriggerCommand.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/commands/WardenSpawnTrackerCommand.cpp
        # 自然生成（ServerWorld::tick 引用）
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/NaturalSpawner.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/SpawnConditions.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/DespawnManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/spawn/VillageSiege.cpp
        # 传送管理（ServerPlayer 引用）
        ${CMAKE_SOURCE_DIR}/src/server/core/TeleportManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/GameModeManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/WhitelistManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/BannedPlayerList.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/BannedIpList.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/OpListManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/KeepAliveManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/core/PositionTracker.cpp
        # 交互管理器（ServerPlayer 引用）
        ${CMAKE_SOURCE_DIR}/src/server/interaction/BlockInteractionManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/MiningManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/ContainerManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/InventoryManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/interaction/SignCommandHelper.cpp
        # 掉落/命令数据访问/支持工具（命令实现引用）
        ${CMAKE_SOURCE_DIR}/src/server/world/drop/BlockDropHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/data/DataAccessor.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/PlayerResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/EntityResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/EffectResolver.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/FunctionSuggestionProvider.cpp
        ${CMAKE_SOURCE_DIR}/src/server/command/support/SpreadAlgorithm.cpp
        # 容器菜单（InventoryManager 引用；InventoryCraftingMenu 定义在 CraftingMenu.cpp 中）
        ${CMAKE_SOURCE_DIR}/src/server/menu/CraftingMenu.cpp
        # Boss 栏（命令引用）
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/BossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/ServerBossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfo.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfoManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/bossbar/ServerDragonBossBar.cpp
        # 函数系统其余部分（命令/调度引用）
        ${CMAKE_SOURCE_DIR}/src/server/function/FunctionLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/MacroFunction.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/StringTemplate.cpp
        ${CMAKE_SOURCE_DIR}/src/server/function/TimerQueue.cpp
        # 命令系统门面（ServerCommandSource 引用）
        # （无独立 CommandSource.cpp，门面代码在 CommandRegistry.cpp/ServerCommandSource.cpp 中）
        # 同步管理器（ServerWorld 引用）
        ${CMAKE_SOURCE_DIR}/src/server/network/sync/chunk/PlayerChunkTracker.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/sync/chunk/ChunkSyncManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/sync/BlockUpdateSyncManager.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/sync/WeatherSyncService.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/outbound/MapPacketBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/outbound/PlayerBroadcaster.cpp
        ${CMAKE_SOURCE_DIR}/src/server/network/outbound/PacketBuilders.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/lighting/engine/BaseLightEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/lighting/engine/BlockLightEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/lighting/engine/SkyLightEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/server/world/lighting/manager/WorldLightManager.cpp
        ${CMAKE_SOURCE_DIR}/src/common/util/concurrent/ReentrantAreaLock.cpp
)

target_include_directories(mc_benchmark
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mc_benchmark
    PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
        mc_gen
        mc_server_storage
        mc_common
        mc_bedrock_addon
        nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(mc_benchmark PRIVATE ws2_32)
endif()

mc_set_compiler_warnings(mc_benchmark)
target_compile_features(mc_benchmark PRIVATE cxx_std_20)
