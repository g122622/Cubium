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
#include "common/command/arguments/ArgumentType.hpp"
#include "common/world/weather/WeatherState.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/weather/WeatherManager.hpp"

#include <sstream>

namespace mc {
namespace command {

namespace {

[[nodiscard]] server::WeatherManager* getWeatherManager(ServerCommandSource& source)
{
    auto* world = source.world();
    if (world == nullptr) {
        return nullptr;
    }
    return world->weatherManager();
}

} // namespace

void WeatherCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    auto clearDurationArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("duration", IntegerArgumentType::integer(0));
    clearDurationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return setClear(ctx); });
    clearNode->addChild(clearDurationArg);
    clearNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("duration", 0);
        return setClear(ctx);
    });

    auto rainNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("rain");
    auto rainDurationArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("duration", IntegerArgumentType::integer(0));
    rainDurationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return setRain(ctx); });
    rainNode->addChild(rainDurationArg);
    rainNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("duration", 0);
        return setRain(ctx);
    });

    auto thunderNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("thunder");
    auto thunderDurationArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("duration", IntegerArgumentType::integer(0));
    thunderDurationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return setThunder(ctx); });
    thunderNode->addChild(thunderDurationArg);
    thunderNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("duration", 0);
        return setThunder(ctx);
    });

    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("query");
    queryNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return query(ctx); });

    auto weatherNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("weather");
    weatherNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(weatherNode,
        support::makeMetadata("Change or query the weather.", "/weather <clear|rain|thunder|query> [duration]", 2));
    weatherNode->addChild(clearNode);
    weatherNode->addChild(rainNode);
    weatherNode->addChild(thunderNode);
    weatherNode->addChild(queryNode);

    dispatcher.registerCommand(weatherNode);
}

i32 WeatherCommand::setClear(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* weatherManager = getWeatherManager(source);
    if (weatherManager == nullptr) {
        source.sendMessage("Weather manager not available");
        return 0;
    }

    i32 duration = context.getArgument<i32>("duration");
    weatherManager->setClear(duration);

    i32 seconds = duration > 0 ? duration / 20 : 300;
    std::ostringstream ss;
    ss << "Changing weather to clear for " << seconds << " seconds";
    source.sendMessage(ss.str());

    return 1;
}

i32 WeatherCommand::setRain(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* weatherManager = getWeatherManager(source);
    if (weatherManager == nullptr) {
        source.sendMessage("Weather manager not available");
        return 0;
    }

    i32 duration = context.getArgument<i32>("duration");
    weatherManager->setRain(duration);

    i32 seconds = duration > 0 ? duration / 20 : 300;
    std::ostringstream ss;
    ss << "Changing weather to rain for " << seconds << " seconds";
    source.sendMessage(ss.str());

    return 1;
}

i32 WeatherCommand::setThunder(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* weatherManager = getWeatherManager(source);
    if (weatherManager == nullptr) {
        source.sendMessage("Weather manager not available");
        return 0;
    }

    i32 duration = context.getArgument<i32>("duration");
    weatherManager->setThunder(duration);

    i32 seconds = duration > 0 ? duration / 20 : 300;
    std::ostringstream ss;
    ss << "Changing weather to thunder for " << seconds << " seconds";
    source.sendMessage(ss.str());

    return 1;
}

i32 WeatherCommand::query(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* weatherMgr = getWeatherManager(source);
    if (weatherMgr == nullptr) {
        source.sendMessage("Weather manager not available");
        return 0;
    }

    auto type = static_cast<i32>(weatherMgr->weatherType());
    f32 rainStrength = weatherMgr->rainStrength();
    f32 thunderStrength = weatherMgr->thunderStrength();

    std::string typeStr;
    switch (type) {
        case 1:
            typeStr = "rain";
            break;
        case 2:
            typeStr = "thunder";
            break;
        default:
            typeStr = "clear";
            break;
    }

    std::ostringstream ss;
    ss << "The weather is " << typeStr;
    ss << " (rain strength: " << rainStrength;
    ss << ", thunder strength: " << thunderStrength << ")";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
