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
#include "entity/inventory/IInventory.hpp"
#include "item/core/ItemStack.hpp"
#include <vector>

namespace mc {
namespace world::village::trade {
class IMerchant;
class MerchantOffer;
} // namespace world::village::trade

using IMerchant = world::village::trade::IMerchant;
using MerchantOffer = world::village::trade::MerchantOffer;

/**
 * @brief 村民交易容器
 *
 * 管理交易界面的3个槽位：支付槽1、支付槽2和结果槽。
 * 当支付槽物品变化时自动匹配交易并更新结果槽。
 */
class MerchantContainer : public IInventory {
public:
    /// 支付槽1索引
    static constexpr i32 SLOT_BUY_A = 0;
    /// 支付槽2索引
    static constexpr i32 SLOT_BUY_B = 1;
    /// 结果槽索引
    static constexpr i32 SLOT_RESULT = 2;
    /// 容器大小
    static constexpr i32 CONTAINER_SIZE = 3;

    /**
     * @brief 构造交易容器
     * @param merchant 商民接口
     */
    explicit MerchantContainer(IMerchant& merchant);

    ~MerchantContainer() override = default;

    // 禁止拷贝
    MerchantContainer(const MerchantContainer&) = delete;
    MerchantContainer& operator=(const MerchantContainer&) = delete;
    // 允许移动
    MerchantContainer(MerchantContainer&&) noexcept = default;
    MerchantContainer& operator=(MerchantContainer&&) noexcept = default;

    // ========== IInventory 接口 ==========

    [[nodiscard]] i32 getContainerSize() const override { return CONTAINER_SIZE; }
    [[nodiscard]] bool isEmpty() const override;
    [[nodiscard]] ItemStack getItem(i32 slot) const override;
    ItemStack removeItem(i32 slot, i32 count) override;
    ItemStack removeItemNoUpdate(i32 slot) override;
    void setItem(i32 slot, const ItemStack& stack) override;
    void clear() override;
    void setChanged() override;
    [[nodiscard]] bool isUsableByPlayer(const Player& player) const override;
    [[nodiscard]] bool canPlaceItem(i32 slot, const ItemStack& stack) const override;

    // ========== 交易特定方法 ==========

    /**
     * @brief 更新结果槽物品
     *
     * 根据支付槽的物品匹配交易报价，将结果物品放入结果槽。
     * 当支付槽物品变化时自动调用。
     */
    void updateSellItem();

    /**
     * @brief 获取当前匹配的交易报价
     * @return 匹配的交易报价指针，无匹配时返回nullptr
     */
    [[nodiscard]] MerchantOffer* getActiveOffer();
    [[nodiscard]] const MerchantOffer* getActiveOffer() const;

    /**
     * @brief 设置选中的交易提示索引
     * @param hint 交易索引（玩家点击的交易行）
     */
    void setSelectionHint(i32 hint);

    /**
     * @brief 获取待获得的经验值
     * @return 待获得的XP数量
     */
    [[nodiscard]] i32 getFutureXp() const { return m_futureXp; }

private:
    /**
     * @brief 检查是否为支付槽
     */
    [[nodiscard]] static bool _isPaymentSlot(i32 slot);

    IMerchant& m_merchant;
    std::vector<ItemStack> m_items;
    MerchantOffer* m_activeOffer = nullptr;
    i32 m_selectionHint = -1;
    i32 m_futureXp = 0;
};

} // namespace mc
