# src/server/CMakeLists.txt
# 服务端可执行文件

add_executable(minecraft-server
    main.cpp
    application/StandaloneServer.cpp
    application/IntegratedServer.cpp
    application/MinecraftServer.cpp
    settings/ServerSettings.cpp
    # 新网络层（1.21.11 IR）：服务端门面 + 握手状态机 + 配置注册表 + Play 路由
    network/ServerNetwork.cpp
    network/ServerHandshake.cpp
    network/RegistryDataBuilder.cpp
    network/EnchantmentNbtBuilder.cpp
    network/ServerPlayRouter.cpp
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
    world/drop/BlockDropHandler.cpp
    world/spawn/NaturalSpawner.cpp
    world/spawn/SpawnConditions.cpp
    world/spawn/DespawnManager.cpp
    world/spawn/VillageSiege.cpp
    world/entity/EntityTracker.cpp
    world/entity/EntityChunkTracker.cpp
    world/entity/ItemPickupManager.cpp
    world/player/ServerPlayerEntityManager.cpp
    world/weather/WeatherManager.cpp
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
    sync/EntitySyncManager.cpp
    sync/BlockUpdateSyncManager.cpp
    sync/ChunkSendManager.cpp
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
)

target_link_libraries(minecraft-server
    PRIVATE
        mc_common
        mc_bedrock_addon
        nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(minecraft-server PRIVATE ws2_32)
endif()

mc_set_compiler_warnings(minecraft-server)
mc_copy_runtime_dlls(minecraft-server)

target_compile_features(minecraft-server PRIVATE cxx_std_20)

set_target_properties(minecraft-server PROPERTIES
    OUTPUT_NAME "minecraft-server"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
