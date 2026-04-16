#include "WeatherUtils.hpp"
#include "../../world/IWorld.hpp"
#include "../../world/block/BlockPos.hpp"
#include "../../world/chunk/ChunkData.hpp"
#include "../../world/biome/Biome.hpp"
#include "../../world/biome/BiomeRegistry.hpp"
#include "../../util/math/random/Random.hpp"
#include <cmath>

namespace mc::weather {

namespace {

[[nodiscard]] const mc::Biome* getBiomeAt(const mc::IWorld& world, const mc::BlockPos& pos)
{
    const mc::ChunkData* chunk = world.getChunk(pos.chunkX(), pos.chunkZ());
    if (chunk == nullptr) {
        return nullptr;
    }

    const mc::BiomeId biomeId = chunk->getBiomeAtBlock(pos.localX(), pos.y, pos.localZ());
    return &mc::BiomeRegistry::instance().get(biomeId);
}

} // namespace

bool WeatherUtils::canSeeSky(const mc::IWorld& world, const mc::BlockPos& pos) {
    // 检查该位置上方是否有非透明方块
    // 简化实现：检查高度是否为最高点
    i32 height = world.getHeight(pos.x, pos.z);
    return pos.y >= height;
}

bool WeatherUtils::canRainAt(const mc::IWorld& world, const mc::BlockPos& pos) {
    if (world.isUltraWarm() || !canSeeSky(world, pos)) {
        return false;
    }

    const mc::Biome* biome = getBiomeAt(world, pos);
    if (biome == nullptr) {
        return false;
    }

    if (biome->climate().precipitation == mc::BiomeClimate::Precipitation::None) {
        return false;
    }

    return getPrecipitationType(biome->temperature()) == 1;
}

bool WeatherUtils::canSnowAt(const mc::IWorld& world, const mc::BlockPos& pos) {
    if (world.isUltraWarm() || !canSeeSky(world, pos)) {
        return false;
    }

    const mc::Biome* biome = getBiomeAt(world, pos);
    if (biome == nullptr) {
        return false;
    }

    if (biome->climate().precipitation == mc::BiomeClimate::Precipitation::None) {
        return false;
    }

    return getPrecipitationType(biome->temperature()) == 2;
}

i32 WeatherUtils::getRandomWeatherDuration(mc::math::IRandom& rng, i32 minTime, i32 maxTime) {
    if (minTime >= maxTime) {
        return minTime;
    }
    i32 range = maxTime - minTime;
    return minTime + rng.nextInt(range);
}

i32 WeatherUtils::getRandomClearDuration(mc::math::IRandom& rng) {
    return getRandomWeatherDuration(rng,
        WeatherConstants::MIN_CLEAR_TIME,
        WeatherConstants::MAX_CLEAR_TIME);
}

i32 WeatherUtils::getRandomRainDuration(mc::math::IRandom& rng) {
    return getRandomWeatherDuration(rng,
        WeatherConstants::MIN_RAIN_TIME,
        WeatherConstants::MAX_RAIN_TIME);
}

i32 WeatherUtils::getRandomThunderDuration(mc::math::IRandom& rng) {
    return getRandomWeatherDuration(rng,
        WeatherConstants::MIN_THUNDER_TIME,
        WeatherConstants::MAX_THUNDER_TIME);
}

f32 WeatherUtils::calculateStarBrightness(f32 rainStrength, i64 dayTime) {
    // 星星只在夜晚可见
    // 夜晚时间: 约 12542 - 23459 (日落到日出)
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
