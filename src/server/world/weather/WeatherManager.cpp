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

#include "WeatherManager.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"
#include "common/world/weather/WeatherConstants.hpp"
#include "common/world/weather/WeatherUtils.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace mc::trace;

namespace mc::server {

WeatherManager::WeatherManager()
    : m_random(std::make_unique<mc::math::Random>(0))
{}

WeatherManager::~WeatherManager() = default;

void WeatherManager::initialize(u64 seed, bool canHaveWeather)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "WeatherManager::initialize");

    m_random = std::make_unique<mc::math::Random>(seed);
    m_canHaveWeather = canHaveWeather;

    // 原版新世界的 level.dat 天气字段全为 0（晴天、计时器为 0），
    // 首个 tick 才按 RAIN_DELAY / THUNDER_DELAY 采样第一次天气变化的等待时间。
    m_state.clearWeatherTime = 0;
    m_state.rainTime = 0;
    m_state.thunderTime = 0;
    m_state.raining = false;
    m_state.thundering = false;
    m_state.rainStrength = 0.0f;
    m_state.thunderStrength = 0.0f;
    m_state.prevRainStrength = 0.0f;
    m_state.prevThunderStrength = 0.0f;
    m_state.weatherCycleEnabled = true;

    m_weatherChanged = false;
    m_strengthChanged = false;
    m_lastRaining = false;
}

void WeatherManager::tick()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "WeatherManager::tick");

    // 重置变化标志
    m_weatherChanged = false;
    m_strengthChanged = false;

    // 保存上一帧强度
    m_state.prevRainStrength = m_state.rainStrength;
    m_state.prevThunderStrength = m_state.thunderStrength;

    // 原版 advanceWeatherCycle：周期推进与强度渐变都只在有天气能力的维度执行。
    if (m_canHaveWeather) {
        if (m_state.weatherCycleEnabled) {
            _tickWeatherCycle();
        }
        _updateStrength();
    }

    // 检查天气变化
    _checkWeatherChange();
}

void WeatherManager::_tickWeatherCycle()
{
    // 处理强制晴天计时器
    if (m_state.clearWeatherTime > 0) {
        --m_state.clearWeatherTime;
        // 强制晴天时把仍在持续的计时器压到 0/1，使下一次 tick 立即按等待区间采样。
        m_state.thunderTime = m_state.thundering ? 0 : 1;
        m_state.rainTime = m_state.raining ? 0 : 1;
        m_state.thundering = false;
        m_state.raining = false;
    } else {
        // 雷暴计时器：正在雷暴时用 THUNDER_DURATION，否则用 THUNDER_DELAY。
        if (m_state.thunderTime > 0) {
            --m_state.thunderTime;
            if (m_state.thunderTime == 0) {
                m_state.thundering = !m_state.thundering;
            }
        } else if (m_state.thundering) {
            m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);
        } else {
            m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDelay(*m_random);
        }

        // 降雨计时器：正在下雨时用 RAIN_DURATION，否则用 RAIN_DELAY。
        if (m_state.rainTime > 0) {
            --m_state.rainTime;
            if (m_state.rainTime == 0) {
                m_state.raining = !m_state.raining;
            }
        } else if (m_state.raining) {
            m_state.rainTime = mc::weather::WeatherUtils::getRandomRainDuration(*m_random);
        } else {
            m_state.rainTime = mc::weather::WeatherUtils::getRandomRainDelay(*m_random);
        }
    }
}

void WeatherManager::_updateStrength()
{
    // 每tick变化 ±0.01

    // 更新降雨强度
    if (m_state.raining) {
        m_state.rainStrength =
            std::min(1.0f, m_state.rainStrength + mc::weather::WeatherConstants::STRENGTH_CHANGE_RATE);
    } else {
        m_state.rainStrength =
            std::max(0.0f, m_state.rainStrength - mc::weather::WeatherConstants::STRENGTH_CHANGE_RATE);
    }

    // 更新雷暴强度：原版只看 thundering 标志。对外可见性由 isThundering
    // 乘以降雨强度决定，因此"雷暴但未下雨"时内部强度仍会正常上升。
    if (m_state.thundering) {
        m_state.thunderStrength =
            std::min(1.0f, m_state.thunderStrength + mc::weather::WeatherConstants::STRENGTH_CHANGE_RATE);
    } else {
        m_state.thunderStrength =
            std::max(0.0f, m_state.thunderStrength - mc::weather::WeatherConstants::STRENGTH_CHANGE_RATE);
    }

    // 检查强度是否变化
    if (m_state.rainStrength != m_state.prevRainStrength || m_state.thunderStrength != m_state.prevThunderStrength) {
        m_strengthChanged = true;
    }
}

void WeatherManager::_checkWeatherChange()
{
    // 检查降雨状态变化（使用带维度门控的查询）
    const bool currentlyRaining = isRaining();

    if (currentlyRaining != m_lastRaining) {
        m_weatherChanged = true;
        if (m_weatherChangeCallback) {
            const bool currentlyThundering = isThundering();
            WeatherType oldType =
                m_lastRaining ? (currentlyThundering ? WeatherType::Thunder : WeatherType::Rain) : WeatherType::Clear;
            WeatherType newType = currentlyRaining ? (currentlyThundering ? WeatherType::Thunder : WeatherType::Rain)
                                                   : WeatherType::Clear;
            m_weatherChangeCallback(oldType, newType);
        }
        m_lastRaining = currentlyRaining;
    }
}

i32 WeatherManager::setClear(i32 duration)
{
    // 原版 /weather clear 未提供 duration 时按 RAIN_DELAY 采样。
    if (duration <= 0) {
        duration = mc::weather::WeatherUtils::getRandomRainDelay(*m_random);
    }

    // 原版 setWeatherParameters(clearTime, 0, false, false)：两个天气计时器都写 0。
    m_state.clearWeatherTime = duration;
    m_state.rainTime = 0;
    m_state.thunderTime = 0;
    m_state.raining = false;
    m_state.thundering = false;

    // 强度会自然渐变到 0
    return duration;
}

i32 WeatherManager::setRain(i32 duration)
{
    if (duration <= 0) {
        duration = mc::weather::WeatherUtils::getRandomRainDuration(*m_random);
    }

    // 原版 setWeatherParameters(0, duration, true, false)：rainTime 与 thunderTime 都写 duration。
    m_state.clearWeatherTime = 0;
    m_state.raining = true;
    m_state.thundering = false;
    m_state.rainTime = duration;
    m_state.thunderTime = duration;

    // 强度会自然渐变到 1
    return duration;
}

i32 WeatherManager::setThunder(i32 duration)
{
    if (duration <= 0) {
        duration = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);
    }

    // 原版 setWeatherParameters(0, duration, true, true)：雷暴同时启用降雨，
    // rainTime 与 thunderTime 相同，因此雷暴结束后会立即转入晴天计时。
    m_state.clearWeatherTime = 0;
    m_state.raining = true;
    m_state.thundering = true;
    m_state.rainTime = duration;
    m_state.thunderTime = duration;

    return duration;
}

void WeatherManager::restoreState(i32 clearWeatherTime, i32 rainTime, bool raining, i32 thunderTime, bool thundering)
{
    m_state.clearWeatherTime = clearWeatherTime;
    m_state.rainTime = rainTime;
    m_state.raining = raining;
    m_state.thunderTime = thunderTime;
    m_state.thundering = thundering;

    // 原版 Level.prepareWeather：正在下雨/雷暴时强度直接置 1，其余情况保持 0。
    m_state.rainStrength = raining ? 1.0f : 0.0f;
    m_state.thunderStrength = thundering ? 1.0f : 0.0f;
    m_state.prevRainStrength = m_state.rainStrength;
    m_state.prevThunderStrength = m_state.thunderStrength;

    // 恢复后不应把存档里已存在的天气误报为"本 tick 刚发生的变化"。
    m_lastRaining = isRaining();
}

void WeatherManager::resetWeather()
{
    m_state.resetWeather();

    // 计时器归零后，下一个 tick 会按 WAIT 区间重新采样；强度自然渐变到 0。
    m_weatherChanged = true;
}

std::optional<BlockPos> WeatherManager::findLightningTarget(
    mc::IWorld& world, const mc::world::chunk::ChunkData& chunk, const BlockPos& randomPos)
{
    MC_ASSERT_RELEASE(m_canHaveWeather);

    const i32 chunkStartX = chunk.x() * world::CHUNK_WIDTH;
    const i32 chunkStartZ = chunk.z() * world::CHUNK_WIDTH;
    const BlockCoord localX = randomPos.x - chunkStartX;
    const BlockCoord localZ = randomPos.z - chunkStartZ;

    // 原版 getHeightmapPos(MOTION_BLOCKING, pos)：取该列最高运动阻挡方块。
    i32 topY = chunk.getTopBlockY(HeightmapType::MotionBlocking, localX, localZ);
    BlockPos targetPos(randomPos.x, topY, randomPos.z);
    if (topY < world::MIN_BUILD_HEIGHT) {
        // 原版对空列返回 getMinY()-1，随后 above(2) 抬到 minY+1。
        targetPos = BlockPos(randomPos.x, world::MIN_BUILD_HEIGHT + 1, randomPos.z);
    }

    // 落点优先选择世界表面的避雷针；否则在柱状范围内随机挑一个可见天空的生物。
    if (auto rod = _findLightningRod(world, targetPos); rod.has_value()) {
        targetPos = *rod;
    } else {
        // 原版 AABB.encapsulatingFullBlocks(pos, pos.atY(getMaxY()+1)).inflate(3)。
        const f32 minX = static_cast<f32>(targetPos.x) - 3.0f;
        const f32 maxX = static_cast<f32>(targetPos.x) + 1.0f + 3.0f;
        const f32 minY = static_cast<f32>(targetPos.y) - 3.0f;
        const f32 maxY = static_cast<f32>(world.getMaxBuildHeight()) + 1.0f + 3.0f;
        const f32 minZ = static_cast<f32>(targetPos.z) - 3.0f;
        const f32 maxZ = static_cast<f32>(targetPos.z) + 1.0f + 3.0f;
        const AxisAlignedBB searchBox(minX, minY, minZ, maxX, maxY, maxZ);

        // 原版只按 LivingEntity 过滤（玩家也属于 LivingEntity），不排除其他实体类型。
        std::vector<Entity*> candidates;
        for (Entity* entity : world.getEntitiesInAABB(searchBox, nullptr)) {
            auto* living = dynamic_cast<LivingEntity*>(entity);
            if (living == nullptr || !living->isAlive()) {
                continue;
            }

            const BlockPos entityPos(static_cast<i32>(std::floor(living->x())),
                static_cast<i32>(std::floor(living->y())),
                static_cast<i32>(std::floor(living->z())));
            if (world.canSeeSky(entityPos)) {
                candidates.push_back(entity);
            }
        }

        if (!candidates.empty()) {
            Entity* chosen = candidates[m_random->nextInt(static_cast<i32>(candidates.size()))];
            targetPos = BlockPos(static_cast<i32>(std::floor(chosen->x())),
                static_cast<i32>(std::floor(chosen->y())),
                static_cast<i32>(std::floor(chosen->z())));
        }
    }

    // 原版 isRainingAt(blockpos)：落点必须允许降水，否则本次闪电作废。
    if (!mc::weather::WeatherUtils::canRainAt(world, targetPos)) {
        return std::nullopt;
    }

    return targetPos;
}

std::optional<BlockPos> WeatherManager::_findLightningRod(mc::IWorld& world, const BlockPos& pos) const
{
    auto* villageManager = world.villageManager();
    if (villageManager == nullptr) {
        return std::nullopt;
    }

    const auto rods = villageManager->getPOIStorage().findAllInRange(
        pos, 128.0f, world::village::poi::PointOfInterestType::LightningRod);

    // 原版 PoiManager.findClosest 取 3D 距离平方最小者，并要求避雷针位于世界表面：
    // poi.y == getHeight(WORLD_SURFACE, x, z) - 1，即 poi.y 等于该列最高非空气方块 Y。
    const world::village::poi::PointOfInterest* nearest = nullptr;
    f32 nearestDistanceSq = std::numeric_limits<f32>::max();
    for (const auto* rod : rods) {
        const BlockPos rodPos = rod->getPosition();
        const auto* rodChunk = world.chunkManager()->tryToGetChunkInMem(rodPos.chunkX(), rodPos.chunkZ());
        if (rodChunk == nullptr) {
            continue;
        }

        const i32 surfaceY = rodChunk->getTopBlockY(HeightmapType::WorldSurface, rodPos.localX(), rodPos.localZ());
        if (rodPos.y != surfaceY) {
            continue;
        }

        const f32 dx = static_cast<f32>(rodPos.x - pos.x);
        const f32 dy = static_cast<f32>(rodPos.y - pos.y);
        const f32 dz = static_cast<f32>(rodPos.z - pos.z);
        const f32 distanceSq = dx * dx + dy * dy + dz * dz;
        if (distanceSq < nearestDistanceSq) {
            nearestDistanceSq = distanceSq;
            nearest = rod;
        }
    }

    if (nearest == nullptr) {
        return std::nullopt;
    }

    // 原版 findLightningRod 返回避雷针上方一格作为闪电落点。
    return nearest->getPosition().up();
}

} // namespace mc::server
