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

#include "Raid.hpp"

#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/illager/EvokerEntity.hpp"
#include "common/entity/entities/monster/illager/IllagerEntities.hpp"
#include "common/entity/entities/monster/illager/RavagerEntity.hpp"
#include "common/entity/entities/monster/illager/WitchEntity.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "common/world/village/Village.hpp"

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
 *
 * @note 当死亡导致当前波被清空时，会启动 300 tick 的波间冷却，
 *       与 Java 版 1.21.11 Raid#tick() 中 `raidCooldownTicks = 300` 一致。
 *       该冷却值会被 _updateBossBar() 用于渲染 Boss 栏的"冷却条"进度。
 */
void Raid::onRaiderDeath(EntityId raider, IWorld& world)
{
    removeRaider(raider);

    if (isWaveDefeated()) {
        if (hasMoreWaves()) {
            m_lastWaveTime = static_cast<i64>(world.currentTick());
            // 启动波间冷却，Boss 栏进度切换为冷却倒计时模式。
            m_raidCooldownTicks = RaidConfig::RAID_COOLDOWN_TICKS;
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
 *
 * @note 每次开始新一波都会将 m_totalHealth 重置为 0，再随生成顺序累加；
 *       这与 Java 版 1.21.11 Raid#spawnGroup() 的 `this.totalHealth = 0.0F`
 *       行为一致，保证 _updateBossBar() 计算的比例始终基于"当前波"的血量。
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

    const i32 totalRaiders = _calculateRaidersForWave(m_wave, m_difficulty);
    auto spawnPos = _findSpawnPosition(world);
    if (!spawnPos.has_value()) {
        return;
    }

    const BlockPos basePos = *spawnPos;
    math::Random rng(world.seed() + static_cast<u64>(m_ticksActive));

    // Java 版 1.21.11：每波开始时重置 totalHealth，本波生成阶段累加，
    // 作为 _updateBossBar() 计算进度时的分母。
    m_totalHealth = 0.0f;

    RaidWave waveData{};
    waveData.waveNumber = m_wave;
    waveData.totalToSpawn = totalRaiders;

    for (i32 i = 0; i < totalRaiders; ++i) {
        const RaiderType type = _selectRaiderType(m_wave, i, totalRaiders);
        const f32 offsetX = (rng.nextFloat() - 0.5f) * 10.0f;
        const f32 offsetZ = (rng.nextFloat() - 0.5f) * 10.0f;
        const BlockPos pos(
            static_cast<BlockCoord>(basePos.x + offsetX), basePos.y, static_cast<BlockCoord>(basePos.z + offsetZ));

        const EntityId raider = _spawnRaider(world, type, pos);
        if (raider != 0) {
            addRaider(raider);
            waveData.raiders.push_back(raider);
            ++waveData.spawnCount;

            // 累加新袭击者的当前血量到波次总血量。Java 版在 addWaveMob() 中做这件事；
            // 我们在生成成功后立即读取，对应同一语义。
            if (auto* const entity = world.getEntity(raider)) {
                if (const auto* living = dynamic_cast<const LivingEntity*>(entity)) {
                    m_totalHealth += living->health();
                }
            }
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
 *
 * @note 波间冷却逻辑对齐 Java 版 1.21.11 Raid#tick()：
 *       - 当前波被清空时，raidCooldownTicks 已在 onRaiderDeath() 中置为 300；
 *       - 此后每 tick 递减 raidCooldownTicks，并相应更新 Boss 栏冷却进度；
 *       - 倒计时归零后才推进到下一波，避免使用旧的 WAVE_INTERVAL 静态阈值。
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
        _updateBossBar(world);
        return;
    }

    if (isWaveDefeated()) {
        if (hasMoreWaves()) {
            // 波间冷却阶段：raidCooldownTicks 在 onRaiderDeath 中被置为 300，
            // 此处每 tick 递减，归零时推进到下一波。对应 Java 版 Raid#tick() 中
            // 的 `if (this.raidCooldownTicks <= 0) { ... } else { ... raidCooldownTicks--; }` 分支。
            if (m_raidCooldownTicks > 0) {
                --m_raidCooldownTicks;
                if (m_raidCooldownTicks == 0) {
                    startNextWave(world);
                }
            }
        } else {
            setVictory();
        }
    }

    _updateBossBar(world);
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
 * @return 归一化进度值，范围 [0.0, 1.0]。
 *
 * @note 该方法直接返回 _updateBossBar() 在最近一次 tick 中缓存的进度值，
 *       避免外部高频调用导致重复遍历袭击者列表。Raid 进入非 Ongoing 状态后
 *       会强制返回 0.0，对应 Java 版 Raid#raidEvent.setProgress(0.0F) 的语义。
 */
f32 Raid::getBossBarProgress() const
{
    if (m_status != RaidStatus::Ongoing) {
        return 0.0f;
    }
    return m_cachedProgress;
}

/**
 * @brief 更新 Boss 栏内部状态。
 *
 * @param world 所属世界，用于查询袭击者实时血量。
 *
 * @note 三段式进度逻辑对齐 Java 版 1.21.11 Raid：
 *       - 战斗中（raidCooldownTicks == 0 且仍有存活袭击者）：
 *         `getHealthOfLivingRaiders() / m_totalHealth`，
 *         分母为 0（波次尚未生成或刚清空）时记为 1.0，避免 NaN；
 *       - 波间冷却（raidCooldownTicks > 0）：
 *         `(RAID_COOLDOWN_TICKS - raidCooldownTicks) / RAID_COOLDOWN_TICKS`，
 *         倒计时启动瞬间为 0、归零时为 1.0；
 *       - 庆祝/结束：状态非 Ongoing 时由 getBossBarProgress() 直接返回 0。
 */
void Raid::_updateBossBar(IWorld& world)
{
    if (m_status != RaidStatus::Ongoing) {
        m_cachedProgress = 0.0f;
        return;
    }

    if (m_raidCooldownTicks > 0) {
        // 波间冷却：倒计时从 300 → 0，进度从 0 → 1.0。
        const f32 cooldownProgress = static_cast<f32>(RaidConfig::RAID_COOLDOWN_TICKS - m_raidCooldownTicks) /
            static_cast<f32>(RaidConfig::RAID_COOLDOWN_TICKS);
        m_cachedProgress = std::clamp(cooldownProgress, 0.0f, 1.0f);
        return;
    }

    // 战斗中：按存活血量比例展示。分母为 0 时意味着本波尚未生成任何袭击者，
    // 此时进度视为 1.0（满血），与 Java 版 totalHealth == 0 时的退化情况一致。
    if (m_totalHealth <= 0.0f) {
        m_cachedProgress = 1.0f;
        return;
    }

    const f32 livingHealth = _getHealthOfLivingRaiders(world);
    m_cachedProgress = std::clamp(livingHealth / m_totalHealth, 0.0f, 1.0f);
}

/**
 * @brief 计算当前所有存活袭击者的总血量。
 *
 * @param world 所属世界。
 * @return 当前追踪的袭击者血量之和；实体不存在或非 LivingEntity 时记为 0。
 *
 * @note 对应 Java 版 1.21.11 Raid#getHealthOfLivingRaiders()。
 *       调用方需保证在主线程访问，避免与实体增删产生竞争。
 */
f32 Raid::_getHealthOfLivingRaiders(IWorld& world) const
{
    f32 total = 0.0f;
    for (const EntityId id : m_raiders) {
        const Entity* const entity = world.getEntity(id);
        if (entity == nullptr) {
            continue;
        }
        const auto* living = dynamic_cast<const LivingEntity*>(entity);
        if (living == nullptr) {
            continue;
        }
        // 实体虽然仍被 Raid 追踪，但可能血量已降至 0（处于死亡过渡阶段），
        // 此时不计入存活血量，与 Java 版 Raider#getHealth() 行为一致。
        if (living->health() > 0.0f) {
            total += living->health();
        }
    }
    return total;
}

/**
 * @brief 显式生成指定波次。
 *
 * @param world 所属世界。
 * @param waveNum 波次编号。
 */
void Raid::_spawnWave(IWorld& world, i32 waveNum)
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
i32 Raid::_calculateRaidersForWave(i32 waveNum, Difficulty difficulty) const
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
RaiderType Raid::_selectRaiderType(i32 waveNum, i32 index, i32 total) const
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
EntityId Raid::_spawnRaider(IWorld& world, RaiderType type, BlockPos pos)
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

    // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化
    auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
    if (mobEntity != nullptr) {
        entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(world, pos);
        mobEntity->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Event);
    }

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
std::optional<BlockPos> Raid::_findSpawnPosition(IWorld& world) const
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
