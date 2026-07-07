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

#include "BiomeFilterPlacement.hpp"
#include "common/world/biome/BiomeGenerationSettings.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {

std::vector<BlockPos> BiomeFilterPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)random;

    const auto& filterConfig = static_cast<const BiomeFilterConfig&>(config);
    const ResourceLocation& placedFeatureId = filterConfig.placedFeatureId;

    // 获取当前位置的生物群系ID
    const BiomeId biomeId = region.getBiome(basePos.x, basePos.y, basePos.z);

    // 从注册表获取生物群系定义
    const Biome& biome = BiomeRegistry::instance().get(biomeId);

    // 检查该生物群系的生成设置中是否包含当前 placed_feature
    if (biome.generationSettings().hasPlacedFeature(placedFeatureId)) {
        return {basePos};
    }

    return {};
}

} // namespace mc
