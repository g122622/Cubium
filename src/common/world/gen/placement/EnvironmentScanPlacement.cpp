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

#include "EnvironmentScanPlacement.hpp"
#include "../../WorldConstants.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/placement/Placement.hpp"
#include <vector>

namespace mc {

std::vector<BlockPos> EnvironmentScanPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)random;
    const auto& scanConfig = static_cast<const EnvironmentScanConfig&>(config);

    BlockPos currentPos = basePos;

    // 步骤1：检查起始位置是否满足 allowedSearchCondition
    if (scanConfig.allowedSearchCondition) {
        if (!scanConfig.allowedSearchCondition->test(region, currentPos)) {
            return {};
        }
    }

    // 步骤2：循环扫描
    const i32 dx = Directions::xOffset(scanConfig.directionOfSearch);
    const i32 dy = Directions::yOffset(scanConfig.directionOfSearch);
    const i32 dz = Directions::zOffset(scanConfig.directionOfSearch);

    for (i32 step = 0; step < scanConfig.maxSteps; ++step) {
        // 检查目标条件
        if (scanConfig.targetCondition && scanConfig.targetCondition->test(region, currentPos)) {
            return {currentPos};
        }

        // 移动一步
        currentPos = BlockPos(currentPos.x + dx, currentPos.y + dy, currentPos.z + dz);

        // 检查是否越界
        if (currentPos.y < world::MIN_BUILD_HEIGHT || currentPos.y >= world::MAX_BUILD_HEIGHT) {
            return {};
        }

        // 检查 allowedSearchCondition
        if (scanConfig.allowedSearchCondition) {
            if (!scanConfig.allowedSearchCondition->test(region, currentPos)) {
                break;
            }
        }
    }

    // 步骤3：循环结束后再检查一次目标条件
    if (scanConfig.targetCondition && scanConfig.targetCondition->test(region, currentPos)) {
        return {currentPos};
    }

    return {};
}

} // namespace mc
