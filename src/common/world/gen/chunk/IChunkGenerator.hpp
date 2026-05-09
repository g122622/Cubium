#pragma once

#include "../settings/DimensionSettings.hpp"
#include "../../border/WorldBorder.hpp"
#include "../../chunk/ChunkStatus.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../biome/Biome.hpp"
#include "../../IWorld.hpp"
#include "../../../core/Types.hpp"
#include <cstddef>
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
 * 访问范围由生成阶段的 taskRange 决定，常见窗口包括 0、1、8。
 *
 * @note 参考 MC 1.16.5 WorldGenRegion
 */
class WorldGenRegion : public IWorld {
public:
    using IWorld::setBlockState;
    using IWorld::getBlockState;

    /**
     * @brief 构造世界生成区域
     * @param mainX 主区块 X
     * @param mainZ 主区块 Z
        * @param chunkRadius 区块半径（0 表示只有中心区块，8 表示 17x17 区域）
        * @param chunks 区块数组（按从左上到右下的顺序排列）
     */
    WorldGenRegion(ChunkCoord mainX, ChunkCoord mainZ, i32 chunkRadius, std::vector<IChunk*> chunks);

    /**
     * @brief 使用固定大小数组构造世界生成区域
     *
     * 该重载用于测试和旧调用路径，要求区块数组数量必须是奇数平方，
     * 例如 1、9、25，对应半径 0、1、2。
     * TODO 未来会移除这个函数，以保证代码干净
     *
     * @tparam N 区块数组元素数量
     * @param mainX 主区块 X
     * @param mainZ 主区块 Z
     * @param chunks 按从左上到右下顺序排列的区块数组
     */
    template<std::size_t N>
    WorldGenRegion(ChunkCoord mainX, ChunkCoord mainZ, const std::array<IChunk*, N>& chunks)
        : WorldGenRegion(mainX, mainZ, inferChunkRadius(N), std::vector<IChunk*>(chunks.begin(), chunks.end()))
    {
        static_assert(inferChunkRadius(N) >= 0, "WorldGenRegion chunk array size must be an odd square");
    }

    // === 区块访问 ===

    /**
     * @brief 获取主区块
     */
    [[nodiscard]] IChunk* getMainChunk() { return m_chunks[centerIndex()]; }
    [[nodiscard]] const IChunk* getMainChunk() const { return m_chunks[centerIndex()]; }

    /**
     * @brief 获取指定相对位置的区块（生成区域特有方法）
     * @param relX 相对 X（范围由 chunkRadius 决定）
     * @param relZ 相对 Z（范围由 chunkRadius 决定）
     */
    [[nodiscard]] IChunk* getChunkAt(i32 relX, i32 relZ);
    [[nodiscard]] const IChunk* getChunkAt(i32 relX, i32 relZ) const;

    /**
     * @brief 获取主区块坐标
     */
    [[nodiscard]] ChunkCoord mainX() const { return m_mainX; }
    [[nodiscard]] ChunkCoord mainZ() const { return m_mainZ; }
    [[nodiscard]] i32 chunkRadius() const { return m_chunkRadius; }
    [[nodiscard]] ChunkCoord minChunkX() const { return m_mainX - m_chunkRadius; }
    [[nodiscard]] ChunkCoord maxChunkX() const { return m_mainX + m_chunkRadius; }
    [[nodiscard]] ChunkCoord minChunkZ() const { return m_mainZ - m_chunkRadius; }
    [[nodiscard]] ChunkCoord maxChunkZ() const { return m_mainZ + m_chunkRadius; }

    // === IWorld 接口实现 ===

    /**
     * @brief 获取方块状态（IWorld 接口）
     */
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取方块（BlockPos 版本）
     */
    [[nodiscard]] const BlockState* getBlockState(const BlockPos& pos) const {
        return getBlockState(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 设置方块状态（IWorld 接口）
     */
    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override;

    /**
     * @brief 设置方块状态（带标志）
     */
    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override {
        (void)flags;
        return setBlockState(x, y, z, state);
    }

    /**
     * @brief 获取流体状态
     * @note 生成区域暂不支持流体查询，返回空流体
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override;

    /**
     * @brief 获取区块数据（IWorld 接口）
     * @note 生成区域不存储 ChunkData，返回 nullptr
     */
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const override;

    /**
     * @brief 检查区块是否存在
     */
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const override;

    /**
     * @brief 获取最高方块 Y 坐标
     */
    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override;

    /**
     * @brief 获取方块光照
     */
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override;

    /**
     * @brief 获取天空光照
     */
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override;

    /**
     * @brief 检查碰撞
     */
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override;

    /**
     * @brief 获取碰撞箱
     */
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override;

    /**
     * @brief 检查是否在世界边界内
     */
    [[nodiscard]] bool isWithinWorldBounds(i32 x, i32 y, i32 z) const override;

    /**
     * @brief 检查实体碰撞
     */
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB& box, const Entity* except = nullptr) const override;

    /**
     * @brief 获取实体碰撞箱
     */
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB& box, const Entity* except = nullptr) const override;

    /**
     * @brief 获取碰撞箱内的实体
     */
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except = nullptr) const override;

    /**
     * @brief 获取范围内的实体
     */
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3& pos, f32 range, const Entity* except = nullptr) const override;

    /**
     * @brief 获取维度 ID
     */
    [[nodiscard]] DimensionId dimension() const override;

    /**
     * @brief 获取世界种子
     */
    [[nodiscard]] u64 seed() const override { return m_seed; }

    /**
     * @brief 获取当前 tick
     */
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    /**
     * @brief 获取一天内的时间
     */
    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }

    /**
     * @brief 是否客户端
     */
    [[nodiscard]] bool isClientSide() override { return false; }

    /**
     * @brief 是否困难模式
     */
    [[nodiscard]] bool isHardcore() const override { return m_hardcore; }

    /**
     * @brief 获取难度
     */
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    /**
     * @brief 获取物理引擎
     */
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }

    /**
     * @brief 获取 Tick 管理器
     */
    [[nodiscard]] world::tick::TickManager& tickManager() override;
    [[nodiscard]] const world::tick::TickManager& tickManager() const override;

    /**
     * @brief 获取随机数生成器
     */
    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    /**
     * @brief 获取世界边界
     * @note 生成区域使用默认世界边界
     */
    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    // === 生成区域特有方法 ===

    /**
     * @brief 获取世界坐标处的生物群系
     */
    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取最高方块 Y 坐标（指定高度图类型）
     */
    [[nodiscard]] i32 getTopBlockY(i32 x, i32 z, HeightmapType type) const;

    /**
     * @brief 设置种子（用于生成）
     */
    void setSeed(u64 seed) { m_seed = seed; }

    /**
     * @brief 设置当前 tick
     */
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    /**
     * @brief 设置一天内的时间
     */
    void setDayTime(i64 dayTime) { m_dayTime = dayTime; }

private:
    /**
     * @brief 根据区块数量推导区块半径
     * TODO 未来会移除这个函数，以保证代码干净
     *
     * @param chunkCount 区块总数
     * @return 区块半径；若数量不是奇数平方则返回 -1
     */
    static constexpr i32 inferChunkRadius(std::size_t chunkCount)
    {
        i32 radius = 0;
        while (true) {
            const std::size_t diameter = static_cast<std::size_t>(radius * 2 + 1);
            const std::size_t expectedCount = diameter * diameter;
            if (expectedCount == chunkCount) {
                return radius;
            }
            if (expectedCount > chunkCount) {
                return -1;
            }
            ++radius;
        }
    }

    ChunkCoord m_mainX;
    ChunkCoord m_mainZ;
    i32 m_chunkRadius;
    i32 m_chunkDiameter;
    std::vector<IChunk*> m_chunks;  // 按行优先顺序存储的动态方阵

    // IWorld 所需的状态
    u64 m_seed = 0;
    u64 m_currentTick = 0;
    i64 m_dayTime = 0;
    bool m_hardcore = false;
    Difficulty m_difficulty = Difficulty::Normal;
    math::Random m_random;
    world::border::WorldBorder m_worldBorder;

    // 将世界坐标转换为区块索引
    [[nodiscard]] i32 worldToChunkIndex(i32 x, i32 z) const;

    [[nodiscard]] i32 centerIndex() const;

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
