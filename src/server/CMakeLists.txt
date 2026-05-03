# src/server/CMakeLists.txt
# 服务端可执行文件

add_executable(minecraft-server
    main.cpp
    application/StandaloneServer.cpp
    application/IntegratedServer.cpp
    application/MinecraftServer.cpp
    settings/ServerSettings.cpp
    network/TcpServer.cpp
    network/TcpSession.cpp
    network/TcpConnection.cpp
    # 核心模块
    core/PlayerManager.cpp
    core/ConnectionManager.cpp
    core/TimeManager.cpp
    core/TeleportManager.cpp
    core/KeepAliveManager.cpp
    core/PositionTracker.cpp
    core/PacketHandler.cpp
    core/GameModeManager.cpp
    # 维度模块
    dimension/ServerDimension.cpp
    dimension/ServerDimensionManager.cpp
    # 世界模块
    world/ServerWorld.cpp
    world/ChunkGenerateTask.cpp
    world/ServerChunkManager.cpp
    world/drop/BlockDropHandler.cpp
    world/spawn/NaturalSpawner.cpp
    world/spawn/SpawnConditions.cpp
    world/spawn/DespawnManager.cpp
    world/entity/EntityTracker.cpp
    world/entity/ItemPickupManager.cpp
    world/player/ServerPlayerEntityManager.cpp
    world/weather/WeatherManager.cpp
    player/ServerPlayer.cpp
    # 交互管理器
    interaction/BlockInteractionManager.cpp
    interaction/MiningManager.cpp
    interaction/ContainerManager.cpp
    interaction/InventoryManager.cpp
    # 同步管理器
    sync/EntitySyncManager.cpp
    sync/BlockUpdateSyncManager.cpp
    sync/ChunkSendManager.cpp
    sync/LightSyncManager.cpp
    # 命令系统
    command/ServerCommandSource.cpp
    command/CommandRegistry.cpp
    command/support/PlayerResolver.cpp
    command/support/EffectResolver.cpp
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
    command/commands/LootCommand.cpp
    command/commands/PublishCommand.cpp
    command/commands/RecipeCommand.cpp
    command/commands/ReloadCommand.cpp
    command/commands/ReplaceItemCommand.cpp
    command/commands/SpectateCommand.cpp
    command/commands/TriggerCommand.cpp
    # 菜单系统
    menu/CraftingMenu.cpp
)

target_link_libraries(minecraft-server
    PRIVATE
        mc_common
        nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(minecraft-server PRIVATE ws2_32)
endif()

mc_set_compiler_warnings(minecraft-server)

target_compile_features(minecraft-server PRIVATE cxx_std_17)

set_target_properties(minecraft-server PROPERTIES
    OUTPUT_NAME "minecraft-server"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
