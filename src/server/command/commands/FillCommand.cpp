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

#include "FillCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/drop/BlockDropHandler.hpp"

#include <algorithm>
#include <memory>
#include <sstream>
#include <vector>

namespace mc {
namespace command {

namespace {

/**
 * @brief 方块填充模式
 */
enum class FillMode {
    Replace, // 替换所有方块
    Destroy, // 破坏原有方块并掉落物品
    Hollow,  // 空心填充（仅外壳，内部填充空气）
    Keep,    // 仅替换空气
    Outline  // 轮廓填充（仅外壳，内部保持不变）
};

/**
 * @brief 计算填充区域的方块数量
 */
i32 calculateBlockCount(const Vector3i& from, const Vector3i& to)
{
    i32 minX = std::min(from.x, to.x);
    i32 maxX = std::max(from.x, to.x);
    i32 minY = std::min(from.y, to.y);
    i32 maxY = std::max(from.y, to.y);
    i32 minZ = std::min(from.z, to.z);
    i32 maxZ = std::max(from.z, to.z);

    return (maxX - minX + 1) * (maxY - minY + 1) * (maxZ - minZ + 1);
}

/**
 * @brief 执行填充操作
 */
i32 executeFill(CommandContext<ServerCommandSource>& context, FillMode mode, const BlockState* filterState = nullptr)
{
    auto& source = context.getSource();
    Vector3i from = BlockPosArgumentType::getBlockPos(context, "from", source);
    Vector3i to = BlockPosArgumentType::getBlockPos(context, "to", source);
    BlockStateInput blockInput = context.getArgument<BlockStateInput>("block");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.fill.failed.noWorld");
        return 0;
    }

    // 检查方块数量限制
    i32 blockCount = calculateBlockCount(from, to);
    if (blockCount > 32768) {
        source.sendError("commands.fill.failed.tooManyBlocks");
        return 0;
    }

    // 获取填充方块
    const BlockState* fillState = blockInput.state();
    if (fillState == nullptr) {
        source.sendError("commands.fill.failed.invalidBlock");
        return 0;
    }

    // 计算边界
    i32 minX = std::min(from.x, to.x);
    i32 maxX = std::max(from.x, to.x);
    i32 minY = std::min(from.y, to.y);
    i32 maxY = std::max(from.y, to.y);
    i32 minZ = std::min(from.z, to.z);
    i32 maxZ = std::max(from.z, to.z);

    i32 blocksModified = 0;

    for (i32 y = minY; y <= maxY; ++y) {
        for (i32 z = minZ; z <= maxZ; ++z) {
            for (i32 x = minX; x <= maxX; ++x) {
                // 检查是否在有效高度范围内
                if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
                    continue;
                }

                // 判断当前位置是否需要填充
                bool shouldFill = false;
                bool isShell = false;
                // 保存旧方块状态，用于在setBlockState之后调用spawnAfterBreak
                const BlockState* prevState = nullptr;
                // 是否需要调用spawnAfterBreak（destroy和hollow的内部空气替换需要）
                bool needSpawnAfterBreak = false;

                switch (mode) {
                    case FillMode::Replace:
                        if (filterState == nullptr) {
                            shouldFill = true;
                        } else {
                            const BlockState* currentState = world->getBlockState(x, y, z);
                            if (currentState != nullptr &&
                                currentState->getBlock().blockId() == filterState->getBlock().blockId()) {
                                shouldFill = true;
                            }
                        }
                        break;

                    case FillMode::Destroy: {
                        // 破坏模式：先掉落物品，再填充
                        const BlockState* currentState = world->getBlockState(x, y, z);
                        if (currentState != nullptr && !currentState->isAir()) {
                            // 获取掉落表管理器
                            const loot::LootTableManager* lootTableManager = world->lootTableManager();
                            if (lootTableManager != nullptr) {
                                // 生成掉落物
                                // 注意：FillCommand destroy 模式不检查 canHarvestBlock，总是掉落物品
                                std::vector<ItemStack> drops = BlockDropHandler::generateDrops(*world,
                                    BlockPos(x, y, z),
                                    *currentState,
                                    nullptr, // 无玩家
                                    nullptr, // 无工具
                                    *lootTableManager);

                                if (!drops.empty()) {
                                    BlockDropHandler::spawnDrops(*world, BlockPos(x, y, z), drops, "");
                                }
                            }
                            prevState = currentState;
                            needSpawnAfterBreak = true;
                        }
                        shouldFill = true;
                        break;
                    }

                    case FillMode::Hollow:
                        // 空心填充：仅外壳填充，内部填充空气
                        isShell = (x == minX || x == maxX || y == minY || y == maxY || z == minZ || z == maxZ);
                        if (isShell) {
                            shouldFill = true;
                        } else {
                            // 内部填充空气
                            const BlockState* airState = BlockRegistry::instance().getBlockState(0);
                            if (airState != nullptr) {
                                const BlockState* currentState = world->getBlockState(x, y, z);
                                if (currentState == nullptr || !currentState->isAir()) {
                                    // 记录旧方块，用于spawnAfterBreak
                                    prevState = currentState;
                                    needSpawnAfterBreak = true;
                                    world->setBlockState(x, y, z, airState);
                                    blocksModified++;
                                    // 内部空气填充后立即调用spawnAfterBreak
                                    if (prevState != nullptr && !prevState->isAir()) {
                                        prevState->getBlock().spawnAfterBreak(
                                            *world, BlockPos(x, y, z), *prevState, nullptr, false);
                                    }
                                    prevState = nullptr;
                                    needSpawnAfterBreak = false;
                                }
                            }
                        }
                        break;

                    case FillMode::Keep:
                        // 仅替换空气
                        {
                            const BlockState* currentState = world->getBlockState(x, y, z);
                            if (currentState == nullptr || currentState->isAir()) {
                                shouldFill = true;
                            }
                        }
                        break;

                    case FillMode::Outline:
                        // 轮廓填充：仅外壳填充，内部保持不变
                        isShell = (x == minX || x == maxX || y == minY || y == maxY || z == minZ || z == maxZ);
                        shouldFill = isShell;
                        break;
                }

                if (shouldFill) {
                    if (world->setBlockState(x, y, z, fillState)) {
                        blocksModified++;
                        // destroy模式：在方块被替换后调用spawnAfterBreak
                        if (needSpawnAfterBreak && prevState != nullptr && !prevState->isAir()) {
                            prevState->getBlock().spawnAfterBreak(
                                *world, BlockPos(x, y, z), *prevState, nullptr, false);
                        }
                    }
                }
            }
        }
    }

    // 发送反馈
    std::ostringstream ss;
    ss << "Filled " << blocksModified << " blocks";
    source.sendMessage(ss.str());

    return blocksModified;
}

} // namespace

void FillCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto fillNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("fill");
    fillNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(fillNode,
        support::makeMetadata("Fills all or parts of a region with a specific block.",
            "/fill <from> <to> <block> [destroy|hollow|keep|outline|replace]",
            2,
            {},
            false));

    // /fill <from> <to> <block>
    auto fromArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "from", BlockPosArgumentType::blockPos());

    auto toArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "to", BlockPosArgumentType::blockPos());

    auto blockArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, BlockStateInput>>(
        "block", BlockStateArgumentType::blockState());
    blockArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _fill(ctx); });

    // /fill <from> <to> <block> destroy
    auto destroyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("destroy");
    destroyNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _fillDestroy(ctx); });

    // /fill <from> <to> <block> hollow
    auto hollowNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("hollow");
    hollowNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _fillHollow(ctx); });

    // /fill <from> <to> <block> keep
    auto keepNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("keep");
    keepNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _fillKeep(ctx); });

    // /fill <from> <to> <block> outline
    auto outlineNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("outline");
    outlineNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _fillOutline(ctx); });

    // /fill <from> <to> <block> replace
    auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");
    replaceNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _fillReplace(ctx); });

    // /fill <from> <to> <block> replace <filter>
    auto filterArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, BlockStateInput>>(
        "filter", BlockStateArgumentType::blockState());
    filterArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        auto& source = ctx.getSource();
        Vector3i from = BlockPosArgumentType::getBlockPos(ctx, "from", source);
        Vector3i to = BlockPosArgumentType::getBlockPos(ctx, "to", source);

        server::ServerWorld* world = source.world();
        if (world == nullptr) {
            source.sendError("commands.fill.failed.noWorld");
            return 0;
        }

        i32 blockCount = calculateBlockCount(from, to);
        if (blockCount > 32768) {
            source.sendError("commands.fill.failed.tooManyBlocks");
            return 0;
        }

        BlockStateInput filterInput = ctx.getArgument<BlockStateInput>("filter");
        return executeFill(ctx, FillMode::Replace, filterInput.state());
    });

    replaceNode->addChild(filterArg);
    blockArg->addChild(destroyNode);
    blockArg->addChild(hollowNode);
    blockArg->addChild(keepNode);
    blockArg->addChild(outlineNode);
    blockArg->addChild(replaceNode);
    toArg->addChild(blockArg);
    fromArg->addChild(toArg);
    fillNode->addChild(fromArg);

    dispatcher.registerCommand(fillNode);
}

i32 FillCommand::_fill(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Replace);
}

i32 FillCommand::_fillDestroy(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Destroy);
}

i32 FillCommand::_fillHollow(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Hollow);
}

i32 FillCommand::_fillKeep(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Keep);
}

i32 FillCommand::_fillOutline(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Outline);
}

i32 FillCommand::_fillReplace(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Replace);
}

} // namespace command
} // namespace mc
