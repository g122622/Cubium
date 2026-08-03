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

#include "../../../core/Types.hpp"
#include "../../core/Item.hpp"
#include "common/item/core/ActionResult.hpp"

namespace mc {

// 前向声明
namespace fluid {
class Fluid;
class FluidState;
} // namespace fluid

/**
 * @brief 桶物品
 *
 * 实现水桶、岩浆桶、空桶的功能。
 * - 空桶：从水源方块或含水方块中取出流体
 * - 装满的桶：放置流体方块或向含水方块注入流体
 */
class BucketItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param containedFluid 桶中装的流体（nullptr表示空桶）
     * @param properties 物品属性
     */
    BucketItem(fluid::Fluid* containedFluid, const ItemProperties& properties);

    ~BucketItem() override = default;

    /**
     * @brief 获取桶中装的流体
     * @return 流体指针，空桶返回 nullptr
     */
    [[nodiscard]] fluid::Fluid* getContainedFluid() const { return m_containedFluid; }

    /**
     * @brief 检查是否为空桶
     */
    [[nodiscard]] bool isEmpty() const { return m_containedFluid == nullptr; }

    /**
     * @brief 在方块上使用物品
     *
     * 对于空桶：尝试从方块中取出流体
     * 对于装满的桶：尝试放置流体或向含水方块注水
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 右键使用物品
     *
     * 处理桶的交互逻辑。
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 与实体交互
     *
     * 空桶与牛/哞菇交互时挤奶。
     *
     * @param stack 物品堆
     * @param player 玩家
     * @param target 目标实体
     * @param hand 使用的手
     * @return 是否成功交互
     */
    bool itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand) override;

    /**
     * @brief 获取填充后的桶物品
     *
     * 当空桶装满流体后，返回对应的桶物品。
     *
     * @param fluid 流体
     * @return 对应的桶物品，如果流体无效则返回 nullptr
     */
    [[nodiscard]] static BucketItem* getFilledBucket(fluid::Fluid& fluid);

    /**
     * @brief 获取空桶物品
     */
    [[nodiscard]] static BucketItem* getEmptyBucket();

protected:
    /**
     * @brief 尝试放置流体
     *
     * @param player 玩家
     * @param world 世界
     * @param pos 放置位置
     * @param hit 射线检测结果
     * @return 是否成功放置
     */
    bool tryPlaceContainedLiquid(Player* player, IWorld& world, const BlockPos& pos, const BlockRaycastResult& hit);

    /**
     * @brief 检查方块是否可以容纳流体
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @return 是否可以容纳流体
     */
    bool canBlockContainFluid(IWorld& world, const BlockPos& pos, const BlockState& state) const;

private:
    fluid::Fluid* m_containedFluid;
};

} // namespace mc
