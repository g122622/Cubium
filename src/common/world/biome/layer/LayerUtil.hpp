#pragma once

#include "LayerContext.hpp"
#include "transformers/SourceLayers.hpp"
#include "transformers/ClimateLayers.hpp"
#include "transformers/EdgeLayers.hpp"
#include "transformers/ZoomLayers.hpp"
#include "transformers/BiomeLayers.hpp"
#include "transformers/MergeLayers.hpp"
#include "../BiomeProvider.hpp"
#include <memory>
#include <functional>

namespace mc {

// Forward declaration
class LayerStack;

namespace LayerUtil {

/**
 * @brief 重复应用缩放层
 *
 * @param seed 世界种子
 * @param zoom 缩放层变换器
 * @param input 输入工厂
 * @param count 重复次数
 * @param contextFactory 上下文工厂函数
 * @return 缩放后的区域工厂
 */
std::unique_ptr<IAreaFactory> repeatZoom(
    u64 seed,
    layer::ZoomLayer& zoom,
    std::unique_ptr<IAreaFactory> input,
    i32 count,
    std::function<std::shared_ptr<LayerContext>(u64)> contextFactory);

/**
 * @brief 构建主世界层链
 *
 * 参考 MC 1.16.5 LayerUtil.func_237216_a_
 *
 * @param seed 世界种子
 * @param legacyBiomeInit 是否使用旧版生物群系初始化
 * @param largeBiomes 是否使用大型生物群系
 * @param biomeSize 生物群系大小参数 (默认 4)
 * @param riverSize 河流大小参数 (默认 4)
 * @return 最终区域工厂
 */
std::unique_ptr<IAreaFactory> buildOverworldLayers(
    u64 seed,
    bool legacyBiomeInit = false,
    bool largeBiomes = false,
    i32 biomeSize = 4,
    i32 riverSize = 4);

/**
 * @brief 创建主世界层堆叠
 *
 * @param seed 世界种子
 * @param isLargeBiomes 是否使用大型生物群系
 * @return 层堆叠
 */
std::unique_ptr<LayerStack> createOverworldLayers(u64 seed, bool isLargeBiomes = false);

/**
 * @brief 创建下界层堆叠
 *
 * @param seed 世界种子
 * @return 层堆叠
 */
std::unique_ptr<LayerStack> createNetherLayers(u64 seed);

/**
 * @brief 创建末地层堆叠
 *
 * @param seed 世界种子
 * @return 层堆叠
 */
std::unique_ptr<LayerStack> createEndLayers(u64 seed);

} // namespace LayerUtil

/**
 * @brief 层堆叠
 *
 * 管理完整的层链，提供生物群系采样接口。
 */
class LayerStack {
public:
    /**
     * @brief 构造层堆叠
     * @param area 最终区域
     */
    explicit LayerStack(std::unique_ptr<IArea> area);

    /**
     * @brief 采样指定位置的生物群系
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 生物群系 ID
     */
    [[nodiscard]] BiomeId sample(i32 x, i32 z) const;

    /**
     * @brief 采样指定区域的生物群系
     */
    [[nodiscard]] std::vector<BiomeId> sampleArea(i32 startX, i32 startZ, i32 width, i32 height) const;

    /**
     * @brief 批量采样指定区域的生物群系
     *
     * 单次加锁批量获取多个坐标的生物群系，减少锁竞争开销。
     *
     * @param startX 起始 X 坐标
     * @param startZ 起始 Z 坐标
     * @param width 宽度
     * @param height 高度
     * @param output 输出数组（大小必须 >= width * height）
     */
    void sampleBatch(i32 startX, i32 startZ, i32 width, i32 height, BiomeId* output) const;

private:
    std::unique_ptr<IArea> m_area;
};

/**
 * @brief 基于层的生物群系提供者
 *
 * 使用 Layer 系统生成生物群系。
 * 参考 MC 1.16.5 OverworldBiomeProvider
 */
class LayerBiomeProvider : public BiomeProvider {
public:
    LayerBiomeProvider(u64 seed, bool isLargeBiomes = false);
    ~LayerBiomeProvider() override = default;

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] f32 getDepth(i32 x, i32 z) const override;
    [[nodiscard]] f32 getScale(i32 x, i32 z) const override;
    [[nodiscard]] const Biome& getBiomeDefinition(BiomeId id) const override;
    void fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) override;

    /**
     * @brief 批量获取生物群系
     *
     * 使用 LayerStack::sampleBatch 优化性能。
     */
    void getBiomesBatch(i32 startX, i32 startY, i32 startZ, i32 width, i32 height,
                         BiomeId* output) const override;

    /**
     * @brief 批量获取噪声坐标的生物群系
     *
     * Layer 系统本质是 2D，噪声坐标会先转换为方块坐标再批量采样。
     */
    void getNoiseBiomesBatch(i32 startNoiseX, i32 startNoiseY, i32 startNoiseZ,
                              i32 width, i32 height, BiomeId* output) const override;

private:
    std::unique_ptr<LayerStack> m_layerStack;
};

} // namespace mc
