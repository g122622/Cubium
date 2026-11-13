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

#include "common/command/arguments/ItemSlotArgument.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

#include <string>

namespace mc {

class BlockPos;
class Entity;
class ItemStack;

namespace server {
class ServerWorld;
}

namespace command::support {

/**
 * @brief 把玩家背包同步给客户端。
 *
 * 统一走 InventoryManager 的下发出口：它按玩家背包菜单构造全量内容（含 2x2 合成格、
 * 合成结果槽与副手槽），并带上服务端权威光标。命令里各自手搓只会退化成「只发
 * PlayerInventory」，把合成格整片抹掉。
 */
void syncPlayerInventoryToClient(ServerCommandSource& source, PlayerId playerId);

/**
 * @brief 读取玩家某个槽位的物品。
 *
 * 槽位编号沿用原版 SlotArgument：0-40 背包、98-106 装备、200-226 末影箱、499 光标。
 *
 * @return 槽位存在于该玩家上时返回 true，并把物品副本写入 outItem。
 */
bool getPlayerSlotItem(ServerCommandSource& source, PlayerId playerId, const ItemSlot& slot, ItemStack& outItem);

/**
 * @brief 写入玩家某个槽位的物品。
 *
 * @return 槽位存在于该玩家上时返回 true。
 */
bool setPlayerSlotItem(ServerCommandSource& source, PlayerId playerId, const ItemSlot& slot, const ItemStack& stack);

/**
 * @brief 读取方块容器某个槽位的物品。
 *
 * @param world 方块所在世界
 * @param pos 容器方块位置
 * @param slotIndex 容器内槽位下标（0 起）
 * @return 方块是容器且槽位合法时返回 true。
 */
bool getBlockContainerSlot(server::ServerWorld& world, const BlockPos& pos, i32 slotIndex, ItemStack& outItem);

/**
 * @brief 写入方块容器某个槽位的物品。
 *
 * @return 方块是容器且槽位合法时返回 true。
 */
bool setBlockContainerSlot(
    server::ServerWorld& world, const BlockPos& pos, i32 slotIndex, const ItemStack& stack);

/**
 * @brief 读取非玩家实体自身槽位的物品（装备、马匹库存、村民交易栏）。
 *
 * @return 该实体上存在此槽位时返回 true。
 */
bool getNonPlayerEntitySlotItem(Entity& entity, const ItemSlot& slot, ItemStack& outItem);

/**
 * @brief 写入非玩家实体自身槽位的物品。
 *
 * @return 该实体上存在此槽位（且物品符合槽位规则）时返回 true。
 */
bool setNonPlayerEntitySlotItem(Entity& entity, const ItemSlot& slot, const ItemStack& stack);

/**
 * @brief 把槽位编号渲染成反馈文本里使用的槽位名。
 */
[[nodiscard]] std::string describeSlot(const ItemSlot& slot);

} // namespace command::support
} // namespace mc
