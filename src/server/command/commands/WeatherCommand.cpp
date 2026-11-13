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

#include "WeatherCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/TimeArgument.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/weather/WeatherManager.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace mc {
namespace command {

namespace {

[[nodiscard]] server::WeatherManager* getWeatherManager(ServerCommandSource& source)
{
    // 原版 WeatherCommand 固定操作服务器主世界（overworld），不随命令执行者所在维度变化。
    auto* server = source.server();
    if (server == nullptr) {
        return nullptr;
    }
    auto* overworld = server->dimensionManager().getOverworld();
    if (overworld == nullptr) {
        return nullptr;
    }
    auto* world = overworld->world();
    return world != nullptr ? world->weatherManager() : nullptr;
}

} // namespace

void WeatherCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    auto clearDurationArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("duration", TimeArgumentType::time(1));
    clearDurationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setClear(ctx);
    });
    clearNode->addChild(clearDurationArg);
    clearNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("duration", 0);
        return _setClear(ctx);
    });

    auto rainNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("rain");
    auto rainDurationArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("duration", TimeArgumentType::time(1));
    rainDurationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setRain(ctx);
    });
    rainNode->addChild(rainDurationArg);
    rainNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("duration", 0);
        return _setRain(ctx);
    });

    auto thunderNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("thunder");
    auto thunderDurationArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("duration", TimeArgumentType::time(1));
    thunderDurationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setThunder(ctx);
    });
    thunderNode->addChild(thunderDurationArg);
    thunderNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("duration", 0);
        return _setThunder(ctx);
    });

    auto weatherNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("weather");
    weatherNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        weatherNode, support::makeMetadata("Sets the weather.", "/weather <clear|rain|thunder> [duration]", 2));
    weatherNode->addChild(clearNode);
    weatherNode->addChild(rainNode);
    weatherNode->addChild(thunderNode);

    dispatcher.registerCommand(weatherNode);
}

i32 WeatherCommand::_setClear(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* weatherManager = getWeatherManager(source);
    if (weatherManager == nullptr) {
        source.sendSuccess(::mc::text::literal("Weather manager not available"), false);
        return 0;
    }

    const i32 duration = context.getArgument<i32>("duration");
    static_cast<void>(weatherManager->setClear(duration));
    source.sendSuccess(::mc::text::translatable("commands.weather.set.clear"), false);
    return 1;
}

i32 WeatherCommand::_setRain(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* weatherManager = getWeatherManager(source);
    if (weatherManager == nullptr) {
        source.sendSuccess(::mc::text::literal("Weather manager not available"), false);
        return 0;
    }

    const i32 duration = context.getArgument<i32>("duration");
    static_cast<void>(weatherManager->setRain(duration));
    source.sendSuccess(::mc::text::translatable("commands.weather.set.rain"), false);
    return 1;
}

i32 WeatherCommand::_setThunder(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* weatherManager = getWeatherManager(source);
    if (weatherManager == nullptr) {
        source.sendSuccess(::mc::text::literal("Weather manager not available"), false);
        return 0;
    }

    const i32 duration = context.getArgument<i32>("duration");
    static_cast<void>(weatherManager->setThunder(duration));
    source.sendSuccess(::mc::text::translatable("commands.weather.set.thunder"), false);
    return 1;
}

} // namespace command
} // namespace mc
