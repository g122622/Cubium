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

#include "VaultBlockEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

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
    if (data.contains("unlocking_start_tick")) {
        m_unlockingStartTick = data["unlocking_start_tick"].get<i64>();
    }
    if (data.contains("ejection_end_tick")) {
        m_ejectionEndTick = data["ejection_end_tick"].get<i64>();
    }
    if (data.contains("unlocking_player_uuid")) {
        m_unlockingPlayerUuid = data["unlocking_player_uuid"].get<std::string>();
    }
    if (data.contains("last_insert_fail_sound_tick")) {
        m_lastInsertFailSoundTick = data["last_insert_fail_sound_tick"].get<i64>();
    }

    // 根据不祥状态更新配置
    if (m_ominous) {
        m_config = getOminousConfig();
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
    data["unlocking_start_tick"] = m_unlockingStartTick;
    data["ejection_end_tick"] = m_ejectionEndTick;
    data["unlocking_player_uuid"] = m_unlockingPlayerUuid;
    data["last_insert_fail_sound_tick"] = m_lastInsertFailSoundTick;
}

std::unique_ptr<BlockEntity> VaultBlockEntity::clone() const
{
    auto copy = std::make_unique<VaultBlockEntity>(m_pos);
    copy->m_state = m_state;
    copy->m_ominous = m_ominous;
    copy->m_config = m_config;
    copy->m_rewardedPlayers = m_rewardedPlayers;
    copy->m_unlockingStartTick = m_unlockingStartTick;
    copy->m_ejectionEndTick = m_ejectionEndTick;
    copy->m_unlockingPlayerUuid = m_unlockingPlayerUuid;
    copy->m_itemsToEject = m_itemsToEject;
    copy->m_totalEjectionsNeeded = m_totalEjectionsNeeded;
    copy->m_lastInsertFailSoundTick = m_lastInsertFailSoundTick;
    copy->m_lastStateUpdateTick = m_lastStateUpdateTick;
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
    // 状态检查：仅非Inactive状态允许操作
    if (m_state == State::Inactive) {
        return false;
    }

    // 检查钥匙物品是否有效
    if (m_config.keyItem == nullptr) {
        return false;
    }

    // 检查玩家手持物品是否匹配钥匙
    ItemStack& heldItem = player.getHeldItem(Hand::MainHand);
    if (heldItem.isEmpty() || heldItem.getItem() != m_config.keyItem || heldItem.getCount() < 1) {
        // 播放插入失败音效（带冷却防刷）
        IWorld* playerWorld = player.world();
        if (playerWorld != nullptr) {
            i64 currentTick = static_cast<i64>(playerWorld->currentTick());
            if (currentTick - m_lastInsertFailSoundTick >= INSERT_FAIL_SOUND_COOLDOWN) {
                playerWorld->playSound(ResourceLocation("minecraft", "block.vault.insert_item_fail"),
                    sound::SoundCategory::Blocks,
                    m_pos.center(),
                    1.0f,
                    1.0f);
                m_lastInsertFailSoundTick = currentTick;
            }
        }
        return false;
    }

    // 检查玩家是否已领取过奖励
    if (m_rewardedPlayers.contains(player.uuid())) {
        // 播放拒绝已奖励玩家音效（带冷却防刷）
        IWorld* playerWorld = player.world();
        if (playerWorld != nullptr) {
            i64 currentTick = static_cast<i64>(playerWorld->currentTick());
            if (currentTick - m_lastInsertFailSoundTick >= INSERT_FAIL_SOUND_COOLDOWN) {
                playerWorld->playSound(ResourceLocation("minecraft", "block.vault.reject_rewarded_player"),
                    sound::SoundCategory::Blocks,
                    m_pos.center(),
                    1.0f,
                    1.0f);
                m_lastInsertFailSoundTick = currentTick;
            }
        }
        return false;
    }

    // 解析战利品表获取弹出物品列表
    IWorld* world = player.world();
    auto itemsToEject = resolveItemsToEject(*world, player);
    if (itemsToEject.empty()) {
        return false;
    }

    // 消耗钥匙物品
    heldItem.shrink(1);

    // 记录已奖励玩家（按插入顺序排列）
    m_rewardedPlayers.insert(player.uuid());
    if (static_cast<i32>(m_rewardedPlayers.size()) > MAX_REWARDED_PLAYERS) {
        // 超过上限时按 FIFO 策略移除最早插入的玩家
        m_rewardedPlayers.erase(m_rewardedPlayers.begin());
    }

    // 设置待弹出物品列表
    m_itemsToEject = std::move(itemsToEject);
    m_totalEjectionsNeeded = static_cast<i32>(m_itemsToEject.size());
    m_unlockingPlayerUuid = player.uuid();

    // 开始解锁
    m_unlockingStartTick = static_cast<i64>(world->currentTick());
    setState(State::Unlocking);

    // 播放插入钥匙音效
    world->playSound(ResourceLocation("minecraft", "block.vault.insert_item"),
        sound::SoundCategory::Blocks,
        m_pos.center(),
        1.0f,
        1.0f);

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
    i64 currentTick = static_cast<i64>(world.currentTick());

    // 控制检测频率：每STATE_UPDATE_INTERVAL tick检测一次
    if (currentTick - m_lastStateUpdateTick < STATE_UPDATE_INTERVAL) {
        return;
    }
    m_lastStateUpdateTick = currentTick;

    // 检测附近是否有玩家进入激活范围
    auto players = detectPlayers(world, m_config.activationRange);
    if (!players.empty()) {
        setState(State::Active);
    }
}

void VaultBlockEntity::tickActive(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    // 控制检测频率
    if (currentTick - m_lastStateUpdateTick < STATE_UPDATE_INTERVAL) {
        return;
    }
    m_lastStateUpdateTick = currentTick;

    // 检测玩家是否离开失活范围（使用更大的范围实现迟滞）
    auto players = detectPlayers(world, m_config.deactivationRange);
    if (players.empty()) {
        setState(State::Inactive);
    }
}

void VaultBlockEntity::tickUnlocking(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    // 解锁动画完成
    if (currentTick - m_unlockingStartTick >= UNLOCKING_DURATION) {
        // 播放打开百叶窗音效
        world.playSound(ResourceLocation("minecraft", "block.vault.open_shutter"),
            sound::SoundCategory::Blocks,
            m_pos.center(),
            1.0f,
            1.0f);

        // 进入弹出阶段
        setState(State::Ejecting);

        // 如果有待弹出物品，立即弹出第一个
        if (!m_itemsToEject.empty()) {
            ejectNextItem(world);
            m_ejectionEndTick = currentTick + EJECTION_INTERVAL;
        } else {
            // 没有物品，直接结束弹出
            m_ejectionEndTick = currentTick + EJECTION_AFTER_LAST_DURATION;
        }
    }
}

void VaultBlockEntity::tickEjecting(IWorld& world)
{
    i64 currentTick = static_cast<i64>(world.currentTick());

    if (m_itemsToEject.empty()) {
        // 所有物品已弹出，等待一段时间后转换状态
        if (currentTick >= m_ejectionEndTick) {
            // 播放关闭百叶窗音效
            world.playSound(ResourceLocation("minecraft", "block.vault.close_shutter"),
                sound::SoundCategory::Blocks,
                m_pos.center(),
                1.0f,
                1.0f);

            // 清理状态
            m_unlockingPlayerUuid.clear();
            m_totalEjectionsNeeded = 0;

            // 检测玩家回到Active或Inactive
            auto players = detectPlayers(world, m_config.deactivationRange);
            setState(players.empty() ? State::Inactive : State::Active);
        }
    } else {
        // 等待弹出间隔
        if (currentTick >= m_ejectionEndTick) {
            // 弹出下一个物品
            ejectNextItem(world);

            if (m_itemsToEject.empty()) {
                // 最后一个物品弹出，等待一段时间
                m_ejectionEndTick = currentTick + EJECTION_AFTER_LAST_DURATION;
            } else {
                // 继续弹出间隔
                m_ejectionEndTick = currentTick + EJECTION_INTERVAL;
            }
        }
    }
}

// ============================================================================
// 奖励逻辑
// ============================================================================

std::vector<ItemStack> VaultBlockEntity::resolveItemsToEject(IWorld& world, Player& player)
{
    // 获取战利品表管理器
    const auto* ltm = world.lootTableManager();
    if (ltm == nullptr) {
        return {};
    }

    // 选择战利品表（根据是否不祥）
    const std::string lootTableId = m_ominous ? m_config.ominousLootTable.toString() : m_config.lootTable.toString();

    const auto* lootTable = ltm->getTable(lootTableId);
    if (lootTable == nullptr) {
        return {};
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
        return {};
    }

    // 生成物品列表
    return lootTable->generate(*context);
}

void VaultBlockEntity::ejectNextItem(IWorld& world)
{
    if (m_itemsToEject.empty()) {
        return;
    }

    // 从列表末尾弹出（栈式弹出顺序）
    ItemStack item = m_itemsToEject.back();
    m_itemsToEject.pop_back();

    if (item.isEmpty()) {
        return;
    }

    // 计算弹出位置：方块上方1.2格
    f64 x = static_cast<f64>(m_pos.x) + 0.5;
    f64 y = static_cast<f64>(m_pos.y) + 1.2;
    f64 z = static_cast<f64>(m_pos.z) + 0.5;

    // 计算弹出进度（用于音高变化）
    f32 progress = (m_totalEjectionsNeeded <= 1)
        ? 1.0f
        : 1.0f - static_cast<f32>(m_itemsToEject.size()) / static_cast<f32>(m_totalEjectionsNeeded - 1);

    // 使用ItemDropHelper弹出物品，速度为UP方向2.0
    ItemDropHelper::spawnItemEntity(&world,
        item,
        x,
        y,
        z,
        0.0f,
        EJECT_VELOCITY,
        0.0f,                   // 向上弹出
        10,                     // pickupDelay: 10 ticks (0.5秒)
        m_unlockingPlayerUuid); // owner: 解锁玩家优先拾取

    // 播放弹出音效，音高随进度变化 (0.8 + 0.4 * progress)
    world.playSound(ResourceLocation("minecraft", "block.vault.eject_item"),
        sound::SoundCategory::Blocks,
        m_pos.center(),
        1.0f,
        0.8f + 0.4f * progress);

    setChanged();
}

std::vector<Player*> VaultBlockEntity::detectPlayers(IWorld& world, f32 range)
{
    std::vector<Player*> result;

    // 获取范围内所有实体
    // 状态转换（Inactive↔Active）检测所有非旁观者玩家，不过滤已奖励玩家。
    // 已奖励玩家的过滤仅在 tryInsertKey 中进行。
    Vector3 center = m_pos.center();
    auto entities = world.getEntitiesInRange(center, range);

    for (auto* entity : entities) {
        // 只筛选玩家
        auto* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        // 排除旁观者模式的玩家（仅排除旁观者，不排除创造模式）
        if (player->isSpectator()) {
            continue;
        }

        result.push_back(player);
    }

    return result;
}

Player* VaultBlockEntity::findPlayerByUuid(IWorld& world, const std::string& uuid)
{
    // 使用 getEntityByUuid() 进行 O(1) UUID 查找，替代遍历玩家列表
    Entity* entity = world.getEntityByUuid(uuid);
    if (entity != nullptr) {
        return dynamic_cast<Player*>(entity);
    }
    return nullptr;
}

} // namespace mc
