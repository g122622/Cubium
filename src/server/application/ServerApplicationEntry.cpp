/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ServerApplicationEntry.hpp"

#include <gflags/gflags.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <csignal>
#include <thread>

// GameTest 集成测试框架——仅 minecraft-server exe 编译（client exe 不链接 mc_test）。
// 经此接入生产服务器的 /gametest 命令 + GameTestTicker 驱动 + @minecraft/server-gametest JS 模块。
#include "common/test/framework/environment/EnvironmentRegistry.hpp"
#include "common/test/framework/registry/GameTestRegistry.hpp"
#include "common/test/framework/ticker/GameTestTicker.hpp"
#include "server/command/CommandRegistry.hpp"               // commandRegistry().dispatcher() 需完整类型
#include "server/mod/bedrock/addon/ServerScriptManager.hpp" // scriptManager()->scriptManager().engine().addModuleFactory
#include "server/test/facade/GameTestCommand.hpp"
#include "server/test/facade/GameTestServer.hpp" // --gametest 无头批量自动跑门面
#include "server/test/minecraft/structure/BehaviorPackStructureSource.hpp"
#include "server/test/minecraft/structure/GameTestStructureBootstrap.hpp"
#include "server/test/native/builtin/BuiltinNativeTests.hpp"
#include "server/test/script/GameTestModuleBinding.hpp"

#include "common/mod/bedrock/addon/pack/BehaviorPackList.hpp" // BehaviorPackList 完整类型（packList()->empty/size）
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp" // JigsawAssembler::getTemplateManager

namespace mc::server {

// ============================================================================
// server 专属 gflags flag 定义（TU 顶层全局作用域）。
// gflags 把连字符规范化为下划线：--config 命中 config、--gametest 命中 gametest。
// ============================================================================
DEFINE_string(config, "", "Use custom config file path");
DEFINE_bool(gametest,
    false,
    "Run all registered GameTests headless and exit "
    "(exit code = number of failed required tests; 0 = all pass)");

std::atomic<bool> ServerApplicationEntry::s_shouldExit{false};

void ServerApplicationEntry::_signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        spdlog::info("Received shutdown signal");
        s_shouldExit = true;
    }
}

void ServerApplicationEntry::_initializeServerGameTest(MinecraftServer& server)
{
    // 内置样例测试（保链接期保留 TU）+ 默认环境 + 程序化空模板
    mc::test::registerBuiltinNativeTests();
    mc::test::EnvironmentRegistry::instance().registerBuiltinDefaults();
    mc::test::ensureBuiltinStructureTemplates();

    // /gametest 命令
    mc::test::GameTestCommand::registerTo(server.commandRegistry().dispatcher());

    // @minecraft/server-gametest JS 模块（行为包可 import { register } from "@minecraft/server-gametest"）
    if (auto* sm = server.scriptManager()) {
        // 注入 scheduler（Test.idle 用 runTimeout 创建定时 Promise），再 move 入引擎。
        auto gameTestFactory = std::make_unique<mc::test::GameTestModuleBinding>();
        gameTestFactory->setScheduler(&sm->scriptManager().scheduler());
        sm->scriptManager().engine().addModuleFactory(std::move(gameTestFactory));
        spdlog::info("[GameTest] Registered @minecraft/server-gametest module factory");
    }

    // 加载行为包：必须在 GameTest 模块工厂注册之后（否则 import "@minecraft/server-gametest" 失败）。
    // 行为包内的 gametest.register(...) 调用在此阶段执行，把 JS 测试注册进 GameTestRegistry。
    if (auto packResult = server.loadBehaviorPacks(); packResult.failed()) {
        spdlog::warn("[GameTest] Behavior pack loading failed: {}", packResult.error().message());
    }

    // 把已加载行为包列表接入 TemplateManager，使 GameTest 结构名能从 behavior_packs 加载 .mcstructure。
    // 须在 loadBehaviorPacks 之后、/gametest run 启动测试之前。source 持 BehaviorPackList 引用，
    // 用 static 保活（与 server 生命周期一致）。
    if (auto* sm = server.scriptManager()) {
        auto* packList = sm->scriptManager().packList();
        if (packList != nullptr && !packList->empty()) {
            static mc::test::BehaviorPackStructureSource s_structureSource(*packList);
            mc::world::gen::jigsaw::JigsawAssembler::getTemplateManager().setStructurePackSource(&s_structureSource);
            spdlog::info("[GameTest] Structure pack source injected ({} behavior pack(s))", packList->size());
        }
    }

    // post-tick 驱动 GameTestTicker（/gametest run/runall 启动的实例由此推进）+
    // 回收已完成的 /gametest 在线实例（ticker 持裸指针，须独立保活容器清理）。
    server.addPostTickCallback([]() {
        mc::test::GameTestTicker::instance().tick();
        mc::test::GameTestCommand::cleanupCompletedInstances();
    });
    spdlog::info("[GameTest] /gametest command + ticker driver attached to server");
}

void ServerApplicationEntry::_cleanupServerGameTest()
{
    // 须在 server.stop()（其内部 ServerScriptManager::shutdown 销毁 JS 引擎）之前调用：
    // 1. forceStop 清 GameTestTicker 的裸指针引用；
    // 2. forceClearAllInstances 析构所有在线实例（unique_ptr），释放其 m_runResult 持有的 Promise 句柄
    //    与 ScriptGameTestFunction 持有的 IScriptBindingContext*（仅 releaseValue，不再访问）。
    // 3. releaseAllScriptResources 释放 GameTestRegistry 单例中 ScriptGameTestFunction 持有的 JS 回调句柄
    //    （registry 跨用例/进程退出常驻，不在 forceClearAllInstances 范围；不释放则引擎销毁后 registry
    //    析构 function 时对已死 JSContext 调 JS_FreeValue 崩溃）。
    // 顺序保证：步骤 1-3 时脚本上下文仍有效，releaseValue 安全。
    mc::test::GameTestTicker::instance().forceStop();
    mc::test::GameTestCommand::forceClearAllInstances();
    mc::test::GameTestRegistry::instance().releaseAllScriptResources();
    spdlog::info("[GameTest] ticker + instances + script resources cleared before server shutdown");
}

void ServerApplicationEntry::onFlagsParsed()
{
    // config：空串 → nullopt（用默认路径）；非空 → 指定路径。
    if (!FLAGS_config.empty()) {
        m_params.configPath = FLAGS_config;
    }

    m_gametestMode = FLAGS_gametest;
}

void ServerApplicationEntry::prepareRun()
{
    // 安装信号处理：SIGINT/SIGTERM 置 s_shouldExit，runApplication 主循环轮询优雅退出。
    std::signal(SIGINT, _signalHandler);
    std::signal(SIGTERM, _signalHandler);
}

int ServerApplicationEntry::runApplication()
{
    // --gametest 模式：走 GameTestServer 无头批量自动跑门面（对齐 Java GameTestMainUtil）。
    // GameTestServer 是 MinecraftServer 子类，不起线程/不联网，在调用线程内同步 tick 推进世界 +
    // GameTestTicker + runner，跑完全部注册测试后返回（exitCode = 失败的 required 测试数）。
    // 与下方 StandaloneServer 生产路径（/gametest 命令交互式 + post-tick 回调）互斥：
    // GameTestServer::initialize 内部已含行为包加载 + JS 模块注册 + 结构源注入 + runner 构造，
    // 不复用 _initializeServerGameTest（后者为在线路径设计，挂 post-tick 回调驱动 ticker）。
    if (m_gametestMode) {
        mc::test::GameTestServer gtServer;
        mc::test::GameTestServerParams gtParams;
        auto gtInit = gtServer.initialize(gtParams);
        if (gtInit.failed()) {
            spdlog::error("Failed to initialize GameTestServer: {}", gtInit.error().toString());
            return 1;
        }
        const int gtExit = gtServer.run();
        gtServer.shutdown();
        spdlog::info("GameTestServer exited with code {}", gtExit);
        return gtExit;
    }

    // 创建服务端实例
    StandaloneServer server;

    // 初始化
    auto initResult = server.initialize(m_params);
    if (initResult.failed()) {
        spdlog::error("Failed to initialize server: {}", initResult.error().toString());
        _cleanupServerGameTest(); // no-op（GameTest 尚未接入），保持错误路径一致
        return 1;
    }

    // 接入 GameTest 框架（/gametest 命令 + GameTestTicker 驱动 + JS gametest 模块）。
    // 仅 minecraft-server exe，须在 initialize 成功后（commandRegistry/scriptManager 已就绪）调用。
    _initializeServerGameTest(server);

    // 启动服务端主循环（非阻塞，内部线程由 StandaloneServer 持有）
    auto runResult = server.run();
    if (runResult.failed()) {
        spdlog::error("Failed to start server: {}", runResult.error().toString());
        _cleanupServerGameTest(); // server 即将析构销毁引擎，先清实例释放 JS 句柄
        return 1;
    }

    // 等待退出信号
    while (!s_shouldExit && server.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 关闭前清理 GameTest（实例持 JS Promise 句柄，须在脚本引擎销毁前析构）。
    _cleanupServerGameTest();

    // 停止服务端（内部会 join 主循环线程，再回写玩家状态、落盘存档、清理资源）
    server.stop();

    spdlog::info("Server exited successfully");
    return 0;
}

void ServerApplicationEntry::onErrorCleanup()
{
    // 异常路径：server 即将析构销毁脚本引擎，先清 GameTest 实例释放 JS 句柄。
    _cleanupServerGameTest();
}

} // namespace mc::server
