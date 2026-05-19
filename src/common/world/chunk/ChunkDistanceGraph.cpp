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

#include "ChunkDistanceGraph.hpp"
#include "ChunkLoadTicket.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <cmath>

namespace mc::world {

namespace {
inline i32 clampLevel(i32 level)
{
    return std::clamp(level, 0, ChunkDistanceGraph::MAX_LEVEL);
}
} // namespace

// ============================================================================
// ChunkDistanceGraph 实现
// ============================================================================

void ChunkDistanceGraph::updateSourceLevel(ChunkCoord x, ChunkCoord z, i32 level, bool isDecreasing)
{
    const u64 key = posToKey(x, z);
    const i32 clampedLevel = clampLevel(level);

    auto sourceIt = m_sourceLevels.find(key);
    const i32 oldSourceLevel = (sourceIt != m_sourceLevels.end()) ? sourceIt->second : MAX_LEVEL;
    i32 newSourceLevel = oldSourceLevel;

    if (isDecreasing) {
        // 仅允许降低（更重要）
        newSourceLevel = std::min(oldSourceLevel, clampedLevel);
    } else {
        // 允许升高/移除
        newSourceLevel = clampedLevel;
    }

    if (newSourceLevel >= MAX_LEVEL) {
        if (sourceIt != m_sourceLevels.end()) {
            m_sourceLevels.erase(sourceIt);
            enqueueUpdate(x, z);
        }
        return;
    }

    if (sourceIt == m_sourceLevels.end() || sourceIt->second != newSourceLevel) {
        m_sourceLevels[key] = newSourceLevel;
        enqueueUpdate(x, z);
    }
}

i32 ChunkDistanceGraph::processUpdates(i32 maxToProcess)
{
    MC_TRACE_EVENT("server.chunk", "ChunkDistanceGraph::processUpdates");

    i32 processed = 0;

    while (!m_updateQueue.empty() && processed < maxToProcess) {
        const u64 key = m_updateQueue.front();
        m_updateQueue.pop();
        m_pendingKeys.erase(key);
        ++processed;

        ChunkCoord x = 0;
        ChunkCoord z = 0;
        keyToPos(key, x, z);
        const i32 currentLevel = getLevel(x, z);

        // 重新计算该区块的最优级别：
        // min(自身源级别, 四邻居级别 + 1)
        i32 recomputedLevel = getSourceLevel(x, z);
        struct Neighbor {
            ChunkCoord x, z;
        };
        const Neighbor neighbors[4] = {
            {x, z - 1}, // 北
            {x, z + 1}, // 南
            {x + 1, z}, // 东
            {x - 1, z}  // 西
        };

        for (const auto& neighbor : neighbors) {
            const i32 neighborLevel = getLevel(neighbor.x, neighbor.z);
            const i32 propagatedLevel = (neighborLevel >= MAX_LEVEL) ? MAX_LEVEL : (neighborLevel + 1);
            if (propagatedLevel < recomputedLevel) {
                recomputedLevel = propagatedLevel;
            }
        }

        if (recomputedLevel > MAX_LEVEL) {
            recomputedLevel = MAX_LEVEL;
        }

        if (recomputedLevel == currentLevel) {
            continue;
        }

        if (recomputedLevel >= MAX_LEVEL) {
            m_levels.erase(key);
        } else {
            m_levels[key] = recomputedLevel;
        }

        onLevelChanged(x, z, currentLevel, recomputedLevel);

        // 当前节点级别变化后，邻居的最优值可能也会变化。
        propagateToNeighbors(x, z, recomputedLevel, recomputedLevel < currentLevel);
    }

    return processed;
}

i32 ChunkDistanceGraph::getLevel(ChunkCoord x, ChunkCoord z) const
{
    u64 key = posToKey(x, z);
    auto it = m_levels.find(key);
    if (it != m_levels.end()) {
        return it->second;
    }
    return MAX_LEVEL; // 未加载
}

void ChunkDistanceGraph::clear()
{
    m_levels.clear();
    m_sourceLevels.clear();
    m_pendingKeys.clear();
    while (!m_updateQueue.empty()) {
        m_updateQueue.pop();
    }
}

i32 ChunkDistanceGraph::getSourceLevel(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);
    auto it = m_sourceLevels.find(key);
    if (it != m_sourceLevels.end()) {
        return it->second;
    }
    return MAX_LEVEL;
}

void ChunkDistanceGraph::onLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel)
{
    MC_TRACE_EVENT("server.chunk",
        "ChunkDistanceGraph::onLevelChanged",
        "x",
        x,
        "z",
        z,
        "oldLevel",
        oldLevel,
        "newLevel",
        newLevel);

    if (m_levelChangeCallback) {
        m_levelChangeCallback(x, z, oldLevel, newLevel);
    }
}

void ChunkDistanceGraph::propagateToNeighbors(ChunkCoord x, ChunkCoord z, i32 level, bool isDecreasing)
{
    // 4个相邻区块：北、南、东、西
    struct Neighbor {
        ChunkCoord x, z;
    };
    Neighbor neighbors[4] = {
        {x, z - 1}, // 北
        {x, z + 1}, // 南
        {x + 1, z}, // 东
        {x - 1, z}  // 西
    };

    i32 propagatedLevel = computePropagatedLevel(level);
    if (propagatedLevel > MAX_LEVEL) {
        propagatedLevel = MAX_LEVEL;
    }

    (void)propagatedLevel;
    (void)isDecreasing;

    for (i32 i = 0; i < 4; ++i) {
        ChunkCoord nx = neighbors[i].x;
        ChunkCoord nz = neighbors[i].z;

        // 无论升/降级，邻居都可能依赖当前节点，统一触发重计算。
        enqueueUpdate(nx, nz);
    }
}

void ChunkDistanceGraph::enqueueUpdate(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);
    if (m_pendingKeys.insert(key).second) {
        m_updateQueue.push(key);
    }
}

} // namespace mc::world
