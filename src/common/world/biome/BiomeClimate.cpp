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

#include "BiomeClimate.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/world/gen/noise/PerlinSimplexNoise.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace biome {

const world::gen::noise::PerlinSimplexNoise& temperatureNoise()
{
    // MC 1.21.11: Biome.java — TEMPERATURE_NOISE
    // WorldgenRandom(LegacyRandomSource(seed)) 等价于 JavaLegacyRandom(seed)
    static const auto instance = [] {
        math::JavaLegacyRandom rng(1234ULL);
        return std::make_unique<world::gen::noise::PerlinSimplexNoise>(rng, std::vector<i32>{0});
    }();
    return *instance;
}

const world::gen::noise::PerlinSimplexNoise& frozenTemperatureNoise()
{
    // MC 1.21.11: Biome.java — FROZEN_TEMPERATURE_NOISE
    static const auto instance = [] {
        math::JavaLegacyRandom rng(3456ULL);
        return std::make_unique<world::gen::noise::PerlinSimplexNoise>(rng, std::vector<i32>{-2, -1, 0});
    }();
    return *instance;
}

const world::gen::noise::PerlinSimplexNoise& biomeInfoNoise()
{
    // MC 1.21.11: Biome.java — BIOME_INFO_NOISE
    static const auto instance = [] {
        math::JavaLegacyRandom rng(2345ULL);
        return std::make_unique<world::gen::noise::PerlinSimplexNoise>(rng, std::vector<i32>{0});
    }();
    return *instance;
}

f32 applyTemperatureModifier(i32 x, i32 z, f32 baseTemperature, BiomeClimate::TemperatureModifier modifier)
{
    if (modifier != BiomeClimate::TemperatureModifier::Frozen) {
        return baseTemperature;
    }

    const f64 d0 =
        frozenTemperatureNoise().getValue(static_cast<f64>(x) * 0.05, static_cast<f64>(z) * 0.05, false) * 7.0;
    const f64 d1 = biomeInfoNoise().getValue(static_cast<f64>(x) * 0.2, static_cast<f64>(z) * 0.2, false);
    const f64 d2 = d0 + d1;

    if (d2 < 0.3) {
        const f64 d3 = biomeInfoNoise().getValue(static_cast<f64>(x) * 0.09, static_cast<f64>(z) * 0.09, false);
        if (d3 < 0.8) {
            return 0.2f;
        }
    }

    return baseTemperature;
}

} // namespace biome
} // namespace world
} // namespace mc
