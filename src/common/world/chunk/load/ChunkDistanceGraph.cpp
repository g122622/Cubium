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

#include "common/world/chunk/load/ChunkDistanceGraph.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <algorithm>

using namespace mc::trace;

namespace mc::world::chunk {

namespace {
inline i32 clampLevel(i32 level)
{
    return std::clamp(level, 0, ChunkDistanceGraph::MAX_LEVEL);
}
} // namespace

// ============================================================================
// ChunkDistanceGraph 实现
//
// 级别语义：
//   - [0, MAX_LEVEL-1]：被源/邻居触达的传播级别；
//   - MAX_LEVEL：传播最外沿（仍被触达，仅不再继续向外传播）——生成 halo 的最外圈，
//     其持有者可能被邻居生成按需复用，必须保留；
//   - UNREACHED_LEVEL：未被任何源触达的哨兵。getLevel 对无级别区块返回它；
//     onLevelChanged 的新级别用它表达"已离开传播范围"，使持有者可据此立即回收。
// ============================================================================

void ChunkDistanceGraph::updateSourceLevel(ChunkCoord x, ChunkCoord z, i32 level, bool isDecreasing)
{
    const u64 key = posToKey(x, z);
    const i32 clampedLevel = clampLevel(level);

    auto sourceIt = m_sourceLevels.find(key);
    const i32 oldSourceLevel = (sourceIt != m_sourceLevels.end()) ? sourceIt->second : UNREACHED_LEVEL;
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
            _enqueueUpdate(x, z);
        }
        return;
    }

    if (sourceIt == m_sourceLevels.end() || sourceIt->second != newSourceLevel) {
        m_sourceLevels[key] = newSourceLevel;
        _enqueueUpdate(x, z);
    }
}

i32 ChunkDistanceGraph::processUpdates(i32 maxToProcess)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkDistanceGraph::processUpdates");

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

        // 重新计算该区块的最优级别：min(自身源级别, 可传播邻居级别 + 1)。
        // 【关键】只有级别 < MAX_LEVEL 的邻居才向外传播：MAX_LEVEL 是传播最外沿，
        // 若最外沿也 +1 再封顶回 MAX_LEVEL，45 会沿连通区域无限扩散，"halo 最外沿"
        // 与"传播范围之外"将无法区分（持有者回收判据随之失效）。
        // 故 recomputedLevel 的取值范围是 [0, MAX_LEVEL]，无源且无可传播邻居时为
        // UNREACHED_LEVEL（> MAX_LEVEL）。
        i32 recomputedLevel = getSourceLevel(x, z);

        for (i32 dz = -1; dz <= 1; ++dz) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dz == 0) continue;
                const i32 neighborLevel = getLevel(x + dx, z + dz);
                if (neighborLevel >= MAX_LEVEL) {
                    continue; // 最外沿/未触达的邻居不向外传播
                }
                const i32 propagatedLevel = neighborLevel + 1;
                if (propagatedLevel < recomputedLevel) {
                    recomputedLevel = propagatedLevel;
                }
            }
        }

        if (recomputedLevel == currentLevel) {
            continue;
        }

        // UNREACHED_LEVEL（无源且无可传播邻居）的条目从 m_levels 剪枝，保持映射有界；
        // MAX_LEVEL（最外沿）正常存储——它是"仍被触达"的状态，必须能从表里读出。
        if (recomputedLevel >= UNREACHED_LEVEL) {
            m_levels.erase(key);
        } else {
            m_levels[key] = recomputedLevel;
        }

        onLevelChanged(x, z, currentLevel, recomputedLevel);

        // 当前节点级别变化后，邻居的最优值可能也会变化。
        _propagateToNeighbors(x, z);
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
    return UNREACHED_LEVEL; // 未被触达
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
    return UNREACHED_LEVEL;
}

void ChunkDistanceGraph::onLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk,
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

void ChunkDistanceGraph::_propagateToNeighbors(ChunkCoord x, ChunkCoord z)
{
    // 八方向传播（棋盘距离/Chebyshev），与 MC 一致。
    // 无论升/降级，邻居的最优级别都可能依赖当前节点，统一触发重计算。
    for (i32 dz = -1; dz <= 1; ++dz) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dz == 0) continue;
            _enqueueUpdate(x + dx, z + dz);
        }
    }
}

void ChunkDistanceGraph::_enqueueUpdate(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);
    if (m_pendingKeys.insert(key).second) {
        m_updateQueue.push(key);
    }
}

} // namespace mc::world::chunk
