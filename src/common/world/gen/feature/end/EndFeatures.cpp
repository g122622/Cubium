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

#include "EndFeatures.hpp"

namespace mc {

void EndFeatureRegistry::initialize()
{
    EndSpikeFeatures::initialize();
    EndGatewayFeatures::initialize();
    IceSpikeFeatures::initialize();
}

std::vector<std::unique_ptr<ConfiguredFeatureBase>> EndFeatureRegistry::getAllSurfaceFeaturesAndClear()
{
    std::vector<std::unique_ptr<ConfiguredFeatureBase>> features;

    auto spikeFeatures = EndSpikeFeatures::getAllFeaturesAndClear();
    for (auto& f : spikeFeatures) {
        features.push_back(std::move(f));
    }

    auto gatewayFeatures = EndGatewayFeatures::getAllFeaturesAndClear();
    for (auto& f : gatewayFeatures) {
        features.push_back(std::move(f));
    }

    return features;
}

std::vector<std::unique_ptr<ConfiguredFeatureBase>> EndFeatureRegistry::getAllVegetationFeaturesAndClear()
{
    std::vector<std::unique_ptr<ConfiguredFeatureBase>> features;

    auto iceSpikeFeatures = IceSpikeFeatures::getAllFeaturesAndClear();
    for (auto& f : iceSpikeFeatures) {
        features.push_back(std::move(f));
    }

    return features;
}

} // namespace mc
