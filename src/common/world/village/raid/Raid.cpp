#include "Raid.hpp"
#include "../Village.hpp"
#include "../../IWorld.hpp"
#include "../../../core/Constants.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/monster/illager/IllagerEntities.hpp"
#include "../../../entity/entities/monster/illager/EvokerEntity.hpp"
#include "../../../entity/entities/monster/illager/RavagerEntity.hpp"
#include "../../../entity/entities/monster/illager/WitchEntity.hpp"
#include "../../../util/math/random/Random.hpp"
#include <algorithm>

namespace mc {
namespace world {
namespace village {
namespace raid {

// ============================================================================
// Raid 实现
// ============================================================================

Raid::Raid(RaidId id, village::Village* village)
    : m_id(id)
    , m_village(village)
    , m_center(village ? village->getCenter() : BlockPos::zero())
{
}

i32 Raid::maxWaves(i32 difficulty) const {
    // 根据难度计算波次
    // 简单：3波，普通：5波，困难：7波
    switch (difficulty) {
        case 0: return RaidConfig::MAX_WAVES_EASY;
        case 1: return RaidConfig::MAX_WAVES_NORMAL;
        case 2: return RaidConfig::MAX_WAVES_HARD;
        default: return RaidConfig::MAX_WAVES_NORMAL;
    }
}

i32 Raid::getAliveRaidersCount() const {
    return static_cast<i32>(m_raiders.size());
}

void Raid::addRaider(EntityId raider) {
    if (std::find(m_raiders.begin(), m_raiders.end(), raider) == m_raiders.end()) {
        m_raiders.push_back(raider);
    }
}

void Raid::removeRaider(EntityId raider) {
    auto it = std::find(m_raiders.begin(), m_raiders.end(), raider);
    if (it != m_raiders.end()) {
        m_raiders.erase(it);
    }
}

void Raid::onRaiderDeath(EntityId raider, IWorld& world) {
    removeRaider(raider);

    // 检查当前波次是否完成
    if (isWaveDefeated()) {
        // 检查是否还有更多波次
        if (hasMoreWaves()) {
            // 准备下一波
            m_lastWaveTime = m_ticksActive;
        } else {
            // 袭击胜利（玩家方）
            setVictory();
        }
    }

    (void)world; // 暂时未使用
}

void Raid::startNextWave(IWorld& world) {
    m_wave++;
    spawnRaiders(world);
}

void Raid::spawnRaiders(IWorld& world) {
    if (m_wave <= 0) {
        m_wave = 1;
    }

    // 计算难度（从世界获取）
    i32 difficulty = world.difficulty();

    // 计算本波掠夺者数量
    i32 totalRaiders = calculateRaidersForWave(m_wave, difficulty);

    // 找到生成位置
    auto spawnPos = findSpawnPosition(world);
    if (!spawnPos.has_value()) {
        return;
    }

    BlockPos basePos = spawnPos.value();
    math::Random rng = math::Random(world.seed() + m_ticksActive);

    // 生成掠夺者
    for (i32 i = 0; i < totalRaiders; ++i) {
        RaiderType type = selectRaiderType(m_wave, i, totalRaiders);

        // 随机偏移生成位置
        f32 offsetX = (rng.nextFloat() - 0.5f) * 10.0f;
        f32 offsetZ = (rng.nextFloat() - 0.5f) * 10.0f;
        BlockPos pos(
            static_cast<BlockCoord>(basePos.x + offsetX),
            basePos.y,
            static_cast<BlockCoord>(basePos.z + offsetZ)
        );

        EntityId raider = spawnRaider(world, type, pos);
        if (raider != 0) {
            addRaider(raider);
        }
    }

    // 更新波次时间
    m_lastWaveTime = m_ticksActive;
    m_groupsSpawned++;
}

bool Raid::isWaveDefeated() const {
    return m_raiders.empty();
}

bool Raid::hasMoreWaves() const {
    // 假设困难难度
    return m_wave < RaidConfig::MAX_WAVES_HARD + m_badOmenLevel - 1;
}

void Raid::tick(IWorld& world) {
    if (m_status != RaidStatus::Ongoing) {
        return;
    }

    m_ticksActive++;

    // 检查超时
    if (m_ticksActive > RaidConfig::RAID_TIMEOUT) {
        setLoss();
        return;
    }

    // 检查村庄是否仍然有效
    if (!isValid()) {
        stop();
        return;
    }

    // 第一波初始化
    if (m_wave <= 0) {
        startNextWave(world);
        return;
    }

    // 检查当前波次状态
    if (isWaveDefeated()) {
        // 波次完成，检查是否还有更多波次
        if (hasMoreWaves()) {
            // 等待波次间隔
            if (m_ticksActive - m_lastWaveTime >= RaidConfig::WAVE_INTERVAL) {
                startNextWave(world);
            }
        } else {
            // 所有波次完成，玩家胜利
            setVictory();
        }
    }

    // 更新Boss栏
    updateBossBar();
}

bool Raid::isValid() const {
    // 检查村庄是否存在
    // TODO: 实际检查村庄状态
    return m_village != nullptr;
}

void Raid::stop() {
    m_status = RaidStatus::Stopped;
    m_raiders.clear();
}

void Raid::setVictory() {
    m_status = RaidStatus::Victory;
    m_raiders.clear();
    // TODO: 给玩家奖励（英雄效果）
}

void Raid::setLoss() {
    m_status = RaidStatus::Loss;
    m_raiders.clear();
}

String Raid::getBossBarTitle() const {
    switch (m_status) {
        case RaidStatus::Victory:
            return "袭击 - 胜利！";
        case RaidStatus::Loss:
            return "袭击 - 失败";
        case RaidStatus::Stopped:
            return "袭击 - 已停止";
        default:
            return "袭击 - 第 " + std::to_string(m_wave) + " 波";
    }
}

f32 Raid::getBossBarProgress() const {
    if (m_status != RaidStatus::Ongoing) {
        return 0.0f;
    }

    // 根据剩余掠夺者数量计算进度
    i32 maxWaves = RaidConfig::MAX_WAVES_HARD;
    f32 waveProgress = static_cast<f32>(m_wave) / static_cast<f32>(maxWaves);

    // 考虑当前波次的掠夺者存活情况
    i32 maxRaiders = RaidConfig::MAX_RAIDERS_PER_WAVE;
    f32 raiderProgress = 1.0f - static_cast<f32>(getAliveRaidersCount()) / static_cast<f32>(maxRaiders);

    return waveProgress + raiderProgress * (1.0f / static_cast<f32>(maxWaves));
}

void Raid::updateBossBar() {
    // TODO: 更新Boss栏显示
}

i32 Raid::calculateRaidersForWave(i32 waveNum, i32 difficulty) const {
    // 基础数量
    i32 baseCount = RaidConfig::MIN_RAIDERS_PER_WAVE;

    // 每波增加
    baseCount += waveNum - 1;

    // 难度调整
    baseCount += difficulty;

    // 不祥之兆等级调整
    baseCount += m_badOmenLevel - 1;

    // 限制范围
    return std::clamp(baseCount, RaidConfig::MIN_RAIDERS_PER_WAVE, RaidConfig::MAX_RAIDERS_PER_WAVE);
}

RaiderType Raid::selectRaiderType(i32 waveNum, i32 index, i32 total) const {
    // 后期波次增加更强的敌人
    if (waveNum >= 5) {
        // 第5波后可能出现唤魔者
        if (index == 0 && waveNum >= 7) {
            return RaiderType::Evoker;
        }
        // 劫掠兽
        if (index == 1 && waveNum >= 6) {
            return RaiderType::Ravager;
        }
    }

    if (waveNum >= 3) {
        // 第3波后出现灾厄村民
        if (index < total / 3) {
            return RaiderType::Vindicator;
        }
    }

    // 默认掠夺者
    return RaiderType::Pillager;
}

EntityId Raid::spawnRaider(IWorld& world, RaiderType type, BlockPos pos) {
    std::unique_ptr<Entity> entity;

    switch (type) {
        case RaiderType::Pillager:
            entity = PillagerEntity::create(&world);
            break;
        case RaiderType::Vindicator:
            entity = VindicatorEntity::create(&world);
            break;
        case RaiderType::Evoker:
            entity = EvokerEntity::create(&world);
            break;
        case RaiderType::Ravager:
            entity = RavagerEntity::create(&world);
            break;
        case RaiderType::Witch:
            entity = WitchEntity::create(&world);
            break;
        default:
            return 0;
    }

    if (!entity) {
        return 0;
    }

    // 设置位置
    entity->setPosition(static_cast<f32>(pos.x) + 0.5f,
                        static_cast<f32>(pos.y),
                        static_cast<f32>(pos.z) + 0.5f);

    // 生成实体
    EntityId id = world.spawnEntity(std::move(entity));

    // 关联掠夺者到袭击
    if (id != 0) {
        Entity* spawnedEntity = world.getEntity(id);
        if (auto* raider = dynamic_cast<AbstractRaiderEntity*>(spawnedEntity)) {
            raider->joinRaid(this, m_wave);
        }
    }

    return id;
}

std::optional<BlockPos> Raid::findSpawnPosition(IWorld& world) const {
    if (m_village == nullptr) {
        return std::nullopt;
    }

    // 在村庄周围寻找合适的生成位置
    math::Random rng = math::Random(world.seed() + m_ticksActive);
    f32 angle = rng.nextFloat() * math::TWO_PI;
    f32 distance = RaidConfig::SPAWN_DISTANCE_MIN + rng.nextFloat() * (RaidConfig::SPAWN_DISTANCE_MAX - RaidConfig::SPAWN_DISTANCE_MIN);

    BlockCoord x = static_cast<BlockCoord>(m_center.x + std::cos(angle) * distance);
    BlockCoord z = static_cast<BlockCoord>(m_center.z + std::sin(angle) * distance);
    BlockCoord y = world.getHeight(x, z);

    return BlockPos(x, y, z);
}

} // namespace raid
} // namespace village
} // namespace world
} // namespace mc
