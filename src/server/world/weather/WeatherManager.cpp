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
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include <algorithm>

using namespace mc::trace;

namespace mc::server {

WeatherManager::WeatherManager()
    : m_random(std::make_unique<mc::math::Random>(0))
{}

WeatherManager::~WeatherManager() = default;

void WeatherManager::initialize(u64 seed)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "WeatherManager::initialize");

    m_random = std::make_unique<mc::math::Random>(seed);

    // 初始化天气状态
    // 初始为晴天，设置随机时间
    m_state.clearWeatherTime = 0;
    m_state.rainTime = mc::weather::WeatherUtils::getRandomRainDuration(*m_random);
    m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);
    m_state.raining = false;
    m_state.thundering = false;
    m_state.rainStrength = 0.0f;
    m_state.thunderStrength = 0.0f;
    m_state.prevRainStrength = 0.0f;
    m_state.prevThunderStrength = 0.0f;
    m_state.weatherCycleEnabled = true;

    m_weatherChanged = false;
    m_strengthChanged = false;
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

    // 处理天气周期
    if (m_state.weatherCycleEnabled) {
        _tickWeatherCycle();
    }

    // 更新强度渐变
    _updateStrength();

    // 检查天气变化
    _checkWeatherChange();
}

void WeatherManager::_tickWeatherCycle()
{
    // 处理晴天计时器
    if (m_state.clearWeatherTime > 0) {
        --m_state.clearWeatherTime;
        // 强制晴天时，重置降雨/雷暴计时器
        m_state.rainTime = m_state.raining ? 0 : 1;
        m_state.thunderTime = m_state.thundering ? 0 : 1;
        m_state.raining = false;
        m_state.thundering = false;
    } else {
        // 处理雷暴计时器
        if (m_state.thunderTime > 0) {
            --m_state.thunderTime;
            if (m_state.thunderTime == 0) {
                // 切换雷暴状态
                m_state.thundering = !m_state.thundering;
            }
        } else if (m_state.thundering) {
            // 雷暴结束，设置新的雷暴间隔
            m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);
        } else {
            // 晴天，设置到下次雷暴的时间
            m_state.thunderTime = m_random->nextInt(
                mc::weather::WeatherConstants::MIN_THUNDER_TIME, mc::weather::WeatherConstants::MAX_THUNDER_TIME);
        }

        // 处理降雨计时器
        if (m_state.rainTime > 0) {
            --m_state.rainTime;
            if (m_state.rainTime == 0) {
                // 切换降雨状态
                m_state.raining = !m_state.raining;
            }
        } else if (m_state.raining) {
            // 降雨结束，设置到下次降雨的时间
            m_state.rainTime = mc::weather::WeatherUtils::getRandomRainDuration(*m_random);
        } else {
            // 晴天，设置到下次降雨的时间
            m_state.rainTime = m_random->nextInt(
                mc::weather::WeatherConstants::MIN_RAIN_TIME, mc::weather::WeatherConstants::MAX_RAIN_TIME);
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

    // 更新雷暴强度（依赖于降雨）
    if (m_state.thundering && m_state.raining) {
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
    // 检查降雨状态变化
    bool currentlyRaining = m_state.isRaining();

    if (currentlyRaining != m_lastRaining) {
        m_weatherChanged = true;
        if (m_weatherChangeCallback) {
            WeatherType oldType = m_lastRaining ? (m_state.isThundering() ? WeatherType::Thunder : WeatherType::Rain)
                                                : WeatherType::Clear;
            WeatherType newType = currentlyRaining ? (m_state.isThundering() ? WeatherType::Thunder : WeatherType::Rain)
                                                   : WeatherType::Clear;
            m_weatherChangeCallback(oldType, newType);
        }
        m_lastRaining = currentlyRaining;
    }
}

void WeatherManager::setClear(i32 duration)
{
    // /weather clear [duration] - duration 单位是秒，乘以 20 转换为 ticks
    if (duration <= 0) {
        duration = mc::weather::WeatherConstants::DEFAULT_COMMAND_DURATION;
    }

    m_state.clearWeatherTime = duration;
    m_state.raining = false;
    m_state.thundering = false;
    m_state.rainTime = mc::weather::WeatherUtils::getRandomRainDuration(*m_random);
    m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);

    // 注意：设置 clearWeatherTime 后，tick() 会处理计时器递减
    // 强度会自然渐变到 0
}

void WeatherManager::setRain(i32 duration)
{
    if (duration <= 0) {
        duration = mc::weather::WeatherConstants::DEFAULT_COMMAND_DURATION;
    }

    m_state.clearWeatherTime = 0;
    m_state.raining = true;
    m_state.thundering = false;
    m_state.rainTime = duration;
    m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);

    // 强度会自然渐变到 1
}

void WeatherManager::setThunder(i32 duration)
{
    if (duration <= 0) {
        duration = mc::weather::WeatherConstants::DEFAULT_COMMAND_DURATION;
    }

    // 雷暴需要同时启用降雨
    m_state.clearWeatherTime = 0;
    m_state.raining = true;
    m_state.thundering = true;
    m_state.rainTime = duration;
    m_state.thunderTime = duration;

    // 注意：/weather thunder 命令会使 rainTime 和 thunderTime 相同
    // 这样雷暴结束后晴天会开始
}

void WeatherManager::resetWeather()
{
    m_state.resetWeather();

    // 设置新的随机计时器
    m_state.rainTime = mc::weather::WeatherUtils::getRandomRainDuration(*m_random);
    m_state.thunderTime = mc::weather::WeatherUtils::getRandomThunderDuration(*m_random);

    m_weatherChanged = true;
}

std::pair<bool, BlockPos> WeatherManager::trySpawnLightning()
{
    // 雷暴时每tick有 1/100000 概率生成闪电

    if (!m_state.isThundering() || !m_state.isRaining()) {
        return {false, BlockPos(0, 0, 0)};
    }

    // 概率检查：每 tick 有 1/100000 概率尝试生成闪电
    if (m_random->nextInt(mc::weather::WeatherConstants::LIGHTNING_CHANCE_DENOMINATOR) != 0) {
        return {false, BlockPos(0, 0, 0)};
    }

    // 需要有效的世界
    if (m_world == nullptr) {
        return {false, BlockPos(0, 0, 0)};
    }

    // 选择加载的区块进行闪电生成
    // 由于 IWorld 接口限制，使用玩家位置附近的区块
    // 获取一个足够大范围内的实体来找到玩家
    auto entities =
        m_world->getEntitiesInRange(Vector3(0, 0, 0), static_cast<f32>(world::CHUNK_LOAD_RADIUS * world::CHUNK_WIDTH));

    // 过滤出玩家
    std::vector<Entity*> playerEntities;
    for (Entity* entity : entities) {
        if (entity && entity->typeId() == entity::EntityTypeIdNumber::PLAYER && entity->isAlive()) {
            playerEntities.push_back(entity);
        }
    }

    // 如果没有玩家，无法生成闪电
    if (playerEntities.empty()) {
        return {false, BlockPos(0, 0, 0)};
    }

    // 选择一个随机玩家
    Entity* randomPlayer = playerEntities[m_random->nextInt(static_cast<i32>(playerEntities.size()))];
    Vector3 playerPos = randomPlayer->position();

    // 区块坐标
    ChunkCoord playerChunkX = static_cast<ChunkCoord>(std::floor(playerPos.x / static_cast<f32>(world::CHUNK_WIDTH)));
    ChunkCoord playerChunkZ = static_cast<ChunkCoord>(std::floor(playerPos.z / static_cast<f32>(world::CHUNK_WIDTH)));

    // 获取该区块
    const ChunkData* chunk = m_world->getChunk(playerChunkX, playerChunkZ);
    if (chunk == nullptr) {
        return {false, BlockPos(0, 0, 0)};
    }

    // 在区块内随机选择位置
    i32 chunkStartX = playerChunkX * world::CHUNK_WIDTH;
    i32 chunkStartZ = playerChunkZ * world::CHUNK_WIDTH;

    // 获取区块内的随机位置
    // 传入 0 作为 Y 起点，然后使用高度图获取正确的 Y
    BlockPos randomPos = _getBlockRandomPos(chunkStartX, 0, chunkStartZ);

    // 获取该位置的最高可站立方块
    i32 topY = chunk->getTopBlockY(HeightmapType::MotionBlocking, randomPos.x - chunkStartX, randomPos.z - chunkStartZ);

    if (topY < world::MIN_BUILD_HEIGHT) {
        return {false, BlockPos(0, 0, 0)};
    }

    BlockPos targetPos(randomPos.x, topY, randomPos.z);

    // 调整位置到附近实体
    BlockPos adjustedPos = _findLightningTargetAround(targetPos);

    // 检查该位置是否可以降雨（生物群系检查）
    if (!m_world->canRainAt(adjustedPos)) {
        return {false, BlockPos(0, 0, 0)};
    }

    // 检查位置是否可以看到天空
    if (!mc::weather::WeatherUtils::canSeeSky(*m_world, adjustedPos)) {
        return {false, BlockPos(0, 0, 0)};
    }

    // 成功生成闪电
    return {true, adjustedPos};
}

BlockPos WeatherManager::_findLightningTargetAround(const BlockPos& pos) const
{
    // 在位置周围搜索生物实体

    if (m_world == nullptr) {
        return pos;
    }

    // 获取高度
    i32 height = m_world->getHeight(pos.x, pos.z);

    // 闪电目标搜索范围（水平半径）
    constexpr f32 LIGHTNING_SEARCH_RADIUS = 3.0f;

    // 创建搜索范围：从地面到世界高度限制
    AxisAlignedBB searchBox(static_cast<f32>(pos.x) - LIGHTNING_SEARCH_RADIUS,
        static_cast<f32>(height),
        static_cast<f32>(pos.z) - LIGHTNING_SEARCH_RADIUS,
        static_cast<f32>(pos.x) + LIGHTNING_SEARCH_RADIUS,
        static_cast<f32>(world::MAX_BUILD_HEIGHT),
        static_cast<f32>(pos.z) + LIGHTNING_SEARCH_RADIUS);

    // 获取范围内的实体
    std::vector<Entity*> entities = m_world->getEntitiesInAABB(searchBox, nullptr);

    // 过滤出活着的生物实体
    std::vector<Entity*> livingEntities;
    for (Entity* entity : entities) {
        if (entity && entity->isAlive()) {
            // 检查是否为生物实体（LivingEntity 或其子类）
            // 使用 EntityTypeId 来判断
            entity::EntityTypeId type = entity->typeId();
            // 排除玩家和一些特殊实体
            if (type != entity::EntityTypeIdNumber::PLAYER && type != entity::EntityTypeIdNumber::ITEM &&
                type != entity::EntityTypeIdNumber::EXPERIENCE_ORB && type != 0) {
                // 检查实体是否可以看到天空
                BlockPos entityPos(
                    static_cast<i32>(entity->x()), static_cast<i32>(entity->y()), static_cast<i32>(entity->z()));
                if (mc::weather::WeatherUtils::canSeeSky(*m_world, entityPos)) {
                    livingEntities.push_back(entity);
                }
            }
        }
    }

    // 如果找到了实体，随机选择一个
    if (!livingEntities.empty()) {
        Entity* target = livingEntities[m_random->nextInt(static_cast<i32>(livingEntities.size()))];
        return BlockPos(static_cast<i32>(target->x()), static_cast<i32>(target->y()), static_cast<i32>(target->z()));
    }

    // 如果没有找到实体，返回原始位置
    // 如果高度无效，调整一下
    if (pos.y < world::MIN_BUILD_HEIGHT + 2) {
        return BlockPos(pos.x, world::MIN_BUILD_HEIGHT + 2, pos.z);
    }

    return pos;
}

BlockPos WeatherManager::_getBlockRandomPos(i32 chunkX, i32 sectionY, i32 chunkZ)
{
    // 使用 LCG (Linear Congruential Generator) 确保分布均匀
    // LCG 参数：乘数 3，增量 1013904223（与 MC 一致）
    constexpr i64 LCG_MULTIPLIER = 3;
    constexpr i64 LCG_INCREMENT = 1013904223;

    m_updateLCG = m_updateLCG * LCG_MULTIPLIER + LCG_INCREMENT;
    i32 i = static_cast<i32>(m_updateLCG >> 2);

    // 计算 x, y, z 偏移
    // x = chunkX + (i & 15)          -> 范围 [0, 15]
    // y = sectionY + ((i >> 16) & 15) -> 范围 [0, 15]
    // z = chunkZ + ((i >> 8) & 15)   -> 范围 [0, 15]
    return BlockPos(chunkX + (i & world::CHUNK_MASK),
        sectionY + ((i >> 16) & world::CHUNK_MASK),
        chunkZ + ((i >> 8) & world::CHUNK_MASK));
}

void WeatherManager::serialize(std::vector<u8>& data) const
{
    // 简单的二进制序列化
    // 格式: [clearWeatherTime(4)] [rainTime(4)] [thunderTime(4)]
    //       [raining(1)] [thundering(1)] [weatherCycleEnabled(1)]
    //       [rainStrength(4)] [thunderStrength(4)]

    auto writeI32 = [&data](i32 value) {
        data.push_back(static_cast<u8>(value & 0xFF));
        data.push_back(static_cast<u8>((value >> 8) & 0xFF));
        data.push_back(static_cast<u8>((value >> 16) & 0xFF));
        data.push_back(static_cast<u8>((value >> 24) & 0xFF));
    };

    auto writeF32 = [&data](f32 value) {
        u32 bits;
        std::memcpy(&bits, &value, sizeof(f32));
        data.push_back(static_cast<u8>(bits & 0xFF));
        data.push_back(static_cast<u8>((bits >> 8) & 0xFF));
        data.push_back(static_cast<u8>((bits >> 16) & 0xFF));
        data.push_back(static_cast<u8>((bits >> 24) & 0xFF));
    };

    writeI32(m_state.clearWeatherTime);
    writeI32(m_state.rainTime);
    writeI32(m_state.thunderTime);
    data.push_back(m_state.raining ? 1 : 0);
    data.push_back(m_state.thundering ? 1 : 0);
    data.push_back(m_state.weatherCycleEnabled ? 1 : 0);
    writeF32(m_state.rainStrength);
    writeF32(m_state.thunderStrength);
}

Result<void> WeatherManager::deserialize(const std::vector<u8>& data, size_t& offset)
{
    // 序列化格式：3 * 4 (i32) + 3 (bool) + 2 * 4 (f32) = 23 字节
    constexpr size_t WEATHER_STATE_SIZE = 23;

    if (data.size() < offset + WEATHER_STATE_SIZE) {
        return Error(ErrorCode::InvalidData, "Insufficient data for weather state");
    }

    auto readI32 = [&data, &offset]() -> i32 {
        i32 value = static_cast<i32>(static_cast<u32>(data[offset]) | (static_cast<u32>(data[offset + 1]) << 8) |
            (static_cast<u32>(data[offset + 2]) << 16) | (static_cast<u32>(data[offset + 3]) << 24));
        offset += 4;
        return value;
    };

    auto readF32 = [&data, &offset]() -> f32 {
        u32 bits = static_cast<u32>(data[offset]) | (static_cast<u32>(data[offset + 1]) << 8) |
            (static_cast<u32>(data[offset + 2]) << 16) | (static_cast<u32>(data[offset + 3]) << 24);
        offset += 4;
        f32 value;
        std::memcpy(&value, &bits, sizeof(f32));
        return value;
    };

    m_state.clearWeatherTime = readI32();
    m_state.rainTime = readI32();
    m_state.thunderTime = readI32();
    m_state.raining = data[offset++] != 0;
    m_state.thundering = data[offset++] != 0;
    m_state.weatherCycleEnabled = data[offset++] != 0;
    m_state.rainStrength = readF32();
    m_state.thunderStrength = readF32();

    // 初始化前一帧强度
    m_state.prevRainStrength = m_state.rainStrength;
    m_state.prevThunderStrength = m_state.thunderStrength;

    return {};
}

} // namespace mc::server
