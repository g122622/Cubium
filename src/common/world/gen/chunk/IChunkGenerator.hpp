#pragma once

#include "../settings/DimensionSettings.hpp"
#include "../../chunk/ChunkStatus.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../biome/Biome.hpp"
#include "../../IWorldWriter.hpp"
#include "../../../core/Types.hpp"
#include <memory>
#include <array>
#include <functional>
#include <vector>

namespace mc {

// 前向声明
class WorldGenRegion;
class WorldGenSpawner;

/**
 * @brief 生成的实体数据（前向声明）
 *
 * 完整定义在 WorldGenSpawner.hpp
 */
struct SpawnedEntityData;

/**
 * @brief 区块生成器接口
 *
 * 参考 MC ChunkGenerator，定义区块生成的核心接口。
 *
 * @note 参考 MC 1.16.5 ChunkGenerator
 */
class IChunkGenerator {
public:
    virtual ~IChunkGenerator() = default;

    // === 生成阶段 ===

    /**
     * @brief 生成结构起点
     * @param region 世界生成区域
     * @param chunk 区块生成器
     *
     * 参考 MC 1.16.5: STRUCTURE_STARTS 阶段
     * 在此阶段确定结构（村庄、神殿等）的起点位置
     */
    virtual void generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk) = 0;

    /**
     * @brief 生成结构引用
     * @param region 世界生成区域
     * @param chunk 区块生成器
     *
     * 参考 MC 1.16.5: STRUCTURE_REFERENCES 阶段
     * 计算结构之间的引用关系，用于结构间的连接
     */
    virtual void generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk) = 0;

    /**
     * @brief 生成生物群系
     * @param region 世界生成区域
     * @param chunk 区块生成器
     */
    virtual void generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk) = 0;

    /**
     * @brief 生成噪声地形
     * @param region 世界生成区域
     * @param chunk 区块生成器
     */
    virtual void generateNoise(WorldGenRegion& region, ChunkPrimer& chunk) = 0;

    /**
     * @brief 生成地表
     * @param region 世界生成区域
     * @param chunk 区块生成器
     */
    virtual void buildSurface(WorldGenRegion& region, ChunkPrimer& chunk) = 0;

    /**
     * @brief 应用雕刻器（洞穴、峡谷等）
     * @param region 世界生成区域
     * @param chunk 区块生成器
     * @param isLiquid 是否是液体雕刻
     */
    virtual void applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid) = 0;

    /**
     * @brief 放置特性（树木、矿石等）
     * @param region 世界生成区域
     * @param chunk 区块生成器
     */
    virtual void placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk) = 0;

    /**
     * @brief 生成初始生物（被动动物）
     *
     * 参考 MC 1.16.5 performWorldGenSpawning
     * 在区块生成时放置被动动物（猪、牛、羊等）。
     * 只放置 Creature 分类（被动动物），不生成怪物。
     *
     * @param region 世界生成区域
     * @param chunk 区块生成器
     * @param outEntities 输出：生成的实体数据列表
     * @return 生成的实体数量
     */
    virtual i32 spawnInitialMobs(WorldGenRegion& region, ChunkPrimer& chunk,
                                  std::vector<SpawnedEntityData>& outEntities) = 0;

    // === 生物群系 ===

    /**
     * @brief 获取指定位置的生物群系
     */
    [[nodiscard]] virtual BiomeId getBiome(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取噪声生物群系（在生物群系坐标）
     */
    [[nodiscard]] virtual BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const = 0;

    // === 高度 ===

    /**
     * @brief 获取生成高度
     * @param x X 坐标
     * @param z Z 坐标
     * @param type 高度图类型
     * @return 高度
     */
    [[nodiscard]] virtual i32 getHeight(i32 x, i32 z, HeightmapType type) const = 0;

    /**
     * @brief 获取生成高度（使用世界表面类型）
     */
    [[nodiscard]] i32 getSpawnHeight(i32 x, i32 z) const {
        return getHeight(x, z, HeightmapType::WorldSurfaceWG);
    }

    // === 基本信息 ===

    [[nodiscard]] virtual u64 seed() const = 0;
    [[nodiscard]] virtual const DimensionSettings& settings() const = 0;
    [[nodiscard]] virtual i32 seaLevel() const = 0;
    [[nodiscard]] virtual i32 getGroundHeight() const { return 64; }
};

/**
 * @brief 世界生成区域
 *
 * 参考 MC WorldGenRegion，提供有限的世界视图给生成器。
 * 只能访问指定区块及其邻居。
 *
 * @note 参考 MC 1.16.5 WorldGenRegion
 */
class WorldGenRegion : public IWorldWriter {
public:
    using IWorldWriter::setBlock;

    /**
     * @brief 构造世界生成区域
     * @param mainX 主区块 X
     * @param mainZ 主区块 Z
     * @param chunks 区块数组（按特定顺序排列）
     */
    WorldGenRegion(ChunkCoord mainX, ChunkCoord mainZ, std::array<IChunk*, 9> chunks);

    // === 区块访问 ===

    /**
     * @brief 获取主区块
     */
    [[nodiscard]] IChunk* getMainChunk() { return m_chunks[4]; }
    [[nodiscard]] const IChunk* getMainChunk() const { return m_chunks[4]; }

    /**
     * @brief 获取指定相对位置的区块
     * @param relX 相对 X（-1, 0, 1）
     * @param relZ 相对 Z（-1, 0, 1）
     */
    [[nodiscard]] IChunk* getChunk(i32 relX, i32 relZ);
    [[nodiscard]] const IChunk* getChunk(i32 relX, i32 relZ) const;

    /**
     * @brief 获取主区块坐标
     */
    [[nodiscard]] ChunkCoord mainX() const { return m_mainX; }
    [[nodiscard]] ChunkCoord mainZ() const { return m_mainZ; }

    // === 方块访问 ===

    /**
     * @brief 获取世界坐标处的方块
     */
    [[nodiscard]] const BlockState* getBlock(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取世界坐标处的方块（BlockPos 版本）
     */
    [[nodiscard]] const BlockState* getBlock(const BlockPos& pos) const {
        return getBlock(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 设置世界坐标处的方块
     */
    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override;

    /**
     * @brief 设置世界坐标处的方块（BlockPos 版本）
     */
    void setBlock(const BlockPos& pos, const BlockState* state) {
        setBlock(pos.x, pos.y, pos.z, state);
    }

    /**
     * @brief 获取世界坐标处的生物群系
     */
    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const;

    // === 高度查询 ===

    /**
     * @brief 获取最高方块 Y 坐标
     */
    [[nodiscard]] i32 getTopBlockY(i32 x, i32 z, HeightmapType type) const;

private:
    ChunkCoord m_mainX;
    ChunkCoord m_mainZ;
    std::array<IChunk*, 9> m_chunks;  // 中心 + 8 邻居

    // 将世界坐标转换为区块索引
    [[nodiscard]] i32 worldToChunkIndex(i32 x, i32 z) const;

    // 将世界坐标转换为本地坐标
    static void worldToLocal(i32 worldX, i32 worldZ, i32& localX, i32& localZ);
};

// ============================================================================
// 区块生成器基类
// ============================================================================

/**
 * @brief 区块生成器基类
 *
 * 提供一些通用的生成器功能。
 */
class BaseChunkGenerator : public IChunkGenerator {
public:
    explicit BaseChunkGenerator(u64 seed, DimensionSettings settings);
    ~BaseChunkGenerator() override = default;

    // === IChunkGenerator 接口 ===

    void generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid) override;
    void placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk) override;
    i32 spawnInitialMobs(WorldGenRegion& region, ChunkPrimer& chunk,
                          std::vector<SpawnedEntityData>& outEntities) override;

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] const DimensionSettings& settings() const override { return m_settings; }
    [[nodiscard]] i32 seaLevel() const override { return m_settings.seaLevel; }

protected:
    u64 m_seed;
    DimensionSettings m_settings;

    // 默认生物群系
    BiomeId m_defaultBiome = Biomes::Plains;

    // 区块生成时的生物放置器
    std::unique_ptr<WorldGenSpawner> m_worldGenSpawner;
};

} // namespace mc
