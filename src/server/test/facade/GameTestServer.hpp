#pragma once

#include "common/core/GameDirectory.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp" // GameMode / Difficulty / WorldType / i64 / i32
#include "common/resource/ResourceLocation.hpp"
#include "server/application/MinecraftServer.hpp" // mc::server::MinecraftServer
#include "server/settings/ServerSettings.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc::test {

class GameTestRunner;
class JUnitTestReporter;
class LogTestReporter;
class FailedTestCollector;
class BehaviorPackStructureSource;

/**
 * @brief GameTestServer 启动参数（对齐 Java `GameTestMainUtil` CLI + `IntegratedServerParams` 世界字段）。
 *
 * 字段分两组：
 * 1. 世界字段（镜像 `IntegratedServerParams`）：GameTestServer 须建一个真实可 tick 的主世界，
 *    复用 `IntegratedServer` 的初始化序列（数据包扫描+vanilla 注入+维度初始化），但不起线程、不联网。
 * 2. GameTest CLI 字段（对齐 Java `--universe`/`--report`/`--tests`/`--verify`）。
 *
 * `gameDirectoryRoot` 为空时用 `GameDirectory::defaultDirectory()`（CI 下应显式传临时目录避免竞态）。
 */
struct GameTestServerParams {
    // === 世界字段 ===
    std::string worldName = "gametest";
    std::string gameDirectoryRoot; // 空 → 默认游戏目录
    i64 seed = 0;
    GameMode defaultGameMode = GameMode::Creative;
    i32 viewDistance = 6;
    i32 simulationDistance = 6;
    i32 tickRate = 20;
    // 超平坦世界（minecraft:flat）：FlatChunkGenerator 只填 4 层（vs normal 全高度 384 密度采样），
    // 区块生成大幅加速；plains 生物群系与 dark_cavern 系列测试一致。classic_flat 地表在 Y=3
    // （bedrock Y=0 + dirt Y=1-2 + grass Y=3），gridStartY=4 把结构方块放在首个 air 层，
    // 结构内容从 Y=5 开始，完全脱离 flat 地形层（Y=0..3），无方块/光照冲突。
    WorldType worldType = WorldType::Flat;
    resource::ResourceLocation worldPresetId{"minecraft", "flat"};
    Difficulty difficulty = Difficulty::Normal;
    bool hardcore = false;
    bool allowCommands = true;
    bool isNewWorld = true;

    // === GameTest CLI 字段 ===
    /// `--tests`：测试名通配符（如 `"ExampleTests.*"`），空表示跑全部。
    std::string testsFilter;
    /// `--report`：JUnit XML 输出路径（相对 gameDirectoryRoot 或绝对）。空表示不写文件。
    std::string reportPath;
    /// `--gametest-packs`：额外行为包扫描目录（仓库内 tests/integrated 等）。
    /// 每个目录下含 manifest.json 的子目录会被当作独立行为包加载。空表示不扫额外目录，
    /// 仅扫 `gameDirectory().behaviorPacksDir()`（默认 ~/minecraft_reborn/behavior_packs）。
    /// 用于让 GameTest 跑仓库内 TS 编译产物的行为包，而非外部目录。
    std::vector<std::filesystem::path> extraBehaviorPackDirs;
    /// `--verify`：×4 旋转 ×repeatCount 压测（对齐 Java `rotateAndMultiply`）。第一阶段 TODO 展开。
    bool verify = false;
    /// `--verify` 时每测试重复次数（Java 默认 100）。
    i32 repeatCount = 1;
    /// 测试网格起始绝对方块坐标（默认原点附近，对齐 Java ±14999992 范围内的安全点）。
    i32 gridStartX = 0;
    /// 超平坦地表 Y=3（grass），结构方块放 Y=4，结构内容从 Y=5 起。
    i32 gridStartY = 4;
    i32 gridStartZ = 0;
    /// 每行测试数（对齐 Java `DEFAULT_TESTS_PER_ROW=8`）。
    i32 testsPerRow = 8;
    /// 单次 run() 最大 tick 数（防卡死，超时强行停止）。0 表示不限。
    std::size_t maxTicks = 600000; // 30000s @ 20TPS，足够 CI
};

/**
 * @brief 无头 GameTest 运行门面：`MinecraftServer` 子类，同步 tick，不起线程、不联网。
 *
 * 对齐 Java 1.21.11 `GameTestServer`（headless）：在调用线程内循环 `MinecraftServer::tick()` 推进世界，
 * 同时驱动 `GameTestTicker`（单例）推进测试实例。退出码 = 失败的 required 测试数（0=全过）。
 *
 * 与 `IntegratedServer`/`StandaloneServer` 的区别：
 * - 不起 `m_serverThread`（同步 tick，`run()` 在调用线程循环）。
 * - 不建 `m_serverNetwork`/本地客户端连接/握手/Play 路由（无玩家、无网络）。
 * - `initialize()` 复用 `IntegratedServer` 的世界初始化序列（数据包+维度+命令注册），尾段挂 `GameTestRunner`。
 *
 * 生命周期：
 * 1. `initialize(params)`：建世界 → 注册 `/gametest` 命令 → 选测试 → 构造 `GameTestRunner`。
 * 2. `run()`：循环 `tick()`（基类世界 tick + `GameTestTicker::instance().tick()` + `runner->tick()`），
 *    直到 `runner->isComplete()` 或 `maxTicks` 超时。
 * 3. `exitCode()`：`runner->failedRequiredCount()`（CI 契约：0=全必需通过）。
 * 4. `stop()`：`requestStop()` + `stopCore()` 落盘 + 关闭 reporter。
 *
 * 门面纪律：外部（CI 宿主、`tests/`）仅经此门面运行测试；内部 `GameTestRunner`/`GameTestTicker` 不对外。
 */
class GameTestServer final : public mc::server::MinecraftServer {
public:
    GameTestServer();
    ~GameTestServer() override;

    GameTestServer(const GameTestServer&) = delete;
    GameTestServer& operator=(const GameTestServer&) = delete;

    // ========== IServer 接口实现 ==========

    [[nodiscard]] mc::Result<void> initialize() override { return initialize(GameTestServerParams{}); }
    void shutdown() override;

    [[nodiscard]] bool isIntegrated() const noexcept override { return false; }
    [[nodiscard]] bool isDedicated() const noexcept override { return false; }

    [[nodiscard]] mc::Result<void> publishToLan(i32 /*port*/, bool /*allowCheats*/) override
    {
        return mc::Error(mc::ErrorCode::Unsupported, "GameTestServer does not support LAN publishing");
    }

    [[nodiscard]] const mc::GameDirectory& gameDirectory() const noexcept override { return m_gameDirectory; }

    // ========== GameTestServer 特有接口 ==========

    /**
     * @brief 初始化世界 + 测试运行器（不起线程）。
     *
     * 复用 `IntegratedServer` 世界初始化序列：数据包扫描+vanilla 注入 → `initializeRegistries(true)` →
     * `initializeCoreManagers()` → 写初始 level.dat → `initializeSharedStorage` → 维度初始化 →
     * `attachWorldBindings` → `initializeWorld` → 交互/同步管理器 → `setupWorldCallbacks`。
     * 尾段：注册 `/gametest` 命令 + 选测试 + 构造 `GameTestRunner` + 挂 reporter。
     */
    [[nodiscard]] mc::Result<void> initialize(const GameTestServerParams& params);

    /**
     * @brief 同步运行主循环直到测试完成或超时。在调用线程内循环 `tick()`。
     *
     * @return 失败的 required 测试数（0=全过）。`initialize` 未成功时返回 1。
     */
    [[nodiscard]] i32 run();

    /**
     * @brief 退出码 = 失败的 required 测试数（CI 契约：0=全必需通过）。
     */
    [[nodiscard]] i32 exitCode() const noexcept;

    /**
     * @brief 停止服务器：落盘 + 关闭 reporter + shutdownManagers。
     */
    void stop();

    [[nodiscard]] const GameTestServerParams& params() const noexcept { return m_params; }

private:
    /**
     * @brief 单 tick：基类世界 tick（`MinecraftServer::tick()`）+ `GameTestTicker` 推进 + runner 推进。
     *
     * 注：`MinecraftServer::tick()` 内部已推进维度/实体/时间；`GameTestTicker::instance().tick()` 推进
     * 测试实例状态机；`runner->tick()` 推进批次调度。三者顺序：世界 tick → ticker → runner。
     */
    void tickOnce();

    /// 从 `GameTestRegistry` 选测试（应用 `testsFilter`）并构造批次。
    [[nodiscard]] bool _selectAndBuildRunner();

    GameTestServerParams m_params;
    mc::server::ServerSettings m_settings; // 基类 m_settings 引用绑定到此
    mc::GameDirectory m_gameDirectory;

    std::unique_ptr<GameTestRunner> m_runner;
    // reporter 经 GlobalTestReporter 单例共享持有，故此处用 shared_ptr（addReporter 拷贝一份）。
    std::shared_ptr<JUnitTestReporter> m_junitReporter;
    std::shared_ptr<LogTestReporter> m_logReporter;
    // 失败测试收集器：run() 结束后从 failedTestNames() 提取失败列表，供重跑过滤使用。
    std::shared_ptr<FailedTestCollector> m_failedCollector;
    // 行为包结构资源源：把 BehaviorPackList 适配为 IStructurePackSource 注入 TemplateManager，
    // 使 GameTest 结构名（如 startertests:mediumglass）能从行为包加载 .mcstructure。地址须稳定，
    // TemplateManager 持非拥有指针，故用 unique_ptr 成员保活。
    std::unique_ptr<BehaviorPackStructureSource> m_structureSource;
    i32 m_exitCode = 0;
    bool m_runnerBuilt = false;
};

} // namespace mc::test
