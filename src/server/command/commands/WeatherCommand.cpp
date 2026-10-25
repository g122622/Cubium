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
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/weather/WeatherManager.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <string_view>

namespace mc {
namespace command {

namespace {

/// 一秒对应的 tick 数
constexpr i32 TICKS_PER_SECOND = 20;

/// 默认天气持续时间（秒）
constexpr i32 DEFAULT_WEATHER_DURATION_SECONDS = 300;

[[nodiscard]] server::WeatherManager* getWeatherManager(ServerCommandSource& source)
{
    auto* world = source.world();
    if (world == nullptr) {
        return nullptr;
    }
    return world->weatherManager();
}

/// 构建天气变化反馈消息
[[nodiscard]] std::string buildWeatherMessage(std::string_view weatherType, i32 duration)
{
    i32 seconds = duration > 0 ? duration / TICKS_PER_SECOND : DEFAULT_WEATHER_DURATION_SECONDS;
    std::ostringstream ss;
    ss << "Changing weather to " << weatherType << " for " << seconds << " seconds";
    return ss.str();
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
    support::applyMetadata(weatherNode,
        support::makeMetadata("Sets the weather.", "/weather <clear|rain|thunder> [duration]", 2));
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
        source.sendMessage("Weather manager not available");
        return 0;
    }

    i32 duration = context.getArgument<i32>("duration");
    weatherManager->setClear(duration);
    source.sendMessage(buildWeatherMessage("clear", duration));

    return 1;
}

i32 WeatherCommand::_setRain(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* weatherManager = getWeatherManager(source);
    if (weatherManager == nullptr) {
        source.sendMessage("Weather manager not available");
        return 0;
    }

    i32 duration = context.getArgument<i32>("duration");
    weatherManager->setRain(duration);
    source.sendMessage(buildWeatherMessage("rain", duration));

    return 1;
}

i32 WeatherCommand::_setThunder(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* weatherManager = getWeatherManager(source);
    if (weatherManager == nullptr) {
        source.sendMessage("Weather manager not available");
        return 0;
    }

    i32 duration = context.getArgument<i32>("duration");
    weatherManager->setThunder(duration);
    source.sendMessage(buildWeatherMessage("thunder", duration));

    return 1;
}

} // namespace command
} // namespace mc
