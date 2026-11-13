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

#include "DifficultyCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include <memory>
#include <sstream>
#include <string>

namespace mc::command {

namespace {

/// 难度的翻译键（options.difficulty.*），客户端据此本地化成 "Hard" 这类显示名。
std::string difficultyDisplayKey(Difficulty difficulty)
{
    return std::string("options.difficulty.") + support::getDifficultyCommandName(difficulty);
}

} // namespace

void DifficultyCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto difficultyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("difficulty");
    difficultyNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(difficultyNode,
        support::makeMetadata("Query or set the world difficulty.", "/difficulty [peaceful|easy|normal|hard]", 2));
    difficultyNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _queryDifficulty(ctx);
    });

    auto peacefulNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("peaceful");
    peacefulNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("difficulty", Difficulty::Peaceful);
        return _setDifficulty(ctx);
    });

    auto easyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("easy");
    easyNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("difficulty", Difficulty::Easy);
        return _setDifficulty(ctx);
    });

    auto normalNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("normal");
    normalNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("difficulty", Difficulty::Normal);
        return _setDifficulty(ctx);
    });

    auto hardNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("hard");
    hardNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("difficulty", Difficulty::Hard);
        return _setDifficulty(ctx);
    });

    difficultyNode->addChild(peacefulNode);
    difficultyNode->addChild(easyNode);
    difficultyNode->addChild(normalNode);
    difficultyNode->addChild(hardNode);

    dispatcher.registerCommand(difficultyNode);
}

i32 DifficultyCommand::_queryDifficulty(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendSuccess(::mc::text::literal("Server not available"), false);
        return 0;
    }

    source.sendSuccess(::mc::text::translatable("commands.difficulty.query",
                           ::mc::text::translatable(difficultyDisplayKey(server->difficulty()))),
        false);
    return static_cast<i32>(server->difficulty());
}

i32 DifficultyCommand::_setDifficulty(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendSuccess(::mc::text::literal("Server not available"), false);
        return 0;
    }

    const Difficulty difficulty = context.getArgument<Difficulty>("difficulty");
    if (server->difficulty() == difficulty) {
        // 原版这里传的是难度 **键**（小写），与成功分支的显示名（"Hard"）不同。
        source.sendFailure(
            ::mc::text::translatable("commands.difficulty.failure", support::getDifficultyCommandName(difficulty)));
        return 0;
    }

    server->setDifficulty(difficulty);
    source.sendSuccess(::mc::text::translatable(
                           "commands.difficulty.success", ::mc::text::translatable(difficultyDisplayKey(difficulty))),
        false);
    return 1;
}

} // namespace mc::command
