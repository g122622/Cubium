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

#include "DiskFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <memory>
#include <utility>

namespace mc::world::gen::feature {

ConfiguredDiskFeature::ConfiguredDiskFeature(std::unique_ptr<DiskConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredDiskFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin) const
{
    if (!m_config || m_config->stateProvider == nullptr || m_config->target == nullptr || m_config->radius == nullptr) {
        return false;
    }

    const i32 r = m_config->radius->sample(random);
    const i32 topY = origin.y + m_config->halfHeight;
    const i32 bottomY = origin.y - m_config->halfHeight - 1;
    const auto& provider = *m_config->stateProvider;
    const auto& target = *m_config->target;

    bool any = false;
    // 遍历 origin 周围 [-r,0,-r]..[r,0,r] 的 XZ 圆盘。
    for (i32 dx = -r; dx <= r; ++dx) {
        for (i32 dz = -r; dz <= r; ++dz) {
            if (dx * dx + dz * dz > r * r) {
                continue;
            }
            // 从 topY 向下到 bottomY+1（i > bottomY），逐格测试 target。
            for (i32 y = topY; y > bottomY; --y) {
                const BlockPos pos(origin.x + dx, y, origin.z + dz);
                if (target.test(region, pos)) {
                    const BlockState* state = provider.getState(region, random, pos.x, pos.y, pos.z);
                    if (state != nullptr) {
                        region.setBlockState(pos, state, 2);
                        any = true;
                    }
                }
            }
        }
    }

    return any;
}

} // namespace mc::world::gen::feature
