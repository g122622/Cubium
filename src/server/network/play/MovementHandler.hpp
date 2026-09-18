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

#pragma once

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "server/network/play/base/PlayHandlerBase.hpp"

namespace mc::server::net {

/**
 * @brief 移动 / 载具输入 / 传送确认 包族
 *
 * 覆盖玩家自身移动（MovePlayer×4）、传送确认（AcceptTeleportation）、客户端输入位掩码
 * （PlayerInput）、载具移动（ServerboundMoveVehicle）与划桨（PaddleBoat）。
 *
 * 反飞行的基线校验落在本族：玩家自身基线在 ServerPlayer 实体上，载具基线挂在骑乘者身上。
 */
class MovementHandler : public PlayHandlerBase {
public:
    explicit MovementHandler(MinecraftServer& server)
        : PlayHandlerBase(server)
    {}

    /// MovePlayer{Pos,PosRot,Rot,StatusOnly}：反飞行校验 + 写入位置 + 区块/追踪刷新 + 村庄进入检测
    void handlePlayerMovePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// AcceptTeleportation：确认服务端下发的传送，重置反飞行基线并刷新追踪
    void handleTeleportConfirmPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// PlayerInput：客户端输入位掩码（前/后/左/右/跳/潜行/疾跑），骑乘时转发给载具
    void handlePlayerInputPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// ServerboundMoveVehicle：NaN 校验 + 载具反飞行 + 写入位置 + 回送校正包
    void handleMoveVehiclePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// PaddleBoat：划桨状态
    void handlePaddleBoatPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// 刷新指定玩家的实体追踪范围（移动/传送确认后调用；门面亦对外转发本方法）
    void updateEntityTrackingForPlayer(PlayerId playerId, f64 x, f64 y, f64 z);
};

} // namespace mc::server::net
