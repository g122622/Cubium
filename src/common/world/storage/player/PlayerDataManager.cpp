#include "PlayerDataManager.hpp"
#include "common/world/storage/db/RocksDBDatabase.hpp"
#include "common/world/storage/db/ColumnFamilies.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/experience/ExperienceManager.hpp"
#include "common/entity/food/FoodStats.hpp"
#include <spdlog/spdlog.h>
#include <mutex>

namespace mc::world::storage {

// ============================================================================
// 构造与析构
// ============================================================================

PlayerDataManager::PlayerDataManager(RocksDBDatabase& db)
    : m_db(db)
{
    spdlog::debug("PlayerDataManager initialized");
}

PlayerDataManager::~PlayerDataManager()
{
    // 自动保存脏数据
    if (!m_dirtyUuids.empty()) {
        auto result = saveAllDirty();
        if (result.failed()) {
            spdlog::error("Failed to save dirty player data on shutdown: {}",
                         result.error().message());
        }
    }
    spdlog::debug("PlayerDataManager shutdown");
}

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
    auto key = makeKey(uuid);
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

    spdlog::debug("Loaded player data for {}", uuid);
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
    auto key = makeKey(data.uuid);
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

    spdlog::debug("Saved player data for {}", data.uuid);

    // 触发回调
    if (m_onPlayerSaved) {
        m_onPlayerSaved(data.uuid);
    }

    return Result<void>::ok();
}

Result<void> PlayerDataManager::deletePlayer(const std::string& uuid)
{
    // 从数据库删除
    auto key = makeKey(uuid);
    auto delResult = m_db.del(cf::PLAYERS, key);
    if (delResult.failed()) {
        return delResult.error();
    }

    // 从缓存和脏集合移除
    removeFromCache(uuid);

    spdlog::debug("Deleted player data for {}", uuid);
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
    auto key = makeKey(uuid);
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

PlayerSaveData PlayerDataManager::fromServerPlayerData(
    const server::ServerPlayerData& playerData)
{
    PlayerSaveData data;
    data.uuid = playerData.uuid;  // 使用真正的 UUID
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
    data.health = 20.0f;  // 从 ServerPlayerData 获取需要扩展该结构
    data.foodLevel = 20;
    data.saturationLevel = 5.0f;

    // 效果
    data.effects = playerData.effects;

    // 其他状态
    data.onGround = playerData.onGround;

    return data;
}

PlayerSaveData PlayerDataManager::fromPlayer(const ServerPlayer& player)
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

std::vector<u8> PlayerDataManager::makeKey(const std::string& uuid)
{
    return std::vector<u8>(uuid.begin(), uuid.end());
}

void PlayerDataManager::removeFromCache(const std::string& uuid)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cache.erase(uuid);
    m_dirtyUuids.erase(uuid);
}

} // namespace mc::world::storage
