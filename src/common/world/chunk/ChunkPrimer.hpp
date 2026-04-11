#pragma once

#include "IChunk.hpp"
#include "ChunkStatus.hpp"
#include "ChunkData.hpp"
#include "../block/Block.hpp"
#include "../gen/spawn/WorldGenSpawner.hpp"
#include "../gen/structure/Structure.hpp"
#include <array>
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
 * 参考 MC ChunkPrimer，这是区块生成过程中的中间状态类。
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
    [[nodiscard]] const BlockState* getBlock(BlockCoord x, BlockCoord y, BlockCoord z) const override;
    void setBlock(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override;
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
    void updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override;

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
    [[nodiscard]] const ChunkStatus& getChunkStatus() const { return *m_chunkStatus; }

    /**
     * @brief 设置当前生成阶段
     */
    void setChunkStatus(const ChunkStatus& status);

    /**
     * @brief 检查是否已完成指定阶段
     */
    [[nodiscard]] bool hasCompletedStatus(const ChunkStatus& status) const {
        return m_chunkStatus->isAtLeast(status);
    }

    // ============================================================================
    // 生物群系管理
    // ============================================================================

    /**
     * @brief 设置生物群系容器
     */
    void setBiomes(BiomeContainer biomes) { m_biomes = std::move(biomes); }

    /**
     * @brief 获取生物群系容器
     */
    [[nodiscard]] const BiomeContainer& getBiomes() const { return m_biomes; }
    [[nodiscard]] BiomeContainer& getBiomes() { return m_biomes; }

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
    [[nodiscard]] const std::vector<BlockCoord>& getLightPositions() const { return m_lightPositions; }

    // ============================================================================
    // 雕刻掩码 (Carving Mask)
    // ============================================================================

    /**
     * @brief 获取雕刻掩码（空气雕刻）
     */
    [[nodiscard]] std::vector<bool>& getCarvingMaskAir() { return m_carvingMaskAir; }
    [[nodiscard]] const std::vector<bool>& getCarvingMaskAir() const { return m_carvingMaskAir; }

    /**
     * @brief 获取雕刻掩码（液体雕刻）
     */
    [[nodiscard]] std::vector<bool>& getCarvingMaskLiquid() { return m_carvingMaskLiquid; }
    [[nodiscard]] const std::vector<bool>& getCarvingMaskLiquid() const { return m_carvingMaskLiquid; }

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
    void addSpawnedEntity(SpawnedEntityData data) {
        m_spawnedEntities.push_back(std::move(data));
    }

    /**
     * @brief 清空生成的实体列表
     */
    void clearSpawnedEntities() {
        m_spawnedEntities.clear();
    }

    /**
     * @brief 获取生成的实体数量
     */
    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    // ============================================================================
    // 结构起点管理
    // ============================================================================

    /**
     * @brief 添加结构起点
     * @param structureName 结构名称
     * @param start 结构起点实例
     */
    void addStructureStart(const String& structureName, std::unique_ptr<world::gen::structure::StructureStart> start) {
        m_structureStarts[structureName] = std::move(start);
    }

    /**
     * @brief 获取结构起点
     * @param structureName 结构名称
     * @return 结构起点指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] world::gen::structure::StructureStart* getStructureStart(const String& structureName) {
        auto it = m_structureStarts.find(structureName);
        return it != m_structureStarts.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 获取所有结构起点
     */
    [[nodiscard]] const std::unordered_map<String, std::unique_ptr<world::gen::structure::StructureStart>>& structureStarts() const {
        return m_structureStarts;
    }

    /**
     * @brief 检查是否有结构起点
     */
    [[nodiscard]] bool hasStructureStarts() const { return !m_structureStarts.empty(); }

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
    [[nodiscard]] ChunkData* getChunkData() { return m_data.get(); }
    [[nodiscard]] const ChunkData* getChunkData() const { return m_data.get(); }

    // ============================================================================
    // 静态工具方法
    // ============================================================================

    /**
     * @brief 将方块坐标打包为短整型
     */
    [[nodiscard]] static u16 packToLocal(BlockCoord x, BlockCoord y, BlockCoord z);

    /**
     * @brief 将短整型解包为方块坐标
     */
    static void unpackFromLocal(u16 packed, i32 yOffset, ChunkCoord chunkX, ChunkCoord chunkZ,
                                BlockCoord& x, BlockCoord& y, BlockCoord& z);

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

    // 雕刻掩码（用于追踪哪些位置被雕刻过）
    std::vector<bool> m_carvingMaskAir;
    std::vector<bool> m_carvingMaskLiquid;

    // 区块生成时生成的实体
    std::vector<SpawnedEntityData> m_spawnedEntities;

    // 结构起点（用于结构生成）
    std::unordered_map<String, std::unique_ptr<world::gen::structure::StructureStart>> m_structureStarts;

    // 辅助方法
    [[nodiscard]] static bool isValidBlockCoord(BlockCoord x, BlockCoord y, BlockCoord z);
    void initializeCarvingMasks();
};

} // namespace mc
