/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice shall be included in all
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
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace mc::world::gen::structure {

StructureCheckResult StructureCheck::checkStart(
    u64 chunkPosId, const ResourceLocation& structureId, bool skipKnownStructures) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 第一层：查询精确缓存 m_loadedChunks
    // 对齐 MC 1.21.11 StructureCheck.checkStart() 中的 loadedChunks 查询
    auto loadedIt = m_loadedChunks.find(chunkPosId);
    if (loadedIt != m_loadedChunks.end()) {
        return _checkStructureInfo(loadedIt->second, structureId, skipKnownStructures);
    }

    // 第二层：查询近似缓存 m_featureChecks
    // 对齐 MC 1.21.11 StructureCheck.checkStart() 中 loadedChunks 未命中时的逻辑：
    // 先查询近似缓存，如果缓存中有结果，直接使用。
    // 近似缓存不依赖 placement 参数——它存储的是外部调用方（如 findNearestStructure）
    // 通过 setFeatureCheckResult() 写入的 isStructureChunk() 检查结果。
    auto featureIt = m_featureChecks.find(chunkPosId);
    if (featureIt != m_featureChecks.end()) {
        // 近似缓存命中：如果缓存值为 false，说明此前已判断此区块不可能包含结构
        if (!featureIt->second) {
            return StructureCheckResult::StartNotPresent;
        }
        // 缓存值为 true，说明放置规则检查通过，但仍需加载区块才能确定
        return StructureCheckResult::ChunkLoadNeeded;
    }

    // 精确缓存和近似缓存均未命中（或未提供放置规则），需要加载区块
    return StructureCheckResult::ChunkLoadNeeded;
}

void StructureCheck::onStructureLoad(
    u64 chunkPosId, const std::unordered_map<ResourceLocation, i32>& structureRefCounts)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // 将精确的结构引用计数数据写入 m_loadedChunks
    m_loadedChunks[chunkPosId] = ChunkStructureEntries{structureRefCounts};

    // 对齐 MC 1.21.11 StructureCheck.storeFullResults()：
    // 精确数据已可用，清除该区块在近似缓存中的条目
    m_featureChecks.erase(chunkPosId);
}

void StructureCheck::incrementReference(u64 chunkPosId, const ResourceLocation& structureId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
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
    std::lock_guard<std::mutex> lock(m_mutex);
    m_loadedChunks.clear();
    m_featureChecks.clear();
}

size_t StructureCheck::loadedChunkCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_loadedChunks.size();
}

StructureCheckResult StructureCheck::_checkStructureInfo(
    const ChunkStructureEntries& entries, const ResourceLocation& structureId, bool skipKnownStructures)
{
    auto it = entries.structureRefCounts.find(structureId);
    if (it != entries.structureRefCounts.end()) {
        // 引用计数 >= 0 表示结构存在于该区块
        // 对齐 MC 1.21.11 StructureCheck.checkStructureInfo()：
        // references >= 0 返回 START_PRESENT
        if (it->second >= 0) {
            // skipKnownStructures 为 true 时（/locate 命令搜索场景），
            // 引用计数 > 0 的结构被视为"已被发现"，应跳过
            if (skipKnownStructures && it->second > 0) {
                return StructureCheckResult::StartNotPresent;
            }
            return StructureCheckResult::StartPresent;
        }
        // 引用计数为 -1 表示结构不存在于此区块（MC 中用于标记"已检查但不存在"）
        return StructureCheckResult::StartNotPresent;
    }
    // 结构 ID 不在条目中，说明此区块的精确数据中不包含该结构
    return StructureCheckResult::StartNotPresent;
}

void StructureCheck::setFeatureCheckResult(u64 chunkPosId, bool canCreate) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // 仅在精确缓存中不存在时才更新近似缓存
    // 对齐 MC 1.21.11 StructureCheck.featureChecks.computeIfAbsent()
    if (m_loadedChunks.find(chunkPosId) == m_loadedChunks.end()) {
        m_featureChecks[chunkPosId] = canCreate;
    }
}

} // namespace mc::world::gen::structure
