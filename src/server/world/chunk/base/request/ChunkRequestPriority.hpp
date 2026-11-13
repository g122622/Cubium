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

#pragma once
#include "common/util/thread/ITask.hpp"
#include "server/world/chunk/base/pipeline/ChunkStatus.hpp"
#include "server/world/chunk/lifecycle/demand/base/ChunkDistanceGraph.hpp"
#include "server/world/chunk/lifecycle/demand/base/ChunkLoadLevel.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mc::server {
/** @brief 将需求级别和阶段映射为稳定的区块请求优先级。 */
inline i32 chunkRequestPriority(ChunkCoord x, ChunkCoord z, const world::chunk::ChunkStatus& target, i32 level)
{
    return std::clamp(level, 0, world::chunk::ChunkDistanceGraph::MAX_LEVEL) * 1024 +
        std::max(0, world::chunk::ChunkStatuses::FULL.ordinal() - target.ordinal()) * 32 +
        static_cast<i32>((std::abs(x) + std::abs(z)) & 0xFF);
}

/** @brief 把区块请求优先级映射为通用线程池档位。 */
inline util::TaskPriority chunkTaskPriority(i32 priority)
{
    if (priority == std::numeric_limits<i32>::max()) {
        return util::TaskPriority::Low;
    }
    auto level = priority / 1024;
    auto border = static_cast<i32>(world::chunk::ChunkLoadLevel::Border);
    return level <= border ? util::TaskPriority::High
                           : (level <= border + 2 ? util::TaskPriority::Normal : util::TaskPriority::Low);
}
} // namespace mc::server
