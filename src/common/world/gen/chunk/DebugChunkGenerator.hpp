#pragma once

#include "IChunkGenerator.hpp"
#include "../../block/Block.hpp"
#include "../../block/BlockRegistry.hpp"
#include <vector>

namespace mc {

/**
 * @brief 调试模式区块生成器
 *
 * 参考 MC 1.16.5 DebugChunkGenerator，生成一个展示所有方块状态的网格世界。
 *
 * 特点：
 * - Y=60 层是屏障方块（Barrier）基座
 * - Y=70 层是所有方块状态的网格
 * - 方块只在奇数坐标位置放置（形成间隔布局）
 * - 不生成结构、生物群系、特性等
 *
 * 用途：
 * - 资源包开发和测试
 * - 方块模型和纹理调试
 * - 方块状态可视化
 *
 * @note 参考 MC 1.16.5 net.minecraft.world.gen.DebugChunkGenerator
 */
class DebugChunkGenerator : public BaseChunkGenerator {
public:
    /**
     * @brief 构造调试区块生成器
     * @param biomeRegistry 生物群系注册表（用于提供默认生物群系）
     */
    explicit DebugChunkGenerator();

    ~DebugChunkGenerator() override = default;

    // === IChunkGenerator 接口 ===

    void generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateNoise(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void buildSurface(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid) override;
    void placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk) override;
    i32 spawnInitialMobs(WorldGenRegion& region, ChunkPrimer& chunk,
                          std::vector<SpawnedEntityData>& outEntities) override;

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z, HeightmapType type) const override;
    [[nodiscard]] i32 getGroundHeight() const override { return 70; }
    [[nodiscard]] i32 seaLevel() const override { return 0; }

    /**
     * @brief 调试世界生成器标识
     * @return true（调试世界生成器）
     */
    [[nodiscard]] bool isDebugGenerator() const override { return true; }

    // === 调试模式特有方法 ===

    /**
     * @brief 获取所有有效方块状态列表
     * @return 方块状态列表的常量引用
     */
    [[nodiscard]] static const std::vector<const BlockState*>& getAllValidStates();

    /**
     * @brief 获取网格宽度
     * @return 网格宽度（X方向方块数）
     */
    [[nodiscard]] static i32 getGridWidth();

    /**
     * @brief 获取网格高度
     * @return 网格高度（Z方向方块数）
     */
    [[nodiscard]] static i32 getGridHeight();

    /**
     * @brief 根据世界坐标获取方块状态
     * @param x 世界X坐标
     * @param z 世界Z坐标
     * @return 方块状态指针，如果位置无方块返回空气状态
     *
     * 参考 MC 1.16.5: DebugChunkGenerator#getBlockStateFor
     * 方块只在奇数坐标放置：
     * - x > 0, z > 0
     * - x % 2 != 0, z % 2 != 0
     * - 索引计算: abs(x/2 * GRID_WIDTH + z/2)
     */
    [[nodiscard]] static const BlockState* getBlockStateFor(i32 x, i32 z);

    /**
     * @brief 初始化所有方块状态列表
     *
     * 必须在 BlockRegistry 完成注册后调用。
     * 收集所有方块的所有状态，计算网格尺寸。
     */
    static void initializeValidStates();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized();

private:
    /// 屏障方块状态
    static const BlockState* s_barrierState;

    /// 空气方块状态
    static const BlockState* s_airState;

    /// 所有有效方块状态（静态缓存）
    static std::vector<const BlockState*> s_allValidStates;

    /// 网格宽度（X方向）
    static i32 s_gridWidth;

    /// 网格高度（Z方向）
    static i32 s_gridHeight;

    /// 是否已初始化
    static bool s_initialized;
};

} // namespace mc
