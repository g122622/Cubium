/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the including without limitation the rights
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

#include "common/world/chunk/gen/ChunkDependencies.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include <cstddef>
#include <utility>
#include <vector>

namespace mc::world::chunk {

ChunkDependencies::ChunkDependencies(std::vector<const ChunkStatus*> dependencyByRadius)
    : m_dependencyByRadius(std::move(dependencyByRadius))
{
    if (m_dependencyByRadius.empty()) {
        return;
    }

    // 构建 radiusByDependency 反向查找表
    // 大小 = 最大 status index + 1
    i32 maxIndex = 0;
    for (const auto* status : m_dependencyByRadius) {
        if (status != nullptr && status->ordinal() > maxIndex) {
            maxIndex = status->ordinal();
        }
    }

    m_radiusByDependency.resize(static_cast<size_t>(maxIndex + 1));

    // 从半径 0 向上遍历，填充 radiusByDependency
    // 对于每个半径 j，其对应的 status 的 index 及以下所有 index 都指向半径 j
    // 这与 Java 的实现一致：for each j, fill radiusByDependency[0..status.index] = j
    for (i32 j = 0; j < static_cast<i32>(m_dependencyByRadius.size()); ++j) {
        const ChunkStatus* status = m_dependencyByRadius[static_cast<size_t>(j)];
        if (status != nullptr) {
            i32 k = status->ordinal();
            for (i32 l = 0; l <= k && l < static_cast<i32>(m_radiusByDependency.size()); ++l) {
                m_radiusByDependency[static_cast<size_t>(l)] = j;
            }
        }
    }
}

const ChunkStatus* ChunkDependencies::get(i32 radius) const
{
    if (radius < 0 || radius >= static_cast<i32>(m_dependencyByRadius.size())) {
        return nullptr;
    }
    return m_dependencyByRadius[static_cast<size_t>(radius)];
}

i32 ChunkDependencies::getRadiusOf(const ChunkStatus& status) const
{
    i32 index = status.ordinal();
    if (index < 0 || index >= static_cast<i32>(m_radiusByDependency.size())) {
        MC_ASSERT_RELEASE(false && "Requesting a ChunkStatus outside of dependency range");
    }
    return m_radiusByDependency[static_cast<size_t>(index)];
}

} // namespace mc::world::chunk
