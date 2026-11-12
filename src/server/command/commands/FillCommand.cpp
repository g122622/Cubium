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
#include "common/util/text/ComponentFactory.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/core/Block.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"

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
 *
 * @param strict 严格模式：抑制本次写入的全部副作用（不触发邻居更新、方块实体副作用、
 *               掉落与形状更新），并在全部写入完成后统一补发一次邻居更新。
 *               对应原版 strict 传 `2 | 816`（UPDATE_CLIENTS | UPDATE_SKIP_ALL_SIDEEFFECTS），
 *               非 strict 传 `2 | 256`（UPDATE_CLIENTS | UPDATE_SKIP_BLOCK_ENTITY_SIDEEFFECTS）。
 */
i32 executeFill(CommandContext<ServerCommandSource>& context, FillMode mode, const BlockState* filterState, bool strict)
{
    auto& source = context.getSource();
    Vector3i from = BlockPosArgumentType::getBlockPos(context, "from", source);
    Vector3i to = BlockPosArgumentType::getBlockPos(context, "to", source);
    BlockStateInput blockInput = context.getArgument<BlockStateInput>("block");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendFailure(::mc::text::literal("World not available"));
        return 0;
    }

    // 检查方块数量限制
    i32 blockCount = calculateBlockCount(from, to);
    if (blockCount > 32768) {
        source.sendFailure(
            ::mc::text::translatable("commands.fill.toobig", {std::to_string(32768), std::to_string(blockCount)}));
        return 0;
    }

    // 获取填充方块
    const BlockState* fillState = blockInput.state();
    if (fillState == nullptr) {
        source.sendFailure(::mc::text::translatable("commands.fill.failed"));
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

    // 严格模式下把邻居更新推迟到全部写入之后统一补发（原版在写入期间抑制副作用，
    // 随后按记录的位置逐个调用 updateNeighboursOnBlockSet）。
    struct UpdatedPosition {
        BlockPos pos;
        const BlockState* oldState;
    };

    std::vector<UpdatedPosition> deferredNeighborUpdates;

    // 写入标志：始终同步客户端；严格模式额外抑制全部副作用，非严格只抑制方块实体副作用。
    const i32 writeFlags = world::BlockUpdateFlags::UPDATE_CLIENTS |
        (strict ? world::BlockUpdateFlags::UPDATE_SKIP_ALL_SIDEEFFECTS
                : world::BlockUpdateFlags::UPDATE_SKIP_BLOCK_ENTITY_SIDEEFFECTS);

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
                // Destroy 模式下原方块是否被实际摧毁（空气目标不计入修改数，对应原版 affector 返回值）
                bool destroyedAny = false;

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

                    case FillMode::Destroy:
                        // 破坏模式：原版 DESTROY 的 affector 即 destroyBlock(pos, true)，
                        // 由统一接口完成破坏粒子、掉落、BLOCK_DESTROY 事件与原方块移除。
                        // 目标本就是空气时 destroyBlock 返回 false，原版此时不计入修改数。
                        destroyedAny = world->destroyBlock(BlockPos(x, y, z), true);
                        shouldFill = true;
                        break;

                    case FillMode::Hollow:
                        // 空心填充：仅外壳填充，内部填充空气
                        isShell = (x == minX || x == maxX || y == minY || y == maxY || z == minZ || z == maxZ);
                        if (isShell) {
                            shouldFill = true;
                        } else {
                            // 内部清空为空气：原版 hollow 内核同样走 Filter 的 destroyBlock(pos, true)
                            const BlockState* currentState = world->getBlockState(x, y, z);
                            if (currentState != nullptr && !currentState->isAir()) {
                                world->destroyBlock(BlockPos(x, y, z), true);
                                blocksModified++;
                                continue;
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
                    // 严格模式需要旧状态来补发邻居更新，故在写入前先取。
                    const BlockState* oldState = strict ? world->getBlockState(x, y, z) : nullptr;

                    if (world->setBlockState(x, y, z, fillState, writeFlags)) {
                        blocksModified++;
                        if (strict) {
                            deferredNeighborUpdates.push_back({BlockPos(x, y, z), oldState});
                        }
                    } else if (destroyedAny) {
                        blocksModified++;
                    }
                }
            }
        }
    }

    // 严格模式：全部写入完成后统一补发邻居更新（原版 updateNeighboursOnBlockSet 的时机）。
    for (const auto& updated : deferredNeighborUpdates) {
        if (updated.oldState != nullptr) {
            world->updateNeighboursOnBlockSet(updated.pos, *updated.oldState);
        }
    }

    // 发送反馈：commands.fill.success = "Successfully filled %s block(s)"。
    source.sendSuccess(::mc::text::translatable("commands.fill.success", std::to_string(blocksModified)), true);

    return blocksModified;
}

} // namespace

void FillCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto fillNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("fill");
    fillNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
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
    blockArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _fill(ctx);
    });

    // /fill <from> <to> <block> destroy
    auto destroyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("destroy");
    destroyNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _fillDestroy(ctx);
    });

    // /fill <from> <to> <block> hollow
    auto hollowNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("hollow");
    hollowNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _fillHollow(ctx);
    });

    // /fill <from> <to> <block> keep
    auto keepNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("keep");
    keepNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _fillKeep(ctx);
    });

    // /fill <from> <to> <block> outline
    auto outlineNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("outline");
    outlineNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _fillOutline(ctx);
    });

    // /fill <from> <to> <block> replace
    auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");
    replaceNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _fillReplace(ctx);
    });

    // /fill <from> <to> <block> replace <filter>
    // 原版的过滤参数是 BlockPredicateArgument（block_predicate），不是 block_state。
    auto filterArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, BlockStateInput>>(
        "filter", BlockPredicateArgumentType::blockPredicate());
    filterArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return executeFill(ctx, FillMode::Replace, ctx.getArgument<BlockStateInput>("filter").state(), false);
    });

    // 过滤条件后的模式不再增加来源层，strict 也是这一层的终点。
    // strict 的语义是"按 replace 填充 + 抑制副作用"，故 FillMode 仍是 Replace，另以 strict 标志区分。
    struct ModeEntry {
        const char* name;
        FillMode mode;
        bool strict;
    };

    for (const ModeEntry& entry : {ModeEntry{"destroy", FillMode::Destroy, false},
             ModeEntry{"hollow", FillMode::Hollow, false},
             ModeEntry{"outline", FillMode::Outline, false},
             ModeEntry{"strict", FillMode::Replace, true}}) {
        auto modeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(entry.name);
        modeNode->setCommand([entry](CommandContext<ServerCommandSource>& ctx) {
            return executeFill(ctx, entry.mode, ctx.getArgument<BlockStateInput>("filter").state(), entry.strict);
        });
        filterArg->addChild(modeNode);
    }
    replaceNode->addChild(filterArg);
    blockArg->addChild(destroyNode);
    blockArg->addChild(hollowNode);
    blockArg->addChild(keepNode);
    blockArg->addChild(outlineNode);
    blockArg->addChild(replaceNode);

    auto strictNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("strict");
    // /fill <from> <to> <block> strict：与 replace 同模式，但抑制写入期间的全部副作用。
    strictNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return executeFill(ctx, FillMode::Replace, nullptr, true);
    });
    blockArg->addChild(strictNode);
    toArg->addChild(blockArg);
    fromArg->addChild(toArg);
    fillNode->addChild(fromArg);

    dispatcher.registerCommand(fillNode);
}

i32 FillCommand::_fill(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Replace, nullptr, false);
}

i32 FillCommand::_fillDestroy(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Destroy, nullptr, false);
}

i32 FillCommand::_fillHollow(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Hollow, nullptr, false);
}

i32 FillCommand::_fillKeep(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Keep, nullptr, false);
}

i32 FillCommand::_fillOutline(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Outline, nullptr, false);
}

i32 FillCommand::_fillReplace(CommandContext<ServerCommandSource>& context)
{
    return executeFill(context, FillMode::Replace, nullptr, false);
}

} // namespace command
} // namespace mc
