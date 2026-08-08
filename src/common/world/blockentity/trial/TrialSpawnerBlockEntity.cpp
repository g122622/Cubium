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

#include "TrialSpawnerBlockEntity.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/ray/Ray.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// ============================================================================
// 配置工厂方法
// ============================================================================

TrialSpawnerBlockEntity::Config TrialSpawnerBlockEntity::getBreezeConfig()
{
    Config config;
    config.baseTotalMobs = 2;
    config.baseSimultaneousMobs = 1;
    config.ticksBetweenSpawn = 20;
    config.supplyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/consumables");
    config.keyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/key");
    config.ominousSupplyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/consumables");
    config.ominousKeyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/key");
    config.spawnPotentials = {
        {ResourceLocation("minecraft", "breeze"), 1},
    };
    return config;
}

TrialSpawnerBlockEntity::Config TrialSpawnerBlockEntity::getMeleeConfig()
{
    Config config;
    config.baseTotalMobs = 6;
    config.baseSimultaneousMobs = 3;
    config.ticksBetweenSpawn = 40;
    config.supplyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/consumables");
    config.keyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/key");
    config.ominousSupplyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/consumables");
    config.ominousKeyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/key");
    config.spawnPotentials = {
        {ResourceLocation("minecraft", "zombie"), 1},
        {ResourceLocation("minecraft", "husk"), 1},
        {ResourceLocation("minecraft", "spider"), 1},
    };
    return config;
}

TrialSpawnerBlockEntity::Config TrialSpawnerBlockEntity::getSmallMeleeConfig()
{
    Config config;
    config.baseTotalMobs = 12;
    config.baseSimultaneousMobs = 4;
    config.ticksBetweenSpawn = 20;
    config.supplyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/consumables");
    config.keyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/key");
    config.ominousSupplyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/consumables");
    config.ominousKeyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/key");
    config.spawnPotentials = {
        {ResourceLocation("minecraft", "silverfish"), 2},
        {ResourceLocation("minecraft", "cave_spider"), 2},
        {ResourceLocation("minecraft", "slime"), 1},
    };
    return config;
}

TrialSpawnerBlockEntity::Config TrialSpawnerBlockEntity::getRangedConfig()
{
    Config config;
    config.baseTotalMobs = 6;
    config.baseSimultaneousMobs = 3;
    config.ticksBetweenSpawn = 40;
    config.supplyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/consumables");
    config.keyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/key");
    config.ominousSupplyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/consumables");
    config.ominousKeyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/key");
    config.spawnPotentials = {
        {ResourceLocation("minecraft", "skeleton"), 1},
        {ResourceLocation("minecraft", "stray"), 1},
        {ResourceLocation("minecraft", "bogged"), 1},
    };
    return config;
}

TrialSpawnerBlockEntity::Config TrialSpawnerBlockEntity::getSlowRangedConfig()
{
    Config config;
    config.baseTotalMobs = 6;
    config.baseSimultaneousMobs = 3;
    config.ticksBetweenSpawn = 80;
    config.supplyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/consumables");
    config.keyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/key");
    config.ominousSupplyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/consumables");
    config.ominousKeyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/key");
    config.spawnPotentials = {
        {ResourceLocation("minecraft", "skeleton"), 1},
        {ResourceLocation("minecraft", "stray"), 1},
        {ResourceLocation("minecraft", "bogged"), 1},
    };
    return config;
}

// ============================================================================
// 构造函数
// ============================================================================

TrialSpawnerBlockEntity::TrialSpawnerBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::TrialSpawner, pos)
    , m_config(getMeleeConfig()) // 默认近战配置
{}

// ============================================================================
// BlockEntity 接口
// ============================================================================

void TrialSpawnerBlockEntity::tick(IWorld& world)
{
    switch (m_state) {
        case State::Inactive:
            tickInactive(world);
            break;
        case State::WaitingForPlayers:
            tickWaitingForPlayers(world);
            break;
        case State::Active:
            tickActive(world);
            break;
        case State::WaitingForRewardEjection:
            tickWaitingForRewardEjection(world);
            break;
        case State::EjectingReward:
            tickEjectingReward(world);
            break;
        case State::Cooldown:
            tickCooldown(world);
            break;
    }
}

bool TrialSpawnerBlockEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    if (data.contains("state")) {
        m_state = static_cast<State>(data["state"].get<i32>());
    }
    if (data.contains("ominous")) {
        m_ominous = data["ominous"].get<bool>();
    }
    if (data.contains("cooldown_ends_at")) {
        m_cooldownEndsAt = data["cooldown_ends_at"].get<i64>();
    }
    if (data.contains("ejecting_reward_ends_at")) {
        m_ejectingRewardEndsAt = data["ejecting_reward_ends_at"].get<i64>();
    }
    if (data.contains("spawned_mobs_count")) {
        m_spawnedMobsCount = data["spawned_mobs_count"].get<i32>();
    }
    if (data.contains("tracked_players")) {
        m_trackedPlayers.clear();
        for (const auto& uuid : data["tracked_players"]) {
            m_trackedPlayers.insert(uuid.get<std::string>());
        }
    }
    if (data.contains("tracked_mobs")) {
        m_trackedMobs.clear();
        for (const auto& uuid : data["tracked_mobs"]) {
            m_trackedMobs.insert(uuid.get<std::string>());
        }
    }
    if (data.contains("current_mobs_count")) {
        m_currentMobsCount = data["current_mobs_count"].get<i32>();
    }
    if (data.contains("total_mobs_to_spawn")) {
        m_totalMobsToSpawn = data["total_mobs_to_spawn"].get<i32>();
    }
    if (data.contains("max_simultaneous_mobs")) {
        m_maxSimultaneousMobs = data["max_simultaneous_mobs"].get<i32>();
    }

    return true;
}

void TrialSpawnerBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    data["state"] = static_cast<i32>(m_state);
    data["ominous"] = m_ominous;
    data["cooldown_ends_at"] = m_cooldownEndsAt;
    data["ejecting_reward_ends_at"] = m_ejectingRewardEndsAt;
    data["spawned_mobs_count"] = m_spawnedMobsCount;

    nlohmann::json trackedPlayersArray = nlohmann::json::array();
    for (const auto& uuid : m_trackedPlayers) {
        trackedPlayersArray.push_back(uuid);
    }
    data["tracked_players"] = trackedPlayersArray;

    nlohmann::json trackedMobsArray = nlohmann::json::array();
    for (const auto& uuid : m_trackedMobs) {
        trackedMobsArray.push_back(uuid);
    }
    data["tracked_mobs"] = trackedMobsArray;

    data["current_mobs_count"] = m_currentMobsCount;
    data["total_mobs_to_spawn"] = m_totalMobsToSpawn;
    data["max_simultaneous_mobs"] = m_maxSimultaneousMobs;
}

std::unique_ptr<BlockEntity> TrialSpawnerBlockEntity::clone() const
{
    auto copy = std::make_unique<TrialSpawnerBlockEntity>(m_pos);
    copy->m_state = m_state;
    copy->m_ominous = m_ominous;
    copy->m_config = m_config;
    copy->m_cooldownEndsAt = m_cooldownEndsAt;
    copy->m_ejectingRewardEndsAt = m_ejectingRewardEndsAt;
    copy->m_lastSpawnTick = m_lastSpawnTick;
    copy->m_lastPlayerScanTick = m_lastPlayerScanTick;
    copy->m_lastEjectionTick = m_lastEjectionTick;
    copy->m_spawnedMobsCount = m_spawnedMobsCount;
    copy->m_currentMobsCount = m_currentMobsCount;
    copy->m_totalMobsToSpawn = m_totalMobsToSpawn;
    copy->m_maxSimultaneousMobs = m_maxSimultaneousMobs;
    copy->m_trackedPlayers = m_trackedPlayers;
    copy->m_trackedMobs = m_trackedMobs;
    copy->m_detectedPlayerUuids = m_detectedPlayerUuids;
    return copy;
}

// ============================================================================
// 状态设置
// ============================================================================

void TrialSpawnerBlockEntity::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        setChanged();
    }
}

void TrialSpawnerBlockEntity::setOminous(bool ominous)
{
    if (m_ominous != ominous) {
        m_ominous = ominous;
        setChanged();
    }
}

void TrialSpawnerBlockEntity::setConfig(const Config& config)
{
    m_config = config;
    setChanged();
}

// ============================================================================
// 玩家检测
// ============================================================================

std::vector<Player*> TrialSpawnerBlockEntity::detectPlayers(IWorld& world, f32 range)
{
    std::vector<Player*> result;

    // 获取范围内所有实体
    Vector3 center = m_pos.center();
    auto entities = world.getEntitiesInRange(center, range);

    for (auto* entity : entities) {
        // 只筛选玩家
        auto* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        // 排除旁观者模式的玩家
        // 注意：试炼刷怪笼应排除创造模式和旁观者，目前先排除旁观者
        // 创造模式玩家不参与怪物计数，但仍然可以触发不祥变体
        if (player->isSpectator()) {
            continue;
        }

        result.push_back(player);
    }

    return result;
}

// ============================================================================
// 红石比较器
// ============================================================================

i32 TrialSpawnerBlockEntity::getComparatorOutput() const
{
    switch (m_state) {
        case State::Inactive:
            return 0;
        case State::WaitingForPlayers:
            return 1;
        case State::Active:
            return 2;
        case State::WaitingForRewardEjection:
            return 3;
        case State::EjectingReward:
        case State::Cooldown:
            return 4;
        default:
            return 0;
    }
}

// ============================================================================
// 不祥变体
// ============================================================================

void TrialSpawnerBlockEntity::applyOminous(Player& player)
{
    // 检查玩家是否持有不祥之兆效果
    const auto* badOmen = player.getEffect(entity::effect::EffectType::BadOmen);
    if (badOmen == nullptr) {
        return;
    }

    // 消耗不祥之兆
    i32 level = badOmen->getEffectLevel();
    player.removeEffect(entity::effect::EffectType::BadOmen);

    // 给予试炼之兆效果（持续 level * TRIAL_OMEN_PER_BAD_OMEN_LEVEL ticks）
    player.addEffect(entity::effect::EffectInstance::trialOmen(level));

    // 转为不祥变体
    setOminous(true);

    // 重新计算怪物数量（基于当前追踪的玩家）
    auto players = detectPlayers(*m_world, m_config.detectionRange);
    i32 playerCount = static_cast<i32>(players.size());
    i32 additionalPlayers = std::max(0, playerCount - 1);
    m_totalMobsToSpawn = calculateTargetTotalMobs(additionalPlayers);
    m_maxSimultaneousMobs = calculateTargetSimultaneousMobs(additionalPlayers);

    // 重置已生成怪物计数
    m_spawnedMobsCount = 0;
    m_currentMobsCount = 0;
    m_trackedMobs.clear();
}

// ============================================================================
// 状态机实现
// ============================================================================

void TrialSpawnerBlockEntity::tickInactive(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    // 控制检测频率
    if (currentTick - m_lastPlayerScanTick < PLAYER_SCAN_INTERVAL) {
        return;
    }
    m_lastPlayerScanTick = currentTick;

    // 闲置状态：检测玩家进入范围
    auto players = detectPlayers(world, m_config.detectionRange);
    if (!players.empty()) {
        // 检查是否有不祥之兆的玩家
        for (auto* player : players) {
            if (player->hasEffect(entity::effect::EffectType::BadOmen)) {
                applyOminous(*player);
                break;
            }
        }

        // 计算需要的怪物数
        i32 playerCount = static_cast<i32>(players.size());
        i32 additionalPlayers = std::max(0, playerCount - 1);
        if (m_ominous) {
            m_totalMobsToSpawn = m_config.baseTotalMobs + m_config.totalMobsAddedPerPlayer * additionalPlayers;
            m_maxSimultaneousMobs =
                m_config.baseSimultaneousMobs + m_config.simultaneousMobsAddedPerPlayer * additionalPlayers;
        } else {
            m_totalMobsToSpawn = m_config.baseTotalMobs;
            m_maxSimultaneousMobs = m_config.baseSimultaneousMobs;
        }

        // 记录追踪玩家
        m_trackedPlayers.clear();
        for (auto* player : players) {
            m_trackedPlayers.insert(player->uuid());
        }

        // 新检测到玩家时，延迟至少 DETECT_PLAYER_SPAWN_BUFFER tick 才开始生成
        m_lastSpawnTick = currentTick + DETECT_PLAYER_SPAWN_BUFFER;

        setState(State::WaitingForPlayers);
    }
}

void TrialSpawnerBlockEntity::tickWaitingForPlayers(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    // 控制检测频率
    if (currentTick - m_lastPlayerScanTick < PLAYER_SCAN_INTERVAL) {
        return;
    }
    m_lastPlayerScanTick = currentTick;

    // 等待玩家：确认玩家仍在范围内，然后激活
    auto players = detectPlayers(world, m_config.detectionRange);
    if (!players.empty()) {
        // 更新追踪的玩家列表
        m_trackedPlayers.clear();
        for (auto* player : players) {
            m_trackedPlayers.insert(player->uuid());
        }
        setState(State::Active);
    } else {
        // 没有玩家了，回到闲置
        m_trackedPlayers.clear();
        m_trackedMobs.clear();
        m_spawnedMobsCount = 0;
        m_currentMobsCount = 0;
        setState(State::Inactive);
    }
}

void TrialSpawnerBlockEntity::tickActive(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    // 更新追踪的怪物
    updateTrackedMobs(world);

    // 检查是否所有怪物已生成且已被击杀
    if (hasFinishedSpawningAllMobs(std::max(0, static_cast<i32>(m_trackedPlayers.size()) - 1)) &&
        haveAllCurrentMobsDied()) {
        // 所有怪物已被击杀，进入奖励弹出阶段
        m_cooldownEndsAt = currentTick + m_config.cooldownTicks;
        m_spawnedMobsCount = 0;
        m_lastSpawnTick = 0;

        // 收集当前检测到的玩家UUID用于弹出奖励
        auto players = detectPlayers(world, m_config.detectionRange);
        m_detectedPlayerUuids.clear();
        for (auto* player : players) {
            m_detectedPlayerUuids.push_back(player->uuid());
        }

        setState(State::WaitingForRewardEjection);
        return;
    }

    // 定期检测玩家更新
    if (currentTick - m_lastPlayerScanTick >= PLAYER_SCAN_INTERVAL) {
        m_lastPlayerScanTick = currentTick;
        auto players = detectPlayers(world, m_config.detectionRange);

        // 更新追踪的玩家列表
        m_trackedPlayers.clear();
        for (auto* player : players) {
            m_trackedPlayers.insert(player->uuid());
        }
    }

    // 检查是否可以生成新怪物
    i32 additionalPlayers = std::max(0, static_cast<i32>(m_trackedPlayers.size()) - 1);
    if (isReadyToSpawnNextMob(world, additionalPlayers)) {
        spawnMob(world);
        m_lastSpawnTick = currentTick;
    }
}

void TrialSpawnerBlockEntity::tickWaitingForRewardEjection(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    // 等待一小段时间后进入弹出状态
    if (currentTick >= m_cooldownEndsAt - m_config.cooldownTicks + 40) {
        // 播放打开百叶窗音效
        world.playSound(ResourceLocation("minecraft", "block.trial_spawner.open_shutter"),
            sound::SoundCategory::Blocks,
            m_pos.center(),
            1.0f,
            1.0f);

        setState(State::EjectingReward);
        m_lastEjectionTick = currentTick;
    }
}

void TrialSpawnerBlockEntity::tickEjectingReward(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    // 每 TIME_BETWEEN_EJECTIONS tick 弹出一次奖励
    if (currentTick - m_lastEjectionTick < TIME_BETWEEN_EJECTIONS) {
        return;
    }

    if (m_detectedPlayerUuids.empty()) {
        // 所有玩家奖励弹完
        world.playSound(ResourceLocation("minecraft", "block.trial_spawner.close_shutter"),
            sound::SoundCategory::Blocks,
            m_pos.center(),
            1.0f,
            1.0f);

        setState(State::Cooldown);
        m_cooldownEndsAt = currentTick + m_config.cooldownTicks;
        return;
    }

    // 弹出奖励给下一个玩家
    std::string playerUuid = m_detectedPlayerUuids.front();
    m_detectedPlayerUuids.erase(m_detectedPlayerUuids.begin());

    // 查找玩家
    Player* player = nullptr;
    {
        // 使用 getEntityByUuid() 进行 O(1) UUID 查找，替代遍历玩家列表
        Entity* entity = world.getEntityByUuid(playerUuid);
        if (entity != nullptr) {
            player = dynamic_cast<Player*>(entity);
        }
    }
    if (player != nullptr) {
        ejectRewardForPlayer(world, *player);
    }

    m_lastEjectionTick = currentTick;
}

void TrialSpawnerBlockEntity::tickCooldown(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    // 冷却期间也检测玩家
    if (currentTick - m_lastPlayerScanTick >= PLAYER_SCAN_INTERVAL) {
        m_lastPlayerScanTick = currentTick;
        auto players = detectPlayers(world, m_config.detectionRange);

        if (!players.empty()) {
            // 冷却中检测到新玩家，直接跳回 ACTIVE
            i32 playerCount = static_cast<i32>(players.size());
            i32 additionalPlayers = std::max(0, playerCount - 1);

            // 更新追踪玩家
            m_trackedPlayers.clear();
            for (auto* player : players) {
                m_trackedPlayers.insert(player->uuid());
            }

            // 重新计算怪物数
            if (m_ominous) {
                m_totalMobsToSpawn = m_config.baseTotalMobs + m_config.totalMobsAddedPerPlayer * additionalPlayers;
                m_maxSimultaneousMobs =
                    m_config.baseSimultaneousMobs + m_config.simultaneousMobsAddedPerPlayer * additionalPlayers;
            } else {
                m_totalMobsToSpawn = m_config.baseTotalMobs;
                m_maxSimultaneousMobs = m_config.baseSimultaneousMobs;
            }

            m_spawnedMobsCount = 0;
            m_currentMobsCount = 0;
            m_trackedMobs.clear();
            m_lastSpawnTick = currentTick + DETECT_PLAYER_SPAWN_BUFFER;

            setState(State::Active);
            return;
        }
    }

    if (currentTick >= m_cooldownEndsAt) {
        // 冷却结束，重置状态
        m_spawnedMobsCount = 0;
        m_currentMobsCount = 0;
        m_trackedMobs.clear();
        m_trackedPlayers.clear();
        setOminous(false);
        setState(State::WaitingForPlayers);
    }
}

// ============================================================================
// 生成和奖励逻辑
// ============================================================================

// ============================================================================
// 生成逻辑辅助方法
// ============================================================================

const ResourceLocation* TrialSpawnerBlockEntity::_selectNextEntity(IWorld& world)
{
    // 如果有缓存的下次生成实体，直接使用
    if (!m_nextSpawnEntityId.path().empty()) {
        return &m_nextSpawnEntityId;
    }

    // 从生成潜力列表中加权随机选择
    const auto& potentials = m_config.spawnPotentials;
    if (potentials.empty()) {
        return nullptr;
    }

    // 计算总权重
    i32 totalWeight = 0;
    for (const auto& entry : potentials) {
        totalWeight += entry.weight;
    }

    if (totalWeight <= 0) {
        return nullptr;
    }

    // 加权随机选择
    i32 roll = world.getRandom().nextInt(totalWeight);
    i32 cumulative = 0;
    for (const auto& entry : potentials) {
        cumulative += entry.weight;
        if (roll < cumulative) {
            m_nextSpawnEntityId = entry.entityId;
            return &m_nextSpawnEntityId;
        }
    }

    // 不应到达此处，但以防万一返回最后一个
    m_nextSpawnEntityId = potentials.back().entityId;
    return &m_nextSpawnEntityId;
}

std::optional<Vector3> TrialSpawnerBlockEntity::_findSpawnPosition(IWorld& world)
{
    // 参考 MC Java TrialSpawner.spawnMob()：在 spawnRange 范围内随机选择位置
    // X: [pos.x + 0.5 - spawnRange, pos.x + 0.5 + spawnRange]
    // Y: pos.y + {-1, 0, +1}
    // Z: [pos.z + 0.5 - spawnRange, pos.z + 0.5 + spawnRange]
    // 然后检查碰撞

    math::Random& rng = world.getRandom();
    i32 spawnRange = static_cast<i32>(m_config.spawnRange);

    // 最多尝试 20 次寻找合适的位置（对应 MC Java 的 SpawnEggItem/SpawnerLogic 最大尝试次数）
    constexpr i32 MAX_SPAWN_ATTEMPTS = 20;

    for (i32 attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; ++attempt) {
        // 随机偏移位置
        f32 x = static_cast<f32>(m_pos.x) + 0.5f + static_cast<f32>(rng.nextInt(spawnRange * 2 + 1) - spawnRange);
        i32 yOff = rng.nextInt(3) - 1; // -1, 0, +1
        f32 y = static_cast<f32>(m_pos.y + yOff);
        f32 z = static_cast<f32>(m_pos.z) + 0.5f + static_cast<f32>(rng.nextInt(spawnRange * 2 + 1) - spawnRange);

        Vector3 spawnPos(x, y, z);

        // 检查碰撞：生成位置处不能有方块碰撞
        // 使用典型生物碰撞箱大小（宽0.6 高1.8）进行碰撞检测
        constexpr f32 ENTITY_HALF_WIDTH = 0.3f;
        constexpr f32 ENTITY_HEIGHT = 1.8f;
        AxisAlignedBB spawnBox(x - ENTITY_HALF_WIDTH,
            y,
            z - ENTITY_HALF_WIDTH,
            x + ENTITY_HALF_WIDTH,
            y + ENTITY_HEIGHT,
            z + ENTITY_HALF_WIDTH);

        if (world.hasBlockCollision(spawnBox)) {
            continue;
        }

        // 视线检测：从生成位置向刷怪笼中心发射射线，
        // 如果射线未命中任何方块（视线畅通），或仅命中刷怪笼方块本身，则允许生成。
        // 参考 MC Java TrialSpawner.inLineOfSight()：
        //   使用 ClipContext.VISUAL 进行视线追踪，raycastBlocks 使用 getShape()
        //   （等效于 MC 的 VISUAL 模式，因为大多数方块的视觉形状等于碰撞形状），
        //   且自动跳过空气和液体，对应 MC 的 ClipContext.Fluid.NONE。
        Vector3 spawnerCenter = m_pos.center();
        Vector3 delta = spawnerCenter - spawnPos;
        f32 distance = delta.length();
        Vector3 direction = delta.normalized();

        Ray ray(spawnPos, direction);
        RaycastContext context(ray, distance);
        BlockRaycastResult result = raycastBlocks(context, world);

        // 视线不通过的条件：射线命中了非刷怪笼方块的某个方块
        bool lineOfSight = result.isMiss() || result.blockPos() == m_pos;
        if (!lineOfSight) {
            continue;
        }

        return spawnPos;
    }

    // 所有尝试都未找到合适位置
    return std::nullopt;
}

void TrialSpawnerBlockEntity::spawnMob(IWorld& world)
{
    // 1. 选择要生成的实体类型
    const ResourceLocation* entityId = _selectNextEntity(world);
    if (entityId == nullptr) {
        return;
    }

    // 2. 查找实体类型
    const entity::EntityType* entityType = entity::EntityRegistry::instance().getType(entityId->toString());
    if (entityType == nullptr || !entityType->canSummon()) {
        // 实体类型未注册或不可生成，清除缓存后放弃
        m_nextSpawnEntityId = ResourceLocation();
        return;
    }

    // 3. 寻找合适的生成位置
    auto spawnPosOpt = _findSpawnPosition(world);
    if (!spawnPosOpt.has_value()) {
        // 没有找到合适的位置，不消耗本次生成机会
        return;
    }
    Vector3 spawnPos = spawnPosOpt.value();
    BlockPos spawnBlockPos(static_cast<i32>(std::floor(spawnPos.x)),
        static_cast<i32>(std::floor(spawnPos.y)),
        static_cast<i32>(std::floor(spawnPos.z)));

    // 4. 创建实体
    // 通过世界获取 ECS 实体注册表（ServerWorld 持有 m_entityRegistry）
    auto* registry = world.entityRegistry();
    if (registry == nullptr) {
        m_nextSpawnEntityId = ResourceLocation();
        return;
    }
    auto entity = entityType->create(&world, *registry);
    if (entity == nullptr) {
        m_nextSpawnEntityId = ResourceLocation();
        return;
    }

    // 5. 设置位置和旋转
    f32 yaw = world.getRandom().nextFloat() * 360.0f;
    entity->setPosition(spawnPos);
    entity->setRotation(yaw, 0.0f);

    // 6. 对 MobEntity 进行初始化
    auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
    if (mobEntity != nullptr) {
        // 设置区域难度并初始化
        entity::combat::DifficultyInstance difficulty = entity::combat::DifficultyInstance::at(world, spawnBlockPos);
        mobEntity->finalizeSpawn(world, difficulty, world::spawn::SpawnReason::Spawner);

        // 试炼刷怪笼生成的怪物不会自然消失
        mobEntity->enablePersistence();
    }

    // 7. 记录 UUID（必须在 spawnEntity 之前，因为 spawnEntity 会转移所有权）
    std::string entityUuid = entity->uuid();

    // 8. 添加实体到世界
    EntityInstanceId id = world.spawnEntity(std::move(entity));
    if (id == INVALID_ENTITY_ID) {
        // 生成失败
        m_nextSpawnEntityId = ResourceLocation();
        return;
    }

    // 9. 追踪生成的怪物
    m_trackedMobs.insert(entityUuid);
    m_spawnedMobsCount++;
    m_currentMobsCount++;

    // 10. 为下次生成随机选择新的实体类型
    m_nextSpawnEntityId = ResourceLocation();
    if (!m_config.spawnPotentials.empty()) {
        i32 totalWeight = 0;
        for (const auto& entry : m_config.spawnPotentials) {
            totalWeight += entry.weight;
        }
        if (totalWeight > 0) {
            i32 roll = world.getRandom().nextInt(totalWeight);
            i32 cumulative = 0;
            for (const auto& entry : m_config.spawnPotentials) {
                cumulative += entry.weight;
                if (roll < cumulative) {
                    m_nextSpawnEntityId = entry.entityId;
                    break;
                }
            }
        }
    }

    // 11. 播放生成效果
    // 参考 MC Java: levelEvent(3011, spawnerPos, flameParticle) + levelEvent(3012, mobPos, flameParticle)
    // flameParticle: 0=普通(FLAME), 1=不祥(SOUL_FIRE_FLAME)
    i32 flameParticle = m_ominous ? 1 : 0;
    world.playEvent(world::WorldEvents::TRIAL_SPAWNER_SPAWN, m_pos, flameParticle);
    world.playEvent(world::WorldEvents::TRIAL_SPAWNER_SPAWN_MOB_AT, spawnBlockPos, flameParticle);

    setChanged();
}

void TrialSpawnerBlockEntity::ejectRewardForPlayer(IWorld& world, Player& player)
{
    // 获取战利品表管理器
    const auto* ltm = world.lootTableManager();
    if (ltm == nullptr) {
        return;
    }

    // 50%概率选择补给表，50%概率选择钥匙表
    // 不祥变体：70%概率补给，30%概率钥匙
    bool isSupply = world.getRandom().nextFloat() < (m_ominous ? 0.7f : 0.5f);

    std::string lootTableId;
    if (isSupply) {
        lootTableId = m_ominous ? m_config.ominousSupplyLootTable.toString() : m_config.supplyLootTable.toString();
    } else {
        lootTableId = m_ominous ? m_config.ominousKeyLootTable.toString() : m_config.keyLootTable.toString();
    }

    const auto* lootTable = ltm->getTable(lootTableId);
    if (lootTable == nullptr) {
        return;
    }

    // 构建战利品上下文
    auto* playerEntity = static_cast<Entity*>(&player);
    auto context =
        loot::LootContextBuilder(world)
            .withRandom(world.getRandom())
            .withParameter(loot::LootParams::THIS_ENTITY, playerEntity)
            .withLootTableResolver([ltm](const std::string& id) -> const loot::LootTable* { return ltm->getTable(id); })
            .withPredicateResolver(
                [ltm](const std::string& id) -> const loot::LootCondition* { return ltm->getPredicate(id); })
            .build(loot::LootParameterSets::chest());

    if (context == nullptr) {
        return;
    }

    // 生成物品列表
    auto drops = lootTable->generate(*context);
    if (drops.empty()) {
        return;
    }

    // 弹出所有物品到世界中，方向为UP，速度为2
    f64 x = static_cast<f64>(m_pos.x) + 0.5;
    f64 y = static_cast<f64>(m_pos.y) + 1.2;
    f64 z = static_cast<f64>(m_pos.z) + 0.5;

    for (const auto& item : drops) {
        if (!item.isEmpty()) {
            ItemDropHelper::spawnItemEntity(&world,
                item,
                x,
                y,
                z,
                0.0f,
                2.0f,
                0.0f,           // 向上弹出
                10,             // pickupDelay
                player.uuid()); // owner
        }
    }

    // 播放弹出音效
    world.playSound(ResourceLocation("minecraft", "block.trial_spawner.eject_item"),
        sound::SoundCategory::Blocks,
        m_pos.center(),
        1.0f,
        1.0f);

    setChanged();
}

void TrialSpawnerBlockEntity::ejectReward(IWorld& world)
{
    // 无指定玩家时使用空上下文生成物品
    const auto* ltm = world.lootTableManager();
    if (ltm == nullptr) {
        return;
    }

    bool isSupply = world.getRandom().nextFloat() < (m_ominous ? 0.7f : 0.5f);
    std::string lootTableId;
    if (isSupply) {
        lootTableId = m_ominous ? m_config.ominousSupplyLootTable.toString() : m_config.supplyLootTable.toString();
    } else {
        lootTableId = m_ominous ? m_config.ominousKeyLootTable.toString() : m_config.keyLootTable.toString();
    }

    const auto* lootTable = ltm->getTable(lootTableId);
    if (lootTable == nullptr) {
        return;
    }

    auto context =
        loot::LootContextBuilder(world)
            .withRandom(world.getRandom())
            .withLootTableResolver([ltm](const std::string& id) -> const loot::LootTable* { return ltm->getTable(id); })
            .withPredicateResolver(
                [ltm](const std::string& id) -> const loot::LootCondition* { return ltm->getPredicate(id); })
            .build(loot::LootParameterSets::empty());

    if (context == nullptr) {
        return;
    }

    auto drops = lootTable->generate(*context);
    if (drops.empty()) {
        return;
    }

    f64 x = static_cast<f64>(m_pos.x) + 0.5;
    f64 y = static_cast<f64>(m_pos.y) + 1.2;
    f64 z = static_cast<f64>(m_pos.z) + 0.5;

    for (const auto& item : drops) {
        if (!item.isEmpty()) {
            ItemDropHelper::spawnItemEntity(&world,
                item,
                x,
                y,
                z,
                world.getRandom().nextFloat(-0.1f, 0.1f),
                2.0f,
                world.getRandom().nextFloat(-0.1f, 0.1f),
                10,
                "");
        }
    }

    world.playSound(ResourceLocation("minecraft", "block.trial_spawner.eject_item"),
        sound::SoundCategory::Blocks,
        m_pos.center(),
        1.0f,
        1.0f);

    setChanged();
}

void TrialSpawnerBlockEntity::updateTrackedMobs(IWorld& world)
{
    bool changed = false;

    // 遍历追踪的怪物UUID，移除不再存活或离开追踪范围的实体
    auto it = m_trackedMobs.begin();
    while (it != m_trackedMobs.end()) {
        Entity* entity = findEntityByUuid(world, *it);
        bool shouldRemove = false;

        if (entity == nullptr) {
            // 实体不存在（已卸载或移除）
            shouldRemove = true;
        } else if (!entity->isAlive()) {
            // 实体已死亡
            shouldRemove = true;
        } else {
            // 检查距离是否超过追踪范围
            Vector3 entityPos = entity->position();
            Vector3 spawnerPos = m_pos.center();
            f32 distSq = entityPos.distanceSquared(spawnerPos);
            if (distSq > MAX_MOB_TRACKING_DISTANCE * MAX_MOB_TRACKING_DISTANCE) {
                shouldRemove = true;
            }
        }

        if (shouldRemove) {
            it = m_trackedMobs.erase(it);
            m_currentMobsCount = std::max(0, m_currentMobsCount - 1);
            changed = true;
        } else {
            ++it;
        }
    }

    if (changed) {
        // 追踪的怪物变化时，延迟下次生成以给予缓冲时间
        i64 currentTick = static_cast<i64>(world.currentTick());
        m_lastSpawnTick = std::max(currentTick + m_config.ticksBetweenSpawn, m_lastSpawnTick);
        setChanged();
    }
}

// ============================================================================
// 辅助方法
// ============================================================================

Entity* TrialSpawnerBlockEntity::findEntityByUuid(IWorld& world, const std::string& uuid)
{
    // 使用 IWorld::getEntityByUuid() 进行 O(1) UUID 查找，
    // 替代 getEntitiesInRange + 遍历比对 UUID 的 O(n) 模式。
    return world.getEntityByUuid(uuid);
}

i32 TrialSpawnerBlockEntity::calculateTargetTotalMobs(i32 additionalPlayers) const
{
    return m_config.baseTotalMobs + m_config.totalMobsAddedPerPlayer * additionalPlayers;
}

i32 TrialSpawnerBlockEntity::calculateTargetSimultaneousMobs(i32 additionalPlayers) const
{
    return m_config.baseSimultaneousMobs + m_config.simultaneousMobsAddedPerPlayer * additionalPlayers;
}

bool TrialSpawnerBlockEntity::hasFinishedSpawningAllMobs(i32 additionalPlayers) const
{
    return m_spawnedMobsCount >= calculateTargetTotalMobs(additionalPlayers);
}

bool TrialSpawnerBlockEntity::haveAllCurrentMobsDied() const
{
    return m_currentMobsCount <= 0;
}

bool TrialSpawnerBlockEntity::isReadyToSpawnNextMob(IWorld& world, i32 additionalPlayers) const
{
    i64 currentTick = static_cast<i64>(world.currentTick());
    return m_spawnedMobsCount < calculateTargetTotalMobs(additionalPlayers) &&
        m_currentMobsCount < calculateTargetSimultaneousMobs(additionalPlayers) &&
        currentTick - m_lastSpawnTick >= m_config.ticksBetweenSpawn;
}

} // namespace mc
