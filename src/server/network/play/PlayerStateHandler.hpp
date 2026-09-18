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
 * @brief 玩家自身状态与界面偏好包族
 *
 * 覆盖 PlayerCommand（疾跑/潜行/起床/骑乘跳跃/开背包/滑翔）、难度修改与锁定
 * （服务端权限校验）、配方书放置请求、进度界面标签页选择。
 */
class PlayerStateHandler : public PlayHandlerBase {
public:
    explicit PlayerStateHandler(MinecraftServer& server)
        : PlayHandlerBase(server)
    {}

    /// PlayerCommand：全 action 分发（0=起床 1/2=疾跑 3/4=骑乘跳跃 5=开背包 6=滑翔）
    void handlePlayerCommandPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// ServerboundChangeDifficulty：主机或 OP(GameMaster+) 方可修改
    void handleChangeDifficultyPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// LockDifficulty：主机或 OP(GameMaster+) 方可锁定
    void handleLockDifficultyPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// PlaceRecipe：配方书放置请求（配方书同步链路尚未打通）
    void handlePlaceRecipePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// SeenAdvancements：进度界面标签页切换 / 关闭
    void handleSeenAdvancementsPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
};

} // namespace mc::server::net
