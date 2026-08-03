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

#include "SpongeBlock.hpp"

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IBucketPickupHandler.hpp"
#include "common/world/block/blocks/LiquidBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidTags.hpp"

#include <functional>
#include <queue>
#include <unordered_set>
#include <utility>

namespace mc {
namespace blocks {

// BlockPos 哈希别名
using BlockPosHash = std::hash<BlockPos>;

// ========== SpongeBlock ==========

SpongeBlock::SpongeBlock(const BlockProperties& properties)
    : Block(properties)
{}

bool SpongeBlock::tryAbsorbWater(IWorld& world, const BlockPos& pos)
{
    i32 absorbedCount = absorb(world, pos);
    if (absorbedCount > 0) {
        // 将海绵变为湿润海绵
        const BlockState& wetSpongeState = VanillaBlocks::WET_SPONGE->defaultState();
        world.setBlockState(pos, &wetSpongeState, 3);

        // 播放水被吸收的视觉效果（事件 2001，data 为水的方块状态 ID）
        const BlockState& waterState = VanillaBlocks::WATER->defaultState();
        world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, pos, waterState.stateId());

        return true;
    }
    return false;
}

void SpongeBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    // 放置时尝试吸水
    tryAbsorbWater(world, pos);
}

void SpongeBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 邻居更新时尝试吸水
    tryAbsorbWater(world, pos);

    // 调用基类方法
    Block::neighborChanged(world, pos, neighborBlock, neighborPos, isMoving);
}

i32 SpongeBlock::absorb(IWorld& world, const BlockPos& pos)
{
    // 使用 BFS 搜索周围的水方块

    // 队列元素：(位置, 深度)
    std::queue<std::pair<BlockPos, i32>> queue;
    queue.push({pos, 0});

    i32 absorbedCount = 0;

    // 已访问的位置集合（用于避免重复处理）
    std::unordered_set<BlockPos, BlockPosHash> visited;
    visited.insert(pos);

    while (!queue.empty()) {
        auto [currentPos, depth] = queue.front();
        queue.pop();

        // 遍历六个方向
        for (Direction dir : Directions::all()) {
            BlockPos neighborPos = currentPos.offset(dir);

            // 获取流体状态
            const fluid::FluidState* fluidState = world.getFluidState(neighborPos);
            if (fluidState == nullptr || fluidState->isEmpty()) {
                continue;
            }

            // 检查是否为水
            if (!fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
                continue;
            }

            // 获取方块状态
            const BlockState* blockState = world.getBlockState(neighborPos);
            if (blockState == nullptr) {
                continue;
            }

            Block& block = blockState->getBlockMutable();

            // 情况1：可舀取的水源（如水源方块）
            IBucketPickupHandler* bucketPickup = dynamic_cast<IBucketPickupHandler*>(&block);
            if (bucketPickup != nullptr) {
                fluid::Fluid* pickedFluid = bucketPickup->pickupFluid(world, neighborPos, *blockState);
                if (pickedFluid != nullptr) {
                    ++absorbedCount;
                    if (depth < MAX_ABSORB_DEPTH && visited.find(neighborPos) == visited.end()) {
                        visited.insert(neighborPos);
                        queue.push({neighborPos, depth + 1});
                    }
                }
            }
            // 情况2：流动水方块
            else if (dynamic_cast<block::LiquidBlock*>(&block) != nullptr) {
                // 移除流动水方块，设置为空气
                const BlockState& airState = VanillaBlocks::AIR->defaultState();
                world.setBlockState(neighborPos, &airState, 3);
                ++absorbedCount;
                if (depth < MAX_ABSORB_DEPTH && visited.find(neighborPos) == visited.end()) {
                    visited.insert(neighborPos);
                    queue.push({neighborPos, depth + 1});
                }
            }
            // 情况3：海洋植物（海带、海带茎、海草、高海草）
            // 不要按 Material 判断，以避免误匹配海泡菜等其他 OCEAN_PLANT 方块。
            else if (blockState->is(VanillaBlocks::KELP) || blockState->is(VanillaBlocks::KELP_PLANT) ||
                blockState->is(VanillaBlocks::SEAGRASS) || blockState->is(VanillaBlocks::TALL_SEAGRASS)) {
                // 在移除方块之前生成掉落物品
                Block::dropResources(world, neighborPos, *blockState);

                const BlockState& airState = VanillaBlocks::AIR->defaultState();
                world.setBlockState(neighborPos, &airState, 3);
                ++absorbedCount;
                if (depth < MAX_ABSORB_DEPTH && visited.find(neighborPos) == visited.end()) {
                    visited.insert(neighborPos);
                    queue.push({neighborPos, depth + 1});
                }
            }

            // 超过最大吸收数量就停止
            if (absorbedCount >= MAX_ABSORB_COUNT) {
                return absorbedCount;
            }
        }
    }

    return absorbedCount;
}

} // namespace blocks
} // namespace mc
