/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "ServerLightQueue.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"

namespace mc::server {

namespace {

/// 将世界坐标打包为区块键，与 ChunkPos::toId 一致（X/Z 各 32 位拼接）
[[nodiscard]] u64 _chunkKey(i32 chunkX, i32 chunkZ) noexcept
{
    ChunkPos pos{chunkX, chunkZ};
    return pos.toId();
}

} // namespace

void ServerLightQueue::queueBlockChange(i32 x, i32 y, i32 z)
{
    const i32 chunkX = x >> world::CHUNK_SHIFT;
    const i32 chunkZ = z >> world::CHUNK_SHIFT;

    // 用 asLong 去重，12 位 Y 可覆盖 ±2048，远超 1.21.11 建筑高度范围，
    // 避免 toId() 8 位 Y 掩码在高建筑维度碰撞
    const i64 posLong = BlockPos::asLong(x, y, z);

    m_chunkTasks[_chunkKey(chunkX, chunkZ)].changedPositionLongs.insert(posLong);
}

void ServerLightQueue::drainAndProcess(WorldLightManager& manager)
{
    if (m_chunkTasks.empty()) {
        return;
    }

    // 取出任务表后清空，避免处理过程中嵌套入队导致无限循环
    std::unordered_map<u64, _ChunkLightTasks> tasks;
    tasks.swap(m_chunkTasks);

    for (auto& [chunkKey, task] : tasks) {
        if (task.changedPositionLongs.empty()) {
            continue;
        }

        // 从区块键还原区块坐标
        const ChunkPos chunkPos = ChunkPos::fromId(chunkKey);
        const i32 chunkX = chunkPos.x;
        const i32 chunkZ = chunkPos.z;

        // 将去重后的 packed long 还原为 BlockPos 列表，交给批量接口
        std::vector<BlockPos> positions;
        positions.reserve(task.changedPositionLongs.size());
        for (const i64 posLong : task.changedPositionLongs) {
            positions.push_back(BlockPos::fromLong(posLong));
        }

        manager.checkBlocks(chunkX, chunkZ, std::move(positions));
    }
}

} // namespace mc::server
