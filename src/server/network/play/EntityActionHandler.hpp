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

// 这三个类型定义在 mc 命名空间；必须在 mc 下前向声明，若放到 mc::server::net 会遮蔽真实类型。
namespace mc {
class Entity;
class ItemStack;
class Player;
} // namespace mc

namespace mc::server::net {

/**
 * @brief 实体交互包族
 *
 * Interact 的三种 action（INTERACT / ATTACK / INTERACT_AT）统一走世界边界、交互距离
 * 与 ATTACK 黑名单三重校验，交互成功时触发 player_interacted_with_entity 成就并挥臂。
 */
class EntityActionHandler : public PlayHandlerBase {
public:
    explicit EntityActionHandler(MinecraftServer& server)
        : PlayHandlerBase(server)
    {}

    /// Interact：0=INTERACT 1=ATTACK 2=INTERACT_AT
    void handleInteractPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

private:
    /// 触发 player_interacted_with_entity 成就（INTERACT/INTERACT_AT 成功时调用）。
    void _triggerPlayerInteractedWithEntity(Player& player, const ItemStack& item, Entity& entity);
};

} // namespace mc::server::net
