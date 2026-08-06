#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp" // LiteralCommandNode / ArgumentCommandNode
#include "server/command/ServerCommandSource.hpp"

#include <string>

namespace mc::test {

/**
 * @brief `/gametest` 命令门面：在线调试入口，注册到 `CommandRegistry`。
 *
 * 对齐基岩版 `/gametest` 命令 + Java `TestCommand`：在 `IntegratedServer` 运行时经此命令在线触发测试，
 * 而非经无头 `GameTestServer`。子命令：
 * - `/gametest run [<testName>]`：运行单个测试（默认全部），结构放置到执行者附近或默认网格起点。
 * - `/gametest runall`：运行全部非 manualOnly 测试。
 * - `/gametest pos <blockPos>`：设置下一个测试的放置原点。
 * - `/gametest locate <testName>`：查询某测试的结构放置位置。
 * - `/gametest clear`：清空所有运行中测试（经 `GameTestTicker::clear()`）。
 *
 * 注册：`CommandRegistry::registerDefaults()` 追加 `GameTestCommand::registerTo(dispatcher)`；
 * `setRequirement` OP-only（权限 2）。
 *
 * 第一阶段实现 `run`/`runall`/`clear` 的核心路径；`pos`/`locate` 留 TODO stub（结构定位需 StructureBlockEntity）。
 *
 * 门面纪律：外部（`CommandRegistry`）仅经此门面注册命令；命令内部经 `GameTestRegistry`/`GameTestTicker`
 * 等内部设施操作，不直接接触 `ServerWorld` 之外的细节。
 */
class GameTestCommand {
public:
    /**
     * @brief 注册 `/gametest` 命令到调度器。
     *
     * 由 `CommandRegistry::registerDefaults()` 调用；`GameTestServer::initialize` 亦调用以确保命令可用。
     */
    static void registerTo(mc::command::CommandDispatcher<mc::command::ServerCommandSource>& dispatcher);

    /**
     * @brief 清理已完成（succeed/fail/stopped）的在线命令路径测试实例。
     *
     * `/gametest run` 启动的实例由本命令静态保活（ticker 持裸指针），完成后须从此处移除以回收内存。
     * 由生产服务器 post-tick 回调每帧调用（驱动 ticker 后）。
     */
    static void cleanupCompletedInstances();

    /**
     * @brief 强制清空所有在线命令路径测试实例（含未完成者）。
     *
     * 生产服务器关闭时调用：在脚本引擎销毁前清空持 `m_runResult`（Promise 句柄）/`IScriptBindingContext*`
     * 的实例，避免静态容器进程退出析构时访问已死 JS 上下文。配合 `GameTestTicker::forceStop()` 使用。
     * `GameTestServer` 路径自有 `stop()` 顺序保护，不经此方法。
     */
    static void forceClearAllInstances();

private:
    // 子命令执行器（返回 i32，对齐项目 CommandCallback 签名）
    static i32 _run(mc::command::CommandContext<mc::command::ServerCommandSource>& context);
    static i32 _runAll(mc::command::CommandContext<mc::command::ServerCommandSource>& context);
    static i32 _pos(mc::command::CommandContext<mc::command::ServerCommandSource>& context);
    static i32 _locate(mc::command::CommandContext<mc::command::ServerCommandSource>& context);
    static i32 _clear(mc::command::CommandContext<mc::command::ServerCommandSource>& context);

    /**
     * @brief 把一批测试函数放入 `GameTestTicker` 运行。
     *
     * @param source 命令源（取 world 与执行者位置）
     * @param testNames 要运行的测试名列表（空=全部）
     * @param filter 通配符（与 testNames 二选一）
     */
    static i32 _launchTests(
        mc::command::ServerCommandSource& source, const std::string& filter, const std::string& feedbackLabel);
};

} // namespace mc::test
