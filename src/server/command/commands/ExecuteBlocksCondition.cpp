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

#include "server/command/commands/ExecuteBlocksCondition.hpp"

#include <algorithm>

namespace mc::command {

bool blocksMatch(const BlockPos& start,
    const BlockPos& end,
    const BlockPos& destination,
    bool masked,
    const BlockStateLookup& lookup)
{
    const i32 minX = std::min(start.x, end.x);
    const i32 minY = std::min(start.y, end.y);
    const i32 minZ = std::min(start.z, end.z);
    const i32 maxX = std::max(start.x, end.x);
    const i32 maxY = std::max(start.y, end.y);
    const i32 maxZ = std::max(start.z, end.z);

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                const BlockPos sourcePos(x, y, z);
                const BlockPos targetPos(
                    destination.x + (x - minX), destination.y + (y - minY), destination.z + (z - minZ));

                const BlockState* sourceState = lookup(sourcePos);
                const BlockState* targetState = lookup(targetPos);
                // 读不到就不算匹配：区块未加载时不能把"未知"当成"相同"。
                if (sourceState == nullptr || targetState == nullptr) {
                    return false;
                }

                // masked 模式跳过源区域里的空气，其余模式逐格严格比较。
                if (masked && sourceState->isAir()) {
                    continue;
                }

                if (sourceState->getBlock().blockId() != targetState->getBlock().blockId() ||
                    sourceState->stateId() != targetState->stateId()) {
                    return false;
                }
            }
        }
    }
    return true;
}

} // namespace mc::command
