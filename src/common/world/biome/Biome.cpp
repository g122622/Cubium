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

#include "Biome.hpp"
#include "BiomeClimate.hpp"
#include "common/world/gen/noise/PerlinSimplexNoise.hpp"
#include <cmath>

namespace mc {
namespace world {
namespace biome {

// ============================================================================
// 构造函数
// ============================================================================

Biome::Biome(BiomeId id, std::string_view name) noexcept
    : m_id(id)
    , m_name(name)
{}

// ============================================================================
// 温度缓存
// ============================================================================

Long2FloatLRUCache& Biome::getTemperatureCache()
{
    // MC 1.21.11: ThreadLocal<Long2FloatLinkedOpenHashMap> 容量 1024
    // 每个 Biome 实例有自己的缓存，但由于 Biome 是只读的（温度不变），
    // 使用线程局部静态缓存即可，key 已经包含位置信息。
    thread_local Long2FloatLRUCache cache(TEMPERATURE_CACHE_SIZE);
    return cache;
}

void Biome::clearTemperatureCache()
{
    getTemperatureCache().clear();
}

f32 Biome::getTemperature(i32 x, i32 y, i32 z, i32 seaLevel) const
{
    // MC 1.21.11: Biome.getTemperature(BlockPos, int seaLevel)
    // 使用 Long2FloatLinkedOpenHashMap 缓存，key = BlockPos.asLong()
    const i64 key = Long2FloatLRUCache::packBlockPos(x, y, z);
    auto& cache = getTemperatureCache();
    const f32 cached = cache.get(key);
    if (!std::isnan(cached)) {
        return cached;
    }

    const f32 temp = getHeightAdjustedTemperature(x, y, z, seaLevel);
    cache.put(key, temp);
    return temp;
}

f32 Biome::getHeightAdjustedTemperature(i32 x, i32 y, i32 z, i32 seaLevel) const
{
    f32 temp = applyTemperatureModifier(x, z, m_climate.temperature, m_climate.temperatureModifier);

    const i32 threshold = seaLevel + 17;
    if (y > threshold) {
        const f64 noiseValue = temperatureNoise().getValue(static_cast<f64>(x) / 8.0, static_cast<f64>(z) / 8.0, false);
        temp -= static_cast<f32>((noiseValue * 8.0 + static_cast<f64>(y - threshold)) * 0.05 / 40.0);
    }

    return temp;
}

f32 Biome::getBaseTemperature() const
{
    return applyTemperatureModifier(0, 0, m_climate.temperature, m_climate.temperatureModifier);
}

} // namespace biome
} // namespace world
} // namespace mc
