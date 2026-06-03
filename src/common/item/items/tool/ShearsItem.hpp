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
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/Material.hpp"
#include <unordered_set>

namespace mc {

// Forward declarations
class Block;
class BlockState;
class IWorld;
class BlockPos;
class LivingEntity;
class Player;

namespace item {
namespace tool {

/**
 * @brief 剪刀物品
 *
 * 剪刀是一种特殊工具，用于：
 * - 剪羊毛（对羊使用）
 * - 高效破坏蜘蛛网、树叶、羊毛
 * - 采集蜘蛛网、红石线、绊线
 */
class ShearsItem : public Item {
public:
    /**
     * @brief 构造剪刀
     * @param properties 物品属性（耐久度等）
     */
    explicit ShearsItem(ItemProperties properties);

    ~ShearsItem() override = default;

    /**
     * @brief 获取挖掘速度
     *
     * 对蜘蛛网和树叶返回 15.0（高效率）
     * 对羊毛返回 5.0
     * 其他方块返回 1.0
     *
     * @param stack 物品堆
     * @param state 目标方块状态
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 getDestroySpeed(const ItemStack& stack, const BlockState& state) const override;

    /**
     * @brief 检查是否能采集方块
     *
     * 剪刀可以采集：蜘蛛网、红石线、绊线
     *
     * @param state 目标方块状态
     * @return 如果可以采集返回 true
     */
    [[nodiscard]] bool canHarvestBlock(const BlockState& state) const override;

    /**
     * @brief 破坏方块时调用
     *
     * 以下方块不消耗耐久：
     * - 树叶 (BlockTags::LEAVES)
     * - 羊毛 (BlockTags::WOOL)
     * - 蛛网、草、蕨、枯萎灌木、藤蔓、绊线
     * - 火方块 (BlockTags::FIRE)
     *
     * 其他硬度>0的方块消耗1点耐久。
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param state 被破坏的方块状态
     * @param pos 方块位置
     * @param entity 破坏者实体
     * @return 是否成功
     */
    bool onBlockDestroyed(
        ItemStack& stack, IWorld& world, const BlockState& state, const BlockPos& pos, LivingEntity& entity) override;

    /**
     * @brief 与实体交互
     *
     * 用于剪羊毛。如果实体实现 IForgeShearable 接口，
     * 调用其 onSheared 方法掉落物品。
     *
     * @param stack 物品堆
     * @param player 玩家
     * @param target 目标实体
     * @param hand 使用的手
     * @return 是否成功交互
     */
    bool itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand) override;
};

} // namespace tool
} // namespace item
} // namespace mc
