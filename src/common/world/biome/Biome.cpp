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
#include "common/core/Types.hpp"
#include "common/util/cache/Long2FloatLRUCache.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/ice/SnowBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/gen/noise/PerlinSimplexNoise.hpp"
#include <cmath>
#include <string_view>

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

bool Biome::shouldFreeze(const IWorld& world, i32 x, i32 y, i32 z, i32 seaLevel, bool checkNeighbors) const
{
    // 温度检查：如果足够温暖可以下雨，则不冻结
    if (warmEnoughToRain(x, y, z, seaLevel)) {
        return false;
    }

    // 高度检查：位置必须在建造高度范围内
    if (!world::isValidY(y)) {
        return false;
    }

    // 光照检查：方块光照必须 < 10（MC 使用 LightLayer.BLOCK）
    if (world.getBlockLight(x, y, z) >= 10) {
        return false;
    }

    // 方块状态和流体状态检查
    // 注意：getBlockState 可能返回 nullptr（表示区块未加载或空段，等同于空气）
    // 只有液体方块才可能冻结，空气/固体方块不会冻结
    const BlockState* blockState = world.getBlockState(x, y, z);
    if (blockState == nullptr || !blockState->isLiquid()) {
        return false;
    }

    const fluid::FluidState* fluidState = world.getFluidState(x, y, z);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 流体必须是水（排除岩浆等其他液体）
    if (!fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
        return false;
    }

    // 邻居水域暴露检查：如果四个水平邻居全是水，则不冻结
    // 这防止了深海中心大面积结冰，只在水的边缘冻结
    if (checkNeighbors) {
        if (world.isWaterAt(x - 1, y, z) && world.isWaterAt(x + 1, y, z) && world.isWaterAt(x, y, z - 1) &&
            world.isWaterAt(x, y, z + 1)) {
            return false;
        }
    }

    return true;
}

bool Biome::shouldSnow(const IWorld& world, i32 x, i32 y, i32 z, i32 seaLevel) const
{
    // 降水类型检查：生物群系必须支持降水
    if (!m_climate.hasPrecipitation) {
        return false;
    }
    if (!coldEnoughToSnow(x, y, z, seaLevel)) {
        return false;
    }

    // 高度检查：位置必须在建造高度范围内
    if (!world::isValidY(y)) {
        return false;
    }

    // 光照检查：方块光照必须 < 10
    if (world.getBlockLight(x, y, z) >= 10) {
        return false;
    }

    // 方块检查：该位置的方块必须是空气或已有雪层
    const BlockState* blockState = world.getBlockState(x, y, z);
    if (blockState != nullptr && !blockState->isAir() && !blockState->is(VanillaBlocks::SNOW)) {
        return false;
    }

    // 雪层方块必须能在此处存活
    // 使用 SnowBlock::canSurviveAt 检查下方方块是否支持雪层放置：
    //   - 下方不能是冰/浮冰/屏障（SNOW_LAYER_CANNOT_SURVIVE_ON）
    //   - 下方是蜂蜜块/灵魂沙/泥巴时允许（SNOW_LAYER_CAN_SURVIVE_ON）
    //   - 否则下方碰撞形状上表面必须完整，或下方为满层(8层)雪层
    BlockPos pos(x, y, z);
    if (!blocks::SnowBlock::canSurviveAt(world, pos)) {
        return false;
    }

    return true;
}

} // namespace biome
} // namespace world
} // namespace mc
