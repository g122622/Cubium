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

#include "PlacedFeature.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {

PlacedFeature::PlacedFeature(
    const ConfiguredFeatureBase* feature, std::unique_ptr<ConfiguredPlacement> placement, ResourceLocation id)
    : m_feature(feature)
    , m_placement(std::move(placement))
    , m_id(std::move(id))
{
    // feature/placement 非空是 PlacedFeature 的构造不变量：PlacedFeatureLoader 在构造前
    // 已校验 configured_feature 解析成功且 placement 链非空，此处断言暴露上游违约而非静默吞错。
    MC_ASSERT_RELEASE(m_feature != nullptr);
    MC_ASSERT_RELEASE(m_placement != nullptr);
}

bool PlacedFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& origin) const
{
    // 走 placement 链得到候选位置列表
    const std::vector<BlockPos> positions = m_placement->getPositions(region, random, origin);

    bool anyPlaced = false;
    for (const BlockPos& pos : positions) {
        if (m_feature->place(region, chunk, generator, random, pos)) {
            anyPlaced = true;
        }
    }
    return anyPlaced;
}

} // namespace mc
