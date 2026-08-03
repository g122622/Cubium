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

#include "SpringFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <memory>
#include <utility>

namespace mc::world::gen::feature {

ConfiguredSpringFeature::ConfiguredSpringFeature(std::unique_ptr<SpringConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

namespace {

/// 判断 state 是否属于配置的 validBlocks（列表或 tag）。
bool isValidBlock(const BlockState* state, const SpringConfig& config)
{
    if (state == nullptr) {
        return false;
    }
    if (config.validTag != nullptr) {
        return config.validTag->contains(*state);
    }
    const Block& block = state->getBlock();
    for (const Block* valid : config.validBlocks) {
        if (valid != nullptr && valid == &block) {
            return true;
        }
    }
    return false;
}

/// 判断 pos 处是否为空气（ChunkData 未持久化空气，nullptr 视为空气）。
bool isEmpty(WorldGenRegion& region, const BlockPos& pos)
{
    const BlockState* state = region.getBlockState(pos);
    return (state == nullptr) || state->isAir();
}

} // namespace

bool ConfiguredSpringFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& /*random*/,
    const BlockPos& origin) const
{
    if (!m_config || m_config->state == nullptr) {
        return false;
    }

    // 上方须为 validBlocks。
    if (!isValidBlock(region.getBlockState(origin.up()), *m_config)) {
        return false;
    }
    // requiresBlockBelow 时下方也须为 validBlocks。
    if (m_config->requiresBlockBelow && !isValidBlock(region.getBlockState(origin.down()), *m_config)) {
        return false;
    }
    // origin 自身须为空气或 validBlocks。
    const BlockState* originState = region.getBlockState(origin);
    const bool originEmpty = (originState == nullptr) || originState->isAir();
    if (!originEmpty && !isValidBlock(originState, *m_config)) {
        return false;
    }

    // 统计 W/E/N/S/Down 中 validBlocks 数 j 与空气数 k。
    i32 j = 0;
    i32 k = 0;
    const BlockPos neighbors[5] = {origin.west(), origin.east(), origin.north(), origin.south(), origin.down()};
    for (const BlockPos& n : neighbors) {
        if (isValidBlock(region.getBlockState(n), *m_config)) {
            ++j;
        }
        if (isEmpty(region, n)) {
            ++k;
        }
    }

    if (j == m_config->rockCount && k == m_config->holeCount) {
        // 对齐 MC SpringFeature: worldgenlevel.setBlock(blockpos, springconfiguration.state.createLegacyBlock(), 2)。
        // state 是 FluidState，先 createLegacyBlock 转回对应方块状态再放置。
        region.setBlockState(origin, m_config->state->getBlockState(), 2);
        // scheduleTick 省略：项目无 scheduleTick API；流体放置后由后续 tick 自然流动。
        return true;
    }
    return false;
}

} // namespace mc::world::gen::feature
