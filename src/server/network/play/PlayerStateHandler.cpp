/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "server/network/play/PlayerStateHandler.hpp"

#include "common/advancement/AdvancementManager.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/IJumpingMount.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <variant>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::net {

void PlayerStateHandler::handlePlayerCommandPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // PlayerCommand action（对齐 MC 1.21.11 ServerboundPlayerCommandPacket.Action）：
    // 0=STOP_SLEEPING 1=START_SPRINTING 2=STOP_SPRINTING 3=START_RIDING_JUMP
    // 4=STOP_RIDING_JUMP 5=OPEN_INVENTORY 6=START_FALL_FLYING。
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PlayerCommand>(&play);
    if (evt == nullptr) {
        return;
    }

    auto* world = m_server.getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    auto* playerEntity = m_server.playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }

    switch (evt->action) {
        case 0: { // STOP_SLEEPING
            if (auto* serverPlayer = playerEntity->asServerPlayer()) {
                serverPlayer->stopSleepInBed(true);
            } else {
                playerEntity->stopSleeping();
            }
            break;
        }
        case 1: // START_SPRINTING
            playerEntity->setSprinting(true);
            break;
        case 2: // STOP_SPRINTING
            playerEntity->setSprinting(false);
            break;
        case 3: { // START_RIDING_JUMP（data=跳跃强度）
            const EntityInstanceId vehicleId = playerEntity->getVehicle();
            if (vehicleId == INVALID_ENTITY_ID) {
                break;
            }
            auto* vehicle = world->getEntity(vehicleId);
            if (vehicle == nullptr) {
                break;
            }
            auto* jumpingMount = dynamic_cast<mc::entity::IJumpingMount*>(vehicle);
            if (jumpingMount != nullptr) {
                jumpingMount->startJumping(evt->data);
            }
            break;
        }
        case 4: { // STOP_RIDING_JUMP
            const EntityInstanceId vehicleId = playerEntity->getVehicle();
            if (vehicleId == INVALID_ENTITY_ID) {
                break;
            }
            auto* vehicle = world->getEntity(vehicleId);
            if (vehicle == nullptr) {
                break;
            }
            auto* jumpingMount = dynamic_cast<mc::entity::IJumpingMount*>(vehicle);
            if (jumpingMount != nullptr) {
                jumpingMount->stopJumping();
            }
            break;
        }
        case 5: // OPEN_INVENTORY
            // 复用既有 Inventory 开包处理体（IntegratedServer 覆写打开菜单）。
            // 经 m_server 虚分发到子类 override，保留 IntegratedServer 开背包覆写。
            m_server.handleOpenPlayerInventoryPacket(playerId, packet);
            break;
        case 6: // START_FALL_FLYING
            if (!playerEntity->tryToStartFallFlying()) {
                playerEntity->stopFallFlying();
            }
            break;
        default:
            spdlog::info("PlayerCommand: player {} sent unknown action {}", playerId, evt->action);
            break;
    }
}

void PlayerStateHandler::handleChangeDifficultyPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ServerboundChangeDifficulty>(&play);
    if (evt == nullptr) {
        return;
    }
    if (evt->difficulty < 0 || evt->difficulty > 3) {
        spdlog::warn("ChangeDifficulty: player {} sent invalid difficulty {}", playerId, evt->difficulty);
        return;
    }
    // 权限校验对齐 Java handleChangeDifficulty：主机 或 OP(>=GameMaster) 方可改难度。
    // 主机判定不依赖 allowCommands（与 resolveOpLevel 的作弊提升解耦）。
    const bool permitted = m_server.isSingleplayerOwner(playerId) ||
        m_server.opListManager().getLevel(player->uuid) >= core::OpLevel::GameMaster;
    if (!permitted) {
        spdlog::warn("ChangeDifficulty: player {} has no permission to change difficulty", playerId);
        return;
    }
    // setDifficulty 内部含锁定守卫与 cb:10 广播，无需在此重复发送。
    m_server.setDifficulty(static_cast<Difficulty>(evt->difficulty));
}

void PlayerStateHandler::handleLockDifficultyPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::LockDifficulty>(&play);
    if (evt == nullptr) {
        return;
    }
    // 权限校验对齐 Java handleLockDifficulty：主机 或 OP(>=GameMaster) 方可锁定。
    const bool permitted = m_server.isSingleplayerOwner(playerId) ||
        m_server.opListManager().getLevel(player->uuid) >= core::OpLevel::GameMaster;
    if (!permitted) {
        spdlog::warn("LockDifficulty: player {} has no permission to lock difficulty", playerId);
        return;
    }
    // setDifficultyLocked 内部已广播 cb:10（携带新 locked 值），客户端据此禁用难度按钮。
    m_server.setDifficultyLocked(evt->locked);
}

void PlayerStateHandler::handlePlaceRecipePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // PlaceRecipe（C→S id=38）：玩家在配方书中点击配方，请求服务端把材料填入合成网格。
    // 字段：containerId(VarInt)/recipe(VarInt RecipeDisplayId)/useMaxItems(bool)。
    //
    // TODO(配方书网络同步链路): 完整实现需 display-id→ResourceLocation 映射（RecipeDisplayId 是
    // 客户端配方表索引，服务端需维护映射）+ 按配方填充合成网格槽位（容器槽位操作 API）。整个配方书
    // 同步链路（recipe_book_add/remove/settings 下行、place_recipe/recipe_book_change_settings 上行）
    // 尚未打通，此处仅确认接收并记 warn，避免 route 兜底静默丢弃。
    (void)packet;
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (player == nullptr || !player->loggedIn) {
        return;
    }
    spdlog::warn("PlaceRecipe from player {} not implemented (recipe book sync chain pending)", playerId);
}

void PlayerStateHandler::handleSeenAdvancementsPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // SeenAdvancements（C→S id=49）：action 0=OPENED_TAB（tab=ResourceLocation），1=CLOSED_SCREEN。
    // 对齐 Java ServerGamePacketListenerImpl.handleSeenAdvancements：
    //  - OPENED_TAB：解析 tab 为 AdvancementPtr 写入 PlayerAdvancements::m_selectedTab（持久化用）。
    //  - CLOSED_SCREEN：vanilla 为空实现（不清除选中标签页）。
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (player == nullptr || !player->loggedIn) {
        return;
    }
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::SeenAdvancements>(&play);
    if (evt == nullptr) {
        return;
    }
    auto* world = m_server.getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }
    auto* playerEntity = m_server.playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }
    auto* serverPlayer = playerEntity->asServerPlayer();
    if (serverPlayer == nullptr) {
        return;
    }
    auto* advancements = serverPlayer->getAdvancements();
    if (advancements == nullptr) {
        return;
    }
    if (evt->action == 0) { // OPENED_TAB
        const auto adv = mc::advancement::AdvancementManager::instance().get(mc::ResourceLocation(evt->tab));
        if (adv == nullptr) {
            spdlog::warn("SeenAdvancements OPENED_TAB: advancement {} not found", evt->tab);
            return;
        }
        advancements->setSelectedTab(adv);
    }
    // action == 1 (CLOSED_SCREEN)：vanilla 空实现，不清除选中标签页。
}

} // namespace mc::server::net
