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

#include "CloneCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"

#include <algorithm>
#include <deque>
#include <memory>
#include <sstream>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace command {

namespace {

/**
 * @brief 检查区域是否已加载
 */
bool isAreaLoaded(IWorld& world, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ)
{
    for (i32 x = minX; x <= maxX; x += world::CHUNK_WIDTH) {
        for (i32 z = minZ; z <= maxZ; z += world::CHUNK_WIDTH) {
            ChunkCoord chunkX = world::toChunkCoord(x);
            ChunkCoord chunkZ = world::toChunkCoord(z);
            if (!world.hasChunk(chunkX, chunkZ)) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 检查两个边界框是否重叠
 */
bool boxesOverlap(i32 min1X,
    i32 min1Y,
    i32 min1Z,
    i32 max1X,
    i32 max1Y,
    i32 max1Z,
    i32 min2X,
    i32 min2Y,
    i32 min2Z,
    i32 max2X,
    i32 max2Y,
    i32 max2Z)
{
    return min1X <= max2X && max1X >= min2X && min1Y <= max2Y && max1Y >= min2Y && min1Z <= max2Z && max1Z >= min2Z;
}

/**
 * @brief 方块信息结构
 */
struct BlockInfo {
    BlockPos pos;
    const BlockState* state;
    nlohmann::json tileEntityData;

    BlockInfo(const BlockPos& p, const BlockState* s, const nlohmann::json& data)
        : pos(p)
        , state(s)
        , tileEntityData(data)
    {}
};

/**
 * @brief 执行克隆操作
 */
i32 executeClone(CommandContext<ServerCommandSource>& context,
    const BlockPos& beginPos,
    const BlockPos& endPos,
    const BlockPos& destPos,
    FilterMode filterMode,
    CloneMode cloneMode,
    const BlockState* filterState = nullptr)
{
    auto& source = context.getSource();

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.clone.failed.noWorld");
        return 0;
    }

    // 计算源区域边界
    i32 srcMinX = std::min(beginPos.x, endPos.x);
    i32 srcMinY = std::min(beginPos.y, endPos.y);
    i32 srcMinZ = std::min(beginPos.z, endPos.z);
    i32 srcMaxX = std::max(beginPos.x, endPos.x);
    i32 srcMaxY = std::max(beginPos.y, endPos.y);
    i32 srcMaxZ = std::max(beginPos.z, endPos.z);

    // 计算目标区域边界
    i32 sizeX = srcMaxX - srcMinX;
    i32 sizeY = srcMaxY - srcMinY;
    i32 sizeZ = srcMaxZ - srcMinZ;
    i32 destMaxX = destPos.x + sizeX;
    i32 destMaxY = destPos.y + sizeY;
    i32 destMaxZ = destPos.z + sizeZ;

    // 检查重叠（normal 模式不允许重叠）
    if (cloneMode == CloneMode::Normal) {
        if (boxesOverlap(srcMinX,
                srcMinY,
                srcMinZ,
                srcMaxX,
                srcMaxY,
                srcMaxZ,
                destPos.x,
                destPos.y,
                destPos.z,
                destMaxX,
                destMaxY,
                destMaxZ)) {
            source.sendError("commands.clone.overlap");
            return 0;
        }
    }

    // 检查方块数量限制
    i32 blockCount = (sizeX + 1) * (sizeY + 1) * (sizeZ + 1);
    constexpr i32 MAX_BLOCKS = 32768;
    if (blockCount > MAX_BLOCKS) {
        std::ostringstream ss;
        ss << "Too many blocks to clone. Maximum is " << MAX_BLOCKS << ", requested " << blockCount;
        source.sendError(ss.str());
        return 0;
    }

    // 检查源区域和目标区域是否已加载
    if (!isAreaLoaded(*world, srcMinX, srcMinY, srcMinZ, srcMaxX, srcMaxY, srcMaxZ)) {
        source.sendError("commands.clone.failed.sourceUnloaded");
        return 0;
    }
    if (!isAreaLoaded(*world, destPos.x, destPos.y, destPos.z, destMaxX, destMaxY, destMaxZ)) {
        source.sendError("commands.clone.failed.destinationUnloaded");
        return 0;
    }

    // 收集方块信息
    // 分三类：普通方块、方块实体、透明/非完整方块
    std::vector<BlockInfo> normalBlocks;
    std::vector<BlockInfo> tileEntityBlocks;
    std::vector<BlockInfo> transparentBlocks;

    // 计算偏移量：目标位置 - 源区域最小坐标
    i32 offsetX = destPos.x - srcMinX;
    i32 offsetY = destPos.y - srcMinY;
    i32 offsetZ = destPos.z - srcMinZ;

    // 用于 move 模式的源位置队列
    std::deque<BlockPos> sourcePositions;

    // 遍历源区域收集方块
    for (i32 y = srcMinY; y <= srcMaxY; ++y) {
        for (i32 z = srcMinZ; z <= srcMaxZ; ++z) {
            for (i32 x = srcMinX; x <= srcMaxX; ++x) {
                // 检查 Y 坐标是否在有效范围内
                if (!world::isValidY(y)) {
                    continue;
                }

                const BlockState* state = world->getBlockState(x, y, z);
                if (state == nullptr) {
                    continue;
                }

                // 根据过滤模式决定是否复制该方块
                bool shouldCopy = false;
                switch (filterMode) {
                    case FilterMode::Replace:
                        shouldCopy = true;
                        break;
                    case FilterMode::Masked:
                        shouldCopy = !state->isAir();
                        break;
                    case FilterMode::Filtered:
                        if (filterState != nullptr &&
                            state->getBlock().blockId() == filterState->getBlock().blockId()) {
                            shouldCopy = true;
                        }
                        break;
                }

                if (!shouldCopy) {
                    continue;
                }

                // 计算目标位置
                BlockPos destBlockPos(x + offsetX, y + offsetY, z + offsetZ);

                // 检查方块实体
                BlockEntity* tileEntity = world->getBlockEntity(BlockPos(x, y, z));
                if (tileEntity != nullptr) {
                    // 保存方块实体数据
                    nlohmann::json tileData;
                    tileEntity->save(tileData);
                    tileEntityBlocks.emplace_back(destBlockPos, state, tileData);
                    sourcePositions.push_back(BlockPos(x, y, z));
                } else if (!state->isOpaqueCube(*world, BlockPos(x, y, z)) && !state->hasOpaqueCollisionShape()) {
                    // 透明或不完整碰撞的方块
                    transparentBlocks.emplace_back(destBlockPos, state, nlohmann::json());
                    sourcePositions.push_front(BlockPos(x, y, z));
                } else {
                    // 普通方块
                    normalBlocks.emplace_back(destBlockPos, state, nlohmann::json());
                    sourcePositions.push_back(BlockPos(x, y, z));
                }
            }
        }
    }

    // 如果是 move 模式，先清空源区域
    // 由于项目中没有注册屏障方块，我们直接设置为空气
    if (cloneMode == CloneMode::Move) {
        // 先清空方块实体的容器内容
        for (const BlockPos& srcPos : sourcePositions) {
            BlockEntity* tileEntity = world->getBlockEntity(srcPos);
            if (tileEntity != nullptr) {
                // 如果是容器，清空内容
                IInventory* inventory = dynamic_cast<IInventory*>(tileEntity);
                if (inventory != nullptr) {
                    inventory->clear();
                }
            }
        }

        // 设置为空气（MC Java 中 /clone move 不调用 spawnAfterBreak，仅清空源区域）
        const BlockState* airState = BlockRegistry::instance().airState();
        for (const BlockPos& srcPos : sourcePositions) {
            if (world::isValidY(srcPos.y)) {
                world->setBlockState(srcPos.x, srcPos.y, srcPos.z, airState);
            }
        }
    }

    // 合并方块列表并反转顺序进行更新
    // 先放置方块实体和透明方块，最后放置普通方块
    std::vector<BlockInfo> allBlocks;
    allBlocks.reserve(normalBlocks.size() + tileEntityBlocks.size() + transparentBlocks.size());
    allBlocks.insert(allBlocks.end(), normalBlocks.begin(), normalBlocks.end());
    allBlocks.insert(allBlocks.end(), tileEntityBlocks.begin(), tileEntityBlocks.end());
    allBlocks.insert(allBlocks.end(), transparentBlocks.begin(), transparentBlocks.end());

    // 反转顺序放置（避免方块更新问题）
    std::reverse(allBlocks.begin(), allBlocks.end());

    // 放置方块
    i32 blocksCloned = 0;
    for (const BlockInfo& info : allBlocks) {
        if (!world::isValidY(info.pos.y)) {
            continue;
        }

        if (world->setBlockState(info.pos.x, info.pos.y, info.pos.z, info.state)) {
            blocksCloned++;
        }
    }

    // 恢复方块实体数据
    for (const BlockInfo& info : tileEntityBlocks) {
        if (!info.tileEntityData.is_null()) {
            BlockEntity* newTileEntity = world->getBlockEntity(info.pos);
            if (newTileEntity != nullptr) {
                // 更新坐标
                nlohmann::json data = info.tileEntityData;
                data["x"] = info.pos.x;
                data["y"] = info.pos.y;
                data["z"] = info.pos.z;
                newTileEntity->load(data);
            }
        }
    }

    // 检查是否成功
    if (blocksCloned == 0) {
        source.sendError("commands.clone.failed");
        return 0;
    }

    // 发送反馈
    std::ostringstream ss;
    ss << "Cloned " << blocksCloned << " blocks";
    source.sendMessage(ss.str());

    return blocksCloned;
}

} // namespace

void CloneCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto cloneNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clone");
    cloneNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(cloneNode,
        support::makeMetadata("Clones blocks from one region to another.",
            "/clone <begin> <end> <destination> [replace|masked|filtered] [normal|force|move]",
            2,
            {},
            false));

    // /clone <begin> <end> <destination>
    auto beginArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "begin", BlockPosArgumentType::blockPos());

    auto endArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "end", BlockPosArgumentType::blockPos());

    auto destArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "destination", BlockPosArgumentType::blockPos());

    // 默认执行：replace + normal
    destArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _doCloneDefault(ctx); });

    // ============ replace 模式 ============
    auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");
    replaceNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _doCloneDefault(ctx); });

    // /clone ... replace force
    auto replaceForceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("force");
    replaceForceNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _doCloneStatic(ctx, FilterMode::Replace, CloneMode::Force);
    });

    // /clone ... replace move
    auto replaceMoveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("move");
    replaceMoveNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _doCloneStatic(ctx, FilterMode::Replace, CloneMode::Move);
    });

    // /clone ... replace normal
    auto replaceNormalNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("normal");
    replaceNormalNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _doCloneStatic(ctx, FilterMode::Replace, CloneMode::Normal);
    });

    replaceNode->addChild(replaceForceNode);
    replaceNode->addChild(replaceMoveNode);
    replaceNode->addChild(replaceNormalNode);

    // ============ masked 模式 ============
    auto maskedNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("masked");
    maskedNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _doCloneStatic(ctx, FilterMode::Masked, CloneMode::Normal);
    });

    // /clone ... masked force
    auto maskedForceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("force");
    maskedForceNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _doCloneStatic(ctx, FilterMode::Masked, CloneMode::Force);
    });

    // /clone ... masked move
    auto maskedMoveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("move");
    maskedMoveNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _doCloneStatic(ctx, FilterMode::Masked, CloneMode::Move);
    });

    // /clone ... masked normal
    auto maskedNormalNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("normal");
    maskedNormalNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _doCloneStatic(ctx, FilterMode::Masked, CloneMode::Normal);
    });

    maskedNode->addChild(maskedForceNode);
    maskedNode->addChild(maskedMoveNode);
    maskedNode->addChild(maskedNormalNode);

    // ============ filtered 模式 ============
    auto filteredNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("filtered");
    auto filterArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, BlockStateInput>>(
        "filter", BlockStateArgumentType::blockState());
    filterArg->setCommand(
        [](CommandContext<ServerCommandSource>& ctx) { return _doCloneFilteredStatic(ctx, CloneMode::Normal); });

    // /clone ... filtered <filter> force
    auto filteredForceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("force");
    filteredForceNode->setCommand(
        [](CommandContext<ServerCommandSource>& ctx) { return _doCloneFilteredStatic(ctx, CloneMode::Force); });

    // /clone ... filtered <filter> move
    auto filteredMoveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("move");
    filteredMoveNode->setCommand(
        [](CommandContext<ServerCommandSource>& ctx) { return _doCloneFilteredStatic(ctx, CloneMode::Move); });

    // /clone ... filtered <filter> normal
    auto filteredNormalNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("normal");
    filteredNormalNode->setCommand(
        [](CommandContext<ServerCommandSource>& ctx) { return _doCloneFilteredStatic(ctx, CloneMode::Normal); });

    filterArg->addChild(filteredForceNode);
    filterArg->addChild(filteredMoveNode);
    filterArg->addChild(filteredNormalNode);
    filteredNode->addChild(filterArg);

    // 构建命令树
    destArg->addChild(replaceNode);
    destArg->addChild(maskedNode);
    destArg->addChild(filteredNode);
    endArg->addChild(destArg);
    beginArg->addChild(endArg);
    cloneNode->addChild(beginArg);

    dispatcher.registerCommand(cloneNode);
}

i32 CloneCommand::_cloneBlocks(CommandContext<ServerCommandSource>& context)
{
    return _doCloneDefault(context);
}

i32 CloneCommand::_doCloneDefault(CommandContext<ServerCommandSource>& context)
{
    return _doCloneStatic(context, FilterMode::Replace, CloneMode::Normal);
}

i32 CloneCommand::_doCloneStatic(
    CommandContext<ServerCommandSource>& context, FilterMode filterMode, CloneMode cloneMode)
{
    auto& source = context.getSource();
    Vector3i begin = BlockPosArgumentType::getBlockPos(context, "begin", source);
    Vector3i end = BlockPosArgumentType::getBlockPos(context, "end", source);
    Vector3i dest = BlockPosArgumentType::getBlockPos(context, "destination", source);

    BlockPos beginPos(begin.x, begin.y, begin.z);
    BlockPos endPos(end.x, end.y, end.z);
    BlockPos destPos(dest.x, dest.y, dest.z);

    return executeClone(context, beginPos, endPos, destPos, filterMode, cloneMode);
}

i32 CloneCommand::_doCloneFilteredStatic(CommandContext<ServerCommandSource>& context, CloneMode cloneMode)
{
    auto& source = context.getSource();
    Vector3i begin = BlockPosArgumentType::getBlockPos(context, "begin", source);
    Vector3i end = BlockPosArgumentType::getBlockPos(context, "end", source);
    Vector3i dest = BlockPosArgumentType::getBlockPos(context, "destination", source);
    BlockStateInput filterInput = context.getArgument<BlockStateInput>("filter");

    BlockPos beginPos(begin.x, begin.y, begin.z);
    BlockPos endPos(end.x, end.y, end.z);
    BlockPos destPos(dest.x, dest.y, dest.z);

    return executeClone(context, beginPos, endPos, destPos, FilterMode::Filtered, cloneMode, filterInput.state());
}

} // namespace command
} // namespace mc
