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

#include "BlockPredicateFilterPlacement.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/placement/Placement.hpp"
#include <vector>

namespace mc {

std::vector<BlockPos> BlockPredicateFilterPlacement::getPositions(
    WorldGenRegion& region, math::Random& /*random*/, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto& filterConfig = static_cast<const BlockPredicateFilterConfig&>(config);
    if (filterConfig.predicate == nullptr) {
        // 无谓词视为永真过滤（防御：正常路径必经 BlockPredicateParser 构造出非空谓词）
        return {basePos};
    }
    // WorldGenRegion 继承 IWorld，可直接作为谓词测试的世界接口
    if (filterConfig.predicate->test(region, basePos)) {
        return {basePos};
    }
    return {};
}

} // namespace mc
