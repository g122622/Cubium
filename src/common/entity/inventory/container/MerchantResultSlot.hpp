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
#include "entity/inventory/Slot.hpp"

namespace mc {
namespace world::village::trade {
class IMerchant;
} // namespace world::village::trade

using IMerchant = world::village::trade::IMerchant;

class MerchantContainer;

/**
 * @brief 村民交易结果槽
 *
 * 结果槽不允许放置物品，当玩家从结果槽取出物品时执行交易：
 * 消耗支付槽的物品，增加交易次数，给予商民经验。
 */
class MerchantResultSlot : public Slot {
public:
    /**
     * @brief 构造交易结果槽
     * @param player 玩家引用
     * @param merchant 商民接口
     * @param container 交易容器
     * @param slotIndex 槽位索引
     * @param x 显示X坐标
     * @param y 显示Y坐标
     */
    MerchantResultSlot(Player& player, IMerchant& merchant, MerchantContainer& container, i32 slotIndex, i32 x, i32 y);

    ~MerchantResultSlot() override = default;

    /**
     * @brief 结果槽不允许放置物品
     */
    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override;

    /**
     * @brief 重写移除方法，跟踪移除数量
     */
    ItemStack remove(i32 amount) override;

    /**
     * @brief 玩家从结果槽取出物品时执行交易
     */
    ItemStack onTake(Player& player, ItemStack stack) override;

private:
    /**
     * @brief 检查并触发成就
     */
    void _checkTakeAchievements(ItemStack& stack);

    Player& m_player;
    IMerchant& m_merchant;
    MerchantContainer& m_container;
    i32 m_removeCount = 0;
};

} // namespace mc
