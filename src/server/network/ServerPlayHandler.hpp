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

#pragma once

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"

namespace mc::server {
class MinecraftServer;
} // namespace mc::server

namespace mc {
class Player;
class ItemStack;
class Entity;
} // namespace mc

namespace mc::server::net {

/**
 * @brief Play 包处理门面（批7 下沉自 MinecraftServer）
 *
 * 承载入站 C→S Play IR 包的整簇处理逻辑：
 *  - routeInboundPlayPacket：按 ir::PlayPacket 变体分发到各 handle*Packet
 *    （ServerPlayRouter::handle 的唯一调用入口）。
 *  - 13 个非纯虚 handle*Packet 方法体（移动/传送确认/心跳/聊天/告示牌/
 *    骑乘输入/载具移动/玩家命令/船桨/实体交互/物品使用/方块交互/方块放置）。
 *  - updateEntityTrackingForPlayer：刷新玩家实体追踪范围（移动/传送确认/
 *    维度切换/登录序列均调用）。
 *
 * 持 MinecraftServer&（非 IServer&）：handleBlockPlacementPacket 调 5 个
 * MinecraftServer 自身纯虚（getHeldItemForPlacement/getSelectedHotbarSlot/
 * setInventoryItem/syncPlayerInventory/tryOpenCraftingContainer，均在
 * MinecraftServer 声明不在 IServer），故须持具体基类引用。其余 manager 访问器
 * （playerManager/dimensionManager/teleportManager/keepAliveManager/
 * positionTracker/playerEntityManager/miningManager/blockInteractionManager/
 * commandRegistry/getPlayerWorld/resolveOpLevel/sendPacketToPlayer）经 public
 * 访问器调用，无需 friend。
 *
 * 多态保留：3 个纯虚 handle（handleHotbarSelect/handleContainerClick/
 * handleCloseContainer）+ handleOpenPlayerInventoryPacket（虚，子类覆写）不
 * 下沉，仍由 MinecraftServer 声明、子类 override。routeInboundPlayPacket
 * 内对应分支经 m_server.handleXxxPacket(...) 虚分发到子类；handlePlayerCommandPacket
 * 的 OPEN_INVENTORY 分支同样经 m_server.handleOpenPlayerInventoryPacket(...)
 * 虚分发，保留 IntegratedServer 开背包覆写。
 */
class ServerPlayHandler {
public:
    explicit ServerPlayHandler(MinecraftServer& server)
        : m_server(server)
    {}

    // 不可拷贝/移动（持引用）。
    ServerPlayHandler(const ServerPlayHandler&) = delete;
    ServerPlayHandler& operator=(const ServerPlayHandler&) = delete;
    ServerPlayHandler(ServerPlayHandler&&) = delete;
    ServerPlayHandler& operator=(ServerPlayHandler&&) = delete;

    /// 路由入站 Play IR 包到对应处理方法（ServerPlayRouter::handle 唯一入口）。
    void route(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// 刷新指定玩家的实体追踪范围（移动/传送确认/维度切换/登录序列调用）。
    void updateEntityTrackingForPlayer(PlayerId playerId, f64 x, f64 y, f64 z);

private:
    void handlePlayerMovePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleTeleportConfirmPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleKeepAlivePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleChatMessagePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleBlockInteractionPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleBlockPlacementPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleUpdateSignPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handlePlayerInputPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleMoveVehiclePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handlePlayerCommandPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handlePaddleBoatPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleInteractPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleUseItemPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handlePingRequestPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handlePongPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleChangeDifficultyPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleLockDifficultyPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleConfigurationAcknowledgedPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handleSeenAdvancementsPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
    void handlePlaceRecipePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// 触发 player_interacted_with_entity 成就（INTERACT/INTERACT_AT 成功时调用）。
    void _triggerPlayerInteractedWithEntity(Player& player, const ItemStack& item, Entity& entity);

    MinecraftServer& m_server;
};

} // namespace mc::server::net
