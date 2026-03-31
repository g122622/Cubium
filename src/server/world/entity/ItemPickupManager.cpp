#include "ItemPickupManager.hpp"
#include "server/application/IServer.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/util/math/MathUtils.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace mc::server {

// ============================================================================
// tick
// ============================================================================

void ItemPickupManager::tick(IServer& server) {
    // 处理物品合并
    processItemMerging(server);

    // 遍历所有玩家实体检查拾取
    server.playerManager().forEachPlayer([&server, this](ServerPlayerData& playerData) {
        // TODO: 需要获取 Player 实体来检查拾取
        // 暂时跳过，等 PlayerManager 完善
        // checkPlayerPickup(server, player);
        return true;  // 继续遍历
    });
}

// ============================================================================
// checkPlayerPickup
// ============================================================================

void ItemPickupManager::checkPlayerPickup(IServer& server, Entity& player) {
    // 计算拾取范围
    f32 range = calculatePickupRange(player);
    Vector3 playerPos = player.position();

    // 查找附近的物品实体
    AxisAlignedBB searchBox(
        playerPos.x - range,
        playerPos.y - range,
        playerPos.z - range,
        playerPos.x + range,
        playerPos.y + player.height() + range,
        playerPos.z + range
    );

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

bool ItemPickupManager::tryPickupItem(
    IServer& server,
    Entity& player,
    ItemEntity& itemEntity)
{
    // 获取物品堆
    ItemStack& stack = const_cast<ItemStack&>(itemEntity.getItemStack());
    if (stack.isEmpty()) {
        return true;  // 空物品，直接移除
    }

    // 检查是否是玩家
    if (player.legacyType() != LegacyEntityType::Player) {
        return false;
    }

    Player* playerEntity = static_cast<Player*>(&player);

    // 使用 ItemEntity::onPlayerPickup 处理拾取逻辑
    // 这确保所有者 UUID 检查等逻辑在一处实现
    bool fullyPickedUp = itemEntity.onPlayerPickup(*playerEntity);

    if (fullyPickedUp || itemEntity.getItemStack().isEmpty()) {
        // 完全拾取，发送背包更新和实体销毁包
        sendInventoryUpdate(server, *playerEntity);
        sendEntityDestroy(server, itemEntity.id(), player.id());
        return fullyPickedUp;
    }

    // 部分拾取，发送背包更新
    sendInventoryUpdate(server, *playerEntity);
    return false;
}

// ============================================================================
// processItemMerging
// ============================================================================

void ItemPickupManager::processItemMerging(IServer& server) {
    // 收集所有存活的物品实体
    std::vector<ItemEntity*> itemEntities;
    server.entityManager().forEachEntity([&itemEntities](Entity* entity) {
        if (entity && entity->isAlive() && entity->legacyType() == LegacyEntityType::Item) {
            itemEntities.push_back(static_cast<ItemEntity*>(entity));
        }
        return true;  // 继续遍历
    });

    if (itemEntities.size() < 2) {
        return;  // 少于2个物品无需合并
    }

    // 使用空间哈希网格优化合并检测
    // 哈希键 = 区块坐标，每个单元格大小为 MERGE_RANGE
    constexpr f32 CELL_SIZE = MERGE_RANGE * 2.0f;  // 单元格大小为合并范围的2倍
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
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},  {0, 0},  {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    constexpr f32 MERGE_RANGE_SQ = MERGE_RANGE * MERGE_RANGE;

    for (const auto& [key, items] : grid) {
        i32 cellX = static_cast<i32>(key >> 32);
        i32 cellZ = static_cast<i32>(key & 0xFFFFFFFF);

        // 获取当前单元格和相邻单元格的所有物品
        std::vector<ItemEntity*> nearbyItems;
        for (const auto& offset : NEIGHBOR_OFFSETS) {
            i64 neighborKey = (static_cast<i64>(cellX + offset[0]) << 32) |
                              (static_cast<i64>(cellZ + offset[1]) & 0xFFFFFFFF);
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

f32 ItemPickupManager::calculatePickupRange(const Entity& player) const {
    f32 range = PICKUP_RANGE;

    // 创造模式不改变范围
    // if (player.isCreative()) { } // 暂时跳过

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

bool ItemPickupManager::canPickup(const Entity& player, const ItemEntity& itemEntity) const {
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

void ItemPickupManager::sendInventoryUpdate(IServer& server, Player& player) {
    // 获取玩家ID
    PlayerId playerId = player.playerId();
    ServerPlayerData* playerData = server.playerManager().getPlayer(playerId);
    if (!playerData || !playerData->hasConnection()) {
        return;
    }

    // 获取玩家背包
    PlayerInventory& inventory = player.inventory();

    // 创建背包内容包
    // 使用 ContainerContentPacket 发送完整背包
    std::vector<ItemStack> items;
    items.reserve(inventory::PLAYER_INVENTORY_SIZE);

    for (i32 i = 0; i < inventory::PLAYER_INVENTORY_SIZE; ++i) {
        items.push_back(inventory.getItem(i));
    }

    ContainerContentPacket contentPacket(inventory::PLAYER_CONTAINER_ID, std::move(items));

    // 序列化数据包
    network::PacketSerializer payload;
    contentPacket.serialize(payload);

    // 创建完整数据包（包含头部）
    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + payload.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::ContainerContent));
    fullPacket.writeU16(0);  // flags
    fullPacket.writeU16(0);  // reserved
    fullPacket.writeU16(0);  // padding
    fullPacket.writeBytes(payload.buffer());

    // 发送给玩家
    playerData->send(fullPacket.buffer().data(), fullPacket.buffer().size());
}

// ============================================================================
// sendEntityDestroy
// ============================================================================

void ItemPickupManager::sendEntityDestroy(
    IServer& server,
    EntityId entityId,
    EntityId collectorId)
{
    // 首先发送 CollectItemPacket 触发拾取动画
    // 这会让客户端播放物品飞向玩家的动画
    network::CollectItemPacket collectPacket;
    collectPacket.setCollectedEntityId(static_cast<u32>(entityId));
    collectPacket.setCollectorEntityId(static_cast<u32>(collectorId));
    collectPacket.setPickupItemCount(1);  // 默认拾取数量

    auto collectResult = collectPacket.serialize();
    if (collectResult.success()) {
        network::PacketSerializer fullPacket;
        fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + collectResult.value().size()));
        fullPacket.writeU16(static_cast<u16>(network::PacketType::CollectItem));
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeBytes(collectResult.value());

        // 广播给所有玩家
        server.connectionManager().broadcast(fullPacket.buffer().data(), fullPacket.buffer().size());
    }

    // 然后发送实体销毁包
    // SPacketDestroyEntities: VarInt count, Array[VarInt] entityIds
    network::PacketSerializer ser;
    ser.writeVarInt(static_cast<i32>(network::PacketType::EntityDestroy));
    ser.writeVarInt(1);  // count = 1
    ser.writeVarInt(static_cast<i32>(entityId));

    // 创建完整数据包（包含长度前缀）
    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::EntityDestroy));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    // 广播给所有玩家
    server.connectionManager().broadcast(fullPacket.buffer().data(), fullPacket.buffer().size());
}

} // namespace mc::server
