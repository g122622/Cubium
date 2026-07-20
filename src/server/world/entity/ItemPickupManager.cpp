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

#include "ItemPickupManager.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/EntityMetadataSerializer.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/profiler/TraceEvents.hpp"
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
#include <cmath>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server {

// ============================================================================
// tick
// ============================================================================

void ItemPickupManager::tick(ServerWorld& world, IServer& server)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Entity, "ItemPickupManager::tick");

    // 物品合并由 ItemEntity::tick 中的 _updateMerge 统一处理，此处不再重复扫描。

    // 获取所有玩家实体并检查拾取
    auto players = world.entityManager().getEntitiesByType(entity::EntityTypeKeys::PLAYER);
    for (Entity* entity : players) {
        if (entity && entity->isAlive()) {
            checkPlayerPickup(world, server, *entity);
        }
    }
}

// ============================================================================
// checkPlayerPickup
// ============================================================================

void ItemPickupManager::checkPlayerPickup(ServerWorld& world, IServer& server, Entity& player)
{
    // 计算拾取范围
    f32 range = _calculatePickupRange(player);
    Vector3 playerPos = player.position();

    // 查找附近的物品实体
    AxisAlignedBB searchBox(playerPos.x - range,
        playerPos.y - range,
        playerPos.z - range,
        playerPos.x + range,
        playerPos.y + player.height() + range,
        playerPos.z + range);

    auto nearbyEntities = world.getEntitiesInAABB(searchBox, &player);

    for (Entity* entity : nearbyEntities) {
        if (!entity || !entity->isAlive()) {
            continue;
        }

        // 只处理物品实体
        if (entity->entityType() != entity::VanillaEntityTypeKeys::ITEM) {
            continue;
        }

        ItemEntity* itemEntity = static_cast<ItemEntity*>(entity);

        // 检查是否可以拾取
        if (!_canPickup(player, *itemEntity)) {
            continue;
        }

        // 尝试拾取
        if (tryPickupItem(world, server, player, *itemEntity)) {
            // 物品被完全拾取，标记移除
            itemEntity->remove();
        }
    }
}

// ============================================================================
// tryPickupItem
// ============================================================================

bool ItemPickupManager::tryPickupItem(ServerWorld& world, IServer& server, Entity& player, ItemEntity& itemEntity)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Entity,
        "ItemPickupManager::tryPickupItem",
        "entityId",
        itemEntity.id(),
        "playerId",
        player.id(),
        "count",
        itemEntity.getCount());

    // 获取物品堆
    ItemStack& stack = const_cast<ItemStack&>(itemEntity.getItemStack());
    if (stack.isEmpty()) {
        return true; // 空物品，直接移除
    }

    // 检查是否是玩家
    if (player.entityType() != entity::VanillaEntityTypeKeys::PLAYER) {
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
    _sendInventoryUpdate(server, *playerEntity);

    if (fullyPickedUp || itemEntity.getItemStack().isEmpty()) {
        _sendCollectItem(server, itemEntity.id(), player.id(), pickedUpCount);
        return fullyPickedUp;
    }

    _sendItemEntityUpdate(world, server, itemEntity);
    return false;
}

// ============================================================================
// _calculatePickupRange
// ============================================================================

f32 ItemPickupManager::_calculatePickupRange(const Entity& player) const
{
    f32 range = PICKUP_RANGE;

    // 潜行时范围缩小
    if (player.entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
        const Player* playerEntity = static_cast<const Player*>(&player);
        if (playerEntity->isSneaking()) {
            range = PICKUP_RANGE_SNEAKING;
        }
    }

    return range;
}

// ============================================================================
// _canPickup
// ============================================================================

bool ItemPickupManager::_canPickup(const Entity& player, const ItemEntity& itemEntity) const
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
// _sendInventoryUpdate
// ============================================================================

void ItemPickupManager::_sendInventoryUpdate(IServer& server, Player& player)
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
// _sendItemEntityUpdate
// ============================================================================

void ItemPickupManager::_sendItemEntityUpdate(ServerWorld& world, IServer& server, const ItemEntity& itemEntity)
{
    const Entity* entity = world.entityManager().getEntity(itemEntity.id());
    if (entity == nullptr) {
        return;
    }

    std::vector<u8> metadata = network::EntityMetadataSerializer::serialize(entity->dataManager(), true);
    if (metadata.size() <= 1) {
        return;
    }

    server.playerManager().forEachPlayer([&](ServerPlayerData& playerData) {
        if (!playerData.hasConnection()) {
            return;
        }

        auto playerTracked = world.entityTracker().getPlayerTrackedEntities(playerData.playerId);
        if (std::find(playerTracked.begin(), playerTracked.end(), itemEntity.id()) == playerTracked.end()) {
            return;
        }

        network::EntityMetadataPacket packet;
        packet.setEntityId(static_cast<u32>(itemEntity.id()));
        packet.setMetadata(metadata);

        auto result = packet.serialize();
        if (result.failed()) {
            return;
        }

        network::PacketSerializer fullPacket;
        fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + result.value().size()));
        fullPacket.writeU16(static_cast<u16>(network::PacketType::EntityMetadata));
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeBytes(result.value());
        playerData.send(fullPacket.buffer().data(), fullPacket.buffer().size());
    });

    const_cast<Entity*>(entity)->dataManager().clearDirty();
}

// ============================================================================
// _sendCollectItem
// ============================================================================

void ItemPickupManager::_sendCollectItem(
    IServer& server, EntityInstanceId entityId, EntityInstanceId collectorId, i32 pickupItemCount)
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
