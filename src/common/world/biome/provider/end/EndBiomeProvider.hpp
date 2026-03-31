#pragma once

#include "../../BiomeProvider.hpp"
#include "../../../gen/noise/OctavesNoiseGenerator.hpp"
#include <memory>

namespace mc {
namespace biome {
namespace end {

/**
 * @brief 末地生物群系提供者
 *
 * 参考 MC 1.16.5 EndBiomeProvider
 * 使用简单的噪声采样区分主岛和外岛区域。
 *
 * 末地生物群系：
 * - The End (末地) - 主岛，包含末地龙战斗区域
 * - Small End Islands (小型末地岛屿) - 外岛的小型岛屿
 * - End Midlands (末地中部) - 外岛的中部区域
 * - End Highlands (末地高地) - 外岛的高地区域
 * - End Barrens (末地荒地) - 外岛的荒地区域
 *
 * 使用示例:
 * @code
 * EndBiomeProvider provider(seed);
 * BiomeId biome = provider.getBiome(x, y, z);
 * @endcode
 */
class EndBiomeProvider : public BiomeProvider {
public:
    /**
     * @brief 构造函数
     * @param seed 世界种子
     */
    explicit EndBiomeProvider(u64 seed);

    ~EndBiomeProvider() override = default;

    // ========== BiomeProvider 接口实现 ==========

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] f32 getDepth(i32 x, i32 z) const override;
    [[nodiscard]] f32 getScale(i32 x, i32 z) const override;
    void fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) override;

    // ========== 末地特有方法 ==========

    /**
     * @brief 检查是否在主岛范围内
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 是否在主岛范围内
     */
    [[nodiscard]] bool isInMainIsland(i32 x, i32 z) const;

    /**
     * @brief 获取岛屿噪声值
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 岛屿噪声值
     */
    [[nodiscard]] f32 getIslandNoise(i32 x, i32 z) const;

private:
    // 岛屿噪声生成器
    std::unique_ptr<SimplexNoiseGenerator> m_islandNoise;

    // 主岛半径（方块单位）
    // MC 1.16.5: 主岛中心半径约为 256 方块 (sqrt(4096) * 4)
    // 距离判断: (x >> 2)^2 + (z >> 2)^2 <= 4096
    // 转换: x^2 + z^2 <= 4096 * 16 = 65536, sqrt(65536) = 256
    static constexpr i32 MAIN_ISLAND_RADIUS = 256;

    /**
     * @brief 根据噪声值和位置选择生物群系
     */
    [[nodiscard]] BiomeId selectBiome(i32 x, i32 z, f32 noise) const;
};

} // namespace end
} // namespace biome
} // namespace mc
