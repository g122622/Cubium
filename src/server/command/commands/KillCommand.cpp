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
 * The above copyright notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "KillCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"

#include <memory>
#include <sstream>

namespace mc {
namespace command {

void KillCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto killNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("kill");
    killNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(
        killNode, support::makeMetadata("Kill entities (players, mobs, etc.).", "/kill [<target>]", 2, {}, false));

    // /kill - 杀死自己
    killNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _killSelf(ctx); });

    // /kill <target> - 杀死目标实体
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entities());
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _killEntities(ctx); });
    killNode->addChild(targetArg);

    dispatcher.registerCommand(killNode);
}

i32 KillCommand::_killSelf(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // /kill 无参数时杀死命令源的执行实体。
    // CommandSourceStack.getEntity() 支持任何实体（不限于玩家），
    // 如通过 /execute as @e[type=zombie] run kill 可杀死该僵尸。
    Entity* entity = source.entity();
    if (entity == nullptr) {
        // 控制台或命令方块执行 /kill 无参数时无关联实体
        source.sendError("commands.kill.failed.notEntity");
        return 0;
    }

    // 调用击杀命令处理
    entity->onKillCommand();

    // 发送反馈消息
    auto* player = dynamic_cast<Player*>(entity);
    if (player != nullptr) {
        std::ostringstream ss;
        ss << "Killed " << player->username();
        source.sendMessage(ss.str());
    } else if (entity->hasCustomName()) {
        std::ostringstream ss;
        ss << "Killed " << entity->customNameText();
        source.sendMessage(ss.str());
    } else {
        std::ostringstream ss;
        ss << "Killed " << entity->getTypeId();
        source.sendMessage(ss.str());
    }

    return 1;
}

i32 KillCommand::_killEntities(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("target");

    // 使用 EntityResolver 解析实体选择器，支持非玩家实体
    auto entities = support::EntityResolver::resolve(source, selector);

    if (entities.empty()) {
        source.sendError("commands.kill.failed.noEntity");
        return 0;
    }

    i32 killedCount = 0;

    for (Entity* entity : entities) {
        if (entity == nullptr || !entity->isAlive()) {
            continue;
        }

        // 对所有实体使用 onKillCommand（基类默认调用 remove()）
        entity->onKillCommand();
        killedCount++;
    }

    // 发送反馈消息
    if (killedCount == 1) {
        Entity* first = entities.front();
        auto* player = dynamic_cast<Player*>(first);
        if (player != nullptr) {
            std::ostringstream ss;
            ss << "Killed " << player->username();
            source.sendMessage(ss.str());
        } else if (first->hasCustomName()) {
            std::ostringstream ss;
            ss << "Killed " << first->customNameText();
            source.sendMessage(ss.str());
        } else {
            std::ostringstream ss;
            ss << "Killed " << first->getTypeId();
            source.sendMessage(ss.str());
        }
    } else {
        std::ostringstream ss;
        ss << "Killed " << killedCount << " entities";
        source.sendMessage(ss.str());
    }

    return killedCount;
}

} // namespace command
} // namespace mc
