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

#include "../../../core/Types.hpp"
#include "../../IWorld.hpp"
#include "../../biome/Biome.hpp"
#include "../../border/WorldBorder.hpp"
#include "../settings/DimensionSettings.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/chunk/gen/ChunkStep.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace mc {

// 前向声明
class WorldGenRegion;
class WorldGenSpawner;

namespace world::chunk {
class ChunkStep;
}
using world::chunk::ChunkStep;

namespace world::biome {
class BiomeSource;
}

/**
 * @brief 生成的实体数据（前向声明）
 *
 * 完整定义在 WorldGenSpawner.hpp
 */
struct SpawnedEntityData;

/**
 * @brief 区块生成器接口
 *
 * 定义区块生成的核心接口。
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
     * 在此阶段确定结构（村庄、神殿等）的起点位置
     */
    virtual void generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk) = 0;

    /**
     * @brief 生成结构引用
     * @param region 世界生成区域
     * @param chunk 区块生成器
     *
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
     * MC 1.21.11: 不再有单独的液体雕刻阶段，含水层系统决定填充内容
     * @param region 世界生成区域
     * @param chunk 区块生成器
     */
    virtual void applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk) = 0;

    /**
     * @brief 放置特性（树木、矿石等）
     * @param region 世界生成区域
     * @param chunk 区块生成器
     */
    virtual void placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk) = 0;

    /**
     * @brief 生成初始生物（被动动物）
     *
     * 在区块生成时放置被动动物（猪、牛、羊等）。
     * 只放置 Creature 分类（被动动物），不生成怪物。
     *
     * @param region 世界生成区域
     * @param chunk 区块生成器
     * @param outEntities 输出：生成的实体数据列表
     * @return 生成的实体数量
     */
    virtual i32 spawnInitialMobs(
        WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities) = 0;

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
    [[nodiscard]] i32 getSpawnHeight(i32 x, i32 z) const { return getHeight(x, z, HeightmapType::WorldSurfaceWG); }

    // === 基本信息 ===

    [[nodiscard]] virtual u64 seed() const = 0;
    [[nodiscard]] virtual const DimensionSettings& settings() const = 0;
    [[nodiscard]] virtual i32 seaLevel() const = 0;
    [[nodiscard]] virtual i32 getGroundHeight() const { return 64; }

    /**
     * @brief 检查是否为调试世界生成器
     *
     * 调试世界生成器（DebugChunkGenerator）会生成一个展示所有方块状态的网格世界，
     * 在调试世界中，方块的放置和破坏被禁止，游戏机制如计划刻、随机刻等也被禁用。
     *
     * @return true 如果是调试世界生成器
     */
    [[nodiscard]] virtual bool isDebugGenerator() const { return false; }

    // === 生物群系源 ===

    /**
     * @brief 获取生物群系源（MC 1.18+）
     * @return 生物群系源指针
     */
    [[nodiscard]] virtual world::biome::BiomeSource* getBiomeSource() { return nullptr; }
    [[nodiscard]] virtual const world::biome::BiomeSource* getBiomeSource() const { return nullptr; }
};

/**
 * @brief 世界生成区域
 *
 * 提供有限的世界视图给生成器。
 * 访问范围由生成阶段的依赖半径决定。
 * 持有 ChunkStep 以验证读写权限。
 */
class WorldGenRegion : public IWorld {
public:
    using IWorld::getBlockState;
    using IWorld::setBlockState;

    /**
     * @brief 构造世界生成区域（无步骤校验模式）
     * @param mainX 主区块 X
     * @param mainZ 主区块 Z
     * @param chunkRadius 区块半径（0 表示只有中心区块，1 表示 3x3 区域，8 表示 17x17 区域）
     * @param chunks 区块数组（按从左上到右下的顺序排列，数量为 (2*chunkRadius+1)^2）
     */
    WorldGenRegion(ChunkCoord mainX, ChunkCoord mainZ, i32 chunkRadius, std::vector<IChunk*> chunks);

    /**
     * @brief 构造世界生成区域（带步骤校验模式）
     * @param mainX 主区块 X
     * @param mainZ 主区块 Z
     * @param generatingStep 当前正在执行的生成步骤
     * @param chunks 区块数组（按从左上到右下的顺序排列）
     */
    WorldGenRegion(ChunkCoord mainX, ChunkCoord mainZ, const ChunkStep& generatingStep, std::vector<IChunk*> chunks);

    // === 区块访问 ===

    /**
     * @brief 获取主区块
     */
    [[nodiscard]] IChunk* getMainChunk() { return m_chunks[static_cast<std::size_t>(_centerIndex())]; }
    [[nodiscard]] const IChunk* getMainChunk() const { return m_chunks[static_cast<std::size_t>(_centerIndex())]; }

    /**
     * @brief 获取指定相对位置的区块（生成区域特有方法）
     * @param relX 相对 X（范围由 chunkRadius 决定）
     * @param relZ 相对 Z（范围由 chunkRadius 决定）
     */
    [[nodiscard]] IChunk* getChunkAt(i32 relX, i32 relZ);
    [[nodiscard]] const IChunk* getChunkAt(i32 relX, i32 relZ) const;

    /**
     * @brief 获取指定世界坐标的区块（MC 1.21.11 对齐）
     *
     * MC 中 WorldGenRegion.getChunk() 返回 ChunkAccess（对应 IChunk）。
     * 这是特性放置和雕刻器使用的主要区块访问方法。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return IChunk 指针，如果区块不在区域内则返回 nullptr
     */
    [[nodiscard]] IChunk* getIChunk(ChunkCoord x, ChunkCoord z);
    [[nodiscard]] const IChunk* getIChunk(ChunkCoord x, ChunkCoord z) const;
    [[nodiscard]] IChunk* getIChunk(ChunkCoord x, ChunkCoord z, const ChunkStatus& requestedStatus);
    [[nodiscard]] const IChunk* getIChunk(ChunkCoord x, ChunkCoord z, const ChunkStatus& requestedStatus) const;

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
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override;

    /**
     * @brief 获取方块（BlockPos 版本）
     */
    [[nodiscard]] const BlockState* getBlockState(const BlockPos& pos) const override
    {
        return getBlockState(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 设置方块状态（IWorld 接口）
     */
    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override;

    /**
     * @brief 设置方块状态（带标志）
     */
    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        (void)flags;
        return setBlockState(x, y, z, state);
    }

    /**
     * @brief 获取流体状态
     *
     * MC 1.21.11: 从区块获取方块状态，再获取流体状态。
     * 雕刻器通过此方法判断当前位置是否有水/熔岩等流体。
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override;

    /**
     * @brief 获取区块数据（IWorld 接口）
     *
     * MC 1.21.11: 从区块数组获取 ChunkPrimer 底层的 ChunkData。
     * 如果区块不在区域内或不是 ChunkPrimer，返回 nullptr。
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
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(
        const AxisAlignedBB& box, const Entity* except = nullptr) const override;

    /**
     * @brief 获取碰撞箱内的实体
     */
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box, const Entity* except = nullptr) const override;

    /**
     * @brief 获取范围内的实体
     */
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(
        const Vector3& pos, f32 range, const Entity* except = nullptr) const override;

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

    /**
     * @brief 获取当前生成步骤
     *
     * 返回构造时传入的 ChunkStep。如果使用无步骤构造函数，返回 nullptr。
     */
    [[nodiscard]] const ChunkStep* generatingStep() const { return m_generatingStep; }

    /**
     * @brief 获取区块写半径
     *
     * 返回当前生成步骤允许写方块状态的半径。
     * -1 = 不写方块（EMPTY, STRUCTURE_STARTS 等）
     * 0 = 只写中心区块（NOISE, SURFACE, CARVERS）
     * 1 = 写中心区块及 1 格邻居（FEATURES）
     * 如果没有生成步骤，返回 -1。
     */
    [[nodiscard]] i32 blockStateWriteRadius() const
    {
        return m_generatingStep ? m_generatingStep->blockStateWriteRadius() : -1;
    }

private:
    ChunkCoord m_mainX;
    ChunkCoord m_mainZ;
    i32 m_chunkRadius;
    i32 m_chunkDiameter;
    std::vector<IChunk*> m_chunks; // 按行优先顺序存储的动态方阵
    const ChunkStep* m_generatingStep = nullptr;

    // IWorld 所需的状态
    u64 m_seed = 0;
    u64 m_currentTick = 0;
    i64 m_dayTime = 0;
    bool m_hardcore = false;
    Difficulty m_difficulty = Difficulty::Normal;
    math::Random m_random;
    world::border::WorldBorder m_worldBorder;

    // 将世界坐标转换为区块索引
    [[nodiscard]] i32 _worldToChunkIndex(i32 x, i32 z) const;

    [[nodiscard]] i32 _centerIndex() const;

    // 将世界坐标转换为本地坐标
    static void _worldToLocal(i32 worldX, i32 worldZ, i32& localX, i32& localZ);
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
    void applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk) override;
    i32 spawnInitialMobs(
        WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities) override;

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
