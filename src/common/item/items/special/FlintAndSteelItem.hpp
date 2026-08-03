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
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {

// Forward declarations
class BlockState;
class IWorld;
class Player;
class LivingEntity;
class BlockItemUseContext;
class Block;

namespace item {
namespace tool {

/**
 * @brief 打火石物品
 *
 * 打火石用于：
 * - 点燃方块（营火、蜡烛等）
 * - 在合适的位置放置火焰
 * - 点燃下界传送门
 *
 * 耐久度: 64 次
 *
 * 参考: net.minecraft.item.FlintAndSteelItem
 */
class FlintAndSteelItem : public Item {
public:
    /**
     * @brief 构造打火石
     * @param properties 物品属性（耐久度等）
     */
    explicit FlintAndSteelItem(ItemProperties properties);

    ~FlintAndSteelItem() override = default;

    /**
     * @brief 在方块上使用物品
     *
     * 功能：
     * 1. 如果点击的方块可以点燃（如未点燃的营火），点燃它
     * 2. 否则，在点击面的相邻位置放置火焰
     *
     * @param context 物品使用上下文
     * @return 动作结果类型
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 检查是否可以在指定位置放置火焰
     *
     * @param world 世界引用
     * @param pos 要放置火焰的位置
     * @return 是否可以放置火焰
     */
    [[nodiscard]] static bool canLightBlock(IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取应该放置的火焰方块
     *
     * 根据位置返回普通火或灵魂火。
     * - 如果下方是灵魂沙/灵魂土，返回灵魂火
     * - 否则返回普通火
     *
     * @param world 世界引用
     * @param pos 位置
     * @return 火焰方块指针（不会为空）
     */
    [[nodiscard]] static Block* getFireForPlacement(IWorld& world, const BlockPos& pos);

private:
};

} // namespace tool
} // namespace item
} // namespace mc
