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

#include "common/world/block/Block.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include "common/world/chunk/IChunk.hpp"
#include "common/world/gen/carver/CarvingMask.hpp"
#include "common/world/gen/spawn/WorldGenSpawner.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

// ============================================================================
// 区块生成器 (中间状态)
// ============================================================================

/**
 * @brief 区块生成器 (ChunkPrimer)
 *
 * 区块生成过程中的中间状态类。
 * 在区块完全生成之前，使用此类存储临时数据。
 * 生成完成后转换为 ChunkData。
 *
 * 使用方法：
 * 1. 创建 ChunkPrimer
 * 2. 按阶段生成：BIOMES -> NOISE -> SURFACE -> CARVERS -> FEATURES -> HEIGHTMAPS
 * 3. 转换为 ChunkData
 *
 * @note 此类不是线程安全的，应该在单个线程中操作
 */
class ChunkPrimer : public IChunk {
public:
    // ============================================================================
    // 构造函数
    // ============================================================================

    /**
     * @brief 创建空区块生成器
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     */
    explicit ChunkPrimer(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 从现有 ChunkData 创建（用于加载）
     */
    explicit ChunkPrimer(std::unique_ptr<ChunkData> data);

    ~ChunkPrimer() override = default;

    // 禁止拷贝
    ChunkPrimer(const ChunkPrimer&) = delete;
    ChunkPrimer& operator=(const ChunkPrimer&) = delete;

    // 允许移动
    ChunkPrimer(ChunkPrimer&&) noexcept = default;
    ChunkPrimer& operator=(ChunkPrimer&&) noexcept = default;

    // ============================================================================
    // IChunk 接口实现
    // ============================================================================

    [[nodiscard]] ChunkCoord x() const override { return m_x; }
    [[nodiscard]] ChunkCoord z() const override { return m_z; }
    [[nodiscard]] ChunkPos pos() const override { return ChunkPos(m_x, m_z); }

    // 方块访问
    [[nodiscard]] const BlockState* getBlockState(BlockCoord x, BlockCoord y, BlockCoord z) const override;
    void setBlockState(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override;
    [[nodiscard]] u32 getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const override;
    void setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId) override;

    // 区块段访问
    [[nodiscard]] ChunkSection* getSection(i32 index) override;
    [[nodiscard]] const ChunkSection* getSection(i32 index) const override;
    [[nodiscard]] bool hasSection(i32 index) const override;
    ChunkSection* createSection(i32 index) override;
    [[nodiscard]] const ChunkSection* const* getSections() const override;

    // 高度图
    [[nodiscard]] BlockCoord getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const override;
    void updateHeightmap(
        HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override;

    // 状态
    [[nodiscard]] ChunkLoadStatus getStatus() const override { return m_status; }
    void setStatus(ChunkLoadStatus status) override { m_status = status; }

    // 标记
    [[nodiscard]] bool isModified() const override { return m_modified; }
    void setModified(bool modified) override { m_modified = modified; }

    // ============================================================================
    // 生成阶段管理
    // ============================================================================

    /**
     * @brief 获取当前生成阶段
     */
    [[nodiscard]] const ChunkStatus& getChunkStatus() const noexcept { return *m_chunkStatus; }

    /**
     * @brief 设置当前生成阶段
     */
    void setChunkStatus(const ChunkStatus& status);

    /**
     * @brief 检查是否已完成指定阶段
     */
    [[nodiscard]] bool hasCompletedStatus(const ChunkStatus& status) const noexcept
    {
        return m_chunkStatus->isAtLeast(status);
    }

    // ============================================================================
    // 生物群系管理
    // ============================================================================

    /**
     * @brief 设置生物群系容器
     */
    void setBiomes(BiomeContainer biomes) noexcept { m_biomes = std::move(biomes); }

    /**
     * @brief 获取生物群系容器
     */
    [[nodiscard]] const BiomeContainer& getBiomes() const noexcept { return m_biomes; }
    [[nodiscard]] BiomeContainer& getBiomes() noexcept { return m_biomes; }

    /**
     * @brief 获取方块位置的生物群系
     */
    [[nodiscard]] BiomeId getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const override;

    // ============================================================================
    // 光源位置
    // ============================================================================

    /**
     * @brief 添加光源位置（用于光照计算）
     */
    void addLightPosition(BlockCoord x, BlockCoord y, BlockCoord z);

    /**
     * @brief 获取所有光源位置
     */
    [[nodiscard]] const std::vector<BlockCoord>& getLightPositions() const noexcept { return m_lightPositions; }

    // ============================================================================
    // 高度图管理
    // ============================================================================

    /**
     * @brief 获取高度图
     */
    [[nodiscard]] Heightmap& getHeightmap(HeightmapType type);
    [[nodiscard]] const Heightmap& getHeightmap(HeightmapType type) const;

    /**
     * @brief 更新所有高度图
     */
    void updateAllHeightmaps();

    // ============================================================================
    // 生成的实体
    // ============================================================================

    /**
     * @brief 获取区块生成时生成的实体列表
     *
     * 区块生成过程中调用 WorldGenSpawner 生成的被动动物会存储在这里。
     * 在区块生成完成后，由 ServerWorld 将这些实体真正创建到世界中。
     */
    [[nodiscard]] std::vector<SpawnedEntityData>& spawnedEntities() { return m_spawnedEntities; }
    [[nodiscard]] const std::vector<SpawnedEntityData>& spawnedEntities() const { return m_spawnedEntities; }

    /**
     * @brief 添加生成的实体数据
     */
    void addSpawnedEntity(SpawnedEntityData data) { m_spawnedEntities.push_back(std::move(data)); }

    /**
     * @brief 清空生成的实体列表
     */
    void clearSpawnedEntities() noexcept { m_spawnedEntities.clear(); }

    /**
     * @brief 获取生成的实体数量
     */
    [[nodiscard]] size_t spawnedEntityCount() const noexcept { return m_spawnedEntities.size(); }

    // ============================================================================
    // 结构起点管理
    // ============================================================================

    /**
     * @brief 添加结构起点
     * @param structureName 结构名称
     * @param start 结构起点实例
     */
    void addStructureStart(
        const std::string& structureName, std::unique_ptr<world::gen::structure::StructureStart> start)
    {
        m_structureStarts[structureName] = std::move(start);
    }

    /**
     * @brief 获取结构起点
     * @param structureName 结构名称
     * @return 结构起点指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] world::gen::structure::StructureStart* getStructureStart(const std::string& structureName) noexcept
    {
        auto it = m_structureStarts.find(structureName);
        return it != m_structureStarts.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 获取所有结构起点
     */
    [[nodiscard]] const std::unordered_map<std::string, std::unique_ptr<world::gen::structure::StructureStart>>&
    structureStarts() const noexcept
    {
        return m_structureStarts;
    }

    /**
     * @brief 检查是否有结构起点
     */
    [[nodiscard]] bool hasStructureStarts() const noexcept { return !m_structureStarts.empty(); }

    // ============================================================================
    // 雕刻掩码
    // ============================================================================

    /**
     * @brief 获取雕刻掩码
     *
     * MC原版中，CarvingMask 在 AIR 和 LIQUID 两个雕刻阶段之间共享。
     * 第一次调用时自动创建掩码。
     *
     * @return 雕刻掩码引用
     */
    [[nodiscard]] CarvingMask& carvingMask();

    /**
     * @brief 检查是否已有雕刻掩码
     */
    [[nodiscard]] bool hasCarvingMask() const noexcept { return m_carvingMask != nullptr; }

    // ============================================================================
    // 转换方法
    // ============================================================================

    /**
     * @brief 转换为 ChunkData
     * @return 完成的区块数据
     */
    [[nodiscard]] std::unique_ptr<ChunkData> toChunkData();

    /**
     * @brief 获取底层 ChunkData（如果存在）
     */
    [[nodiscard]] ChunkData* getChunkData() noexcept { return m_data.get(); }
    [[nodiscard]] const ChunkData* getChunkData() const noexcept { return m_data.get(); }

    // ============================================================================
    // 静态工具方法
    // ============================================================================

    /**
     * @brief 将方块坐标打包为短整型
     */
    [[nodiscard]] static u16 packToLocal(BlockCoord x, BlockCoord y, BlockCoord z) noexcept;

    /**
     * @brief 将短整型解包为方块坐标
     */
    static void unpackFromLocal(u16 packed,
        i32 yOffset,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        BlockCoord& x,
        BlockCoord& y,
        BlockCoord& z) noexcept;

private:
    ChunkCoord m_x;
    ChunkCoord m_z;

    // 底层数据
    std::unique_ptr<ChunkData> m_data;

    // 生成状态
    const ChunkStatus* m_chunkStatus = &ChunkStatuses::EMPTY;
    ChunkLoadStatus m_status = ChunkLoadStatus::Empty;
    bool m_modified = false;

    // 生物群系
    BiomeContainer m_biomes;

    // 高度图
    std::unordered_map<HeightmapType, Heightmap> m_heightmaps;

    // 光源位置
    std::vector<BlockCoord> m_lightPositions;

    // 区块生成时生成的实体
    std::vector<SpawnedEntityData> m_spawnedEntities;

    // 结构起点（用于结构生成）
    std::unordered_map<std::string, std::unique_ptr<world::gen::structure::StructureStart>> m_structureStarts;

    // 雕刻掩码（AIR 和 LIQUID 两个雕刻阶段共享）
    std::unique_ptr<CarvingMask> m_carvingMask;

    // 辅助方法
    [[nodiscard]] static bool _isValidBlockCoord(BlockCoord x, BlockCoord y, BlockCoord z) noexcept;
};

} // namespace mc
