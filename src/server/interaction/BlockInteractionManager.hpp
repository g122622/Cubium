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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/protocol/GameActions.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include <functional>
#include <optional>
#include <string>

namespace mc {
class BlockState;
class Player;
class ServerPlayer;
namespace loot {
class LootTableManager;
}
namespace server {
struct ServerPlayerData; // 前向声明（struct 而非 class）
class IServer;
} // namespace server
} // namespace mc

namespace mc::server {

// 前向声明
class ServerWorld;
namespace core {
class PlayerManager;
class ConnectionManager;
} // namespace core
namespace interaction {
class InventoryManager;
}

/**
 * @brief 方块交互结果
 */
struct BlockInteractionResult {
    bool success = false;
    std::string message;
};

/**
 * @brief 方块破坏结果
 */
struct BlockBreakResult {
    bool blockBroken = false;
    u32 newBlockStateId = 0;
    std::string message;
};

/**
 * @brief 方块放置结果
 */
struct BlockPlacementResult {
    bool success = false;
    bool blockPlaced = false;
    bool itemConsumed = false;
    BlockPos position;
    u32 newBlockStateId = 0;
    std::string message;
};

namespace interaction {

/**
 * @brief 方块交互管理器
 *
 * 处理玩家与方块的交互：
 * - 方块破坏（距离验证、工具检测、掉落物生成）
 * - 方块放置（位置验证、碰撞检测）
 * - 方块使用（工作台、熔炉等）
 *
 * 网络发送通过回调实现，由 MinecraftServer 设置。
 */
class BlockInteractionManager {
public:
    /**
     * @brief 构造函数
     */
    BlockInteractionManager(core::PlayerManager& playerManager, loot::LootTableManager& lootTableManager);

    /**
     * @brief 设置物品栏管理器（用于物品消耗）
     */
    void setInventoryManager(InventoryManager* inventoryManager);

    /**
     * @brief 设置服务器接口（用于告示牌命令执行等）
     * @param server 服务器接口指针
     */
    void setServer(IServer* server) { m_server = server; }

    /**
     * @brief 处理方块交互数据包
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param action 交互动作
     * @return 交互结果
     */
    [[nodiscard]] Result<BlockInteractionResult> handleBlockInteraction(
        PlayerId playerId, const BlockPos& pos, network::BlockInteractionAction action);

    /**
     * @brief 处理方块放置数据包
     * @param playerId 玩家ID
     * @param pos 点击的方块位置
     * @param hitPos 击中点
     * @param face 点击的面
     * @param heldItem 手持物品
     * @return 放置结果
     */
    [[nodiscard]] Result<BlockPlacementResult> handleBlockPlacement(
        PlayerId playerId, const BlockPos& pos, const Vector3& hitPos, Direction face, const ItemStack& heldItem);

    /**
     * @brief 处理方块使用（右键激活）
     * @param playerId 玩家ID
     * @param pos 目标方块位置
     * @param hand 使用的手
     * @param hitPos 击中点
     * @param face 击中面
     * @return 交互结果
     */
    [[nodiscard]] Result<BlockInteractionResult> handleBlockUse(
        PlayerId playerId, const BlockPos& pos, Hand hand, const Vector3& hitPos, Direction face);

    /**
     * @brief 处理方块破坏（挖掘完成）
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @return 破坏结果
     */
    [[nodiscard]] Result<BlockBreakResult> handleBlockBreak(PlayerId playerId, const BlockPos& pos);

    /**
     * @brief 设置方块破坏回调
     */
    void setOnBlockBreak(std::function<void(PlayerId, const BlockPos&, const BlockState&)> callback);

    /**
     * @brief 设置方块放置回调
     */
    void setOnBlockPlace(std::function<void(PlayerId, const BlockPos&, const BlockState&)> callback);

private:
    /**
     * @brief 验证玩家数据有效性
     *
     * 检查玩家是否存在且已登录。
     *
     * @param playerId 玩家ID
     * @return 玩家数据指针，如果无效则返回 nullptr
     */
    [[nodiscard]] ServerPlayerData* _validatePlayer(PlayerId playerId) const noexcept;

    /**
     * @brief 执行基础交互前置检查
     *
     * 包含：玩家验证、距离验证、Y范围验证（可选）
     *
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param checkYRange 是否检查Y范围
     * @return 错误信息，如果检查通过则返回 std::nullopt
     */
    [[nodiscard]] std::optional<Error> _validateInteractionPreconditions(
        PlayerId playerId, const BlockPos& pos, bool checkYRange = true) const;

    /**
     * @brief 获取方块状态并检查是否为空气
     *
     * @param pos 方块位置
     * @return 方块状态指针，如果无效或为空气则返回 nullptr
     */
    [[nodiscard]] const BlockState* _getNonAirBlockState(ServerWorld& world, const BlockPos& pos) const noexcept;

    /**
     * @brief 检查是否可以在当前世界执行修改操作
     *
     * 检查调试世界状态。
     *
     * @return 如果禁止修改则返回错误，否则返回 std::nullopt
     */
    [[nodiscard]] std::optional<Error> _checkWorldModificationAllowed(ServerWorld& world) const noexcept;

    /**
     * @brief 获取玩家手持物品
     *
     * @param playerId 玩家ID
     * @return 手持物品堆，如果无法获取则返回空堆
     */
    [[nodiscard]] ItemStack _getHeldTool(PlayerId playerId) const noexcept;

    /**
     * @brief 获取真实玩家实体，用于 loot / owner 上下文。
     */
    [[nodiscard]] Player* _getPlayerEntity(PlayerId playerId, ServerWorld& world) const noexcept;

    /**
     * @brief 将方块设置为空气并触发回调
     *
     * @param pos 方块位置
     * @param oldState 原方块状态
     * @param playerId 玩家ID（用于回调）
     * @return 空气方块状态ID，如果失败返回 0
     */
    u32 _setBlockToAir(ServerWorld& world, const BlockPos& pos, const BlockState& oldState, PlayerId playerId);

    /**
     * @brief 验证玩家是否可以与方块交互（距离检查）
     */
    [[nodiscard]] bool _canInteract(PlayerId playerId, const BlockPos& pos) const noexcept;

    /**
     * @brief 验证玩家是否可以破坏方块
     */
    [[nodiscard]] bool _canBreakBlock(
        ServerWorld& world, PlayerId playerId, const BlockPos& pos, const BlockState* state) const noexcept;

    /**
     * @brief 检查候选放置方块是否与玩家碰撞箱相交
     *
     * 用于阻止将有碰撞体的方块放置到玩家体内。
     */
    [[nodiscard]] bool _wouldCollideWithPlayer(
        PlayerId playerId, const BlockPos& placePos, const BlockState& state) const noexcept;

    /**
     * @brief 获取玩家当前所在维度的世界
     */
    [[nodiscard]] ServerWorld* _getPlayerWorld(PlayerId playerId) const noexcept;

    /**
     * @brief 生成方块掉落物
     */
    void _generateBlockDrops(
        ServerWorld& world, const BlockPos& pos, const BlockState& state, PlayerId playerId, const ItemStack* tool);

    /**
     * @brief 处理告示牌命令执行
     *
     * 当玩家右键点击告示牌时，检查文本中的点击事件并执行命令。
     *
     * @param pos 告示牌位置
     * @param player 执行命令的玩家
     * @return 如果执行了命令返回 true
     */
    bool _handleSignCommand(ServerWorld& world, const BlockPos& pos, mc::ServerPlayer& player);

private:
    core::PlayerManager& m_playerManager;
    loot::LootTableManager& m_lootTableManager;
    InventoryManager* m_inventoryManager = nullptr;
    IServer* m_server = nullptr;

    std::function<void(PlayerId, const BlockPos&, const BlockState&)> m_onBlockBreak;
    std::function<void(PlayerId, const BlockPos&, const BlockState&)> m_onBlockPlace;
};

} // namespace interaction
} // namespace mc::server
