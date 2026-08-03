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
 * IMPLIED, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/container/MerchantContainer.hpp"
#include "world/village/trade/Merchant.hpp"

#include <memory>

namespace mc {

class MerchantResultSlot;

using MerchantOffers = world::village::trade::MerchantOffers;

/**
 * @brief 村民交易容器菜单
 *
 * 管理交易界面的完整槽位布局：
 * - 支付槽1 (index 0)：第一个支付物品
 * - 支付槽2 (index 1)：第二个支付物品（可选）
 * - 结果槽 (index 2)：交易结果物品
 * - 玩家主背包 (index 3-29)：3x9 格
 * - 玩家快捷栏 (index 30-38)：1x9 格
 *
 * 当玩家从结果槽取出物品时，自动扣除支付槽物品并执行交易。
 * 当玩家点击交易列表中的某一项时，自动从背包中填充支付槽。
 */
class MerchantContainerMenu : public AbstractContainerMenu {
public:
    /// 支付槽1索引
    static constexpr i32 SLOT_PAYMENT_1 = 0;
    /// 支付槽2索引
    static constexpr i32 SLOT_PAYMENT_2 = 1;
    /// 结果槽索引
    static constexpr i32 SLOT_RESULT = 2;

    /// 玩家主背包起始索引
    static constexpr i32 INV_SLOT_START = 3;
    /// 玩家主背包结束索引（不含）
    static constexpr i32 INV_SLOT_END = 30;
    /// 玩家快捷栏起始索引
    static constexpr i32 HOTBAR_SLOT_START = 30;
    /// 玩家快捷栏结束索引（不含）
    static constexpr i32 HOTBAR_SLOT_END = 39;

    /// 交易槽位显示坐标
    static constexpr i32 PAYMENT1_X = 136;
    static constexpr i32 PAYMENT2_X = 162;
    static constexpr i32 RESULT_X = 220;
    static constexpr i32 ROW_Y = 37;

    /// 玩家背包起始坐标
    static constexpr i32 PLAYER_INV_X = 108;
    static constexpr i32 PLAYER_INV_Y = 84;

    /**
     * @brief 构造交易容器菜单
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param merchant 商民接口
     */
    MerchantContainerMenu(ContainerId id, PlayerInventory* playerInventory, IMerchant& merchant);

    ~MerchantContainerMenu() override = default;

    // ========== 容器接口 ==========

    /**
     * @brief 检查玩家是否仍可与商民交易
     */
    [[nodiscard]] bool stillValid(const Player& player) const override;

    /**
     * @brief 容器内容变化时更新结果槽
     */
    void slotsChanged(IInventory* inventory) override;

    /**
     * @brief 关闭容器时返回支付物品
     */
    void removed(Player& player) override;

    /**
     * @brief 结果槽不允许Shift+点击全取
     */
    [[nodiscard]] bool canMergeSlot(const ItemStack& stack, const Slot& slot) const override;

    // ========== 交易菜单方法 ==========

    /**
     * @brief 设置玩家选中的交易索引
     * @param hint 交易列表索引
     *
     * 当玩家在交易列表中点击某一交易时，设置选中提示并更新结果槽。
     */
    void setSelectionHint(i32 hint);

    /**
     * @brief 获取商民的交易报价列表
     */
    [[nodiscard]] MerchantOffers& getOffers() const;

    /**
     * @brief 设置交易报价列表
     */
    void setOffers(MerchantOffers offers);

    /**
     * @brief 获取商民经验值
     */
    [[nodiscard]] i32 getTraderXp() const;

    /**
     * @brief 获取待获得的经验值
     */
    [[nodiscard]] i32 getFutureXp() const;

    /**
     * @brief 设置商民经验值
     */
    void setXp(i32 xp);

    /**
     * @brief 获取商民等级
     */
    [[nodiscard]] i32 getTraderLevel() const { return m_merchantLevel; }

    /**
     * @brief 设置商民等级
     */
    void setMerchantLevel(i32 level) { m_merchantLevel = level; }

    /**
     * @brief 是否显示经验进度条
     */
    [[nodiscard]] bool showProgressBar() const { return m_showProgressBar; }

    /**
     * @brief 设置是否显示经验进度条
     */
    void setShowProgressBar(bool show) { m_showProgressBar = show; }

    /**
     * @brief 是否可以补货
     */
    [[nodiscard]] bool canRestock() const { return m_canRestock; }

    /**
     * @brief 设置是否可以补货
     */
    void setCanRestock(bool restock) { m_canRestock = restock; }

    /**
     * @brief 当玩家选择交易时，尝试从背包填充支付槽
     * @param offerIndex 交易列表中的索引
     */
    void tryMoveItems(i32 offerIndex);

protected:
    /**
     * @brief Shift+点击快速移动
     */
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

    /**
     * @brief 获取结果槽索引
     */
    [[nodiscard]] i32 getResultSlotIndex() const override { return SLOT_RESULT; }

private:
    /**
     * @brief 初始化槽位布局
     */
    void _initSlots(PlayerInventory* playerInventory);

    /**
     * @brief 从玩家背包移动物品到支付槽
     * @param paymentSlot 支付槽索引（0或1）
     * @param stack 需要匹配的物品堆
     */
    void _moveFromInventoryToPaymentSlot(i32 paymentSlot, const ItemStack& targetStack);

    /**
     * @brief 播放交易音效
     */
    void _playTradeSound();

    IMerchant& m_merchant;
    std::unique_ptr<MerchantContainer> m_tradeContainer;
    i32 m_merchantLevel = 1;
    bool m_showProgressBar = true;
    bool m_canRestock = false;
};

} // namespace mc
