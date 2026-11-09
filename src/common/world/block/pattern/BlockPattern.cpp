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

#include "BlockPattern.hpp"

#include "BlockInWorld.hpp"
#include "BlockPatternCache.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace mc::blockpattern {

// ============================================================================
// BlockPattern
// ============================================================================

BlockPattern::BlockPattern(std::vector<std::vector<std::vector<Predicate>>> pattern)
    : m_pattern(std::move(pattern))
{
    m_depth = static_cast<i32>(m_pattern.size());
    if (m_depth > 0) {
        m_height = static_cast<i32>(m_pattern[0].size());
        if (m_height > 0) {
            m_width = static_cast<i32>(m_pattern[0][0].size());
        } else {
            m_width = 0;
        }
    } else {
        m_height = 0;
        m_width = 0;
    }
}

const BlockInWorld& BlockPatternMatch::getBlock(i32 widthIdx, i32 heightIdx, i32 depthIdx) const
{
    // 从匹配期间的共享缓存读取，返回匹配时刻的快照。
    return m_cache->get(
        BlockPattern::translateAndRotate(m_frontTopLeft, m_forwards, m_up, widthIdx, heightIdx, depthIdx));
}

BlockPos BlockPattern::translateAndRotate(
    BlockPos origin, Direction forwards, Direction up, i32 widthIdx, i32 heightIdx, i32 depthIdx)
{
    // forwards 与 up 必须正交（非平行、非相反）
    MC_ASSERT_RELEASE_MSG(forwards != up && forwards != Directions::opposite(up),
        "BlockPattern::translateAndRotate: forwards and up must be orthogonal");

    // vec3i = forwards 的单位向量
    const i32 fx = Directions::xOffset(forwards);
    const i32 fy = Directions::yOffset(forwards);
    const i32 fz = Directions::zOffset(forwards);

    // vec3i1 = up 的单位向量
    const i32 ux = Directions::xOffset(up);
    const i32 uy = Directions::yOffset(up);
    const i32 uz = Directions::zOffset(up);

    // vec3i2 = vec3i × vec3i1（右手叉积）
    // cross(a, b) = (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x)
    const i32 cx = fy * uz - fz * uy;
    const i32 cy = fz * ux - fx * uz;
    const i32 cz = fx * uy - fy * ux;

    // 最终位置 = origin + up * (-heightIdx) + 侧向 * widthIdx + forwards * depthIdx
    return BlockPos(origin.x + ux * (-heightIdx) + cx * widthIdx + fx * depthIdx,
        origin.y + uy * (-heightIdx) + cy * widthIdx + fy * depthIdx,
        origin.z + uz * (-heightIdx) + cz * widthIdx + fz * depthIdx);
}

std::optional<BlockPatternMatch> BlockPattern::find(IWorld& world, const BlockPos& pos) const
{
    // 在搜索开始时创建一次缓存，所有候选位置/朝向共享它。
    auto cache = std::make_shared<BlockPatternCache>(world, false);
    const i32 maxDim = std::max({m_width, m_height, m_depth});

    // 遍历 [pos, pos + (maxDim-1, maxDim-1, maxDim-1)] 范围内所有候选起始位置。
    // 迭代顺序：x 最快、y 次之、z 最慢。
    for (i32 dz = 0; dz < maxDim; ++dz) {
        for (i32 dy = 0; dy < maxDim; ++dy) {
            for (i32 dx = 0; dx < maxDim; ++dx) {
                const BlockPos candidate(pos.x + dx, pos.y + dy, pos.z + dz);

                // 尝试所有 (forwards, up) 方向组合
                for (Direction forwards : Directions::all()) {
                    for (Direction up : Directions::all()) {
                        // forwards 与 up 必须正交
                        if (up == forwards || up == Directions::opposite(forwards)) {
                            continue;
                        }
                        auto match = _matches(cache, candidate, forwards, up);
                        if (match.has_value()) {
                            return match;
                        }
                    }
                }
            }
        }
    }

    return std::nullopt;
}

std::optional<BlockPatternMatch> BlockPattern::matches(
    IWorld& world, const BlockPos& pos, Direction forwards, Direction up) const
{
    // 该公开重载用于单元测试，内部自建一个缓存。
    auto cache = std::make_shared<BlockPatternCache>(world, false);
    return _matches(cache, pos, forwards, up);
}

std::optional<BlockPatternMatch> BlockPattern::_matches(
    const std::shared_ptr<BlockPatternCache>& cache, const BlockPos& pos, Direction forwards, Direction up) const
{
    // 遍历顺序：width(i) 外层、height(j) 中层、depth(k) 内层。
    for (i32 i = 0; i < m_width; ++i) {
        for (i32 j = 0; j < m_height; ++j) {
            for (i32 k = 0; k < m_depth; ++k) {
                // 谓词按 [depth][height][width] 下标访问
                const BlockPos worldPos = BlockPattern::translateAndRotate(pos, forwards, up, i, j, k);
                const BlockInWorld& blockInWorld = cache->get(worldPos);

                if (!m_pattern[static_cast<size_t>(k)][static_cast<size_t>(j)][static_cast<size_t>(i)](blockInWorld)) {
                    return std::nullopt;
                }
            }
        }
    }

    return BlockPatternMatch(cache, pos, forwards, up, m_width, m_height, m_depth);
}

} // namespace mc::blockpattern
