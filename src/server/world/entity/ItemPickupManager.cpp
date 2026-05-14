#include "ItemPickupManager.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/IServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::server {

// ============================================================================
// tick
// ============================================================================

void ItemPickupManager::tick(IServer& server)
{
    // 处理物品合并
    processItemMerging(server);

    // 获取所有玩家实体并检查拾取
    auto players = server.entityManager().getEntitiesByType(LegacyEntityType::Player);
    for (Entity* entity : players) {
        if (entity && entity->isAlive()) {
            checkPlayerPickup(server, *entity);
        }
    }
}

// ============================================================================
// checkPlayerPickup
// ============================================================================

void ItemPickupManager::checkPlayerPickup(IServer& server, Entity& player)
{
    // 计算拾取范围
    f32 range = calculatePickupRange(player);
    Vector3 playerPos = player.position();

    // 查找附近的物品实体
    AxisAlignedBB searchBox(playerPos.x - range,
        playerPos.y - range,
        playerPos.z - range,
        playerPos.x + range,
        playerPos.y + player.height() + range,
        playerPos.z + range);

    auto nearbyEntities = server.world().getEntitiesInAABB(searchBox, &player);

    for (Entity* entity : nearbyEntities) {
        if (!entity || !entity->isAlive()) {
            continue;
        }

        // 只处理物品实体
        if (entity->legacyType() != LegacyEntityType::Item) {
            continue;
        }

        ItemEntity* itemEntity = static_cast<ItemEntity*>(entity);

        // 检查是否可以拾取
        if (!canPickup(player, *itemEntity)) {
            continue;
        }

        // 尝试拾取
        if (tryPickupItem(server, player, *itemEntity)) {
            // 物品被完全拾取，标记移除
            itemEntity->remove();
        }
    }
}

// ============================================================================
// tryPickupItem
// ============================================================================

bool ItemPickupManager::tryPickupItem(IServer& server, Entity& player, ItemEntity& itemEntity)
{
    // 获取物品堆
    ItemStack& stack = const_cast<ItemStack&>(itemEntity.getItemStack());
    if (stack.isEmpty()) {
        return true; // 空物品，直接移除
    }

    // 检查是否是玩家
    if (player.legacyType() != LegacyEntityType::Player) {
        return false;
    }

    Player* playerEntity = static_cast<Player*>(&player);

    // 使用 ItemEntity::onPlayerPickup 处理拾取逻辑
    // 这确保所有者 UUID 检查等逻辑在一处实现
    const i32 pickupCount = stack.getCount();

    bool fullyPickedUp = itemEntity.onPlayerPickup(*playerEntity);
    const i32 remainingCount = itemEntity.getItemStack().isEmpty() ? 0 : itemEntity.getItemStack().getCount();
    const i32 pickedUpCount = pickupCount - remainingCount;
    if (pickedUpCount <= 0) {
        return false;
    }

    player.playSound(SoundEvents::ENTITY_ITEM_PICKUP, 0.2f, 1.0f);
    sendInventoryUpdate(server, *playerEntity);

    if (fullyPickedUp || itemEntity.getItemStack().isEmpty()) {
        sendCollectItem(server, itemEntity.id(), player.id(), pickedUpCount);
        return fullyPickedUp;
    }

    sendItemEntityUpdate(server, itemEntity);
    return false;
}

// ============================================================================
// processItemMerging
// ============================================================================

void ItemPickupManager::processItemMerging(IServer& server)
{
    // 收集所有存活的物品实体
    std::vector<ItemEntity*> itemEntities;
    server.entityManager().forEachEntity([&itemEntities](Entity* entity) {
        if (entity && entity->isAlive() && entity->legacyType() == LegacyEntityType::Item) {
            itemEntities.push_back(static_cast<ItemEntity*>(entity));
        }
        return true; // 继续遍历
    });

    if (itemEntities.size() < 2) {
        return; // 少于2个物品无需合并
    }

    // 使用空间哈希网格优化合并检测
    // 哈希键 = 区块坐标，每个单元格大小为 MERGE_RANGE
    constexpr f32 CELL_SIZE = MERGE_RANGE * 2.0f; // 单元格大小为合并范围的2倍
    std::unordered_map<i64, std::vector<ItemEntity*>> grid;

    // 将物品分配到网格单元格
    for (ItemEntity* item : itemEntities) {
        Vector3 pos = item->position();
        i32 cellX = static_cast<i32>(std::floor(pos.x / CELL_SIZE));
        i32 cellZ = static_cast<i32>(std::floor(pos.z / CELL_SIZE));
        i64 key = (static_cast<i64>(cellX) << 32) | (static_cast<i64>(cellZ) & 0xFFFFFFFF);
        grid[key].push_back(item);
    }

    // 只检查同一单元格和相邻单元格内的物品
    constexpr i32 NEIGHBOR_OFFSETS[9][2] = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 0}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
    constexpr f32 MERGE_RANGE_SQ = MERGE_RANGE * MERGE_RANGE;

    for (const auto& [key, items] : grid) {
        i32 cellX = static_cast<i32>(key >> 32);
        i32 cellZ = static_cast<i32>(key & 0xFFFFFFFF);

        // 获取当前单元格和相邻单元格的所有物品
        std::vector<ItemEntity*> nearbyItems;
        for (const auto& offset : NEIGHBOR_OFFSETS) {
            i64 neighborKey =
                (static_cast<i64>(cellX + offset[0]) << 32) | (static_cast<i64>(cellZ + offset[1]) & 0xFFFFFFFF);
            auto it = grid.find(neighborKey);
            if (it != grid.end()) {
                nearbyItems.insert(nearbyItems.end(), it->second.begin(), it->second.end());
            }
        }

        // 在当前单元格内的物品之间检查合并
        for (size_t i = 0; i < items.size(); ++i) {
            ItemEntity* item1 = items[i];
            if (!item1 || !item1->isAlive()) {
                continue;
            }

            Vector3 pos1 = item1->position();

            for (ItemEntity* item2 : nearbyItems) {
                if (item2 == item1 || !item2 || !item2->isAlive()) {
                    continue;
                }

                // 只处理 item2 在 items 中的索引大于 i 的情况，避免重复
                // 通过指针比较来确定处理顺序
                if (item2 <= item1) {
                    continue;
                }

                // 检查距离
                Vector3 pos2 = item2->position();
                f32 distSq = math::distanceSq(pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z);

                if (distSq <= MERGE_RANGE_SQ) {
                    // 尝试合并
                    if (item1->tryMergeWith(*item2)) {
                        spdlog::debug("ItemEntity {} merged into {}", item2->id(), item1->id());
                    }
                }
            }
        }
    }
}

// ============================================================================
// calculatePickupRange
// ============================================================================

f32 ItemPickupManager::calculatePickupRange(const Entity& player) const
{
    f32 range = PICKUP_RANGE;

    // 潜行时范围缩小
    if (player.legacyType() == LegacyEntityType::Player) {
        const Player* playerEntity = static_cast<const Player*>(&player);
        if (playerEntity->isSneaking()) {
            range = PICKUP_RANGE_SNEAKING;
        }
    }

    return range;
}

// ============================================================================
// canPickup
// ============================================================================

bool ItemPickupManager::canPickup(const Entity& player, const ItemEntity& itemEntity) const
{
    // 检查是否可拾取
    if (!itemEntity.canBePickedUp()) {
        return false;
    }

    // 所有者限制检查由 ItemEntity::onPlayerPickup 处理
    // 这里只检查基本的可拾取状态

    return true;
}

// ============================================================================
// sendInventoryUpdate
// ============================================================================

void ItemPickupManager::sendInventoryUpdate(IServer& server, Player& player)
{
    // 获取玩家ID
    PlayerId playerId = player.playerId();
    ServerPlayerData* playerData = server.playerManager().getPlayer(playerId);
    if (!playerData || !playerData->hasConnection()) {
        return;
    }

    // 获取玩家背包
    PlayerInventory& inventory = player.inventory();

    // 创建玩家背包包
    PlayerInventoryPacket inventoryPacket(inventory);

    // 序列化数据包
    network::PacketSerializer payload;
    inventoryPacket.serialize(payload);

    // 创建完整数据包（包含头部）
    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + payload.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::PlayerInventory));
    fullPacket.writeU16(0); // flags
    fullPacket.writeU16(0); // reserved
    fullPacket.writeU16(0); // padding
    fullPacket.writeBytes(payload.buffer());

    // 发送给玩家
    playerData->send(fullPacket.buffer().data(), fullPacket.buffer().size());
}

// ============================================================================
// sendItemEntityUpdate
// ============================================================================

void ItemPickupManager::sendItemEntityUpdate(IServer& server, const ItemEntity& itemEntity)
{
    network::SpawnEntityPacket packet;
    packet.setEntityId(static_cast<u32>(itemEntity.id()));

    std::array<u8, 16> uuid = {};
    uuid[0] = static_cast<u8>(itemEntity.id() & 0xFF);
    uuid[1] = static_cast<u8>((itemEntity.id() >> 8) & 0xFF);
    uuid[2] = static_cast<u8>((itemEntity.id() >> 16) & 0xFF);
    uuid[3] = static_cast<u8>((itemEntity.id() >> 24) & 0xFF);
    packet.setUuid(uuid);

    packet.setEntityTypeId(itemEntity.getTypeId());
    packet.setPosition(itemEntity.x(), itemEntity.y(), itemEntity.z());
    packet.setRotation(itemEntity.yaw(), itemEntity.pitch());

    const auto velocity = itemEntity.velocity();
    packet.setVelocity(static_cast<i16>(std::clamp(velocity.x * 8000.0f, -32768.0f, 32767.0f)),
        static_cast<i16>(std::clamp(velocity.y * 8000.0f, -32768.0f, 32767.0f)),
        static_cast<i16>(std::clamp(velocity.z * 8000.0f, -32768.0f, 32767.0f)));
    packet.setItemStack(itemEntity.getItemStack());

    auto result = packet.serialize();
    if (result.failed()) {
        return;
    }

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + result.value().size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::SpawnEntity));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(result.value());

    server.playerManager().forEachPlayer([&](ServerPlayerData& playerData) {
        if (!playerData.hasConnection()) {
            return;
        }

        auto trackedEntities = server.entityTracker().getPlayerTrackedEntities(playerData.playerId);
        if (std::find(trackedEntities.begin(), trackedEntities.end(), itemEntity.id()) == trackedEntities.end()) {
            return;
        }

        playerData.send(fullPacket.buffer().data(), fullPacket.buffer().size());
    });
}

// ============================================================================
// sendCollectItem
// ============================================================================

void ItemPickupManager::sendCollectItem(IServer& server, EntityId entityId, EntityId collectorId, i32 pickupItemCount)
{
    network::CollectItemPacket collectPacket;
    collectPacket.setCollectedEntityId(static_cast<u32>(entityId));
    collectPacket.setCollectorEntityId(static_cast<u32>(collectorId));
    collectPacket.setPickupItemCount(pickupItemCount);

    auto collectResult = collectPacket.serialize();
    if (collectResult.failed()) {
        return;
    }

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + collectResult.value().size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::CollectItem));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(collectResult.value());

    server.connectionManager().broadcast(fullPacket.buffer().data(), fullPacket.buffer().size());
}

} // namespace mc::server
