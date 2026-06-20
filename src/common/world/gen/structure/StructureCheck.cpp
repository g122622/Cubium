/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software to
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "StructureCheck.hpp"

namespace mc::world::gen::structure {

StructureCheckResult StructureCheck::checkStart(u64 chunkPosId, const ResourceLocation& structureId) const
{
    // 查询已加载区块的精确结构引用计数
    auto loadedIt = m_loadedChunks.find(chunkPosId);
    if (loadedIt != m_loadedChunks.end()) {
        return _checkStructureInfo(loadedIt->second, structureId);
    }

    // 精确缓存中无数据，需要加载区块后通过 onStructureLoad() 写入
    return StructureCheckResult::ChunkLoadNeeded;
}

void StructureCheck::onStructureLoad(
    u64 chunkPosId, const std::unordered_map<ResourceLocation, i32>& structureRefCounts)
{
    // 将精确的结构引用计数数据写入 m_loadedChunks
    // 对齐 MC 1.21.11 StructureCheck.onStructureLoad()：
    // 区块结构数据加载完成后，遍历所有有效 StructureStart，将其引用计数缓存起来
    m_loadedChunks[chunkPosId] = ChunkStructureEntries{structureRefCounts};
}

void StructureCheck::incrementReference(u64 chunkPosId, const ResourceLocation& structureId)
{
    auto it = m_loadedChunks.find(chunkPosId);
    if (it != m_loadedChunks.end()) {
        auto& entries = it->second;
        auto refIt = entries.structureRefCounts.find(structureId);
        if (refIt != entries.structureRefCounts.end()) {
            // 已有条目，递增引用计数
            ++refIt->second;
        } else {
            // 结构 ID 不在已有条目中，初始化引用计数为 1
            entries.structureRefCounts[structureId] = 1;
        }
    } else {
        // 区块尚未在精确缓存中，创建新条目
        ChunkStructureEntries entries;
        entries.structureRefCounts[structureId] = 1;
        m_loadedChunks[chunkPosId] = std::move(entries);
    }
}

void StructureCheck::clearCache()
{
    m_loadedChunks.clear();
}

size_t StructureCheck::loadedChunkCount() const
{
    return m_loadedChunks.size();
}

StructureCheckResult StructureCheck::_checkStructureInfo(
    const ChunkStructureEntries& entries, const ResourceLocation& structureId)
{
    auto it = entries.structureRefCounts.find(structureId);
    if (it != entries.structureRefCounts.end()) {
        // 引用计数 >= 0 表示结构存在于该区块
        // 对齐 MC 1.21.11 StructureCheck.checkStructureInfo()：
        // references >= 0 返回 START_PRESENT
        if (it->second >= 0) {
            return StructureCheckResult::StartPresent;
        }
        // 引用计数为 -1 表示结构不存在于此区块（MC 中用于标记"已检查但不存在"）
        return StructureCheckResult::StartNotPresent;
    }
    // 结构 ID 不在条目中，说明此区块的精确数据中不包含该结构
    return StructureCheckResult::StartNotPresent;
}

} // namespace mc::world::gen::structure
