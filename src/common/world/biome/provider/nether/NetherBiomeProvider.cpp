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

#include "NetherBiomeProvider.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../BiomeRegistry.hpp"
#include "../../layer/BiomeValues.hpp"
#include <cmath>

namespace mc {
namespace biome {
namespace nether {

// ============================================================================
// 下界生物群系 ID 常量
// ============================================================================

// MC 1.16.5 下界生物群系 ID
// 参考 BiomeValues.hpp
namespace NetherBiomes {
constexpr BiomeId NetherWastes = 8;     // 下界荒地 (默认)
constexpr BiomeId SoulSandValley = 170; // 灵魂沙谷
constexpr BiomeId CrimsonForest = 171;  // 绯红森林
constexpr BiomeId WarpedForest = 172;   // 诡异森林
constexpr BiomeId BasaltDeltas = 173;   // 玄武岩三角洲
} // namespace NetherBiomes

// ============================================================================
// 构造函数
// ============================================================================

NetherBiomeProvider::NetherBiomeProvider(u64 seed)
    : BiomeProvider(seed)
{
    math::Random rng(seed);

    // 初始化噪声生成器
    // 参考 MC 1.16.5 NetherBiomeProvider 构造函数
    m_temperatureNoise = std::make_unique<SimplexNoiseGenerator>(rng);
    m_humidityNoise = std::make_unique<SimplexNoiseGenerator>(rng);
    m_biomeNoise = std::make_unique<PerlinNoiseGenerator>(rng, 0, 0);
}

// ============================================================================
// 生物群系获取
// ============================================================================

BiomeId NetherBiomeProvider::getBiome(i32 x, i32 y, i32 z) const
{
    // 下界使用 3D 生物群系采样
    // 参考 MC 1.16.5 NetherBiomeProvider.getNoiseBiomeAt
    return getNoiseBiome(x >> 2, y >> 2, z >> 2);
}

BiomeId NetherBiomeProvider::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    // 采样噪声值
    const f32 temperature = getTemperature(noiseX, noiseY, noiseZ);
    const f32 humidity = getHumidity(noiseX, noiseY, noiseZ);
    const f32 biomeNoise = getBiomeNoise(noiseX, noiseY, noiseZ);

    return selectBiome(temperature, humidity, biomeNoise);
}

f32 NetherBiomeProvider::getDepth(i32 x, i32 z) const
{
    // 下界地形深度（平坦地形）
    MC_UNUSED(x);
    MC_UNUSED(z);
    return 0.0f;
}

f32 NetherBiomeProvider::getScale(i32 x, i32 z) const
{
    // 下界地形比例（平坦地形）
    MC_UNUSED(x);
    MC_UNUSED(z);
    return 0.0f;
}

// ============================================================================
// 噪声采样
// ============================================================================

f32 NetherBiomeProvider::getTemperature(i32 x, i32 y, i32 z) const
{
    // 温度噪声（Simplex 3D）
    // 参考 MC NetherBiomeProvider.temperature
    const f32 nx = static_cast<f32>(x) * TEMPERATURE_SCALE;
    const f32 ny = static_cast<f32>(y) * TEMPERATURE_SCALE;
    const f32 nz = static_cast<f32>(z) * TEMPERATURE_SCALE;
    return m_temperatureNoise->noise(nx, ny, nz);
}

f32 NetherBiomeProvider::getHumidity(i32 x, i32 y, i32 z) const
{
    // 湿度噪声（Simplex 3D）
    // 参考 MC NetherBiomeProvider.humidity
    const f32 nx = static_cast<f32>(x) * HUMIDITY_SCALE;
    const f32 ny = static_cast<f32>(y) * HUMIDITY_SCALE;
    const f32 nz = static_cast<f32>(z) * HUMIDITY_SCALE;
    return m_humidityNoise->noise(nx, ny, nz);
}

f32 NetherBiomeProvider::getBiomeNoise(i32 x, i32 y, i32 z) const
{
    // 生物群系选择噪声（Perlin 3D）
    // 参考 MC NetherBiomeProvider.biomeNoise
    const f32 nx = static_cast<f32>(x) * BIOME_SCALE;
    const f32 ny = static_cast<f32>(y) * BIOME_SCALE;
    const f32 nz = static_cast<f32>(z) * BIOME_SCALE;
    return m_biomeNoise->noise(nx, ny, nz);
}

// ============================================================================
// 生物群系选择
// ============================================================================

BiomeId NetherBiomeProvider::selectBiome(f32 temperature, f32 humidity, f32 biomeNoise) const
{
    // 参考 MC 1.16.5 NetherBiomeProvider.getBiomeFromNoise
    //
    // 下界生物群系选择逻辑：
    // 1. 首先检查是否为玄武岩三角洲（高 biomeNoise）
    // 2. 然后检查灵魂沙谷（低温度、低湿度）
    // 3. 然后检查绯红森林（低温度、高湿度）
    // 4. 然后检查诡异森林（高温度、低湿度）
    // 5. 默认为下界荒地

    // 玄武岩三角洲：biomeNoise > 0.5
    // 玄武岩三角洲特征：大量玄武岩和岩浆块，黑色颗粒效果
    if (biomeNoise > 0.5f) {
        return NetherBiomes::BasaltDeltas;
    }

    // 灵魂沙谷：温度 < -0.5，湿度 < 0
    // 灵魂沙谷特征：大量灵魂沙和灵魂土，蓝色迷雾，骷髅
    if (temperature < -0.5f && humidity < 0.0f) {
        return NetherBiomes::SoulSandValley;
    }

    // 绯红森林：温度 < 0，湿度 > 0
    // 绯红森林特征：绯红菌和疣猪兽，红色主题
    if (temperature < 0.0f && humidity > 0.0f) {
        return NetherBiomes::CrimsonForest;
    }

    // 诡异森林：温度 > 0.5，湿度 > 0
    // 诡异森林特征：诡异菌和末影人，青色主题
    if (temperature > 0.5f && humidity > 0.0f) {
        return NetherBiomes::WarpedForest;
    }

    // 默认：下界荒地
    // 下界荒地特征：下界岩为主，猪灵、恶魂等
    return NetherBiomes::NetherWastes;
}

// ============================================================================
// 生物群系容器填充
// ============================================================================

void NetherBiomeProvider::fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ)
{
    // 噪声坐标中：一个区块对应 4x4 个水平采样点
    const i32 startNoiseX = chunkX << 2;
    const i32 startNoiseZ = chunkZ << 2;

    for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
        for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
            const i32 noiseX = startNoiseX + bx;
            const i32 noiseZ = startNoiseZ + bz;

            for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
                // BiomeContainer 的 Y 槽位按 16 方块分段，这里采样分段中心点。
                const i32 sampleBlockY = (by << 4) + 8;
                const i32 noiseY = sampleBlockY >> 2;
                container.setBiome(bx, by, bz, getNoiseBiome(noiseX, noiseY, noiseZ));
            }
        }
    }
}

} // namespace nether
} // namespace biome
} // namespace mc
