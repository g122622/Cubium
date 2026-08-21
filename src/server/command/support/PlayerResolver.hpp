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

#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <string>
#include <vector>

namespace mc {

class PlayerInventory;

namespace command::support {

/**
 * @brief 解析单个玩家选择器。
 *
 * @param source 命令源
 * @param selector 玩家选择器
 * @return 匹配到的玩家 ID，失败时返回 0
 * @note 当前仅落地与现有项目能力直接兼容的玩家解析语义。
 */
[[nodiscard]] PlayerId resolveSinglePlayerId(const ServerCommandSource& source, const EntitySelector& selector);

/**
 * @brief 解析多个玩家选择器。
 *
 * @param source 命令源
 * @param selector 玩家选择器
 * @return 匹配到的玩家 ID 列表
 */
[[nodiscard]] std::vector<PlayerId> resolvePlayerIds(const ServerCommandSource& source, const EntitySelector& selector);

/**
 * @brief 通过 PlayerId 获取玩家名称。
 *
 * 查找 PlayerManager 中对应 PlayerId 的 ServerPlayerData，返回其 username。
 * 如果服务器不可用或玩家不在线，返回回退名称 "player_<id>"。
 *
 * @param source 命令源，用于获取服务器实例
 * @param playerId 玩家 ID
 * @return 玩家名称字符串
 */
[[nodiscard]] std::string resolvePlayerName(const ServerCommandSource& source, PlayerId playerId);

/**
 * @brief 将游戏模式转换为命令输出名称。
 */
[[nodiscard]] const char* getGameModeCommandName(GameMode mode) noexcept;

/**
 * @brief 将难度转换为命令输出名称。
 */
[[nodiscard]] const char* getDifficultyCommandName(Difficulty difficulty) noexcept;

/**
 * @brief 解析玩家物品栏（背包）。
 *
 * 优先经 InventoryManager（网络层数据）取真实玩家背包——真实玩家登录时由 LoginFlow 调
 * initializeInventory 注册，InventoryManager 持有与客户端同步的权威背包。若 InventoryManager
 * 无该 playerId 的条目（返回 nullptr），回退经 ServerPlayerEntityManager 取实体层
 * Player::m_inventory——SimulatedPlayer 不走登录流程故不在 InventoryManager 注册，其权威背包
 * 是实体层 Player::m_inventory（useItemOnBlock 等均直接操作实体层 inventory()）。
 *
 * 此回退修复 /give /clear /replaceitem /loot 等经 server->playerInventory(playerId) 的命令对
 * SimulatedPlayer 完全失效的缺陷（getInventory 返 nullptr 致命令 continue 跳过）。
 * 对齐 EffectCommand/GameModeCommand/TeleportCommand 经 ServerPlayerEntityManager 旁路
 * SimulatedPlayer 的既有模式。
 *
 * @param source 命令源（取 server 与 world 上下文）
 * @param playerId 玩家 ID
 * @return 物品栏指针；玩家不存在或无世界上下文时返回 nullptr
 */
[[nodiscard]] PlayerInventory* resolvePlayerInventory(ServerCommandSource& source, PlayerId playerId);

} // namespace command::support
} // namespace mc
