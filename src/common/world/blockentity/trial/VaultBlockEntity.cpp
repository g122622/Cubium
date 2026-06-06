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

#include "VaultBlockEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

// ============================================================================
// 配置工厂方法
// ============================================================================

VaultBlockEntity::Config VaultBlockEntity::getNormalConfig()
{
    Config config;
    config.lootTable = ResourceLocation("minecraft", "chests/trial_chambers/reward");
    config.ominousLootTable = ResourceLocation("minecraft", "chests/trial_chambers/reward_ominous");
    config.activationRange = 4.0f;
    config.deactivationRange = 4.5f;
    config.keyItem = Items::TRIAL_KEY;
    return config;
}

VaultBlockEntity::Config VaultBlockEntity::getOminousConfig()
{
    Config config;
    config.lootTable = ResourceLocation("minecraft", "chests/trial_chambers/reward_ominous");
    config.ominousLootTable = ResourceLocation("minecraft", "chests/trial_chambers/reward_ominous");
    config.activationRange = 4.0f;
    config.deactivationRange = 4.5f;
    config.keyItem = Items::OMINOUS_TRIAL_KEY;
    return config;
}

// ============================================================================
// 构造函数
// ============================================================================

VaultBlockEntity::VaultBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Vault, pos)
    , m_config(getNormalConfig())
{}

// ============================================================================
// BlockEntity 接口
// ============================================================================

void VaultBlockEntity::tick(IWorld& world)
{
    switch (m_state) {
        case State::Inactive:
            tickInactive(world);
            break;
        case State::Active:
            tickActive(world);
            break;
        case State::Unlocking:
            tickUnlocking(world);
            break;
        case State::Ejecting:
            tickEjecting(world);
            break;
    }
}

bool VaultBlockEntity::load(const nlohmann::json& data)
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
    if (data.contains("rewarded_players")) {
        m_rewardedPlayers.clear();
        for (const auto& uuid : data["rewarded_players"]) {
            m_rewardedPlayers.insert(uuid.get<std::string>());
        }
    }

    return true;
}

void VaultBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    data["state"] = static_cast<i32>(m_state);
    data["ominous"] = m_ominous;

    nlohmann::json rewardedArray = nlohmann::json::array();
    for (const auto& uuid : m_rewardedPlayers) {
        rewardedArray.push_back(uuid);
    }
    data["rewarded_players"] = rewardedArray;
}

std::unique_ptr<BlockEntity> VaultBlockEntity::clone() const
{
    auto copy = std::make_unique<VaultBlockEntity>(m_pos);
    copy->m_state = m_state;
    copy->m_ominous = m_ominous;
    copy->m_config = m_config;
    copy->m_rewardedPlayers = m_rewardedPlayers;
    copy->m_unlockingStartTick = m_unlockingStartTick;
    copy->m_ejectingEndTick = m_ejectingEndTick;
    copy->m_unlockingPlayerUuid = m_unlockingPlayerUuid;
    return copy;
}

// ============================================================================
// 状态设置
// ============================================================================

void VaultBlockEntity::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        setChanged();
    }
}

void VaultBlockEntity::setOminous(bool ominous)
{
    if (m_ominous != ominous) {
        m_ominous = ominous;
        m_config = ominous ? getOminousConfig() : getNormalConfig();
        setChanged();
    }
}

void VaultBlockEntity::setConfig(const Config& config)
{
    m_config = config;
    setChanged();
}

// ============================================================================
// 钥匙交互
// ============================================================================

bool VaultBlockEntity::tryInsertKey(Player& player)
{
    // 检查状态
    if (m_state != State::Active) {
        return false;
    }

    // 检查玩家是否已领取过奖励
    if (m_rewardedPlayers.count(player.uuid()) > 0) {
        // TODO(trial_chambers): 播放拒绝音效
        return false;
    }

    // 检查玩家手中是否持有正确的钥匙
    // TODO(trial_chambers): 实现物品检查和消耗
    // if (player.getMainHandItem().getItem() != m_config.keyItem) {
    //     return false;
    // }
    // player.getMainHandItem().shrink(1);

    // 开始解锁
    m_unlockingPlayerUuid = player.uuid();
    m_unlockingStartTick = static_cast<i64>(m_world->currentTick());
    setState(State::Unlocking);

    // TODO(trial_chambers): 播放解锁音效和动画
    return true;
}

// ============================================================================
// 红石比较器
// ============================================================================

i32 VaultBlockEntity::getComparatorOutput() const
{
    switch (m_state) {
        case State::Active:
        case State::Inactive:
            return 0;
        case State::Unlocking:
        case State::Ejecting:
            return 15;
        default:
            return 0;
    }
}

// ============================================================================
// 状态机实现
// ============================================================================

void VaultBlockEntity::tickInactive(IWorld& world)
{
    // 检测附近玩家
    auto players = detectPlayers(world);
    if (!players.empty()) {
        setState(State::Active);
    }
}

void VaultBlockEntity::tickActive(IWorld& world)
{
    // 活跃状态：等待玩家插入钥匙
    // 检测玩家是否离开范围
    auto players = detectPlayers(world);
    if (players.empty()) {
        setState(State::Inactive);
    }

    // TODO(trial_chambers): 显示钥匙物品（客户端渲染用）
}

void VaultBlockEntity::tickUnlocking(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    if (currentTick - m_unlockingStartTick >= UNLOCKING_DURATION) {
        // 解锁完成，进入弹出阶段
        m_ejectingEndTick = currentTick + EJECTING_DURATION;
        setState(State::Ejecting);

        // 弹出奖励
        // TODO(trial_chambers): 查找解锁玩家
        // Player* player = world.getPlayerByUuid(m_unlockingPlayerUuid);
        // if (player != nullptr) {
        //     ejectReward(world, *player);
        //     m_rewardedPlayers.insert(m_unlockingPlayerUuid);
        //     if (m_rewardedPlayers.size() > MAX_REWARDED_PLAYERS) {
        //         // 移除最早的玩家
        //     }
        // }
    }
}

void VaultBlockEntity::tickEjecting(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    if (currentTick >= m_ejectingEndTick) {
        // 弹出完成，回到活跃状态
        m_unlockingPlayerUuid.clear();
        setState(State::Active);
    }
}

// ============================================================================
// 奖励逻辑
// ============================================================================

void VaultBlockEntity::ejectReward(IWorld& world, Player& player)
{
    // TODO(trial_chambers): 实现完整的战利品弹出逻辑
    // 1. 80%概率从稀有表抽1次，20%概率从普通表抽1次
    // 2. 总是从普通表抽1-3次
    // 3. 普通宝库25%概率从独有表抽1次；不祥宝库75%概率
    // 4. 生成物品实体弹出到世界中
    // 5. 播放弹出音效和粒子效果
}

std::vector<Player*> VaultBlockEntity::detectPlayers(IWorld& world)
{
    std::vector<Player*> players;
    // TODO(trial_chambers): 实现范围玩家检测
    // 使用 world.getEntitiesInAABB() 查找 activationRange 范围内的玩家
    return players;
}

} // namespace mc
