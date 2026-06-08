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

#include "Dimension.hpp"
#include "../../core/Constants.hpp"
#include "../biome/source/EndBiomeSource.hpp"
#include "../biome/source/MultiNoiseBiomeSource.hpp"
#include "../gen/chunk/NoiseChunkGenerator.hpp"
#include "../gen/settings/DimensionSettings.hpp"
#include "DimensionManager.hpp"

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

Dimension::Dimension(DimensionId id, DimensionType type, std::unique_ptr<IChunkGenerator> generator)
    : m_id(id)
    , m_type(std::move(type))
    , m_generator(std::move(generator))
{}

// ============================================================================
// 更新
// ============================================================================

void Dimension::tick()
{
    // 基类默认无操作
    // 子类可覆盖以实现维度特定逻辑（如末地的末影龙战斗）
}

// ============================================================================
// 工厂方法
// ============================================================================

std::unique_ptr<Dimension> Dimension::createOverworld(u64 seed)
{
    DimensionType dimType = DimensionType::overworld();

    // 创建主世界生物群系源（3D 多噪声）
    auto settings = DimensionSettings::overworld();
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    auto generator = std::make_unique<NoiseChunkGenerator>(seed, std::move(settings), std::move(biomeSource));

    auto dimension = std::make_unique<Dimension>(0, // DimensionId::Overworld
        std::move(dimType),
        std::move(generator));

    // 主世界默认出生点（海平面高度+1）
    dimension->m_spawnPoint = Vector3d(0.0, static_cast<f64>(world::SEA_LEVEL) + 1.0, 0.0);

    return dimension;
}

std::unique_ptr<Dimension> Dimension::createNether(u64 seed)
{
    DimensionType dimType = DimensionType::nether();

    // 下界使用 NoiseChunkGenerator + MultiNoiseBiomeSource
    auto settings = DimensionSettings::nether();
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(seed);
    auto generator = std::make_unique<NoiseChunkGenerator>(seed, std::move(settings), std::move(biomeSource));

    auto dimension = std::make_unique<Dimension>(DimensionManager::NETHER, std::move(dimType), std::move(generator));

    // 下界默认出生点（海平面高度+1）
    dimension->m_spawnPoint = Vector3d(0.0, static_cast<f64>(world::SEA_LEVEL) + 1.0, 0.0);

    return dimension;
}

std::unique_ptr<Dimension> Dimension::createTheEnd(u64 seed)
{
    DimensionType dimType = DimensionType::theEnd();

    // 末地使用 NoiseChunkGenerator + EndBiomeSource
    auto settings = DimensionSettings::end();
    auto biomeSource = std::make_unique<world::biome::source::EndBiomeSource>(seed);
    auto generator = std::make_unique<NoiseChunkGenerator>(seed, std::move(settings), std::move(biomeSource));

    auto dimension = std::make_unique<Dimension>(DimensionManager::THE_END, std::move(dimType), std::move(generator));

    // 末地默认出生点（末地传送门平台固定位置：x=100, y=49, z=0）
    dimension->m_spawnPoint = Vector3d(100.0, 49.0, 0.0);

    return dimension;
}

} // namespace mc
