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

#include "PlayerDataManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectManager.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/experience/ExperienceManager.hpp"
#include "common/entity/food/FoodStats.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/world/storage/db/ColumnFamilies.hpp"
#include "common/world/storage/db/RocksDBDatabase.hpp"
#include "common/world/storage/player/PlayerSaveData.hpp"
#include "server/core/ServerPlayerData.hpp"
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::world::storage {

// ============================================================================
// 构造与析构
// ============================================================================

PlayerDataManager::PlayerDataManager(RocksDBDatabase& db)
    : m_db(db)
{}

PlayerDataManager::~PlayerDataManager() {}

// ============================================================================
// 玩家数据操作
// ============================================================================

Result<PlayerSaveData*> PlayerDataManager::loadPlayer(const std::string& uuid)
{
    // 先检查缓存
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_cache.find(uuid);
        if (it != m_cache.end()) {
            return it->second.get();
        }
    }

    // 从数据库加载
    auto key = _makeKey(uuid);
    auto dataResult = m_db.get(cf::PLAYERS, key);
    if (dataResult.failed()) {
        // NotFound 表示玩家不存在，返回 nullptr
        if (dataResult.error().code() == ErrorCode::NotFound) {
            return nullptr;
        }
        // 其他数据库错误
        return dataResult.error();
    }

    auto& data = dataResult.value();
    if (data.empty()) {
        // 玩家不存在
        return nullptr;
    }

    // 反序列化
    auto playerResult = PlayerSaveData::deserialize(data);
    if (playerResult.failed()) {
        return playerResult.error();
    }

    // 缓存
    auto playerData = std::make_unique<PlayerSaveData>(std::move(playerResult.value()));
    PlayerSaveData* ptr = playerData.get();

    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_cache[uuid] = std::move(playerData);
    }

    return ptr;
}

Result<void> PlayerDataManager::savePlayer(const PlayerSaveData& data)
{
    // 标记为脏
    markDirty(data.uuid);

    // 更新缓存
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_cache.find(data.uuid);
        if (it != m_cache.end()) {
            *it->second = data;
        } else {
            m_cache[data.uuid] = std::make_unique<PlayerSaveData>(data);
        }
    }

    return Result<void>::ok();
}

Result<void> PlayerDataManager::savePlayerImmediate(const PlayerSaveData& data)
{
    // 序列化
    auto serializeResult = data.serialize();
    if (serializeResult.failed()) {
        return serializeResult.error();
    }

    // 写入数据库
    auto key = _makeKey(data.uuid);
    auto putResult = m_db.put(cf::PLAYERS, key, serializeResult.value(), true);
    if (putResult.failed()) {
        return putResult.error();
    }

    // 从脏集合移除
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_dirtyUuids.erase(data.uuid);
    }

    // 更新缓存
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_cache.find(data.uuid);
        if (it != m_cache.end()) {
            *it->second = data;
        } else {
            m_cache[data.uuid] = std::make_unique<PlayerSaveData>(data);
        }
    }

    // 触发回调
    if (m_onPlayerSaved) {
        m_onPlayerSaved(data.uuid);
    }

    return Result<void>::ok();
}

Result<void> PlayerDataManager::deletePlayer(const std::string& uuid)
{
    // 从数据库删除
    auto key = _makeKey(uuid);
    auto delResult = m_db.del(cf::PLAYERS, key);
    if (delResult.failed()) {
        return delResult.error();
    }

    // 从缓存和脏集合移除
    _removeFromCache(uuid);

    return Result<void>::ok();
}

bool PlayerDataManager::hasPlayer(const std::string& uuid) const
{
    // 先检查缓存
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        if (m_cache.find(uuid) != m_cache.end()) {
            return true;
        }
    }

    // 检查数据库
    auto key = _makeKey(uuid);
    return m_db.exists(cf::PLAYERS, key);
}

PlayerSaveData* PlayerDataManager::getCachedPlayer(const std::string& uuid)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_cache.find(uuid);
    if (it != m_cache.end()) {
        return it->second.get();
    }
    return nullptr;
}

const PlayerSaveData* PlayerDataManager::getCachedPlayer(const std::string& uuid) const
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_cache.find(uuid);
    if (it != m_cache.end()) {
        return it->second.get();
    }
    return nullptr;
}

// ============================================================================
// 从服务器数据转换
// ============================================================================

PlayerSaveData PlayerDataManager::fromServerPlayerData(const server::ServerPlayerData& playerData)
{
    PlayerSaveData data;
    data.uuid = playerData.uuid; // 使用真正的 UUID
    data.username = playerData.username;

    // 位置
    data.posX = playerData.x;
    data.posY = playerData.y;
    data.posZ = playerData.z;
    data.yaw = playerData.yaw;
    data.pitch = playerData.pitch;

    // 游戏模式
    data.gameMode = playerData.gameMode;

    // 生命值
    data.health = 20.0f; // 从 ServerPlayerData 获取需要扩展该结构
    data.foodLevel = 20;
    data.saturationLevel = 5.0f;

    // 效果
    data.effects = playerData.effects;

    // 其他状态
    data.onGround = playerData.onGround;

    return data;
}

PlayerSaveData PlayerDataManager::fromPlayer(const Player& player)
{
    PlayerSaveData data;

    // 基本信息 - 使用真正的 UUID
    data.uuid = player.uuid();
    data.username = player.username();

    // 位置
    const auto& pos = player.position();
    data.posX = pos.x;
    data.posY = pos.y;
    data.posZ = pos.z;

    data.yaw = player.yaw();
    data.pitch = player.pitch();

    // 维度
    data.dimension = player.dimension();

    // 游戏模式
    data.gameMode = player.gameMode();

    // 生命值
    data.health = player.health();
    data.maxHealth = player.maxHealth();

    // 饥饿值
    const auto& foodStats = player.foodStats();
    data.foodLevel = foodStats.foodLevel();
    data.saturationLevel = foodStats.saturationLevel();
    data.exhaustionLevel = foodStats.exhaustionLevel();
    data.foodTickTimer = foodStats.foodTimer();

    // 经验
    const auto& xpManager = player.experienceManager();
    data.experienceLevel = xpManager.getLevel();
    data.experienceProgress = xpManager.getProgress();
    data.totalExperience = xpManager.getTotalExperience();
    data.xpSeed = xpManager.getXpSeed();

    // 能力
    const auto& abilities = player.abilities();
    data.invulnerable = abilities.invulnerable;
    data.canFly = abilities.canFly;
    data.flying = abilities.flying;
    data.flySpeed = abilities.flySpeed;
    data.walkSpeed = abilities.walkSpeed;

    // 重生点
    auto spawnPoint = player.getSpawnPoint();
    if (spawnPoint.has_value()) {
        data.spawnPoint = spawnPoint.value();
        data.spawnForced = player.isSpawnForced();
    }

    // 进入下界位置
    auto netherPos = player.getEnteredNetherPosition();
    if (netherPos.has_value()) {
        data.enteredNetherPosition = netherPos.value();
    }

    // 最后死亡位置
    auto deathLoc = player.getLastDeathLocation();
    if (deathLoc.has_value()) {
        data.lastDeathLocation = deathLoc.value();
    }

    // 睡眠状态
    data.sleeping = player.isSleeping();
    data.sleepTimer = player.getSleepTimer();
    auto sleepingPos = player.getSleepingPosition();
    if (sleepingPos.has_value()) {
        data.sleepingPosition = sleepingPos.value();
    }

    // 空气供应
    data.airSupply = player.air();

    // 状态
    data.onGround = player.isOnGround();
    data.sprinting = player.isSprinting();
    data.sneaking = player.isSneaking();

    // 冲量上下文
    auto impulsePos = player.currentImpulseImpactPos();
    if (impulsePos.has_value()) {
        data.currentImpulseImpactPos = impulsePos.value();
    }
    data.ignoreFallDamageFromCurrentImpulse = player.isIgnoringFallDamageFromCurrentImpulse();
    data.currentImpulseContextResetGraceTime = player.currentImpulseContextResetGraceTime();

    // 背包物品
    const auto& inventory = player.inventory();
    data.inventoryItems.clear();
    data.inventoryItems.reserve(PlayerInventory::TOTAL_SIZE);
    for (i32 slot = 0; slot < PlayerInventory::TOTAL_SIZE; ++slot) {
        ItemStack stack = inventory.getItem(slot);
        if (stack.isEmpty()) {
            data.inventoryItems.emplace_back(std::nullopt);
        } else {
            data.inventoryItems.emplace_back(std::move(stack));
        }
    }
    data.selectedSlot = inventory.getSelectedSlot();

    // 鼠标持有物品
    const ItemStack& carriedItem = inventory.getCarriedItem();
    if (!carriedItem.isEmpty()) {
        data.carriedItem = carriedItem;
    }

    // 药水效果
    const auto& effectManager = player.effectManager();
    const auto& effects = effectManager.getAllEffects();
    data.effects = effects;

    return data;
}

// ============================================================================
// 从保存数据恢复到玩家实体
// ============================================================================

void PlayerDataManager::applyToPlayer(Player& player, const PlayerSaveData& data)
{
    // ========== 位置和旋转 ==========
    player.setPosition(static_cast<f32>(data.posX), static_cast<f32>(data.posY), static_cast<f32>(data.posZ));
    player.setRotation(data.yaw, data.pitch);

    // ========== 维度 ==========
    player.setDimension(data.dimension);

    // ========== 游戏模式 ==========
    player.setGameMode(data.gameMode);

    // ========== 生命值 ==========
    player.setHealth(data.health);

    // ========== 饥饿值 ==========
    auto& foodStats = player.foodStats();
    foodStats.setFoodLevel(data.foodLevel);
    foodStats.setSaturationLevel(data.saturationLevel);
    foodStats.setExhaustionLevel(data.exhaustionLevel);
    foodStats.setFoodTimer(data.foodTickTimer);

    // ========== 经验 ==========
    player.experienceManager().setExperience(data.experienceLevel, data.experienceProgress, data.totalExperience);
    player.experienceManager().setXpSeed(data.xpSeed);

    // ========== 能力 ==========
    auto& abilities = player.abilities();
    abilities.invulnerable = data.invulnerable;
    abilities.canFly = data.canFly;
    abilities.flying = data.flying;
    abilities.flySpeed = data.flySpeed;
    abilities.walkSpeed = data.walkSpeed;

    // ========== 重生点 ==========
    if (data.spawnPoint.has_value()) {
        player.setSpawnPoint(data.spawnPoint->getDimensionId(), data.spawnPoint->getPos(), data.spawnForced);
    }

    // ========== 进入下界位置 ==========
    if (data.enteredNetherPosition.has_value()) {
        player.setEnteredNetherPosition(data.enteredNetherPosition.value());
    }

    // ========== 最后死亡位置 ==========
    if (data.lastDeathLocation.has_value()) {
        player.setLastDeathLocation(data.lastDeathLocation.value());
    } else {
        player.setLastDeathLocation(std::nullopt);
    }

    // ========== 睡眠状态 ==========
    if (data.sleeping) {
        player.setSleeping(true);
        if (data.sleepingPosition.has_value()) {
            player.setSleepingPosition(data.sleepingPosition.value());
        }
        player.setSleepTimer(data.sleepTimer);
    }

    // ========== 空气供应 ==========
    player.setAir(data.airSupply);

    // ========== 状态标志 ==========
    player.setOnGround(data.onGround);
    if (data.sprinting) {
        player.setSprinting(true);
    }
    if (data.sneaking) {
        player.setSneaking(true);
    }

    // ========== 冲量上下文 ==========
    // 先重置冲量上下文，再逐字段恢复，避免 setIgnoreFallDamageFromCurrentImpulse 的副作用
    // （该方法会设置 40 tick 宽限期，覆盖保存数据中的实际值）
    player.resetCurrentImpulseContext();
    if (data.currentImpulseImpactPos.has_value()) {
        player.setCurrentImpulseImpactPos(data.currentImpulseImpactPos.value());
    }
    if (data.ignoreFallDamageFromCurrentImpulse) {
        player.setIgnoreFallDamageFromCurrentImpulse(true);
    }
    // 恢复冲量上下文宽限期：setIgnoreFallDamageFromCurrentImpulse(true) 会设置 40 tick，
    // 但保存数据中可能有不同的值，需要用 applyPostImpulseGraceTime 调整。
    // applyPostImpulseGraceTime 取最大值，所以如果保存值 > 40 则扩展，否则保持 40。
    if (data.currentImpulseContextResetGraceTime > 0) {
        player.applyPostImpulseGraceTime(data.currentImpulseContextResetGraceTime);
    }

    // ========== 背包物品 ==========
    auto& inventory = player.inventory();
    for (i32 slot = 0; slot < static_cast<i32>(data.inventoryItems.size()) && slot < PlayerInventory::TOTAL_SIZE;
        ++slot) {
        const auto& itemOpt = data.inventoryItems[slot];
        if (itemOpt.has_value() && !itemOpt->isEmpty()) {
            inventory.setItem(slot, itemOpt->copy());
        }
    }
    inventory.setSelectedSlot(data.selectedSlot);

    // ========== 鼠标持有物品 ==========
    if (data.carriedItem.has_value() && !data.carriedItem->isEmpty()) {
        inventory.setCarriedItem(data.carriedItem->copy());
    }

    // ========== 药水效果 ==========
    auto& effectManager = player.effectManager();
    effectManager.removeAllEffects(player);
    for (const auto& effect : data.effects) {
        effectManager.addEffect(entity::effect::EffectInstance(effect), player);
    }
}

// ============================================================================
// 批量操作
// ============================================================================

Result<size_t> PlayerDataManager::saveAllDirty()
{
    std::vector<std::string> dirtyUuids;
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        dirtyUuids.assign(m_dirtyUuids.begin(), m_dirtyUuids.end());
    }

    if (dirtyUuids.empty()) {
        return 0;
    }

    size_t savedCount = 0;
    for (const auto& uuid : dirtyUuids) {
        PlayerSaveData* data = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            auto it = m_cache.find(uuid);
            if (it != m_cache.end()) {
                data = it->second.get();
            }
        }

        if (data == nullptr) {
            continue;
        }

        auto result = savePlayerImmediate(*data);
        if (result.success()) {
            ++savedCount;
        } else {
            spdlog::error("Failed to save player {}: {}", uuid, result.error().message());
        }
    }

    if (savedCount > 0) {
        spdlog::info("Saved {} dirty player data", savedCount);
    }

    return savedCount;
}

Result<size_t> PlayerDataManager::saveAll()
{
    std::vector<std::string> allUuids;
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        allUuids.reserve(m_cache.size());
        for (const auto& [uuid, _] : m_cache) {
            allUuids.push_back(uuid);
        }
    }

    if (allUuids.empty()) {
        return 0;
    }

    size_t savedCount = 0;
    for (const auto& uuid : allUuids) {
        PlayerSaveData* data = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            auto it = m_cache.find(uuid);
            if (it != m_cache.end()) {
                data = it->second.get();
            }
        }

        if (data == nullptr) {
            continue;
        }

        auto result = savePlayerImmediate(*data);
        if (result.success()) {
            ++savedCount;
        } else {
            spdlog::error("Failed to save player {}: {}", uuid, result.error().message());
        }
    }

    spdlog::info("Saved {} player data (total)", savedCount);
    return savedCount;
}

void PlayerDataManager::markDirty(const std::string& uuid)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_dirtyUuids.insert(uuid);
}

size_t PlayerDataManager::dirtyCount() const
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_dirtyUuids.size();
}

std::vector<std::string> PlayerDataManager::getDirtyUuids() const
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return std::vector<std::string>(m_dirtyUuids.begin(), m_dirtyUuids.end());
}

void PlayerDataManager::clearCache()
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cache.clear();
    m_dirtyUuids.clear();
}

// ============================================================================
// 统计
// ============================================================================

size_t PlayerDataManager::cacheSize() const
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_cache.size();
}

// ============================================================================
// 私有方法
// ============================================================================

std::vector<u8> PlayerDataManager::_makeKey(const std::string& uuid)
{
    return std::vector<u8>(uuid.begin(), uuid.end());
}

void PlayerDataManager::_removeFromCache(const std::string& uuid)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cache.erase(uuid);
    m_dirtyUuids.erase(uuid);
}

} // namespace mc::world::storage
