#include "server/test/facade/GameTestServer.hpp"

#include "common/test/framework/batch/GameTestBatch.hpp"
#include "common/test/framework/environment/AllOfEnvironment.hpp"
#include "common/test/framework/environment/EnvironmentRegistry.hpp"
#include "common/test/framework/environment/TimeOfDayEnvironment.hpp" // TimeOfDayEnvironment（night/day 批时间环境）
#include "common/test/framework/environment/WeatherEnvironment.hpp"   // WeatherEnvironment（day/night 批强制晴天）
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
#include "server/mod/bedrock/addon/ServerScriptManager.hpp"                // scriptManager()->engine().addModuleFactory
#include "server/test/facade/GameTestCommand.hpp"                          // GameTestCommand::registerTo
#include "server/test/minecraft/structure/BehaviorPackStructureSource.hpp" // 行为包 .mcstructure 资源源
#include "server/test/minecraft/structure/GameTestStructureBootstrap.hpp"  // ensureBuiltinStructureTemplates
#include "server/test/native/builtin/BuiltinNativeTests.hpp"               // registerBuiltinNativeTests
#include "server/test/runner/GameTestRunner.hpp"
#include "server/test/runner/GameTestRunnerBuilder.hpp" // GameTestRunner::builder() 返回值完整类型
#include "server/test/runner/reporter/GlobalTestReporter.hpp"
#include "server/test/runner/reporter/JUnitTestReporter.hpp"
#include "server/test/runner/reporter/LogTestReporter.hpp"
#include "server/test/script/GameTestModuleBinding.hpp" // @minecraft/server-gametest JS 绑定
#include "server/world/ServerWorld.hpp"

#include "common/mod/bedrock/addon/pack/BehaviorPackList.hpp" // BehaviorPackList 完整类型（packList()->empty/size）
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp" // JigsawAssembler::getTemplateManager

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <unordered_map>
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
    // GameTestServer 是无头门面，从不创建任何玩家实体（maxPlayers=1 仅是容量上限）。
    // EntityManager::tick 的模拟距离门控在"无在线玩家"时会冻结所有非玩家实体
    // （_isEntityInSimulationRange 遇 playerChunks.empty() 直接返回 false，不调 tick()），
    // 致 spawn 的 fox/chicken 等 AI 永不执行，测试必超时。故此处强制 simulationDistance=32：
    // EntityManager 以 "m_simulationDistance < 32" 为 freezeEnabled 判据，32 关闭冻结全量 tick
    // （对齐原版 >=32 等价无 EntityTickingRange 限制）。GameTestServer 无客户端无玩家，
    // simulationDistance 仅影响实体激活门控，不影响区块加载（用 viewDistance），设 32 无副作用。
    // 忽略 params.simulationDistance（保留字段供未来按需收紧，但当前必须 32 才能跑通实体测试）。
    constexpr i32 kGameTestSimulationDistance = 32;
    m_settings.simulationDistance.set(kGameTestSimulationDistance);
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
    // addModuleFactory 仅把工厂存入 engine 的 vector（不依赖 engine.initialize），故可在 initializeWorld
    // （含 ScriptManager::initialize→engine.initialize 创建 runtime）之前调；工厂在 loadPlugins 创建
    // QuickJSContext 时才被消费。对齐在线路径（main.cpp::initializeServerGameTest 在 server.initialize 后
    // 注册工厂 + loadBehaviorPacks）。
    if (auto* sm = scriptManager()) {
        // 先创建工厂注入 scheduler（Test.idle 用 ScriptScheduler::runTimeout 创建定时 Promise），
        // 再 move 入引擎。scheduler 由 ScriptManager 拥有，生命周期长于插件加载/引擎重建。
        auto gameTestFactory = std::make_unique<GameTestModuleBinding>();
        gameTestFactory->setScheduler(&sm->scriptManager().scheduler());
        sm->scriptManager().engine().addModuleFactory(std::move(gameTestFactory));
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
            // 新世界目录可能尚不存在（GameTestServer 默认 worldName="gametest" 在 saves/ 下无残留），
            // LevelDatCodec::writeInitial→_atomicWrite 仅 ofstream 打开文件不建父目录，缺此步会报
            // "Cannot open level.dat for writing" 致 initializeSharedStorage 随后 "World not found"。
            // IntegratedServer 走 quick_play_world（saves/ 下已存在）故未暴露此缺口。
            std::error_code ec;
            std::filesystem::create_directories(worldDir, ec);
            if (ec) {
                spdlog::warn("Failed to create world dir '{}': {}", worldDir.string(), ec.message());
            }
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
    // simulationDistance 用 kGameTestSimulationDistance（32）而非 params.simulationDistance，
    // 确保 ServerWorld config → EntityManager simulationDistance=32 关闭冻结门控（见上方注释）。
    auto dimInitResult = m_dimensionManager->initialize(static_cast<u64>(params.seed),
        params.viewDistance,
        kGameTestSimulationDistance,
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

    // 加载行为包：必须在 initializeWorld（含 ScriptManager::initialize→engine.initialize 创建 runtime）
    // 之后，loadPlugins 检查 isInitialized 且需 runtime 已就绪；否则 "Skip behavior pack loading:
    // script system not initialized"。行为包内 gametest.register(...) 把 JS 测试注册进 GameTestRegistry。
    // 失败仅 warn 不阻断（缺包时仍可跑原生内置测试）。对齐在线路径 main.cpp::initializeServerGameTest。
    if (auto packResult = loadBehaviorPacks(); packResult.failed()) {
        spdlog::warn("[GameTest] Behavior pack loading failed: {}", packResult.error().message());
    }

    // 额外行为包扫描目录（仓库内 tests/integrated 等）：每个目录下含 manifest.json 的子目录
    // 作为独立行为包加载。用于让 GameTest 跑仓库内 TS 编译产物的行为包。须在 loadBehaviorPacks
    // 之后（script system 已 initialized）、_selectAndBuildRunner 之前（注册发生在 runner 选测试前）。
    // loadPlugins 内部会 startPlugins，二次调用时已 Running 的包被状态门控跳过，新包正常 start。
    if (auto* sm = scriptManager()) {
        for (const auto& extraDir : m_params.extraBehaviorPackDirs) {
            if (extraDir.empty() || !std::filesystem::is_directory(extraDir)) {
                spdlog::warn("[GameTest] Skip extra behavior pack dir (not a directory): '{}'", extraDir.string());
                continue;
            }
            spdlog::info("[GameTest] Loading extra behavior packs from '{}'", extraDir.string());
            auto extraResult = sm->loadPlugins(extraDir.string());
            if (extraResult.failed()) {
                spdlog::warn("[GameTest] Extra behavior pack loading failed for '{}': {}",
                    extraDir.string(),
                    extraResult.error().message());
            }
        }
    }

    // 把已加载的行为包列表接入 TemplateManager，使 GameTest 结构名（如 startertests:mediumglass）
    // 能从 behavior_packs/<包>/structures/<ns>/<path>.mcstructure 加载。须在 loadBehaviorPacks 之后
    // （packList 已扫描填充），且在 _selectAndBuildRunner 之前（runner 构造批次放置结构即取模板）。
    if (auto* sm = scriptManager()) {
        auto* packList = sm->scriptManager().packList();
        if (packList != nullptr && !packList->empty()) {
            m_structureSource = std::make_unique<BehaviorPackStructureSource>(*packList);
            mc::world::gen::jigsaw::JigsawAssembler::getTemplateManager().setStructurePackSource(
                m_structureSource.get());
            spdlog::info("[GameTest] Structure pack source injected ({} behavior pack(s))", packList->size());
        }
    }

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
    // 同时过滤 suite:broken 标签的测试：基岩 /gametest runset 默认集合不含 broken 标签，
    // suite:broken 是社区约定标记"vanilla 已知失败"的测试（如 phantoms_should_fly_from_cats
    // 验证 vanilla phantom 卡猫 bug，原版亦失败）。--gametest 门面对齐 runset 语义跳过此类测试，
    // 不计入失败。TODO: 如需显式跑 broken 测试，可加 --include-broken 参数走 runall 语义。
    std::vector<std::shared_ptr<BaseGameTestFunction>> runnable;
    runnable.reserve(selected.size());
    for (auto& fn : selected) {
        if (fn == nullptr || fn->data().manualOnly()) {
            continue;
        }
        bool broken = false;
        for (const auto& tag : fn->tags()) {
            if (tag == "suite:broken") {
                broken = true;
                break;
            }
        }
        if (broken) {
            continue;
        }
        runnable.push_back(fn);
    }

    if (runnable.empty()) {
        return false;
    }

    // 按批次名（batchName）分组构造批次，对齐基岩 /gametest runset 的预定义批次语义。
    // 基岩预定义 "day"/"night" 批次：day 批设白天，night 批设夜晚。此前实现把所有测试塞进单 "default"
    // 批（空 environment），batch("night") 仅设 batchName 字符串不真正设夜晚——致依赖昼夜的测试
    // （蜘蛛夜晚攻击/亡灵白天燃烧的负向判定等）无法工作。现按 batchName 分组：
    //   - "night"    → TimeOfDayEnvironment(18000)  夜晚（skyDarkening≈11，露天亮度≈0.083<0.5）
    //   - "day"/其他 → TimeOfDayEnvironment(6000)   正午（skyDarkening=0，露天亮度=1.0>0.5）
    // day 批用 6000（正午）而非 1000（6:00AM）：getBrightness 对齐 vanilla
    // getLightLevelDependentMagicValue 后用 getMaxLocalRawBrightness（含 getSkyDarkening 时间衰减）+
    // gamma 曲线 f/(4-3f)。dayTime=1000 时项目 getCelestialAngle(1000)=0.0417 → skyDarkening≈4 →
    // 露天 rawBrightness=15-4=11 → f=0.733 → f1=0.407 <0.5，致亡灵白天燃烧测试
    // （skeleton_burns_in_daylight）isInDaylight 的 brightness>0.5 判定失败不燃烧而超时。
    // dayTime=6000 正午 skyDarkening=0 → rawBrightness=15 → f=1.0 → f1=1.0 >0.5 燃烧正常。
    // environment 在 _applyBatchEnvironmentSetup 经 MinecraftEnvironmentApplier 应用（调
    // TimeManager::setDayTime）。daylight cycle 开着 dayTime 每 tick 递增，但单批测试通常数百 tick
    // 内完成，18000+数百仍 <24000 在夜晚区间，6000+数百仍 <12000 在白天区间。
    // TODO: 完整对齐基岩 GameTestBatchFactory 按 environment 分组 + MAX_TESTS_PER_BATCH=50 切片。
    std::unordered_map<std::string, std::shared_ptr<TestEnvironmentDefinition>> envCache;
    auto getEnvForBatch = [&](const std::string& name) -> std::shared_ptr<TestEnvironmentDefinition> {
        const bool isNight = (name == "night");
        const std::string key = isNight ? std::string{"night"} : std::string{"day"};
        auto it = envCache.find(key);
        if (it != envCache.end()) {
            return it->second;
        }
        // 复合环境：时间 + 强制晴天。day/night 批除设时间外，必须强制晴天（WeatherEnvironment::Clear），
        // 否则世界初始化时天气随机下雨（isRaining=true）会让 calculateSkyDarkening 在基础值上 +3
        // （雷暴 +10），污染所有依赖露天 brightness 的测试：
        //   - lighting 包：露天 brightness=15-skyDarkening，下雨时=12 而非 15，brightness_open_sky 等
        //     断言失败；skyLight 本身不受影响（原始天空光不含 skyDarkening），但 brightness 综合值受污染。
        //   - mob_behavior 包：亡灵白天燃烧判定 brightness>0.5 经 gamma 曲线，下雨 skyDarkening=3 致
        //     rawBrightness=12 → f=0.8 → f1=0.667 仍 >0.5（恰能燃烧），但雷暴 skyDarkening=10 致
        //     rawBrightness=5 → f=0.333 → f1=0.2 <0.5 不燃烧，skeleton_burns_in_daylight 随机失败。
        // 强制晴天对齐基岩 GameTest day/night 批「确定晴天」语义，消除天气随机性导致的测试 flaky。
        auto env = std::make_shared<AllOfEnvironment>(std::vector<std::shared_ptr<TestEnvironmentDefinition>>{
            std::make_shared<TimeOfDayEnvironment>(isNight ? 18000 : 6000),
            std::make_shared<WeatherEnvironment>(WeatherEnvironment::Type::Clear),
        });
        envCache[key] = env;
        return env;
    };

    // 按 batchName 聚合测试到有序批次（保留首次出现顺序，对齐基岩 runset 顺序执行语义）
    // 注意：用 fn->data().batchName()（TestData 的批次名，JS batch("night") 设此字段），而非
    // fn->batchName()（BaseGameTestFunction::m_batchName，ScriptGameTestFunction 构造时被传入
    // className "MobBehaviorTests"，是历史命名混淆，非真实批次名）。
    std::vector<std::string> batchOrder;
    std::unordered_map<std::string, std::vector<std::shared_ptr<BaseGameTestFunction>>> byBatch;
    for (auto& fn : runnable) {
        const std::string& bn = fn->data().batchName();
        if (byBatch.find(bn) == byBatch.end()) {
            batchOrder.push_back(bn);
        }
        byBatch[bn].push_back(fn);
    }

    std::vector<GameTestBatch> batches;
    for (const auto& bn : batchOrder) {
        batches.emplace_back(bn,
            std::move(byBatch[bn]),
            GameTestRegistry::instance().getBeforeBatchFunction(bn),
            GameTestRegistry::instance().getAfterBatchFunction(bn),
            getEnvForBatch(bn));
    }

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

    // 释放 GameTestRegistry 中脚本测试函数的 JS 回调句柄。registry 是进程级单例，跨 GameTestServer
    // 实例常驻；ScriptGameTestFunction 持的 JS 句柄绑当前引擎 runtime。须在 stopCore（销毁引擎）前释放，
    // 否则引擎销毁后 registry 析构 function 时对已死 JSContext 调 JS_FreeValue → use-after-free 崩溃
    // （atexit 阶段，测试已结束但进程退出时报 ACCESS_VIOLATION）。function 对象本身保留（仅清句柄）。
    GameTestRegistry::instance().releaseAllScriptResources();

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
