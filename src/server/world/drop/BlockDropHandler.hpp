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
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <vector>

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

namespace entity {
class ExperienceDropHandler;
}

/**
 * @brief 矿石类型枚举
 *
 * 用于确定矿石经验掉落的类型。
 */
enum class OreType : i32 {
    None = -1,        // 非矿石
    Coal = 0,         // 煤矿 (0-2 经验)
    Diamond = 1,      // 钻石矿 (3-7 经验)
    Emerald = 2,      // 绿宝石矿 (3-7 经验)
    Lapis = 3,        // 青金石矿 (2-5 经验)
    NetherQuartz = 4, // 下界石英矿 (2-5 经验)
    NetherGold = 5,   // 下界金矿 (0-1 经验)
    Redstone = 6,     // 红石矿 (1-5 经验)
    Spawner = 7       // 刷怪笼 (15-44 经验)
};

/**
 * @brief 方块掉落处理器
 *
 * 处理方块破坏时的掉落物生成。
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
    [[nodiscard]] static std::vector<ItemStack> generateDrops(IWorld& world,
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
    static std::vector<EntityInstanceId> spawnDrops(server::ServerWorld& world,
        const BlockPos& pos,
        const std::vector<ItemStack>& drops,
        const std::string& throwerUuid = "");

    /**
     * @brief 在实体管理器中生成掉落物实体（内置服务端用）
     *
     * 与 spawnDrops(ServerWorld&, ...) 逻辑一致，但直接写入 EntityManager，
     * 避免 IntegratedServer 依赖 ServerWorld。
     */
    static std::vector<EntityInstanceId> spawnDrops(EntityManager& entityManager,
        PhysicsEngine* physicsEngine,
        const BlockPos& pos,
        const std::vector<ItemStack>& drops,
        const std::string& throwerUuid = "");

    /**
     * @brief 检查玩家是否能采集方块
     *
     * 条件:
     * - 方块硬度 >= 0（不是基岩等不可破坏方块）
     * - 使用正确工具 或 方块不需要工具
     *
     * @param state 方块状态
     * @param player 玩家（可为null）
     * @param tool 使用的工具（可为null）
     * @return 如果可以采集返回true
     */
    [[nodiscard]] static bool canHarvestBlock(const BlockState& state, const Player* player, const ItemStack* tool);

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

    // ========================================================================
    // 经验掉落
    // ========================================================================

    /**
     * @brief 判断方块是否掉落经验
     *
     * 检查方块是否是掉落经验的矿石类型。
     *
     * @param state 方块状态
     * @return 如果方块掉落经验返回对应的矿石类型，否则返回 OreType::None
     */
    [[nodiscard]] static OreType getOreType(const BlockState& state);

    /**
     * @brief 生成矿石经验掉落
     *
     * 当矿石被破坏时，在位置生成经验球。
     * 只有使用正确工具破坏时才会掉落经验。
     *
     * @param entityManager 实体管理器
     * @param physicsEngine 物理引擎（可选）
     * @param pos 方块位置
     * @param oreType 矿石类型
     * @param rng 随机数生成器
     * @return 生成的经验球数量
     */
    static i32 spawnOreExperience(server::ServerWorld& world, const BlockPos& pos, OreType oreType, math::Random& rng);

    static i32 spawnOreExperience(EntityManager& entityManager,
        PhysicsEngine* physicsEngine,
        const BlockPos& pos,
        OreType oreType,
        math::Random& rng);

    /**
     * @brief 处理方块破坏的经验掉落
     *
     * 检查方块是否是矿石，如果是则生成经验球。
     * 自动判断矿石类型和是否使用正确工具。
     *
     * @param entityManager 实体管理器
     * @param physicsEngine 物理引擎（可选）
     * @param pos 方块位置
     * @param state 方块状态
     * @param tool 使用的工具（可为null）
     * @param rng 随机数生成器
     * @return 生成的经验球数量
     */
    static i32 handleBlockBreakExperience(server::ServerWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const ItemStack* tool,
        math::Random& rng);

    static i32 handleBlockBreakExperience(EntityManager& entityManager,
        PhysicsEngine* physicsEngine,
        const BlockPos& pos,
        const BlockState& state,
        const ItemStack* tool,
        math::Random& rng);

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
    [[nodiscard]] static std::unique_ptr<loot::LootContext> buildLootContext(IWorld& world,
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
};

} // namespace mc
