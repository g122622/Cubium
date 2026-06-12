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

namespace mc {
namespace world {
namespace biome {

Biome::Biome(BiomeId id, std::string_view name) noexcept
    : m_id(id)
    , m_name(name)
{}

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
