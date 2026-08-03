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

#include "common/core/Types.hpp"
#include "common/world/biome/BiomeSource.hpp"
#include "common/world/biome/climate/ParameterList.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include "common/world/biome/climate/Sampler.hpp"
#include <memory>
#include <vector>

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
 * 在三维空间中采样，通过最近邻匹配确定生物群系。支持主世界和下界。
 *
 * 对齐 MC 1.21.11 MultiNoiseBiomeSource：本类无状态（不持有 NoiseRouter），气候采样直接
 * 复用 RandomState.sampler()（由 NoiseChunkGenerator 与本类共享的 RandomState 持有）。
 * 6 个气候密度函数由 RandomState 的 NoiseRouter 管理，Sampler 仅引用其 DensityFunction 指针。
 *
 * 生命周期约束：m_sampler 引用的 RandomState 必须在本类之前销毁。生产中 RandomState 由
 * NoiseChunkGenerator 持有，与本类同属一个 dimension，RandomState 不会先于 biome source 销毁。
 */
class MultiNoiseBiomeSource : public IBiomeSource {
public:
    /**
     * @brief 构造多噪声生物群系源
     * @param worldSeed 世界种子（IBiomeSource 基类用于 findBiome 等随机搜索）
     * @param parameters 气候参数到生物群系的映射
     * @param sampler 气候采样器（引用 RandomState.sampler()，生命周期由 RandomState 保证）
     */
    MultiNoiseBiomeSource(u64 worldSeed, climate::ParameterList<BiomeId> parameters, const climate::Sampler& sampler);

    [[nodiscard]] BiomeId getNoiseBiome(i32 quartX, i32 quartY, i32 quartZ) const override;
    [[nodiscard]] const std::vector<BiomeId>& possibleBiomes() const override;

    /**
     * @brief 获取气候参数列表
     */
    [[nodiscard]] const climate::ParameterList<BiomeId>& parameters() const { return m_parameters; }

    /**
     * @brief 获取气候采样器（引用 RandomState.sampler()）
     */
    [[nodiscard]] const climate::Sampler& sampler() const { return m_sampler; }

    /**
     * @brief 通过 TargetPoint 直接查找生物群系
     * @param target 采样得到的气候目标点
     * @return 最匹配的生物群系 ID
     */
    [[nodiscard]] BiomeId getNoiseBiome(const climate::TargetPoint& target) const;

    /**
     * @brief 创建主世界生物群系源
     * @param rs 世界随机状态（提供气候采样器，与生成器共享同一 RandomState）
     * @param largeBiomes 是否使用大型生物群系（保留参数，对齐原版签名；气候采样由 rs.sampler() 统一）
     * @param amplified 是否使用放大化预设（同上）
     * @return 配置好的 MultiNoiseBiomeSource
     */
    [[nodiscard]] static std::unique_ptr<MultiNoiseBiomeSource> createOverworld(
        const gen::RandomState& rs, bool largeBiomes, bool amplified);

    /**
     * @brief 创建下界生物群系源
     * @param rs 世界随机状态（提供气候采样器，与生成器共享同一 RandomState）
     * @return 配置好的 MultiNoiseBiomeSource
     */
    [[nodiscard]] static std::unique_ptr<MultiNoiseBiomeSource> createNether(const gen::RandomState& rs);

private:
    climate::ParameterList<BiomeId> m_parameters;
    const climate::Sampler& m_sampler;
};

} // namespace source
} // namespace biome
} // namespace world
} // namespace mc
