#include "server/test/facade/GameTestServer.hpp"

#include "common/test/framework/batch/GameTestBatch.hpp"
#include "common/test/framework/environment/EnvironmentRegistry.hpp"
#include "common/test/framework/function/BaseGameTestFunction.hpp"
#include "common/test/framework/registry/GameTestRegistry.hpp"
#include "common/test/framework/ticker/GameTestTicker.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/storage/core/LevelDatCodec.hpp"
#include "common/world/storage/core/WorldStoragePaths.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/mod/bedrock/addon/ServerScriptManager.hpp"               // scriptManager()->engine().addModuleFactory
#include "server/test/facade/GameTestCommand.hpp"                         // GameTestCommand::registerTo
#include "server/test/minecraft/structure/GameTestStructureBootstrap.hpp" // ensureBuiltinStructureTemplates
#include "server/test/native/builtin/BuiltinNativeTests.hpp"              // registerBuiltinNativeTests
#include "server/test/runner/GameTestRunner.hpp"
#include "server/test/runner/GameTestRunnerBuilder.hpp" // GameTestRunner::builder() 返回值完整类型
#include "server/test/runner/reporter/GlobalTestReporter.hpp"
#include "server/test/runner/reporter/JUnitTestReporter.hpp"
#include "server/test/runner/reporter/LogTestReporter.hpp"
#include "server/test/script/GameTestModuleBinding.hpp" // @minecraft/server-gametest JS 绑定
#include "server/world/ServerWorld.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <utility>

namespace mc::test {

GameTestServer::GameTestServer()
    : mc::server::MinecraftServer(m_settings)
{}

GameTestServer::~GameTestServer()
{
    if (m_initialized) {
        stop();
    }
}

mc::Result<void> GameTestServer::initialize(const GameTestServerParams& params)
{
    if (m_initialized) {
        return mc::Error(mc::ErrorCode::AlreadyExists, "GameTestServer already initialized");
    }

    m_params = params;

    // === 应用世界参数到设置（镜像 IntegratedServer::initialize）===
    m_settings.viewDistance.set(params.viewDistance);
    m_settings.simulationDistance.set(params.simulationDistance);
    m_settings.defaultGameMode.set(static_cast<i32>(params.defaultGameMode));
    m_settings.levelSeed.set(params.seed != 0 ? std::to_string(params.seed) : "");
    m_settings.maxPlayers.set(1); // 无头测试无玩家
    m_settings.tickRate.set(params.tickRate);
    m_settings.worldName.set(params.worldName);

    // === 游戏目录 + 数据包扫描（worldgen 100% 数据驱动，必须注入 vanilla 包）===
    m_gameDirectory = params.gameDirectoryRoot.empty() ? mc::GameDirectory::defaultDirectory()
                                                       : mc::GameDirectory::fromRoot(params.gameDirectoryRoot);
    auto dirResult = m_gameDirectory.ensureDirectoriesExist();
    if (dirResult.failed()) {
        spdlog::warn("Failed to create game directories: {}", dirResult.error().toString());
    }

    auto dataPackDir = m_gameDirectory.dataPacksDir();
    auto scanResult = m_dataPackList.scanDirectory(dataPackDir);
    if (scanResult.failed()) {
        spdlog::warn(
            "Failed to scan data pack directory '{}': {}", dataPackDir.string(), scanResult.error().toString());
    } else if (scanResult.value() > 0) {
        spdlog::info("Scanned {} data packs from '{}'", scanResult.value(), dataPackDir.string());
    }

    const auto vanillaDataPackDir = mc::GameDirectory::defaultDirectory().dataPacksDir() / "Vanilla";
    if (m_dataPackList.ensureVanillaBuiltinPack(vanillaDataPackDir) > 0) {
        spdlog::info("Injected builtin vanilla data pack from '{}'", vanillaDataPackDir.string());
    }

    // === 注册表（注册实体类型，因 GameTest 需 spawn 实体）===
    initializeRegistries(true);

    // 确保内置样例测试已注册（静态初始化已触发，此调用保链接期保留 TU）
    registerBuiltinNativeTests();
    // 注入框架内置程序化空模板（gametest:empty_3x3 等），解决结构资源 .nbt 缺失致放置必 fail。
    // 须在 _selectAndBuildRunner 之前，runner 构造批次时实例放置结构即取模板。
    ensureBuiltinStructureTemplates();
    // 注册内置默认环境（"default" → 空 AllOfEnvironment）
    EnvironmentRegistry::instance().registerBuiltinDefaults();

    spdlog::info("=== Cubium GameTestServer ===");
    spdlog::info("World: {}, Seed: {}", params.worldName, params.seed);

    // === 核心管理器 ===
    initializeCoreManagers();

    // 注册 @minecraft/server-gametest JS 绑定模块工厂到脚本引擎。
    // GameTestServer 是 server 专属（不与 client 共享编译），可安全 include server/test/script/ 头。
    // ServerScriptManager（client 共享）不能 include 此头，故注册放此处。
    // 经 scriptManager()->engine().addModuleFactory 公共钩子注入（与 @minecraft/server 同机制）。
    if (auto* sm = scriptManager()) {
        sm->scriptManager().engine().addModuleFactory(std::make_unique<GameTestModuleBinding>());
        spdlog::info("[GameTest] Registered @minecraft/server-gametest module factory");
    }

    // 加载 OP 列表（allowCommands=true 时 GameTestServer 自身需 OP 权限执行 /gametest）
    if (m_gameDirectory.isValid()) {
        const auto opsPath = m_gameDirectory.root() / "ops.json";
        auto opsResult = m_opListManager->load(opsPath);
        if (opsResult.failed()) {
            spdlog::warn("Failed to load ops: {}", opsResult.error().message());
        }
    }

    // === 新世界预写初始 level.dat（镜像 IntegratedServer）===
    if (m_params.isNewWorld) {
        world::storage::WorldStoragePaths storagePaths =
            world::storage::WorldStoragePaths::fromGameDirectory(m_gameDirectory);
        std::filesystem::path worldDir = storagePaths.worldDir(m_params.worldName);
        if (!std::filesystem::exists(worldDir / "level.dat")) {
            world::storage::CreateWorldRequest request(m_params.worldName, // displayName
                m_params.worldName,
                static_cast<u64>(m_params.seed),
                m_params.worldType,
                m_params.worldPresetId,
                m_params.defaultGameMode,
                m_params.difficulty,
                m_params.hardcore,
                m_params.allowCommands,
                m_params.viewDistance);
            auto initResult = world::storage::LevelDatCodec::writeInitial(worldDir, request);
            if (initResult.failed()) {
                spdlog::warn("Failed to write initial level.dat: {}", initResult.error().message());
            }
        }
    }

    auto storageInitResult = initializeSharedStorage(m_gameDirectory, params.worldName);
    if (storageInitResult.failed()) {
        return mc::Error(mc::ErrorCode::InitializationFailed,
            "Failed to initialize shared world storage: " + storageInitResult.error().message());
    }

    // === 维度初始化 ===
    auto dimInitResult = m_dimensionManager->initialize(static_cast<u64>(params.seed),
        params.viewDistance,
        params.simulationDistance,
        params.worldType,
        params.worldPresetId);
    if (dimInitResult.failed()) {
        return mc::Error(mc::ErrorCode::InitializationFailed,
            "Failed to initialize dimension manager: " + dimInitResult.error().message());
    }

    auto* overworld = m_dimensionManager->getOverworld();
    MC_ASSERT_RELEASE_MSG(overworld != nullptr, "GameTestServer: overworld is null after dimension init");
    MC_ASSERT_RELEASE_MSG(overworld->world() != nullptr, "GameTestServer: overworld ServerWorld is null");

    // 为所有维度绑定世界回调。Dimension/ServerDimension 在 namespace mc（非 mc::world/mc::server），
    // 此文件位于 namespace mc::test，经外围 namespace mc 解析到 mc::Dimension/mc::ServerDimension。
    m_dimensionManager->forEachDimension([this](Dimension& dim) {
        auto* serverDim = static_cast<ServerDimension*>(&dim);
        auto* world = serverDim->world();
        if (world) {
            attachWorldBindings(*world);
            attachWorldCommandBindings(*world);
        }
    });

    auto worldResult = initializeWorld();
    if (worldResult.failed()) {
        return mc::Error(
            mc::ErrorCode::InitializationFailed, "Failed to initialize world: " + worldResult.error().message());
    }

    // === 注册 /gametest 命令（CommandRegistry 在 initializeWorld 内创建，构造时已 registerDefaults）===
    // 追加 GameTestCommand 到调度器
    GameTestCommand::registerTo(commandRegistry().dispatcher());

    setupRaidManagerCallbacks();
    setupDragonFightBossBar();
    initializeInteractionManagers();
    initializeSyncManagers();
    initializeChunkSyncManagers();
    setupWorldCallbacks();

    // === 选测试 + 构造 runner ===
    if (!_selectAndBuildRunner()) {
        // 无测试可跑不视为初始化失败（退出码由 run() 据空 runner 判定）
        spdlog::warn("GameTestServer: no tests selected for filter '{}'", m_params.testsFilter);
    }

    m_running = true;
    m_initialized = true;
    spdlog::info("GameTestServer initialized");
    return mc::Result<void>::ok();
}

bool GameTestServer::_selectAndBuildRunner()
{
    // 从 GameTestRegistry 选测试（应用 testsFilter 通配符）
    auto allFunctions = GameTestRegistry::instance().allTestFunctions();
    std::vector<std::shared_ptr<BaseGameTestFunction>> selected;
    if (m_params.testsFilter.empty()) {
        selected = allFunctions;
    } else {
        selected = GameTestRegistry::instance().getTestsByPattern(m_params.testsFilter);
    }

    // 过滤 manualOnly 测试（不在自动 runall 中跑，对齐 Java）
    std::vector<std::shared_ptr<BaseGameTestFunction>> runnable;
    runnable.reserve(selected.size());
    for (auto& fn : selected) {
        if (fn != nullptr && !fn->data().manualOnly()) {
            runnable.push_back(fn);
        }
    }

    if (runnable.empty()) {
        return false;
    }

    // 构造单批次（第一阶段简化：全部 runnable 测试一个批次，环境用 "default"）。
    // TODO: 按 TestData.environment() 分组 + MAX_TESTS_PER_BATCH=50 切片（GameTestBatchFactory，对齐 Java）
    auto defaultEnv = EnvironmentRegistry::instance().getEnvironment("default");
    std::vector<GameTestBatch> batches;
    batches.emplace_back(std::string{"default"},
        std::move(runnable),
        GameTestRegistry::instance().getBeforeBatchFunction("default"),
        GameTestRegistry::instance().getAfterBatchFunction("default"),
        defaultEnv);

    // 构造 runner（overworld 经 dimensionManager 取，_selectAndBuildRunner 在 initialize 末尾调用时维度已就绪）
    auto* overworldDim = m_dimensionManager->getOverworld();
    MC_ASSERT_RELEASE_MSG(overworldDim != nullptr && overworldDim->world() != nullptr,
        "GameTestServer: overworld not ready when building runner");
    const BlockPos gridStart{m_params.gridStartX, m_params.gridStartY, m_params.gridStartZ};
    m_runner = GameTestRunner::builder()
                   .world(*overworldDim->world())
                   .ticker(GameTestTicker::instance())
                   .batches(std::move(batches))
                   .gridStart(gridStart)
                   .testsPerRow(static_cast<std::size_t>(m_params.testsPerRow))
                   .build();

    // 挂 reporter（LogTestReporter 始终挂；JUnit 仅当 reportPath 非空）
    m_logReporter = std::make_shared<LogTestReporter>();
    GlobalTestReporter::instance().addReporter(m_logReporter);
    if (!m_params.reportPath.empty()) {
        // 解析 reportPath：相对路径相对 m_gameDirectory.root()
        std::filesystem::path reportFull(m_params.reportPath);
        if (reportFull.is_relative()) {
            reportFull = m_gameDirectory.root() / reportFull;
        }
        m_junitReporter = std::make_shared<JUnitTestReporter>(reportFull.string());
        GlobalTestReporter::instance().addReporter(m_junitReporter);
    }

    m_runnerBuilt = true;
    return true;
}

i32 GameTestServer::run()
{
    if (!m_initialized || !m_runnerBuilt) {
        // 初始化未成功或无测试：返回 1（CI 视为失败）
        m_exitCode = 1;
        return m_exitCode;
    }

    MC_ASSERT_RELEASE_MSG(m_runner != nullptr, "GameTestServer::run: runner is null");
    m_runner->start();

    // 同步 tick 循环：世界 tick → GameTestTicker → runner，直到完成或超时
    std::size_t tickCount = 0;
    while (m_running.load() && !m_runner->isComplete()) {
        tickOnce();
        ++tickCount;
        if (m_params.maxTicks > 0 && tickCount >= m_params.maxTicks) {
            spdlog::error("GameTestServer: run timed out after {} ticks", m_params.maxTicks);
            break;
        }
    }

    // 末尾再 tick 一次确保 reporter 收到最终状态
    if (m_runner->isComplete()) {
        GlobalTestReporter::instance().onAllFinished(m_runner->tracker());
    }

    m_exitCode = static_cast<i32>(m_runner->failedRequiredCount());
    spdlog::info("GameTestServer run finished: total={}, passed={}, failed={}, exitCode={}",
        m_runner->totalTestCount(),
        m_runner->passedCount(),
        m_runner->failedCount(),
        m_exitCode);
    return m_exitCode;
}

i32 GameTestServer::exitCode() const noexcept
{
    return m_exitCode;
}

void GameTestServer::tickOnce()
{
    // 基类世界 tick（维度/实体/时间/区块）
    MinecraftServer::tick();
    // GameTestTicker 推进测试实例状态机（单例）
    GameTestTicker::instance().tick();
    // runner 推进批次调度
    if (m_runner != nullptr) {
        m_runner->tick();
    }
}

void GameTestServer::shutdown()
{
    stop();
}

void GameTestServer::stop()
{
    if (!m_initialized) {
        return;
    }

    spdlog::info("Stopping GameTestServer...");
    m_running = false;

    // 清理 ticker（避免悬垂实例指针）。
    // clear() 进入 HALTING 状态（对齐 Java，规避迭代中清空 UB）；stopCore 会关闭脚本引擎，
    // 之后无 tick 推进 HALTING 清空，故紧接 forceStop() 立即清空实例，避免 JS 回调访问死上下文。
    GameTestTicker::instance().clear();
    GameTestTicker::instance().forceStop();

    // 移除 reporter
    if (m_logReporter != nullptr) {
        GlobalTestReporter::instance().clear();
    }

    m_runner.reset();
    m_junitReporter.reset();
    m_logReporter.reset();

    // 落盘 + 关闭核心管理器（无玩家/网络，stopCore 会安全处理空连接）
    stopCore();

    m_initialized = false;
    spdlog::info("GameTestServer stopped.");
}

} // namespace mc::test
