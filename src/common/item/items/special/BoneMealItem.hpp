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

#include "../../core/Item.hpp"
#include "common/item/core/ActionResult.hpp"

// Forward declarations
namespace mc {
namespace math {
class IRandom;
}
class Player;
class IWorld;
class BlockPos;
class BlockState;
class ItemStack;
} // namespace mc

namespace mc {
namespace item::items {

/**
 * @brief 骨粉物品
 *
 * 骨粉是一种特殊的物品，可以用于加速植物生长。
 * 主要功能：
 * 1. 对 IGrowable 方块使用：加速生长
 * 2. 在水下使用：生成海草
 *
 * 参考: net.minecraft.item.BoneMealItem
 */
class BoneMealItem : public Item {
public:
    /**
     * @brief 构造骨粉物品
     * @param properties 物品属性
     */
    explicit BoneMealItem(ItemProperties properties);

    ~BoneMealItem() override = default;

    /**
     * @brief 在方块上使用物品
     *
     * 对植物使用骨粉，加速生长。
     * 成功时消耗一个骨粉并生成快乐村民粒子。
     *
     * @param context 物品使用上下文
     * @return 动作结果类型
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 应用骨粉效果
     *
     * 静态方法，可直接调用以应用骨粉效果。
     * 检查目标方块是否实现 IGrowable，如果是则调用 grow()。
     *
     * @param stack 物品堆
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家（可为nullptr）
     * @return 是否成功应用
     */
    static bool applyBonemeal(ItemStack& stack, IWorld& world, const BlockPos& pos, Player* player);

    /**
     * @brief 在水下生成海草
     *
     * 在水下使用骨粉时，有概率在周围生成海草。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param random 随机数生成器
     * @return 是否成功生成
     */
    static bool growSeagrass(IWorld& world, const BlockPos& pos, math::IRandom& random);
};

} // namespace item::items
} // namespace mc
