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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OF OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "BonusChestFeature.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace mc::world::gen::feature {

namespace {

/// SPAWN_BONUS_CHEST 战利品表（项目无常量类，用字面量）。
const ResourceLocation SPAWN_BONUS_CHEST_LOOT("minecraft", "chests/spawn_bonus_chest");

} // namespace

bool ConfiguredBonusChestFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& pos) const
{
    // MC: 对区块内 XZ 列做两次独立洗牌后嵌套遍历，寻找第一个"空或无碰撞"的顶点放宝箱。
    // 区块 X/Z 方块坐标范围 [origin, origin+15]。
    std::vector<i32> xs(static_cast<size_t>(world::CHUNK_WIDTH));
    std::vector<i32> zs(static_cast<size_t>(world::CHUNK_WIDTH));
    for (i32 i = 0; i < world::CHUNK_WIDTH; ++i) {
        xs[static_cast<size_t>(i)] = pos.x + i;
        zs[static_cast<size_t>(i)] = pos.z + i;
    }

    // Fisher-Yates 洗牌（用项目 math::Random，禁 mt19937）
    for (i32 i = world::CHUNK_WIDTH - 1; i > 0; --i) {
        const i32 j = random.nextInt(i + 1);
        std::swap(xs[static_cast<size_t>(i)], xs[static_cast<size_t>(j)]);
    }
    for (i32 i = world::CHUNK_WIDTH - 1; i > 0; --i) {
        const i32 j = random.nextInt(i + 1);
        std::swap(zs[static_cast<size_t>(i)], zs[static_cast<size_t>(j)]);
    }

    const BlockState* chestDefault = VanillaBlocks::getState(VanillaBlocks::CHEST);
    const BlockState* torchDefault = VanillaBlocks::getState(VanillaBlocks::TORCH);

    for (i32 x : xs) {
        for (i32 z : zs) {
            const i32 topY = region.getTopBlockY(x, z, HeightmapType::MotionBlockingNoLeaves);
            const BlockPos top(x, topY, z);
            const BlockState* topState = region.getBlockState(top);
            // MC: isEmptyBlock(blockpos) || getCollisionShape(...).isEmpty()
            const bool emptyOrNoCollision =
                (topState == nullptr) || topState->isAir() || topState->getCollisionShape().isEmpty();
            if (!emptyOrNoCollision) {
                continue;
            }

            // 放置宝箱
            region.setBlockState(top, chestDefault);
            BlockEntity* be = region.getBlockEntity(top);
            if (be != nullptr && be->getType() == BlockEntityType::Chest) {
                auto* chest = static_cast<blockentity::ChestEntity*>(be);
                chest->setLootTable(SPAWN_BONUS_CHEST_LOOT, random.nextLong());
            }

            // 在水平四邻居放火把（若可存活：下方为固体支撑）
            for (Direction dir : Directions::horizontal()) {
                const BlockPos torchPos = top.offset(dir);
                const BlockState* below = region.getBlockState(torchPos.down());
                if (below != nullptr && below->isSolid()) {
                    region.setBlockState(torchPos, torchDefault);
                }
            }

            return true;
        }
    }

    return false;
}

} // namespace mc::world::gen::feature
