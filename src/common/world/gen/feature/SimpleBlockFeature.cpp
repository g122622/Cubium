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
 */

#include "SimpleBlockFeature.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include <memory>
#include <utility>

namespace mc::world::gen::feature::cave {

// ============================================================================
// SimpleBlockFeature
// ============================================================================

bool SimpleBlockFeature::place(
    WorldGenRegion& region, math::Random& random, const BlockPos& pos, const SimpleBlockConfig& config)
{
    MC_UNUSED(random);

    // 取本次放置的目标状态：由提供者按其 kind 采样（simple 直接返回、weighted 按权重等）。
    if (config.provider == nullptr) {
        return false;
    }
    const BlockState* toPlace = config.provider->getState(region, random, pos.x, pos.y, pos.z);
    if (toPlace == nullptr) {
        return false;
    }

    const BlockState* currentState = region.getBlockState(pos);
    // ChunkData 对未初始化 section 返回 nullptr 表示空气（空气不持久化）。
    // 空气可被替换（AIR.canBeReplaced()=true），故 nullptr 视为可放置。
    const bool replaceable = (currentState == nullptr) || currentState->canBeReplaced();

    // 如果当前位置不可替换，则放置失败
    if (!replaceable) {
        return false;
    }

    // canSurvive 终判：防止放置到无法支撑该方块的位置（如草落在地表上方的空气格）。
    // 对应 MC SimpleBlockFeature 调用 blockstate.canSurvive(level, pos)；
    // 本项目用 isValidPosition 承担 canSurvive 语义（如 BushBlock 检查下方 dirt/耕地）。
    // isValidPosition 接收 IBlockReader&（IWorld 的标记派生，无新增成员），WorldGenRegion 是 IWorld
    // 的另一子类，需先提升到 IWorld& 再向下转 IBlockReader&（与 WouldSurvivePredicate 同模式，安全）。
    const Block& block = toPlace->getBlock();
    auto& blockReader = static_cast<IBlockReader&>(static_cast<IWorld&>(region));
    if (!block.isValidPosition(*toPlace, blockReader, pos)) {
        return false;
    }

    region.setBlockState(pos, toPlace, 3);
    return true;
}

// ============================================================================
// ConfiguredSimpleBlockFeature
// ============================================================================

ConfiguredSimpleBlockFeature::ConfiguredSimpleBlockFeature(
    std::unique_ptr<SimpleBlockConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredSimpleBlockFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);

    if (!m_config) {
        return false;
    }

    return SimpleBlockFeature::place(region, random, pos, *m_config);
}

} // namespace mc::world::gen::feature::cave
