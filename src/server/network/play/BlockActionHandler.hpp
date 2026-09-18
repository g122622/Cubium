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

// 定义在 mc 命名空间；必须在 mc 下前向声明，若放到 mc::server::net 会遮蔽真实类型。
namespace mc {
class Player;
} // namespace mc

namespace mc::server::net {

/**
 * @brief 方块与物品动作包族
 *
 * 覆盖挖掘（PlayerAction 的 0-2，走 MiningManager，带 sequence 需 ack）、物品动作
 * （PlayerAction 的 3-6：丢弃/副手交换/释放使用，不带 sequence 不 ack）、使用物品
 * （UseItem，右键空气）、放置与交互（UseItemOn）、告示牌编辑（SignUpdate）。
 *
 * 本族是唯一触及 `MinecraftServer` 自身纯虚的包族（`getHeldItemForPlacement` /
 * `getSelectedHotbarSlot` / `setInventoryItem` / `syncPlayerInventory` /
 * `tryOpenCraftingContainer`），故 `PlayHandlerBase` 持 `MinecraftServer&`。
 */
class BlockActionHandler : public PlayHandlerBase {
public:
    explicit BlockActionHandler(MinecraftServer& server)
        : PlayHandlerBase(server)
    {}

    /// PlayerAction：挖掘 action 0-2 走 MiningManager；物品 action 3-6 转 handlePlayerItemAction
    void handleBlockInteractionPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// PlayerAction 的物品分支（3=DROP_ALL_ITEMS 4=DROP_ITEM 5=SWAP_ITEM_WITH_OFFHAND
    /// 6=RELEASE_USE_ITEM），对齐 Java ServerGamePacketListenerImpl.handlePlayerAction。
    void handlePlayerItemAction(PlayerId playerId, i32 action);

    /// UseItemOn：放置/使用方块，含命中精度、Y 上限、冷却、传送等待等前置校验
    void handleBlockPlacementPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// UseItem：右键空气使用物品
    void handleUseItemPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// SignUpdate：告示牌文本编辑（编辑者校验 + 涂蜡拒改 + 变更广播）
    void handleUpdateSignPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

private:
    /// 触发 default_block_use 成就（任意方块使用/放置成功后调用，无条件触发所有监听实例）。
    void _triggerAnyBlockUse(Player& player);
};

} // namespace mc::server::net
