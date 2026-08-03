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

#include "BlockPileFeature.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <memory>
#include <utility>

namespace mc::world::gen::feature {

ConfiguredBlockPileFeature::ConfiguredBlockPileFeature(std::unique_ptr<BlockPileConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

namespace {

/// 对齐 MC BlockPileFeature.mayPlaceOn：dirt_path 按 nextBoolean，否则 isFaceSturdy(UP, FULL)。
bool mayPlaceOn(WorldGenRegion& region, const BlockPos& pos, math::Random& random)
{
    const BlockPos below = pos.down();
    const BlockState* belowState = region.getBlockState(below);
    if (belowState == nullptr) {
        return false;
    }
    if (belowState->getBlock().blockLocation() == ResourceLocation("minecraft", "dirt_path")) {
        return random.nextBoolean();
    }
    return belowState->isFaceSturdy(region, below, Direction::Up, SupportType::Full);
}

/// 对齐 MC BlockPileFeature.tryPlaceBlock：目标格为空且 mayPlaceOn 才放置。
void tryPlaceBlock(
    WorldGenRegion& region, const BlockPos& pos, math::Random& random, const state::BlockStateProvider& provider)
{
    const BlockState* current = region.getBlockState(pos);
    const bool empty = (current == nullptr) || current->isAir();
    if (!empty || !mayPlaceOn(region, pos, random)) {
        return;
    }
    const BlockState* state = provider.getState(region, random, pos.x, pos.y, pos.z);
    if (state != nullptr) {
        region.setBlockState(pos, state, 260);
    }
}

} // namespace

bool ConfiguredBlockPileFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin) const
{
    if (!m_config || m_config->stateProvider == nullptr) {
        return false;
    }
    if (origin.y < region.getMinBuildHeight() + 5) {
        return false;
    }

    // i=2+nextInt(2), j=2+nextInt(2)；遍历 [origin(-i,0,-j), origin(i,1,j)]。
    const i32 i = 2 + random.nextInt(2);
    const i32 j = 2 + random.nextInt(2);
    const auto& provider = *m_config->stateProvider;

    for (i32 px = origin.x - i; px <= origin.x + i; ++px) {
        for (i32 py = origin.y; py <= origin.y + 1; ++py) {
            for (i32 pz = origin.z - j; pz <= origin.z + j; ++pz) {
                const i32 dx = origin.x - px;
                const i32 dz = origin.z - pz;
                // MC: k*k + l*l <= nextFloat()*10 - nextFloat()*6
                const f32 threshold = random.nextFloat() * 10.0f - random.nextFloat() * 6.0f;
                if (static_cast<f32>(dx * dx + dz * dz) <= threshold) {
                    tryPlaceBlock(region, BlockPos(px, py, pz), random, provider);
                } else if (random.nextFloat() < 0.031f) {
                    tryPlaceBlock(region, BlockPos(px, py, pz), random, provider);
                }
            }
        }
    }

    return true;
}

} // namespace mc::world::gen::feature
