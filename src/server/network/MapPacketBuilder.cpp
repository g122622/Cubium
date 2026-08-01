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

#include "server/network/MapPacketBuilder.hpp"

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/items/map/FilledMapItem.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/world/map/MapData.hpp"
#include "common/world/map/MapDataManager.hpp"
#include "common/world/map/MapDecoration.hpp"
#include "server/application/IServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <algorithm>
#include <optional>
#include <vector>

namespace mc::server::net {

namespace {

/// 把 ITextComponent 序列化成 JSON 字节（opaque Component，客户端按 json::parse + fromJson
/// 解析）。nullptr 时返回 nullopt。
std::optional<std::vector<u8>> componentToJsonBytes(const mc::text::ITextComponent* component)
{
    if (component == nullptr) {
        return std::nullopt;
    }
    const std::string jsonStr = component->toJson().dump();
    return std::vector<u8>(jsonStr.begin(), jsonStr.end());
}

} // namespace

void MapPacketBuilder::pushDirtyMaps(ServerWorld& world)
{
    IServer* server = world.server();
    if (server == nullptr) {
        return;
    }

    auto* mapDataManager = world.mapDataManager();
    if (mapDataManager == nullptr) {
        return;
    }

    auto& connMgr = server->connectionManager();
    auto& playerEntityMgr = server->playerEntityManager();
    namespace irplay = mc::network::ir::play;

    // 遍历每个在线玩家，扫描其背包中持有的已填充地图，
    // 对每张处于脏状态的地图全量推送 colorPatch + decorations。
    // （MapInfo 持有者追踪机制当前未填充，改用 MapData::isDirty() 判定，
    //  在 MapDataManager::tick 清脏之前读取，故本方法须先于 tick 调用。）
    const std::vector<PlayerId> playerIds = playerEntityMgr.getPlayerIds();
    for (const PlayerId playerId : playerIds) {
        Player* playerEntity = playerEntityMgr.getPlayerEntity(playerId, world);
        if (playerEntity == nullptr) {
            continue;
        }
        auto* serverPlayer = playerEntity->asServerPlayer();
        if (serverPlayer == nullptr || !serverPlayer->isOnline()) {
            continue;
        }

        // 扫描整个背包（41 槽）找已填充地图，去重 mapId。
        std::vector<i32> heldMapIds;
        PlayerInventory& inv = serverPlayer->inventory();
        for (i32 slot = 0; slot < PlayerInventory::TOTAL_SIZE; ++slot) {
            const ItemStack stack = inv.getItem(slot);
            if (stack.isEmpty() || !item::items::FilledMapItem::isFilledMap(stack)) {
                continue;
            }
            const i32 mapId = item::items::FilledMapItem::getMapId(stack);
            if (mapId < 0) {
                continue;
            }
            // 去重（同一张图可能在多个槽位）
            if (std::find(heldMapIds.begin(), heldMapIds.end(), mapId) == heldMapIds.end()) {
                heldMapIds.push_back(mapId);
            }
        }

        for (const i32 mapId : heldMapIds) {
            world::map::MapData* mapData = mapDataManager->getMapData(mapId);
            if (mapData == nullptr || !mapData->isDirty()) {
                continue;
            }

            irplay::MapItemData pkt;
            pkt.mapId = mapId;
            pkt.scale = static_cast<u8>(mapData->scale());
            pkt.locked = mapData->locked();

            // colorPatch：全图 128×128，colors 局部行优先（startX=startY=0, width=height=128）。
            {
                irplay::MapPatchWire patch;
                patch.startX = 0;
                patch.startY = 0;
                patch.width = static_cast<u8>(world::map::MapData::MAP_SIZE);
                patch.height = static_cast<u8>(world::map::MapData::MAP_SIZE);
                const auto& fullColors = mapData->colors();
                patch.colors.assign(fullColors.begin(), fullColors.end());
                pkt.colorPatch = std::move(patch);
            }

            // decorations 全量推送（客户端 apply 是全量替换语义）。
            {
                std::vector<irplay::MapDecorationWire> decos;
                decos.reserve(mapData->decorations().size());
                for (const auto& [decoKey, deco] : mapData->decorations()) {
                    (void)decoKey;
                    irplay::MapDecorationWire w;
                    // DecorationType 枚举值即 Java MAP_DECORATION_TYPE registry id；
                    // holderRegistry wire = 纯 VarInt(registryId)，直接写 id（PLAYER→0）。
                    w.typeRegistryId = static_cast<u32>(deco.type());
                    w.x = deco.x();
                    w.y = deco.y();
                    w.rotation = deco.rotation();
                    w.name = componentToJsonBytes(deco.customName());
                    decos.push_back(std::move(w));
                }
                if (!decos.empty()) {
                    pkt.decorations = std::move(decos);
                }
            }

            const mc::network::ir::IrPacket irPacket{
                mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}};
            (void)connMgr.sendToPlayer(playerId, irPacket);
        }
    }
}

} // namespace mc::server::net
