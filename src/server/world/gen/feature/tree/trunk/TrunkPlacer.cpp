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
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/tree/TreeFeature.hpp"
#include <set>

namespace mc {

TrunkPlacer::TrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : m_baseHeight(baseHeight)
    , m_heightRandA(heightRandA)
    , m_heightRandB(heightRandB)
{}

i32 TrunkPlacer::getHeight(math::IRandom& random) const
{
    const i32 first = random.nextInt(m_heightRandA + 1);
    const i32 second = random.nextInt(m_heightRandB + 1);
    return m_baseHeight + first + second;
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

    // 空间探测允许已有原木，但实际放置只能替换植被，不能覆盖已有树干或实心方块。
    if (!TreeFeature::isReplaceableAt(world, pos)) {
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

    if (TreeFeature::isReplaceableAt(world, pos)) {
        return true;
    }
    const BlockState* state = world.getBlockState(pos);
    return state != nullptr && BlockTags::LOGS().contains(*state);
}

void TrunkPlacer::placeDirtAt(WorldGenRegion& world, const BlockPos& groundPos, std::set<BlockPos>& trunkBlocks)
{
    if (groundPos.y < world::MIN_BUILD_HEIGHT) {
        return;
    }

    // 获取下方方块
    const BlockState* state = world.getBlockState(groundPos);
    if (state == nullptr) {
        return;
    }

    // 草方块和菌丝要转为树根泥土，其余泥土标签成员保留。
    if (!BlockTags::DIRT().contains(*state) || state->is(VanillaBlocks::GRASS_BLOCK) ||
        state->is(VanillaBlocks::MYCELIUM)) {
        if (VanillaBlocks::DIRT) {
            world.setBlockState(groundPos, &VanillaBlocks::DIRT->defaultState());
            // 装饰器的最低位置包括底部泥土写入，遗漏它会使落叶取点的高度偏移一格。
            trunkBlocks.insert(groundPos);
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
