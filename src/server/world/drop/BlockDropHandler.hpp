#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/entity/loot/LootContext.hpp"
#include "common/world/block/BlockPos.hpp"
#include <vector>
#include <memory>

namespace mc {

// Forward declarations
class BlockState;
class Player;
class IWorld;
class EntityManager;
class PhysicsEngine;

namespace server {
class ServerWorld;
}

namespace loot {
class LootTableManager;
}

/**
 * @brief 方块掉落处理器
 *
 * 处理方块破坏时的掉落物生成。
 * 参考 MC 1.16.5 PlayerInteractionManager.tryHarvestBlock
 *
 * 使用 LootTable 系统 (src/common/entity/loot/) 生成掉落。
 *
 * 用法示例:
 * @code
 * auto drops = BlockDropHandler::generateDrops(*world, pos, state, player, tool, lootTableManager);
 * if (!drops.empty()) {
 *     BlockDropHandler::spawnDrops(*world, pos, drops, player->uuid());
 * }
 * @endcode
 */
class BlockDropHandler {
public:
    /**
     * @brief 处理方块破坏掉落
     *
     * 流程:
     * 1. 检查是否可采集 (canHarvestBlock)
     * 2. 构建 LootContext (工具、位置、时运、精准采集等)
     * 3. 从 Block::getLootTable() 获取掉落表
     * 4. 调用 LootTable::generate() 生成掉落
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 被破坏的方块状态
     * @param player 破坏者（可为null）
     * @param tool 使用的工具（可为null）
     * @param lootTableManager 掉落表管理器
     * @return 生成的掉落物列表
     */
    [[nodiscard]] static std::vector<ItemStack> generateDrops(
        IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const Player* player,
        const ItemStack* tool,
        const loot::LootTableManager& lootTableManager);

    /**
     * @brief 在世界中生成掉落物实体
     *
     * 在方块位置生成 ItemEntity，带有随机散射速度。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param drops 掉落物列表
     * @param throwerUuid 投掷者UUID（防止立即拾取）
     * @return 生成的实体ID列表
     */
    static std::vector<EntityId> spawnDrops(
        server::ServerWorld& world,
        const BlockPos& pos,
        const std::vector<ItemStack>& drops,
        const String& throwerUuid = "");

    /**
     * @brief 在实体管理器中生成掉落物实体（内置服务端用）
     *
     * 与 spawnDrops(ServerWorld&, ...) 逻辑一致，但直接写入 EntityManager，
     * 避免 IntegratedServer 依赖 ServerWorld。
     */
    static std::vector<EntityId> spawnDrops(
        EntityManager& entityManager,
        PhysicsEngine* physicsEngine,
        const BlockPos& pos,
        const std::vector<ItemStack>& drops,
        const String& throwerUuid = "");

    /**
     * @brief 检查玩家是否能采集方块
     *
     * 参考 MC 1.16.5 PlayerInteractionManager.canHarvestBlock
     * 条件:
     * - 方块硬度 >= 0（不是基岩等不可破坏方块）
     * - 使用正确工具 或 方块不需要工具
     *
     * @param state 方块状态
     * @param player 玩家（可为null）
     * @param tool 使用的工具（可为null）
     * @return 如果可以采集返回true
     */
    [[nodiscard]] static bool canHarvestBlock(
        const BlockState& state,
        const Player* player,
        const ItemStack* tool);

    /**
     * @brief 获取方块的默认掉落
     *
     * 当方块没有掉落表时，使用默认掉落逻辑。
     * 例如：方块本身（创造模式或其他特殊情况）。
     *
     * @param state 方块状态
     * @return 默认掉落物列表（通常为空）
     */
    [[nodiscard]] static std::vector<ItemStack> getDefaultDrops(const BlockState& state);

private:
    /**
     * @brief 构建 LootContext 用于掉落表生成
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 被破坏的方块状态
     * @param player 玩家
     * @param tool 工具
     * @param random 随机数生成器
     * @return 构建的掉落上下文
     */
    [[nodiscard]] static std::unique_ptr<loot::LootContext> buildLootContext(
        IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const Player* player,
        const ItemStack* tool,
        math::Random& random);

    /**
     * @brief 检查工具是否有精准采集附魔
     *
     * @param tool 工具
     * @return 如果有精准采集返回true
     */
    [[nodiscard]] static bool hasSilkTouch(const ItemStack* tool);

    /**
     * @brief 获取工具的时运附魔等级
     *
     * @param tool 工具
     * @return 时运等级（0-3）
     */
    [[nodiscard]] static i32 getFortuneLevel(const ItemStack* tool);

    /**
     * @brief 应用时运加成到掉落数量
     *
     * 参考 MC 1.16.5 Fortune 逻辑。
     *
     * @param baseCount 基础数量
     * @param fortuneLevel 时运等级
     * @param random 随机数生成器
     * @return 加成后的数量
     */
    [[nodiscard]] static i32 applyFortuneBonus(i32 baseCount, i32 fortuneLevel, math::Random& random);
};

} // namespace mc
