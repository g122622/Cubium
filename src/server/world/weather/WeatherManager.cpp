#include "WeatherManager.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/chunk/IChunk.hpp"
#include <algorithm>

namespace mc::server {

WeatherManager::WeatherManager()
    : m_random(std::make_unique<mc::math::Random>(0))
{}

WeatherManager::~WeatherManager() = default;

void WeatherManager::initialize(u64 seed)
{
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
    // 重置变化标志
    m_weatherChanged = false;
    m_strengthChanged = false;

    // 保存上一帧强度
    m_state.prevRainStrength = m_state.rainStrength;
    m_state.prevThunderStrength = m_state.thunderStrength;

    // 处理天气周期
    if (m_state.weatherCycleEnabled) {
        tickWeatherCycle();
    }

    // 更新强度渐变
    updateStrength();

    // 检查天气变化
    checkWeatherChange();
}

void WeatherManager::tickWeatherCycle()
{
    // 参考 MC 1.16.5 ServerWorld.tick() 中的天气逻辑

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

void WeatherManager::updateStrength()
{
    // 参考 MC 1.16.5 ServerWorld.tick() 中的强度渐变逻辑
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

void WeatherManager::checkWeatherChange()
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
    // 参考 MC 1.16.5 WeatherCommand
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
    // 参考 MC 1.16.5 ServerWorld.tickEnvironment()
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

    // MC 1.16.5 ServerWorld.tickEnvironment() 选择加载的区块进行闪电生成
    // 由于 IWorld 接口限制，使用玩家位置附近的区块
    // 获取一个足够大范围内的实体来找到玩家
    auto entities = m_world->getEntitiesInRange(Vector3(0, 0, 0), static_cast<f32>(world::CHUNK_LOAD_RADIUS * 16));

    // 过滤出玩家
    std::vector<Entity*> playerEntities;
    for (Entity* entity : entities) {
        if (entity && entity->legacyType() == LegacyEntityType::Player && entity->isAlive()) {
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
    ChunkCoord playerChunkX = static_cast<ChunkCoord>(std::floor(playerPos.x / 16.0));
    ChunkCoord playerChunkZ = static_cast<ChunkCoord>(std::floor(playerPos.z / 16.0));

    // 获取该区块
    const ChunkData* chunk = m_world->getChunk(playerChunkX, playerChunkZ);
    if (chunk == nullptr) {
        return {false, BlockPos(0, 0, 0)};
    }

    // 在区块内随机选择位置
    i32 chunkStartX = playerChunkX * 16;
    i32 chunkStartZ = playerChunkZ * 16;

    // 获取区块内的随机位置
    // MC 1.16.5: getBlockRandomPos(chunkX, 0, chunkZ, 15)
    // 传入 0 作为 Y 起点，然后使用高度图获取正确的 Y
    BlockPos randomPos = getBlockRandomPos(chunkStartX, 0, chunkStartZ);

    // 获取该位置的最高可站立方块
    i32 topY = chunk->getTopBlockY(HeightmapType::MotionBlocking, randomPos.x - chunkStartX, randomPos.z - chunkStartZ);

    if (topY < world::MIN_BUILD_HEIGHT) {
        return {false, BlockPos(0, 0, 0)};
    }

    BlockPos targetPos(randomPos.x, topY, randomPos.z);

    // 调整位置到附近实体
    BlockPos adjustedPos = findLightningTargetAround(targetPos);

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

BlockPos WeatherManager::findLightningTargetAround(const BlockPos& pos) const
{
    // 参考 MC 1.16.5 ServerWorld.adjustPosToNearbyEntity()
    // 在位置周围搜索生物实体

    if (m_world == nullptr) {
        return pos;
    }

    // 获取高度
    i32 height = m_world->getHeight(pos.x, pos.z);

    // 创建搜索范围：从地面到世界高度限制
    AxisAlignedBB searchBox(static_cast<f32>(pos.x) - 3.0f,
        static_cast<f32>(height),
        static_cast<f32>(pos.z) - 3.0f,
        static_cast<f32>(pos.x) + 3.0f,
        static_cast<f32>(world::MAX_BUILD_HEIGHT),
        static_cast<f32>(pos.z) + 3.0f);

    // 获取范围内的实体
    std::vector<Entity*> entities = m_world->getEntitiesInAABB(searchBox, nullptr);

    // 过滤出活着的生物实体
    std::vector<Entity*> livingEntities;
    for (Entity* entity : entities) {
        if (entity && entity->isAlive()) {
            // 检查是否为生物实体（LivingEntity 或其子类）
            // 这里使用 LegacyEntityType 来判断
            LegacyEntityType type = entity->legacyType();
            // 排除玩家和一些特殊实体
            if (type != LegacyEntityType::Player && type != LegacyEntityType::Item &&
                type != LegacyEntityType::ExperienceOrb && type != LegacyEntityType::Unknown) {
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

BlockPos WeatherManager::getBlockRandomPos(i32 chunkX, i32 sectionY, i32 chunkZ)
{
    // 参考 MC 1.16.5 World.getBlockRandomPos()
    // 使用 LCG (Linear Congruential Generator) 确保分布均匀
    m_updateLCG = m_updateLCG * 3 + 1013904223;
    i32 i = static_cast<i32>(m_updateLCG >> 2);

    // MC 1.16.5: return new BlockPos(p_217383_1_ + (i & 15), p_217383_2_ + (i >> 16 & p_217383_4_), p_217383_3_ + (i >>
    // 8 & 15)); 第四个参数是 Y 轴掩码，闪电生成时传入 15 x = chunkX + (i & 15)          -> 范围 [0, 15] y = sectionY +
    // ((i >> 16) & 15) -> 范围 [0, 15] z = chunkZ + ((i >> 8) & 15)   -> 范围 [0, 15]

    return BlockPos(chunkX + (i & 15), sectionY + ((i >> 16) & 15), chunkZ + ((i >> 8) & 15));
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
    if (data.size() < offset + 22) { // 3 * 4 + 3 + 2 * 4 = 23 bytes
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
