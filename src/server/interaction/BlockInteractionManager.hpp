#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/Block.hpp"
#include "common/util/Direction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include <functional>
#include <optional>

namespace mc {
class BlockState;
namespace loot {
class LootTableManager;
}
}

namespace mc::server {

// 前向声明
class ServerWorld;
namespace core {
class PlayerManager;
class ConnectionManager;
}
namespace interaction {
class InventoryManager;
}

/**
 * @brief 方块交互结果
 */
struct BlockInteractionResult {
    bool success = false;
    String message;
};

/**
 * @brief 方块破坏结果
 */
struct BlockBreakResult {
    bool blockBroken = false;
    u32 newBlockStateId = 0;
    String message;
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
    String message;
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
    BlockInteractionManager(ServerWorld& world,
                           core::PlayerManager& playerManager,
                           loot::LootTableManager& lootTableManager);

    /**
     * @brief 设置物品栏管理器（用于物品消耗）
     */
    void setInventoryManager(InventoryManager* inventoryManager);

    /**
     * @brief 处理方块交互数据包
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param action 交互动作
     * @return 交互结果
     */
    [[nodiscard]] Result<BlockInteractionResult> handleBlockInteraction(
        PlayerId playerId,
        const BlockPos& pos,
        network::BlockInteractionAction action);

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
        PlayerId playerId,
        const BlockPos& pos,
        const Vector3& hitPos,
        Direction face,
        const ItemStack& heldItem);

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
        PlayerId playerId,
        const BlockPos& pos,
        Hand hand,
        const Vector3& hitPos,
        Direction face);

    /**
     * @brief 处理方块破坏（挖掘完成）
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @return 破坏结果
     */
    [[nodiscard]] Result<BlockBreakResult> handleBlockBreak(
        PlayerId playerId,
        const BlockPos& pos);

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
     * @brief 验证玩家是否可以与方块交互（距离检查）
     */
    [[nodiscard]] bool canInteract(PlayerId playerId, const BlockPos& pos) const;

    /**
     * @brief 验证玩家是否可以破坏方块
     */
    [[nodiscard]] bool canBreakBlock(PlayerId playerId, const BlockPos& pos, const BlockState* state) const;

    /**
     * @brief 生成方块掉落物
     */
    void generateBlockDrops(const BlockPos& pos, const BlockState& state, PlayerId playerId, const ItemStack* tool);

private:
    ServerWorld& m_world;
    core::PlayerManager& m_playerManager;
    loot::LootTableManager& m_lootTableManager;
    InventoryManager* m_inventoryManager = nullptr;

    std::function<void(PlayerId, const BlockPos&, const BlockState&)> m_onBlockBreak;
    std::function<void(PlayerId, const BlockPos&, const BlockState&)> m_onBlockPlace;
};

} // namespace interaction
} // namespace mc::server