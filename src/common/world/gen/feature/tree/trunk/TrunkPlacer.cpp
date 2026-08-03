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

#include "TrunkPlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/CherryBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <set>

namespace mc {

TrunkPlacer::TrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : m_baseHeight(baseHeight)
    , m_heightRandA(heightRandA)
    , m_heightRandB(heightRandB)
{}

i32 TrunkPlacer::getHeight(math::Random& random) const
{
    return m_baseHeight + random.nextInt(m_heightRandA + 1) + random.nextInt(m_heightRandB + 1);
}

void TrunkPlacer::placeBlock(
    WorldGenRegion& world, const BlockPos& pos, std::set<BlockPos>& trunkBlocks, const BlockState* trunkBlock)
{
    // 检查是否在有效范围内
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return;
    }

    if (trunkBlock == nullptr) {
        return;
    }

    // 设置方块
    world.setBlockState(pos, trunkBlock);

    // 记录树干方块位置
    trunkBlocks.insert(pos);
}

bool TrunkPlacer::canPlaceAt(WorldGenRegion& world, const BlockPos& pos)
{
    // 检查位置是否在有效范围内
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    // 获取当前位置的方块
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (state == nullptr || state->isAir()) {
        return true; // 空气或其他可替换方块
    }

    // 检查是否是树叶
    if (state->is(VanillaBlocks::OAK_LEAVES) || state->is(VanillaBlocks::SPRUCE_LEAVES) ||
        state->is(VanillaBlocks::BIRCH_LEAVES) || state->is(VanillaBlocks::JUNGLE_LEAVES) ||
        state->is(VanillaBlocks::ACACIA_LEAVES) || state->is(VanillaBlocks::DARK_OAK_LEAVES) ||
        state->is(block_registry::CherryBlocks::CHERRY_LEAVES)) {
        return true;
    }

    return false;
}

void TrunkPlacer::placeDirtUnder(WorldGenRegion& world, const BlockPos& pos)
{
    // 检查下方位置
    BlockPos belowPos = pos.down();
    if (belowPos.y < world::MIN_BUILD_HEIGHT) {
        return;
    }

    // 获取下方方块
    const BlockState* state = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);
    if (state == nullptr) {
        return;
    }

    // 如果不是草方块或泥土，放置泥土
    if (!state->is(VanillaBlocks::GRASS_BLOCK) && !state->is(VanillaBlocks::DIRT)) {
        if (VanillaBlocks::DIRT) {
            world.setBlockState(belowPos.x, belowPos.y, belowPos.z, &VanillaBlocks::DIRT->defaultState());
        }
    }
}

void TrunkPlacer::placeTrunkLayer2x2(
    WorldGenRegion& world, const BlockPos& pos, std::set<BlockPos>& trunkBlocks, const BlockState* trunkBlock)
{
    for (i32 dx = 0; dx < 2; ++dx) {
        for (i32 dz = 0; dz < 2; ++dz) {
            placeBlock(world, BlockPos(pos.x + dx, pos.y, pos.z + dz), trunkBlocks, trunkBlock);
        }
    }
}

} // namespace mc
