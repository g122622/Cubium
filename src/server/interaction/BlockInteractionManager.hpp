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
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
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

/**
 * @brief 右键对方块的交互结果（对应 vanilla `ServerPlayerGameMode.useItemOn` / 物品 `Item::useOn`）
 *
 * 一次右键（`use_item_on` 或 `use_item`）只会产出一次交互，具体落在三条分支之一：
 * - `BlockUseItemOn`：方块侧 `Block::useItemOn`（手持物品参与，主手未潜行）
 * - `BlockUseWithoutItem`：方块侧 `Block::useWithoutItem`（空手，或 `useItemOn` 返回
 *   `TryEmptyHandInteraction` 后的空手回退）
 * - `ItemUseOn`：物品侧 `Item::onItemUse`（方块侧返回 Pass 后，矿车/骨粉/桶/锄头等生效）
 *
 * 结果字段 `result` 为 `BlockActionResult`，其 `heldItemTransformedTo` 承载"交互后应写回
 * 手持槽的新物品"（如花盆取物、书架的交换结果）；无该字段时按原地修改后的权威手持槽同步。
 */
struct BlockUseResult {
    enum class Kind : u8 {
        BlockUseItemOn,      ///< Block::useItemOn
        BlockUseWithoutItem, ///< Block::useWithoutItem
        ItemUseOn            ///< Item::onItemUse / BlockItem 放置
    };

    bool success = false;      ///< 交互被处理（对应 vanilla consumesAction）
    bool itemConsumed = false; ///< 手持物品是否被消耗/改动（供调用方决定是否再同步）
    Kind kind = Kind::BlockUseWithoutItem;
    ActionResultType actionResult = ActionResultType::Pass; ///< 原始结果（含 swingSource / wasItemInteraction 细分）
    std::optional<ItemStack> heldItemTransformedTo;         ///< 方块侧声明的"交互后应写回的手持物"
    std::string message;
};

namespace interaction {

/**
 * @brief 方块交互管理器
 *
 * 处理玩家与方块的交互：
 * - 方块破坏（距离验证、工具检测、掉落物生成）
 * - 方块放置（位置验证、碰撞检测）
 * - 方块/物品右键（对齐 vanilla `ServerPlayerGameMode.useItemOn` 的派发顺序）
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
     * @brief 处理方块放置（对应 vanilla `BlockItem.place`，经 `Item::useOn` 触发）
     *
     * 承担 block-item 的放置全流程：位置/支撑/实体碰撞校验 → `BlockItem::tryPlace` →
     * 按 hand 消耗权威手持槽。由 `handleItemUseOn` 在识别出手持物为 block-item 时调用，
     * 也保留为独立入口供测试与后续直接放置需求使用。
     *
     * @param playerId 玩家ID
     * @param pos 点击的方块位置
     * @param hitPos 击中点
     * @param face 点击的面
     * @param hand 使用的手（决定消耗哪个槽位）
     * @param heldItem 手持物品（调用方局部拷贝）
     * @return 放置结果
     */
    [[nodiscard]] Result<BlockPlacementResult> handleBlockPlacement(PlayerId playerId,
        const BlockPos& pos,
        const Vector3& hitPos,
        Direction face,
        Hand hand,
        const ItemStack& heldItem);

    /**
     * @brief 处理右键交互（对应 vanilla `ServerPlayerGameMode.useItemOn`）
     *
     * 严格按 vanilla 顺序派发：
     * 1. 旁观者：`Block::getMenuProvider` 有菜单则直接打开并返回 Consume，否则 Pass
     * 2. `isSecondaryUseActive && 任一手非空` → 直接落到第 5 步
     * 3. `Block::useItemOn`（仅主手，且第 2 步未命中）
     * 4. 第 3 步返回 `TryEmptyHandInteraction` 且为主手 → `Block::useWithoutItem`
     * 5. 手持非空且未在冷却 → `Item::onItemUse`（创造模式恢复数量，对齐 `hasInfiniteMaterials`）
     *
     * 消耗写回：`Block::useItemOn` / `useWithoutItem` 收到的是玩家权威手持槽引用，
     * 其修改直接落盘；`Item::onItemUse` 收到的是局部拷贝，由本方法按 itemId/damage 变化
     * 判定"物品已自管消耗"后回写权威槽（骨粉等仅 shrink 局部拷贝者由外层补 shrink）。
     *
     * @param playerId 玩家ID
     * @param pos 点击的方块位置
     * @param hand 使用的手
     * @param hitPos 击中点
     * @param face 击中面
     * @return 交互结果
     */
    [[nodiscard]] Result<BlockUseResult> handleBlockUse(
        PlayerId playerId, const BlockPos& pos, Hand hand, const Vector3& hitPos, Direction face);

    /**
     * @brief 处理物品在方块上使用（对应 vanilla `Item::useOn` 分支）
     *
     * 独立入口，供 `handleBlockUse` 第 5 步复用，也便于无方块上下文（如 `use_item` 包的空手路径）
     * 的单测直接驱动。未配潜行门控与旁观者分支——那两层由 `handleBlockUse` 负责。
     *
     * @param playerId 玩家ID
     * @param pos 点击的方块位置
     * @param hitPos 击中点
     * @param face 击中面
     * @param hand 使用的手
     * @param heldItem 调用方局部拷贝（供 `Item::onItemUse` 读取/修改上下文）
     * @return 交互结果
     */
    [[nodiscard]] Result<BlockUseResult> handleItemUseOn(PlayerId playerId,
        const BlockPos& pos,
        const Vector3& hitPos,
        Direction face,
        Hand hand,
        const ItemStack& heldItem);

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
     * 包含：玩家验证、距离验证、Y范围验证（可选）、出生点保护（可选）
     *
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param checkYRange 是否检查Y范围
     * @param checkSpawnProtection 是否检查出生点保护（右键使用不检查，对齐 vanilla
     *        `ServerGamePacketListenerImpl`：出生点保护只在 `ServerPlayerGameMode.handleBlockBreakAction`
     *        的 START_DESTROY 中经 `ServerLevel.mayInteract` 检查，不作用于 useItemOn）
     * @return 错误信息，如果检查通过则返回 std::nullopt
     */
    [[nodiscard]] std::optional<Error> _validateInteractionPreconditions(
        PlayerId playerId, const BlockPos& pos, bool checkYRange, bool checkSpawnProtection) const;

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
     * @brief 按 hand 取玩家权威手持槽（只读）
     *
     * @param playerId 玩家ID
     * @param hand 手
     * @return 手持物品堆引用；物品栏不可用时返回静态空堆
     */
    [[nodiscard]] const ItemStack& _handStackFor(PlayerId playerId, Hand hand) const noexcept;

    /**
     * @brief 任一只手持物是否非空（用于 isSecondaryUseActive 门控）
     */
    [[nodiscard]] bool _anyHandHasItem(PlayerId playerId) const noexcept;

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
     *
     * 对齐 vanilla ServerGamePacketListenerImpl/ServerPlayerGameMode 中的
     * player.isWithinBlockInteractionRange(pos, padding)：使用 generic.block_interaction_range
     * 属性（生存 4.5、创造 5.0，padding 1.0 为容差）做眼位到方块 AABB 最近点距离判定，
     * 而非硬编码 6 格。
     *
     * @param player 玩家实体（用于读属性）；为 nullptr 时回退到 PlayerData 中心距离保底逻辑
     * @param playerId 玩家ID（player 为 nullptr 时用于取 PlayerData）
     * @param pos 目标方块坐标
     */
    [[nodiscard]] bool _canInteract(const Player* player, PlayerId playerId, const BlockPos& pos) const noexcept;

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

    /**
     * @brief 派发方块侧 `useItemOn` / `useWithoutItem` 并回写权威手持槽
     *
     * 统一承担：取真实玩家实体、推游戏模式、按 hand 解析权威手持槽、调用方块虚方法、
     * 应用 `heldItemTransformedTo` 或原地变更后的同步。
     *
     * @param playerId 玩家ID
     * @param world 玩家所在世界
     * @param pos 方块位置
     * @param hand 手
     * @param hitPos 击中点
     * @param face 击中面
     * @param withItem true 调 `useItemOn`（手持物参与），false 调 `useWithoutItem`
     * @param outHandStack 非空时回填 hand 对应的权威手持槽指针（供调用方在方块交互未消费后
     *        继续用同一份 stack 派发 `Item::useOn`）
     * @return 方块交互结果，未处理时返回 `std::nullopt`
     */
    [[nodiscard]] std::optional<BlockUseResult> _dispatchBlockUse(PlayerId playerId,
        ServerWorld& world,
        const BlockPos& pos,
        Hand hand,
        const Vector3& hitPos,
        Direction face,
        bool withItem,
        ItemStack** outHandStack);

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
