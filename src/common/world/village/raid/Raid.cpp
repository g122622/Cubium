#include "Raid.hpp"

#include "../../../entity/combat/DifficultyHelper.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/entities/monster/illager/EvokerEntity.hpp"
#include "../../../entity/entities/monster/illager/IllagerEntities.hpp"
#include "../../../entity/entities/monster/illager/RavagerEntity.hpp"
#include "../../../entity/entities/monster/illager/WitchEntity.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../IWorld.hpp"
#include "../Village.hpp"

#include <algorithm>
#include <cmath>

namespace mc::world::village::raid {

/**
 * @brief 构造袭击对象并初始化中心点。
 *
 * @param id 袭击 ID。
 * @param village 关联村庄。
 */
Raid::Raid(RaidId id, village::Village* village)
    : m_id(id)
    , m_village(village)
    , m_center(village != nullptr ? village->getCenter() : BlockPos::zero())
{}

/**
 * @brief 根据难度返回基础波次数。
 *
 * @param difficulty 世界难度。
 * @return 基础波次数。
 */
i32 Raid::maxWaves(Difficulty difficulty) const
{
    return entity::combat::DifficultyHelper::getRaidWaves(difficulty);
}

/**
 * @brief 获取当前追踪的存活袭击者数量。
 *
 * @return 袭击者数量。
 */
i32 Raid::getAliveRaidersCount() const
{
    return static_cast<i32>(m_raiders.size());
}

/**
 * @brief 追踪一个新袭击者。
 *
 * @param raider 袭击者实体 ID。
 */
void Raid::addRaider(EntityId raider)
{
    if (std::find(m_raiders.begin(), m_raiders.end(), raider) == m_raiders.end()) {
        m_raiders.push_back(raider);
    }
}

/**
 * @brief 取消追踪一个袭击者。
 *
 * @param raider 袭击者实体 ID。
 */
void Raid::removeRaider(EntityId raider)
{
    const auto it = std::find(m_raiders.begin(), m_raiders.end(), raider);
    if (it != m_raiders.end()) {
        m_raiders.erase(it);
    }
}

/**
 * @brief 处理袭击者死亡逻辑。
 *
 * @param raider 死亡的袭击者实体 ID。
 * @param world 所属世界。
 */
void Raid::onRaiderDeath(EntityId raider, IWorld& world)
{
    removeRaider(raider);

    if (isWaveDefeated()) {
        if (hasMoreWaves()) {
            m_lastWaveTime = static_cast<i64>(world.currentTick());
        } else {
            setVictory();
        }
    }
}

/**
 * @brief 推进到下一波并立即生成。
 *
 * @param world 所属世界。
 */
void Raid::startNextWave(IWorld& world)
{
    ++m_wave;
    spawnRaiders(world);
}

/**
 * @brief 生成当前波袭击者。
 *
 * @param world 所属世界。
 */
void Raid::spawnRaiders(IWorld& world)
{
    if (m_wave <= 0) {
        m_wave = 1;
    }

    m_difficulty = world.difficulty();
    if (!entity::combat::DifficultyHelper::allowsMobSpawning(m_difficulty)) {
        stop();
        return;
    }

    const i32 totalRaiders = calculateRaidersForWave(m_wave, m_difficulty);
    auto spawnPos = findSpawnPosition(world);
    if (!spawnPos.has_value()) {
        return;
    }

    const BlockPos basePos = *spawnPos;
    math::Random rng(world.seed() + static_cast<u64>(m_ticksActive));

    RaidWave waveData{};
    waveData.waveNumber = m_wave;
    waveData.totalToSpawn = totalRaiders;

    for (i32 i = 0; i < totalRaiders; ++i) {
        const RaiderType type = selectRaiderType(m_wave, i, totalRaiders);
        const f32 offsetX = (rng.nextFloat() - 0.5f) * 10.0f;
        const f32 offsetZ = (rng.nextFloat() - 0.5f) * 10.0f;
        const BlockPos pos(
            static_cast<BlockCoord>(basePos.x + offsetX), basePos.y, static_cast<BlockCoord>(basePos.z + offsetZ));

        const EntityId raider = spawnRaider(world, type, pos);
        if (raider != 0) {
            addRaider(raider);
            waveData.raiders.push_back(raider);
            ++waveData.spawnCount;
        }
    }

    waveData.spawned = true;
    waveData.defeated = waveData.raiders.empty();
    m_waves.push_back(std::move(waveData));
    m_lastWaveTime = static_cast<i64>(world.currentTick());
    ++m_groupsSpawned;
}

/**
 * @brief 判断当前波是否已被清空。
 *
 * @return 当前波是否已无追踪袭击者。
 */
bool Raid::isWaveDefeated() const
{
    return m_raiders.empty();
}

/**
 * @brief 判断是否还有后续波次。
 *
 * @return 是否还应继续生成下一波。
 */
bool Raid::hasMoreWaves() const
{
    const i32 extraWaves = std::max(0, m_badOmenLevel - 1) * RaidConfig::BAD_OMEN_WAVE_BONUS;
    const i32 targetWaves = maxWaves(m_difficulty) + extraWaves;
    return targetWaves > 0 && m_wave < targetWaves;
}

/**
 * @brief 更新袭击状态机。
 *
 * @param world 所属世界。
 */
void Raid::tick(IWorld& world)
{
    if (m_status != RaidStatus::Ongoing) {
        return;
    }

    if (!entity::combat::DifficultyHelper::allowsMobSpawning(world.difficulty())) {
        stop();
        return;
    }

    ++m_ticksActive;

    if (m_ticksActive > RaidConfig::RAID_TIMEOUT) {
        setLoss();
        return;
    }

    if (!isValid()) {
        stop();
        return;
    }

    if (m_wave <= 0) {
        startNextWave(world);
        return;
    }

    if (isWaveDefeated()) {
        if (hasMoreWaves()) {
            if (m_ticksActive - m_lastWaveTime >= RaidConfig::WAVE_INTERVAL) {
                startNextWave(world);
            }
        } else {
            setVictory();
        }
    }

    updateBossBar();
}

/**
 * @brief 判断袭击是否仍然有效。
 *
 * @return 关联村庄存在时返回 true。
 */
bool Raid::isValid() const
{
    return m_village != nullptr;
}

/**
 * @brief 停止袭击并清空追踪状态。
 */
void Raid::stop()
{
    m_status = RaidStatus::Stopped;
    m_raiders.clear();
}

/**
 * @brief 标记袭击为胜利。
 */
void Raid::setVictory()
{
    m_status = RaidStatus::Victory;
    m_raiders.clear();
}

/**
 * @brief 标记袭击为失败。
 */
void Raid::setLoss()
{
    m_status = RaidStatus::Loss;
    m_raiders.clear();
}

/**
 * @brief 生成 Boss 栏标题。
 *
 * @return 标题文本。
 */
std::string Raid::getBossBarTitle() const
{
    switch (m_status) {
        case RaidStatus::Victory:
            return "Raid - Victory";
        case RaidStatus::Loss:
            return "Raid - Defeat";
        case RaidStatus::Stopped:
            return "Raid - Stopped";
        default:
            return "Raid - Wave " + std::to_string(m_wave);
    }
}

/**
 * @brief 计算 Boss 栏进度。
 *
 * @return 归一化进度值。
 */
f32 Raid::getBossBarProgress() const
{
    if (m_status != RaidStatus::Ongoing) {
        return 0.0f;
    }

    const i32 totalWaves = std::max(1, maxWaves(m_difficulty) + std::max(0, m_badOmenLevel - 1));
    const f32 waveProgress = static_cast<f32>(m_wave) / static_cast<f32>(totalWaves);
    const f32 raiderProgress =
        1.0f - static_cast<f32>(getAliveRaidersCount()) / static_cast<f32>(RaidConfig::MAX_RAIDERS_PER_WAVE);
    return std::clamp(waveProgress + raiderProgress / static_cast<f32>(totalWaves), 0.0f, 1.0f);
}

/**
 * @brief 更新 Boss 栏状态。
 *
 * @note 当前暂无同步目标，因此此处保持空实现。
 */
void Raid::updateBossBar() {}

/**
 * @brief 显式生成指定波次。
 *
 * @param world 所属世界。
 * @param waveNum 波次编号。
 */
void Raid::spawnWave(IWorld& world, i32 waveNum)
{
    MC_ASSERT_RELEASE(waveNum > 0);
    m_wave = waveNum;
    spawnRaiders(world);
}

/**
 * @brief 计算指定波次的袭击者数量。
 *
 * @param waveNum 波次编号。
 * @param difficulty 世界难度。
 * @return 该波次应生成的袭击者数量。
 */
i32 Raid::calculateRaidersForWave(i32 waveNum, Difficulty difficulty) const
{
    MC_ASSERT_RELEASE(waveNum > 0);

    i32 baseCount = RaidConfig::MIN_RAIDERS_PER_WAVE;
    baseCount += waveNum - 1;
    baseCount += static_cast<i32>(difficulty);
    baseCount += std::max(0, m_badOmenLevel - 1);

    return std::clamp(baseCount, RaidConfig::MIN_RAIDERS_PER_WAVE, RaidConfig::MAX_RAIDERS_PER_WAVE);
}

/**
 * @brief 为当前生成槽位选择袭击者类型。
 *
 * @param waveNum 波次编号。
 * @param index 当前索引。
 * @param total 本波总数。
 * @return 选择结果。
 */
RaiderType Raid::selectRaiderType(i32 waveNum, i32 index, i32 total) const
{
    MC_ASSERT_RELEASE(waveNum > 0);
    MC_ASSERT_RELEASE(index >= 0);
    MC_ASSERT_RELEASE(total > 0);

    if (waveNum >= 7 && index == 0) {
        return RaiderType::Evoker;
    }

    if (waveNum >= 6 && index == 1) {
        return RaiderType::Ravager;
    }

    if (waveNum >= 4 && index == total - 1) {
        return RaiderType::Witch;
    }

    if (waveNum >= 3 && index < total / 3) {
        return RaiderType::Vindicator;
    }

    return RaiderType::Pillager;
}

/**
 * @brief 生成单个袭击者实体。
 *
 * @param world 所属世界。
 * @param type 袭击者类型。
 * @param pos 生成位置。
 * @return 实体 ID，失败返回 0。
 */
EntityId Raid::spawnRaider(IWorld& world, RaiderType type, BlockPos pos)
{
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

    entity->setPosition(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f);

    const EntityId id = world.spawnEntity(std::move(entity));
    if (id != 0) {
        Entity* const spawnedEntity = world.getEntity(id);
        if (auto* const raider = dynamic_cast<AbstractRaiderEntity*>(spawnedEntity)) {
            raider->joinRaid(this, m_wave);
        }
    }

    return id;
}

/**
 * @brief 在村庄周边选择袭击者生成点。
 *
 * @param world 所属世界。
 * @return 生成点；若当前没有可用村庄则返回空值。
 */
std::optional<BlockPos> Raid::findSpawnPosition(IWorld& world) const
{
    if (m_village == nullptr) {
        return std::nullopt;
    }

    math::Random rng(world.seed() + static_cast<u64>(m_ticksActive));
    const f32 angle = rng.nextFloat() * math::TWO_PI;
    const f32 distance = RaidConfig::SPAWN_DISTANCE_MIN +
        rng.nextFloat() * (RaidConfig::SPAWN_DISTANCE_MAX - RaidConfig::SPAWN_DISTANCE_MIN);

    const BlockCoord x = static_cast<BlockCoord>(m_center.x + std::cos(angle) * distance);
    const BlockCoord z = static_cast<BlockCoord>(m_center.z + std::sin(angle) * distance);
    const BlockCoord y = world.getHeight(x, z);
    return BlockPos(x, y, z);
}

// ========== 英雄追踪实现 ==========

/**
 * @brief 添加英雄（参与袭击的玩家）。
 *
 * 参考 MC 1.16.5 Raid.addHero()
 */
void Raid::addHero(Uuid playerUuid, EntityId entityId)
{
    if (m_heroes.find(playerUuid) == m_heroes.end()) {
        m_heroes.insert(playerUuid);
        m_participants.emplace_back(playerUuid, entityId);
    }
}

/**
 * @brief 检查玩家是否为英雄。
 */
bool Raid::isHero(Uuid playerUuid) const
{
    return m_heroes.find(playerUuid) != m_heroes.end();
}

/**
 * @brief 增加玩家贡献值。
 */
void Raid::addContribution(Uuid playerUuid, i32 amount)
{
    for (auto& participant : m_participants) {
        if (participant.uuid == playerUuid) {
            participant.contribution += amount;
            return;
        }
    }
    // 如果玩家不在列表中，不自动添加（需要先调用 addHero）
}

/**
 * @brief 获取玩家贡献值。
 */
i32 Raid::getContribution(Uuid playerUuid) const
{
    for (const auto& participant : m_participants) {
        if (participant.uuid == playerUuid) {
            return participant.contribution;
        }
    }
    return 0;
}

} // namespace mc::world::village::raid
