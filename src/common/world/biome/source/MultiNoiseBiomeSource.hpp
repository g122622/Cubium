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
 */

#pragma once

#include "common/world/biome/BiomeSource.hpp"
#include "common/world/biome/climate/ParameterList.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include "common/world/biome/climate/Sampler.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include <memory>

namespace mc {
namespace world {
namespace gen {
class RandomState;
} // namespace gen
} // namespace world
} // namespace mc

namespace mc {
namespace world {
namespace biome {
namespace source {

/**
 * @brief 基于 Climate 参数的多噪声生物群系源
 *
 * 使用 6 个气候参数（temperature, humidity, continentalness, erosion, depth, weirdness）
 * 在三维空间中采样，通过最近邻匹配确定生物群系。
 * 支持主世界和下界。
 *
 * 与 NoiseChunkGenerator 共享同一个 RandomState，气候密度函数的 NormalNoise
 * 叶子跨区块复用，避免重复重建 PerlinNoise 倍频置换表。
 *
 * 注意：此类拥有 NoiseRouter，Climate::Sampler 引用的 DensityFunction
 * 由 router 管理，确保生命周期正确。
 */
class MultiNoiseBiomeSource : public IBiomeSource {
public:
    /**
     * @brief 构造多噪声生物群系源
     * @param rs 世界随机状态（提供噪声缓存，与生成器共享）
     * @param parameters 气候参数到生物群系的映射
     * @param router 噪声路由器（拥有权转移，内部 NormalNoise 叶子共享 rs 缓存）
     */
    MultiNoiseBiomeSource(const gen::RandomState& rs,
        climate::ParameterList<BiomeId> parameters,
        std::unique_ptr<gen::density::NoiseRouter> router);

    [[nodiscard]] BiomeId getNoiseBiome(i32 quartX, i32 quartY, i32 quartZ) const override;
    [[nodiscard]] const std::vector<BiomeId>& possibleBiomes() const override;

    /**
     * @brief 获取气候参数列表
     */
    [[nodiscard]] const climate::ParameterList<BiomeId>& parameters() const { return m_parameters; }

    /**
     * @brief 获取气候采样器
     */
    [[nodiscard]] const climate::Sampler& sampler() const { return m_sampler; }

    /**
     * @brief 获取噪声路由器
     */
    [[nodiscard]] const gen::density::NoiseRouter& router() const { return *m_router; }

    /**
     * @brief 通过 TargetPoint 直接查找生物群系
     * @param target 采样得到的气候目标点
     * @return 最匹配的生物群系 ID
     */
    [[nodiscard]] BiomeId getNoiseBiome(const climate::TargetPoint& target) const;

    /**
     * @brief 创建主世界生物群系源
     * @param rs 世界随机状态（与生成器共享同一缓存）
     * @param largeBiomes 是否使用大型生物群系
     * @param amplified 是否使用放大化预设
     * @return 配置好的 MultiNoiseBiomeSource
     */
    [[nodiscard]] static std::unique_ptr<MultiNoiseBiomeSource> createOverworld(
        const gen::RandomState& rs, bool largeBiomes, bool amplified);

    /**
     * @brief 创建下界生物群系源
     * @param rs 世界随机状态（与生成器共享同一缓存）
     * @return 配置好的 MultiNoiseBiomeSource
     */
    [[nodiscard]] static std::unique_ptr<MultiNoiseBiomeSource> createNether(const gen::RandomState& rs);

private:
    climate::ParameterList<BiomeId> m_parameters;
    std::unique_ptr<gen::density::NoiseRouter> m_router;
    climate::Sampler m_sampler;
};

} // namespace source
} // namespace biome
} // namespace world
} // namespace mc
