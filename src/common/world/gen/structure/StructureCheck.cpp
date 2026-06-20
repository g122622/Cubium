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
    ChunkStructureEntries entries;
    entries.structureRefCounts = structureRefCounts;
    m_loadedChunks[chunkPosId] = std::move(entries);

    // 从 m_featureChecks 中移除该区块的近似缓存条目
    // 精确数据已到达，近似缓存不再需要
    for (auto& [structureId, chunkMap] : m_featureChecks) {
        chunkMap.erase(chunkPosId);
    }
}

void StructureCheck::incrementReference(u64 chunkPosId, const ResourceLocation& structureId)
{
    auto it = m_loadedChunks.find(chunkPosId);
    if (it != m_loadedChunks.end()) {
        auto& entries = it->second;
        auto refIt = entries.structureRefCounts.find(structureId);
        if (refIt != entries.structureRefCounts.end()) {
            ++refIt->second;
        } else {
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
    m_featureChecks.clear();
}

size_t StructureCheck::loadedChunkCount() const
{
    return m_loadedChunks.size();
}

size_t StructureCheck::featureCheckCount() const
{
    size_t count = 0;
    for (const auto& [id, chunkMap] : m_featureChecks) {
        count += chunkMap.size();
    }
    return count;
}

StructureCheckResult StructureCheck::_checkStructureInfo(
    const ChunkStructureEntries& entries, const ResourceLocation& structureId)
{
    auto it = entries.structureRefCounts.find(structureId);
    if (it != entries.structureRefCounts.end()) {
        if (it->second >= 0) {
            // 引用计数 >= 0 表示结构存在于此区块
            return StructureCheckResult::StartPresent;
        }
        // 引用计数为 -1 表示结构不存在于此区块
        return StructureCheckResult::StartNotPresent;
    }
    // 结构 ID 不在条目中，说明此区块的精确数据中不包含该结构
    return StructureCheckResult::StartNotPresent;
}

} // namespace mc::world::gen::structure
