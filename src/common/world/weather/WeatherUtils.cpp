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

#include "WeatherUtils.hpp"

#include "common/core/Types.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/weather/WeatherConstants.hpp"

namespace mc::weather {

namespace {

[[nodiscard]] const mc::Biome* getBiomeAt(const mc::IWorld& world, const mc::BlockPos& pos)
{
    const mc::world::chunk::ChunkData* chunk = world.getChunk(pos.chunkX(), pos.chunkZ());
    if (chunk == nullptr) {
        return nullptr;
    }

    const mc::BiomeId biomeId = chunk->getBiomeAtBlock(pos.localX(), pos.y, pos.localZ());
    return &mc::BiomeRegistry::instance().get(biomeId);
}

} // namespace

bool WeatherUtils::canSeeSky(const mc::IWorld& world, const mc::BlockPos& pos)
{
    // 直接使用 IWorld::canSeeSky，它基于天空光照等级判断
    return world.canSeeSky(pos);
}

bool WeatherUtils::canRainAt(const mc::IWorld& world, const mc::BlockPos& pos)
{
    if (world.isUltraWarm() || !canSeeSky(world, pos)) {
        return false;
    }

    // MC 1.21.11 Level.precipitationAt: 若该位置上方存在运动阻挡方块（MOTION_BLOCKING
    // 高度图 Y > pos.y），则降水被遮挡。getHeight 返回最高阻挡方块上方的空气层 Y，
    // 与 getHeightmapPos(MOTION_BLOCKING).getY() 语义一致。
    if (world.getHeight(pos.x, pos.z) > pos.y) {
        return false;
    }

    const mc::Biome* biome = getBiomeAt(world, pos);
    if (biome == nullptr) {
        return false;
    }

    if (!biome->hasPrecipitation()) {
        return false;
    }

    // MC 1.21.11: 使用高度调整温度，高海拔位置可能降温至降雪阈值
    // 注意：不使用缓存的 getTemperature()，因为天气检查可能在同一位置
    // 遇到不同生物群系（如测试场景），直接计算更安全
    const f32 adjustedTemp = biome->getHeightAdjustedTemperature(pos.x, pos.y, pos.z, mc::world::SEA_LEVEL);
    return getPrecipitationType(adjustedTemp) == 1;
}

bool WeatherUtils::canSnowAt(const mc::IWorld& world, const mc::BlockPos& pos)
{
    if (world.isUltraWarm() || !canSeeSky(world, pos)) {
        return false;
    }

    // MC 1.21.11 Level.precipitationAt: 若该位置上方存在运动阻挡方块（MOTION_BLOCKING
    // 高度图 Y > pos.y），则降水被遮挡。getHeight 返回最高阻挡方块上方的空气层 Y，
    // 与 getHeightmapPos(MOTION_BLOCKING).getY() 语义一致。
    if (world.getHeight(pos.x, pos.z) > pos.y) {
        return false;
    }

    const mc::Biome* biome = getBiomeAt(world, pos);
    if (biome == nullptr) {
        return false;
    }

    if (!biome->hasPrecipitation()) {
        return false;
    }

    // MC 1.21.11: 使用高度调整温度，高海拔位置可能降温至降雪阈值
    // 注意：不使用缓存的 getTemperature()，因为天气检查可能在同一位置
    // 遇到不同生物群系（如测试场景），直接计算更安全
    const f32 adjustedTemp = biome->getHeightAdjustedTemperature(pos.x, pos.y, pos.z, mc::world::SEA_LEVEL);
    return getPrecipitationType(adjustedTemp) == 2;
}

i32 WeatherUtils::getRandomWeatherDuration(mc::math::IRandom& rng, i32 minTime, i32 maxTime)
{
    if (minTime >= maxTime) {
        return minTime;
    }
    i32 range = maxTime - minTime;
    return minTime + rng.nextInt(range);
}

i32 WeatherUtils::getRandomClearDuration(mc::math::IRandom& rng)
{
    return getRandomWeatherDuration(rng, WeatherConstants::MIN_CLEAR_TIME, WeatherConstants::MAX_CLEAR_TIME);
}

i32 WeatherUtils::getRandomRainDuration(mc::math::IRandom& rng)
{
    return getRandomWeatherDuration(rng, WeatherConstants::MIN_RAIN_TIME, WeatherConstants::MAX_RAIN_TIME);
}

i32 WeatherUtils::getRandomThunderDuration(mc::math::IRandom& rng)
{
    return getRandomWeatherDuration(rng, WeatherConstants::MIN_THUNDER_TIME, WeatherConstants::MAX_THUNDER_TIME);
}

f32 WeatherUtils::calculateStarBrightness(f32 rainStrength, i64 dayTime)
{
    // 星星只在夜晚可见
    // 夜晚时间: 约 12542 - 23459 (日落到日出)
    // 注意：dayTime 参数应该是 dayTimeOfDay() 的结果 (0-23999)
    if (dayTime < 12542 || dayTime > 23459) {
        return 0.0f;
    }

    // 计算夜晚深度 (0.0 - 1.0)
    // 最暗时约在 18000 (午夜)
    f32 nightDepth = 0.0f;
    if (dayTime < 18000) {
        // 日落到午夜 (12542 -> 18000)
        nightDepth = static_cast<f32>(dayTime - 12542) / (18000 - 12542);
    } else {
        // 午夜到日出 (18000 -> 23459)
        nightDepth = static_cast<f32>(23459 - dayTime) / (23459 - 18000);
    }

    // 降雨时星星不可见
    return nightDepth * (1.0f - rainStrength);
}

} // namespace mc::weather
