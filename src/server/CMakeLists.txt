# src/server/CMakeLists.txt
# 服务端可执行文件

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
)

target_link_libraries(minecraft-server
    PRIVATE
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
