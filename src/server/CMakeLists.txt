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
    world/ChunkWorkerPool.cpp
    world/ServerChunkManager.cpp
    world/drop/BlockDropHandler.cpp
    world/spawn/NaturalSpawner.cpp
    world/spawn/SpawnConditions.cpp
    world/entity/EntityTracker.cpp
    world/entity/ItemPickupManager.cpp
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
