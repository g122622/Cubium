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

#include "SetBlockCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/core/Block.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"

#include <memory>
#include <sstream>
#include <vector>

namespace mc {
namespace command {

namespace {

/**
 * @brief 执行 setblock 命令
 *
 * @param context 命令上下文
 * @param onlyIfAir keep模式：仅当目标位置为空气时放置
 * @param doDrop destroy模式：破坏原有方块并掉落物品
 * @return 成功放置返回1，失败返回0
 */
i32 executeSetBlock(CommandContext<ServerCommandSource>& context, bool onlyIfAir, bool doDrop)
{
    auto& source = context.getSource();
    Vector3i position = BlockPosArgumentType::getBlockPos(context, "pos", source);
    BlockStateInput blockInput = context.getArgument<BlockStateInput>("block");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        // 原版只有统一的 commands.setblock.failed；具体原因写服务端日志便于排查。
        spdlog::warn(
            "SetBlockCommand: world not available for position ({}, {}, {})", position.x, position.y, position.z);
        source.sendFailure(::mc::text::translatable("commands.setblock.failed"));
        return 0;
    }

    // 获取方块状态
    const BlockState* state = blockInput.state();
    if (state == nullptr) {
        spdlog::warn("SetBlockCommand: invalid block state at ({}, {}, {})", position.x, position.y, position.z);
        source.sendFailure(::mc::text::translatable("commands.setblock.failed"));
        return 0;
    }

    // 将位置转换为 BlockPos
    BlockPos pos(position.x, position.y, position.z);

    // keep模式：仅当目标位置为空气时放置
    if (onlyIfAir) {
        const BlockState* existingBlock = world->getBlockState(position.x, position.y, position.z);
        if (existingBlock != nullptr && !existingBlock->isAir()) {
            spdlog::warn("SetBlockCommand: target block is not air in keep mode at ({}, {}, {})",
                position.x,
                position.y,
                position.z);
            source.sendFailure(::mc::text::translatable("commands.setblock.failed"));
            return 0;
        }
    }

    // destroy模式：先摧毁原有方块（破坏粒子 + 掉落 + BLOCK_DESTROY 事件 + 写回流体遗留方块）
    if (doDrop) {
        world->destroyBlock(pos, true);
    }

    // 放置方块
    bool success = world->setBlockState(position.x, position.y, position.z, state);
    if (!success) {
        source.sendFailure(::mc::text::translatable("commands.setblock.failed"));
        return 0;
    }

    // 发送反馈：commands.setblock.success 接受 x/y/z 三个参数，并广播给其他管理员。
    source.sendSuccess(::mc::text::translatable("commands.setblock.success",
                           {std::to_string(position.x), std::to_string(position.y), std::to_string(position.z)}),
        true);

    return 1;
}

} // namespace

void SetBlockCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto setblockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("setblock");
    setblockNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(setblockNode,
        support::makeMetadata(
            "Changes a block to another block.", "/setblock <pos> <block> [destroy|keep|replace]", 2, {}, false));

    // /setblock <pos> <block> - 默认replace模式
    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());

    auto blockArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, BlockStateInput>>(
        "block", BlockStateArgumentType::blockState());
    blockArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setBlockState(ctx);
    });

    // /setblock <pos> <block> destroy
    auto destroyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("destroy");
    destroyNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setBlockDestroy(ctx);
    });

    // /setblock <pos> <block> keep
    auto keepNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("keep");
    keepNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setBlockKeep(ctx);
    });

    // /setblock <pos> <block> replace
    auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");
    replaceNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setBlockReplace(ctx);
    });

    // strict：源区域必须完全加载，等价于默认的 replace 模式（与 /fill 同形）。
    auto strictNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("strict");
    strictNode->setCommand(blockArg->getCommand());

    blockArg->addChild(destroyNode);
    blockArg->addChild(keepNode);
    blockArg->addChild(replaceNode);
    blockArg->addChild(strictNode);
    posArg->addChild(blockArg);
    setblockNode->addChild(posArg);

    dispatcher.registerCommand(setblockNode);
}

i32 SetBlockCommand::_setBlockState(CommandContext<ServerCommandSource>& context)
{
    // 默认replace模式
    return executeSetBlock(context, false, false);
}

i32 SetBlockCommand::_setBlockDestroy(CommandContext<ServerCommandSource>& context)
{
    // destroy模式：先破坏再放置
    return executeSetBlock(context, false, true);
}

i32 SetBlockCommand::_setBlockKeep(CommandContext<ServerCommandSource>& context)
{
    // keep模式：仅当目标位置为空气时放置
    return executeSetBlock(context, true, false);
}

i32 SetBlockCommand::_setBlockReplace(CommandContext<ServerCommandSource>& context)
{
    // replace模式：直接替换
    return executeSetBlock(context, false, false);
}

} // namespace command
} // namespace mc
