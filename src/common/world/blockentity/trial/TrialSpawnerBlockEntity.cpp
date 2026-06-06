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
 * copies of substantial portions of the Software.
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
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"

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
    return config;
}

TrialSpawnerBlockEntity::Config TrialSpawnerBlockEntity::getSlowRangedConfig()
{
    Config config;
    config.baseTotalMobs = 6;
    config.baseSimultaneousMobs = 3;
    config.ticksBetweenSpawn = 80; // 低频率
    config.supplyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/consumables");
    config.keyLootTable = ResourceLocation("minecraft", "spawners/trial_chamber/key");
    config.ominousSupplyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/consumables");
    config.ominousKeyLootTable = ResourceLocation("minecraft", "spawners/ominous/trial_chamber/key");
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
}

std::unique_ptr<BlockEntity> TrialSpawnerBlockEntity::clone() const
{
    auto copy = std::make_unique<TrialSpawnerBlockEntity>(m_pos);
    copy->m_state = m_state;
    copy->m_ominous = m_ominous;
    copy->m_config = m_config;
    copy->m_cooldownEndsAt = m_cooldownEndsAt;
    copy->m_ejectingRewardEndsAt = m_ejectingRewardEndsAt;
    copy->m_spawnedMobsCount = m_spawnedMobsCount;
    copy->m_currentMobsCount = m_currentMobsCount;
    copy->m_totalMobsToSpawn = m_totalMobsToSpawn;
    copy->m_maxSimultaneousMobs = m_maxSimultaneousMobs;
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

std::vector<Player*> TrialSpawnerBlockEntity::detectPlayers(IWorld& world)
{
    std::vector<Player*> players;
    // TODO(trial_chambers): 实现范围玩家检测
    // 使用 world.getEntitiesInAABB() 查找 detectionRange 范围内的玩家
    return players;
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

    // 给予试炼之兆效果
    player.addEffect(entity::effect::EffectInstance::trialOmen(level));

    // 转为不祥变体
    setOminous(true);

    // 更新总怪物数和同时怪物数
    auto players = detectPlayers(*m_world);
    i32 playerCount = static_cast<i32>(players.size());
    m_totalMobsToSpawn = m_config.baseTotalMobs + m_config.totalMobsAddedPerPlayer * playerCount;
    m_maxSimultaneousMobs = m_config.baseSimultaneousMobs + m_config.simultaneousMobsAddedPerPlayer * playerCount;
}

// ============================================================================
// 状态机实现
// ============================================================================

void TrialSpawnerBlockEntity::tickInactive(IWorld& world)
{
    // 闲置状态：检测玩家进入范围
    auto players = detectPlayers(world);
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
        m_totalMobsToSpawn = m_config.baseTotalMobs + (m_ominous ? m_config.totalMobsAddedPerPlayer : 0) * playerCount;
        m_maxSimultaneousMobs =
            m_config.baseSimultaneousMobs + (m_ominous ? m_config.simultaneousMobsAddedPerPlayer : 0) * playerCount;

        // 记录追踪玩家
        for (auto* player : players) {
            m_trackedPlayers.insert(player->uuid());
        }

        setState(State::WaitingForPlayers);
    }
}

void TrialSpawnerBlockEntity::tickWaitingForPlayers(IWorld& world)
{
    // 等待玩家：确认玩家仍在范围内，然后激活
    auto players = detectPlayers(world);
    if (!players.empty()) {
        setState(State::Active);
        m_lastSpawnTick = static_cast<i64>(world.currentTick());
    } else {
        // 没有玩家了，回到闲置
        m_trackedPlayers.clear();
        setState(State::Inactive);
    }
}

void TrialSpawnerBlockEntity::tickActive(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    // 更新追踪的怪物
    updateTrackedMobs(world);

    // 检查是否所有怪物已生成且已被击杀
    if (m_spawnedMobsCount >= m_totalMobsToSpawn && m_currentMobsCount <= 0) {
        setState(State::WaitingForRewardEjection);
        return;
    }

    // 检查是否可以生成新怪物
    if (m_spawnedMobsCount < m_totalMobsToSpawn && m_currentMobsCount < m_maxSimultaneousMobs) {
        if (currentTick - m_lastSpawnTick >= m_config.ticksBetweenSpawn) {
            spawnMob(world);
            m_lastSpawnTick = currentTick;
        }
    }
}

void TrialSpawnerBlockEntity::tickWaitingForRewardEjection(IWorld& world)
{
    // 等待奖励弹出：立即进入弹出状态
    setState(State::EjectingReward);
    m_ejectingRewardEndsAt = static_cast<i64>(world.currentTick()) + m_config.ejectingRewardTicks;
}

void TrialSpawnerBlockEntity::tickEjectingReward(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    // 弹出奖励物品
    ejectReward(world);

    if (currentTick >= m_ejectingRewardEndsAt) {
        setState(State::Cooldown);
        m_cooldownEndsAt = currentTick + m_config.cooldownTicks;
    }
}

void TrialSpawnerBlockEntity::tickCooldown(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    if (currentTick >= m_cooldownEndsAt) {
        // 重置状态
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

void TrialSpawnerBlockEntity::spawnMob(IWorld& world)
{
    // TODO(trial_chambers): 实现怪物生成逻辑
    // 1. 确定生成位置（spawnRange范围内，寻找合适的位置）
    // 2. 根据刷怪笼类型确定生成实体类型
    //    - 近战型: 僵尸/尸壳/蜘蛛 (由池别名决定)
    //    - 小型近战: 史莱姆/洞穴蜘蛛/蠹虫/幼年僵尸
    //    - 远程型: 骷髅/沼骸/流浪者
    //    - 旋风人: 旋风人
    // 3. 生成实体并添加到世界
    // 4. 追踪生成的实体UUID
    // 5. 播放生成音效和粒子效果

    m_spawnedMobsCount++;
    m_currentMobsCount++;
    setChanged();
}

void TrialSpawnerBlockEntity::ejectReward(IWorld& world)
{
    // TODO(trial_chambers): 实现奖励弹出逻辑
    // 1. 50%概率选择补给表，50%概率选择钥匙表
    //    不祥变体：70%概率补给，30%概率钥匙
    // 2. 参与的每个玩家抽取一次
    // 3. 从选定的战利品表生成物品
    // 4. 将物品弹出到世界上
    // 5. 播放弹出音效和粒子效果
}

void TrialSpawnerBlockEntity::updateTrackedMobs(IWorld& world)
{
    // TODO(trial_chambers): 实现怪物追踪更新
    // 1. 遍历m_trackedMobs中的UUID
    // 2. 检查对应实体是否仍然存活
    // 3. 移除已死亡的实体UUID
    // 4. 更新m_currentMobsCount
}

} // namespace mc
