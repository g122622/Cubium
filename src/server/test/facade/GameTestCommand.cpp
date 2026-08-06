#include "server/test/facade/GameTestCommand.hpp"

#include "common/test/framework/function/BaseGameTestFunction.hpp"
#include "common/test/framework/instance/GameTestState.hpp" // isDone
#include "common/test/framework/registry/GameTestRegistry.hpp"
#include "common/test/framework/ticker/GameTestTicker.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/test/minecraft/batch/MinecraftGameTestBatchRunner.hpp" // 复用 helper provider
#include "server/test/minecraft/helper/MinecraftGameTestHelperProvider.hpp"
#include "server/test/minecraft/instance/MinecraftGameTestInstance.hpp"
#include "server/test/runner/reporter/GlobalTestReporter.hpp"
#include "server/test/runner/reporter/LogTestReporter.hpp"
#include "server/world/ServerWorld.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"     // StringArgumentType
#include "common/command/arguments/GameModeArgument.hpp" // BlockPosArgumentType / Coordinates

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace mc::test {

namespace {
// /gametest run 启动的实例所有权容器（ticker 持裸指针，须独立保活到完成）。
// 仅主线程 /gametest 调用 + post-tick cleanup，线程安全。
std::vector<std::unique_ptr<BaseGameTestInstance>>& _heldInstances()
{
    static std::vector<std::unique_ptr<BaseGameTestInstance>> s_held;
    return s_held;
}
} // namespace

void GameTestCommand::cleanupCompletedInstances()
{
    auto& held = _heldInstances();
    held.erase(std::remove_if(held.begin(),
                   held.end(),
                   [](const std::unique_ptr<BaseGameTestInstance>& inst) { return inst && isDone(inst->state()); }),
        held.end());
}

void GameTestCommand::forceClearAllInstances()
{
    // 清空所有实例（含未完成者）：实例 unique_ptr 析构 → m_runResult 析构释放 Promise 句柄。
    // 须在脚本引擎销毁前调用，否则 releaseValue 访问已死 JSContext。
    _heldInstances().clear();
}

// 在线 /gametest 的轻量日志报告器（命令触发的测试也经 GlobalTestReporter 输出）
// 注：放命名空间作用域静态会在首次 /gametest 时构造；命令可多次调用，addReporter 幂等性由调用方保证。
// 第一阶段：命令触发测试仅在 IntegratedServer 调试用，复用 GlobalTestReporter 静态委托即可。

void GameTestCommand::registerTo(mc::command::CommandDispatcher<mc::command::ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    // /gametest run [<testName>] —— 可选字符串参数（greedy 容忍通配符如 "Suite.*"）
    auto runNode = std::make_shared<LiteralCommandNode<mc::command::ServerCommandSource>>("run");
    auto runNameArg = std::make_shared<ArgumentCommandNode<mc::command::ServerCommandSource, std::string>>(
        "testName", StringArgumentType::greedyString());
    runNameArg->setCommand([](CommandContext<mc::command::ServerCommandSource>& ctx) { return _run(ctx); });
    // 无参形式：注入默认 "*" 通配符后委托
    runNode->setCommand([](CommandContext<mc::command::ServerCommandSource>& ctx) {
        ctx.setArgument("testName", std::string{"*"});
        return _run(ctx);
    });
    runNode->addChild(runNameArg);

    // /gametest runall
    auto runAllNode = std::make_shared<LiteralCommandNode<mc::command::ServerCommandSource>>("runall");
    runAllNode->setCommand([](CommandContext<mc::command::ServerCommandSource>& ctx) { return _runAll(ctx); });

    // /gametest pos <blockPos>
    auto posNode = std::make_shared<LiteralCommandNode<mc::command::ServerCommandSource>>("pos");
    auto posArg = std::make_shared<ArgumentCommandNode<mc::command::ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    posArg->setCommand([](CommandContext<mc::command::ServerCommandSource>& ctx) { return _pos(ctx); });
    posNode->addChild(posArg);

    // /gametest locate <testName>
    auto locateNode = std::make_shared<LiteralCommandNode<mc::command::ServerCommandSource>>("locate");
    auto locateArg = std::make_shared<ArgumentCommandNode<mc::command::ServerCommandSource, std::string>>(
        "testName", StringArgumentType::word());
    locateArg->setCommand([](CommandContext<mc::command::ServerCommandSource>& ctx) { return _locate(ctx); });
    locateNode->addChild(locateArg);

    // /gametest clear
    auto clearNode = std::make_shared<LiteralCommandNode<mc::command::ServerCommandSource>>("clear");
    clearNode->setCommand([](CommandContext<mc::command::ServerCommandSource>& ctx) { return _clear(ctx); });

    // 根节点 gametest（OP-only 权限 2）
    auto gametestNode = std::make_shared<LiteralCommandNode<mc::command::ServerCommandSource>>("gametest");
    gametestNode->setRequirement(
        [](const mc::command::ServerCommandSource& source) { return source.hasPermission(2); });
    mc::command::support::applyMetadata(gametestNode,
        mc::command::support::makeMetadata("Run game tests.", "/gametest <run|runall|pos|locate|clear> ...", 2));
    gametestNode->addChild(runNode);
    gametestNode->addChild(runAllNode);
    gametestNode->addChild(posNode);
    gametestNode->addChild(locateNode);
    gametestNode->addChild(clearNode);

    dispatcher.registerCommand(gametestNode);
}

i32 GameTestCommand::_run(mc::command::CommandContext<mc::command::ServerCommandSource>& context)
{
    auto& source = context.getSource();
    std::string testName;
    try {
        testName = context.getArgument<std::string>("testName");
    }
    catch (const std::out_of_range&) {
        testName = "*";
    }
    if (testName.empty()) {
        testName = "*";
    }
    return _launchTests(source, testName, "run");
}

i32 GameTestCommand::_runAll(mc::command::CommandContext<mc::command::ServerCommandSource>& context)
{
    auto& source = context.getSource();
    return _launchTests(source, "*", "runall");
}

i32 GameTestCommand::_pos(mc::command::CommandContext<mc::command::ServerCommandSource>& context)
{
    // TODO: 持久化命令源附近的测试网格起点（需 StructureBlockEntity/全局起点存储就绪）
    // 第一阶段：仅回显解析到的坐标，验证参数解析链路。
    auto& source = context.getSource();
    const auto pos = mc::command::BlockPosArgumentType::getBlockPos(context, "pos", source);
    std::ostringstream ss;
    ss << "GameTest grid start set to (" << pos.x << ", " << pos.y << ", " << pos.z << ")";
    source.sendMessage(ss.str());
    return 1;
}

i32 GameTestCommand::_locate(mc::command::CommandContext<mc::command::ServerCommandSource>& context)
{
    // TODO: 查询并返回某测试结构放置位置（需结构放置记录/StructureBlockEntity 就绪）
    auto& source = context.getSource();
    source.sendError("locate is not implemented yet (structure placement tracking pending)");
    return 0;
}

i32 GameTestCommand::_clear(mc::command::CommandContext<mc::command::ServerCommandSource>& context)
{
    auto& source = context.getSource();
    GameTestTicker::instance().clear();
    source.sendMessage("Cleared all running game tests");
    return 1;
}

i32 GameTestCommand::_launchTests(
    mc::command::ServerCommandSource& source, const std::string& filter, const std::string& feedbackLabel)
{
    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("No world available to run game tests");
        return 0;
    }

    // 选测试（"*" → 全部；否则按 pattern 匹配）
    std::vector<std::shared_ptr<BaseGameTestFunction>> selected;
    if (filter == "*" || filter.empty()) {
        selected = GameTestRegistry::instance().allTestFunctions();
    } else {
        selected = GameTestRegistry::instance().getTestsByPattern(filter);
    }

    // 过滤 manualOnly
    std::vector<std::shared_ptr<BaseGameTestFunction>> runnable;
    runnable.reserve(selected.size());
    for (auto& fn : selected) {
        if (fn != nullptr && !fn->data().manualOnly()) {
            runnable.push_back(fn);
        }
    }

    if (runnable.empty()) {
        std::ostringstream ss;
        ss << "No tests matched filter '" << filter << "'";
        source.sendError(ss.str());
        return 0;
    }

    // 网格起点：执行者位置（无执行者时用世界出生点附近）
    // TODO: 经 /gametest pos 持久化起点；第一阶段用执行者脚下 + 偏移
    BlockPos gridStart{0, -59, 0};
    if (source.isPlayer()) {
        const auto playerPos = source.position();
        gridStart =
            BlockPos{static_cast<i32>(playerPos.x), static_cast<i32>(playerPos.y), static_cast<i32>(playerPos.z)};
    }

    // 为每个测试创建实例：helper provider + MinecraftGameTestInstance，放置结构，加入 ticker
    auto provider = std::make_unique<MinecraftGameTestHelperProvider>(*world);
    // provider 经 clone 给每个实例独立副本（实例持有 provider 所有权）
    BlockPos nextOrigin = gridStart;
    std::size_t launched = 0;
    for (auto& fn : runnable) {
        auto instanceProvider = provider->clone();
        auto instance =
            std::make_unique<MinecraftGameTestInstance>(*fn, std::move(instanceProvider), *world, nextOrigin);
        instance->spawnStructureIfNeeded();
        // 推进下一格（线性 X，按结构 X 跨度 + padding 间隔，对齐 MinecraftGameTestBatchRunner 简化布局）
        const auto* bounds = instance->bounds();
        const i32 spanX = bounds ? bounds->rotatedSize().x : 1;
        nextOrigin.x += spanX + fn->data().padding() * 2 + 2;

        GameTestTicker::instance().add(*instance);
        // 实例所有权：ticker 持裸指针，须独立保活到完成。/gametest 在线路径经 _heldInstances()
        // 静态容器持有，post-tick 回调每帧 cleanupCompletedInstances() 回收已完成实例。
        _heldInstances().push_back(std::move(instance));
        ++launched;
    }

    std::ostringstream ss;
    ss << feedbackLabel << ": launched " << launched << " game test(s)";
    source.sendMessage(ss.str());
    return static_cast<i32>(launched);
}

} // namespace mc::test
