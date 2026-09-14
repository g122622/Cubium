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

#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <mutex>
#include <unordered_map>

namespace mc::world::gen::structure {

/**
 * @brief 结构存在性检查结果
 *
 * 对齐 MC 1.21.11 StructureCheckResult，表示区块中结构存在性的三态判断结果。
 */
enum class StructureCheckResult {
    StartPresent,    ///< 结构起点存在于该区块
    StartNotPresent, ///< 结构起点不存在于该区块
    ChunkLoadNeeded  ///< 需要完整加载区块才能确定
};

/**
 * @brief 结构存在性检查缓存
 *
 * 对齐 MC 1.21.11 StructureCheck，缓存已加载区块的结构引用计数，
 * 避免重复加载区块结构数据来判断结构是否存在。
 *
 * 实现两层缓存：
 * 1. 精确缓存层（m_loadedChunks）：记录已加载区块的精确结构引用计数。
 * 2. 近似缓存层（m_featureChecks）：基于放置规则的频率缩减和排斥区检查，
 *    在精确缓存未命中时提供快速的排除判断，避免加载区块。
 *
 * 当区块结构数据加载完成后，通过 onStructureLoad() 写入精确缓存；
 * 当结构引用被递增时，通过 incrementReference() 更新精确缓存；
 * 通过 checkStart() 可以快速查询某区块是否包含某结构。
 */
class StructureCheck {
public:
    /**
     * @brief 区块级别的结构引用计数条目
     *
     * 记录某个区块中每种结构的引用计数。
     * 对齐 MC 1.21.11 StructureCheck.loadedChunks 中的 Object2IntMap<Structure>。
     * 引用计数 >= 0 表示结构存在于该区块（0 表示存在但尚未被其他区块引用）。
     */
    struct ChunkStructureEntries {
        /// 结构 ID 到引用计数的映射
        std::unordered_map<ResourceLocation, i32> structureRefCounts;
    };

    StructureCheck() = default;

    /**
     * @brief 检查指定区块中是否存在某结构的起点
     *
     * 查询两层缓存。首先查询精确缓存 m_loadedChunks，
     * 如果未命中，则查询近似缓存 m_featureChecks（基于放置规则的快速排除判断）。
     *
     * 对齐 MC 1.21.11 StructureCheck.checkStart() 的三层判断逻辑：
     * 1. 精确缓存命中：直接返回 StartPresent 或 StartNotPresent
     * 2. 近似缓存命中：若放置规则检查不通过，返回 StartNotPresent；否则返回 ChunkLoadNeeded
     * 3. 均未命中：返回 ChunkLoadNeeded
     *
     * @param chunkPosId 区块坐标打包的 64 位 ID（高32位=X，低32位=Z）
     * @param structureId 结构 ID
     * @param skipKnownStructures 是否跳过已发现的结构（/locate 命令搜索时使用）：
     *        当为 true 时，引用计数 > 0 的结构被视为 StartNotPresent，
     *        即只返回引用计数为 0 的"全新"结构起点
     * @return 检查结果
     */
    [[nodiscard]] StructureCheckResult checkStart(
        u64 chunkPosId, const ResourceLocation& structureId, bool skipKnownStructures = false) const;

    /**
     * @brief 区块结构数据加载完成时通知缓存
     *
     * 将区块中所有结构的引用计数写入 m_loadedChunks（精确缓存），
     * 同时清除该区块在 m_featureChecks（近似缓存）中的所有条目，
     * 因为精确数据已经可用，不再需要近似数据。
     *
     * 对齐 MC 1.21.11 StructureCheck.onStructureLoad()。
     *
     * @param chunkPosId 区块坐标打包的 64 位 ID（高32位=X，低32位=Z）
     * @param structureRefCounts 结构 ID 到引用计数的映射（新建 StructureStart 的计数为 0）
     */
    void onStructureLoad(u64 chunkPosId, const std::unordered_map<ResourceLocation, i32>& structureRefCounts);

    /**
     * @brief 递增指定区块中某结构的引用计数
     *
     * 在结构引用阶段（STRUCTURE_REFERENCES），当发现其他区块引用了某结构时，
     * 递增该区块中该结构的引用计数。
     * 对齐 MC 1.21.11 StructureCheck.incrementReference()。
     *
     * @param chunkPosId 区块坐标打包的 64 位 ID（高32位=X，低32位=Z）
     * @param structureId 结构 ID
     */
    void incrementReference(u64 chunkPosId, const ResourceLocation& structureId);

    /**
     * @brief 清理所有缓存
     *
     * 清空 m_loadedChunks 和 m_featureChecks 缓存。
     * 在世界卸载或维度重新加载时调用。
     * 对齐 MC 1.21.11 中 StructureCheck 随 ServerLevel 生命周期销毁的行为。
     */
    void clearCache();

    /**
     * @brief 缓存已加载区块的条目数
     */
    [[nodiscard]] size_t loadedChunkCount() const;

    /**
     * @brief 设置近似缓存中某个区块的放置规则检查结果
     *
     * 对齐 MC 1.21.11 StructureCheck.featureChecks.computeIfAbsent()，
     * 当精确缓存未命中且放置规则检查完成时，将结果缓存到 m_featureChecks。
     * 调用方（如 findNearestStructure）在执行完 isStructureChunk() 检查后，
     * 可将结果写入近似缓存，后续对同一区块的查询可直接使用缓存结果。
     *
     * @param chunkPosId 区块坐标打包的 64 位 ID
     * @param canCreate 放置规则检查是否通过（true = 可能包含结构，false = 一定不含结构）
     */
    void setFeatureCheckResult(u64 chunkPosId, bool canCreate) const;

private:
    /**
     * @brief 根据 loadedChunks 中的精确数据判断结构存在性
     *
     * @param entries 区块结构条目
     * @param structureId 结构 ID
     * @param skipKnownStructures 是否跳过已发现的结构
     */
    [[nodiscard]] static StructureCheckResult _checkStructureInfo(
        const ChunkStructureEntries& entries, const ResourceLocation& structureId, bool skipKnownStructures);

    /// 已加载区块的结构引用计数缓存（精确数据）
    /// key: 区块坐标打包的 64 位 ID（高32位=X，低32位=Z），value: 该区块中所有结构的引用计数
    std::unordered_map<u64, ChunkStructureEntries> m_loadedChunks;

    /// 近似缓存层：基于放置规则的快速排除判断
    /// key: 区块坐标打包的 64 位 ID，value: 该区块是否通过了放置规则的频率缩减检查
    /// 对齐 MC 1.21.11 StructureCheck.featureChecks 中的 Long2BooleanMap
    /// 当精确缓存未命中时，查询此缓存可避免加载区块
    mutable std::unordered_map<u64, bool> m_featureChecks;

    /// 互斥锁：保护 m_loadedChunks 和 m_featureChecks 的并发访问
    /// 对齐 MC 1.21.11 中通过 server.execute() 将 onStructureLoad 调度到主线程的线程安全机制。
    /// MC 使用线程约束（main thread confinement）保证线程安全，
    /// 而我们使用互斥锁，因为 chunk generation 在多线程的 UniversalWorkerPool 中执行。
    mutable std::mutex m_mutex;
};

} // namespace mc::world::gen::structure
