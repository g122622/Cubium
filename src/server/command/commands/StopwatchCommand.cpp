/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
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

#include "StopwatchCommand.hpp"

#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/CommandSuggestionProviders.hpp"
#include "server/world/ServerWorld.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mc::command {
namespace {

/// 秒表状态：所属世界 + 启动时的游戏刻。原版每个维度各有一份，这里同样按维度区分。
struct StopwatchState {
    i64 startTick = 0;
};

std::unordered_map<DimensionId, std::unordered_map<ResourceLocation, StopwatchState>>& _stopwatches()
{
    static std::unordered_map<DimensionId, std::unordered_map<ResourceLocation, StopwatchState>> s_stopwatches;
    return s_stopwatches;
}

/// 当前游戏刻（原版以 ServerLevel.getGameTime() 为基准）。
u64 _currentTick(ServerCommandSource& source)
{
    auto* world = source.world();
    return world != nullptr ? world->currentTick() : 0;
}

} // namespace

void StopwatchCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto stopwatchNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("stopwatch");
    stopwatchNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(stopwatchNode,
        support::makeMetadata("Manages stopwatches.", "/stopwatch <create|query|remove|restart> <id>", 2, {}, true));

    // 四个子命令的参数节点结构完全相同，只是处理函数不同；依次构建。
    // suggestIds 对应原版 StopwatchCommand 的 .suggests(SUGGEST_STOPWATCHES)：query /
    // restart / remove 声明了补全，create 没有；差异体现在命令树里建议提供器位上。
    const auto addSubcommand = [&stopwatchNode](const char* name, auto handler, bool suggestIds) {
        auto subNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(name);
        auto idArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
            "id", ResourceLocationArgumentType::resourceLocation());
        if (suggestIds) {
            idArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
        }
        // /stopwatch query <id> [<scale>]：原版 query 还有一个可选的换算倍数。
        if (std::string_view(name) == "query") {
            auto scaleArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f64>>(
                "scale", DoubleArgumentType::doubleArg());
            scaleArg->setCommand([handler](CommandContext<ServerCommandSource>& ctx) {
                return handler(ctx);
            });
            idArg->addChild(scaleArg);
        }
        idArg->setCommand([handler](CommandContext<ServerCommandSource>& ctx) {
            return handler(ctx);
        });
        subNode->addChild(idArg);
        stopwatchNode->addChild(subNode);
    };
    addSubcommand(
        "create",
        [](CommandContext<ServerCommandSource>& ctx) {
            return _create(ctx);
        },
        false);
    addSubcommand(
        "query",
        [](CommandContext<ServerCommandSource>& ctx) {
            return _query(ctx);
        },
        true);
    addSubcommand(
        "remove",
        [](CommandContext<ServerCommandSource>& ctx) {
            return _remove(ctx);
        },
        true);
    addSubcommand(
        "restart",
        [](CommandContext<ServerCommandSource>& ctx) {
            return _restart(ctx);
        },
        true);

    dispatcher.registerCommand(stopwatchNode);
}

i32 StopwatchCommand::_create(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto& stopwatches = _stopwatches()[source.dimensionId()];
    const ResourceLocation id = context.getArgument<ResourceLocation>("id");
    if (stopwatches.contains(id)) {
        source.sendError("A stopwatch named '" + id.toString() + "' already exists");
        return 0;
    }
    stopwatches.emplace(id, StopwatchState{static_cast<i64>(_currentTick(source))});
    source.sendMessage("Created new stopwatch '" + id.toString() + "'");
    return 1;
}

i32 StopwatchCommand::_restart(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto& stopwatches = _stopwatches()[source.dimensionId()];
    const ResourceLocation id = context.getArgument<ResourceLocation>("id");
    auto it = stopwatches.find(id);
    if (it == stopwatches.end()) {
        source.sendError("No stopwatch named '" + id.toString() + "' exists");
        return 0;
    }
    it->second.startTick = static_cast<i64>(_currentTick(source));
    source.sendMessage("Restarted stopwatch '" + id.toString() + "'");
    return 1;
}

i32 StopwatchCommand::_remove(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto& stopwatches = _stopwatches()[source.dimensionId()];
    const ResourceLocation id = context.getArgument<ResourceLocation>("id");
    if (stopwatches.erase(id) == 0) {
        source.sendError("No stopwatch named '" + id.toString() + "' exists");
        return 0;
    }
    source.sendMessage("Removed stopwatch '" + id.toString() + "'");
    return 1;
}

i32 StopwatchCommand::_query(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto& stopwatches = _stopwatches()[source.dimensionId()];
    const ResourceLocation id = context.getArgument<ResourceLocation>("id");
    auto it = stopwatches.find(id);
    if (it == stopwatches.end()) {
        source.sendError("No stopwatch named '" + id.toString() + "' exists");
        return 0;
    }
    const i64 elapsed = static_cast<i64>(_currentTick(source)) - it->second.startTick;
    // 原版 query 的 scale：把经过的 tick 换算成秒/分钟等（省略时为 1）。
    const f64 scale = context.hasArgument("scale") ? context.getArgument<f64>("scale") : 1.0;
    const f64 value = static_cast<f64>(elapsed) * scale;
    source.sendMessage("Stopwatch '" + id.toString() + "' elapsed: " + std::to_string(value) + " ticks");
    return 1;
}

} // namespace mc::command
