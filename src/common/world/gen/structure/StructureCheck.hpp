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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/resource/ResourceLocation.hpp"
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
 * 当前实现了精确缓存层（m_loadedChunks），对齐 MC 中 StructureCheck.loadedChunks。
 * 当区块结构数据加载完成后，通过 onStructureLoad() 写入缓存；
 * 当结构引用被递增时，通过 incrementReference() 更新缓存；
 * 通过 checkStart() 可以快速查询某区块是否包含某结构。
 *
 * TODO: 后续集成生物群系检查层（对齐 MC StructureCheck.featureChecks），
 * 在 StructurePlacement 判断逻辑中添加近似缓存，避免重复执行昂贵的生物群系检查。
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
     * 查询 m_loadedChunks 精确缓存。如果该区块数据尚未加载，返回 ChunkLoadNeeded。
     * 对齐 MC 1.21.11 StructureCheck.checkStart() 中对 loadedChunks 的查询逻辑。
     *
     * TODO: 当前此方法尚无调用方。后续需在以下场景集成调用方：
     * 1. NoiseChunkGenerator 在 STRUCTURE_STARTS 阶段生成结构起点前，
     *    先调用 checkStart() 查询缓存，避免对已有结构数据的区块重复生成；
     * 2. /locate 命令搜索最近结构时，调用 checkStart() 快速跳过不含结构的区块；
     * 3. 集成 featureChecks 近似缓存层后，在 loadedChunks 未命中时查询 featureChecks，
     *    避免对同一区块重复执行昂贵的生物群系检查。
     *
     * @param chunkPosId 区块坐标打包的 64 位 ID（高32位=X，低32位=Z）
     * @param structureId 结构 ID
     * @return 检查结果
     */
    [[nodiscard]] StructureCheckResult checkStart(u64 chunkPosId, const ResourceLocation& structureId) const;

    /**
     * @brief 区块结构数据加载完成时通知缓存
     *
     * 将区块中所有结构的引用计数写入 m_loadedChunks（精确缓存）。
     * 对齐 MC 1.21.11 StructureCheck.onStructureLoad()。
     *
     * 在 NoiseChunkGenerator::generateStructureStarts() 完成后调用，
     * 此时 StructureStart 的引用计数为 0（引用计数在 STRUCTURE_REFERENCES 阶段递增）。
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
     * 清空 m_loadedChunks 缓存。
     * 在世界卸载或维度重新加载时调用。
     * 对齐 MC 1.21.11 中 StructureCheck 随 ServerLevel 生命周期销毁的行为。
     */
    void clearCache();

    /**
     * @brief 缓存已加载区块的条目数
     */
    [[nodiscard]] size_t loadedChunkCount() const;

private:
    /**
     * @brief 根据 loadedChunks 中的精确数据判断结构存在性
     */
    [[nodiscard]] static StructureCheckResult _checkStructureInfo(
        const ChunkStructureEntries& entries, const ResourceLocation& structureId);

    /// 已加载区块的结构引用计数缓存（精确数据）
    /// key: 区块坐标打包的 64 位 ID（高32位=X，低32位=Z），value: 该区块中所有结构的引用计数
    std::unordered_map<u64, ChunkStructureEntries> m_loadedChunks;
};

} // namespace mc::world::gen::structure
