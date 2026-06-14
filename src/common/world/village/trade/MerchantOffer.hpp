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
#include <memory>
#include <optional>

namespace mc {

// 前向声明
class Player;

namespace nbt {
namespace tags {
struct compound_tag;
}
} // namespace nbt

namespace world {
namespace village {
namespace trade {

// 前向声明
class IMerchant;

/**
 * @brief 商人交易优惠
 *
 * 单个交易项，包含买入物品A、可选的买入物品B、卖出物品、使用次数等。
 */
class MerchantOffer {
public:
    /**
     * @brief 默认构造函数
     */
    MerchantOffer() = default;

    /**
     * @brief 构造函数（单物品交易）
     * @param buyA 买入物品
     * @param sell 卖出物品
     * @param maxUses 最大使用次数
     * @param xp 交易给予的经验
     * @param priceMultiplier 价格乘数
     */
    MerchantOffer(ItemStack buyA, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier);

    /**
     * @brief 构造函数（双物品交易）
     * @param buyA 第一买入物品
     * @param buyB 第二买入物品（可选）
     * @param sell 卖出物品
     * @param maxUses 最大使用次数
     * @param xp 交易给予的经验
     * @param priceMultiplier 价格乘数
     */
    MerchantOffer(ItemStack buyA, ItemStack buyB, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier);

    // ========== 交易检查 ==========

    /**
     * @brief 检查交易是否可行
     * @param offeredA 玩家提供的第一物品
     * @param offeredB 玩家提供的第二物品
     * @return 是否可以交易
     */
    [[nodiscard]] bool canAccept(const ItemStack& offeredA, const ItemStack& offeredB) const;

    /**
     * @brief 检查交易是否可行（单物品）
     */
    [[nodiscard]] bool canAccept(const ItemStack& offered) const;

    // ========== 交易操作 ==========

    /**
     * @brief 执行交易
     * @param player 玩家
     * @param merchant 商人
     * @return 是否成功
     */
    bool apply(class Player& player, IMerchant& merchant);

    /**
     * @brief 增加使用次数
     */
    void increaseUses() noexcept { ++m_uses; }

    /**
     * @brief 补货（重置使用次数）
     */
    void restock();

    /**
     * @brief 尝试从支付物品中扣除交易所需数量
     * @param buyA 第一个支付物品（会被修改）
     * @param buyB 第二个支付物品（会被修改）
     * @return 是否成功扣除
     *
     * 检查提供的物品是否满足交易条件，如果满足则扣除相应数量。
     */
    bool take(ItemStack& buyA, ItemStack& buyB);

    /**
     * @brief 获取交易结果的拷贝
     * @return 卖出物品的拷贝
     */
    [[nodiscard]] ItemStack assemble() const { return m_sell.copy(); }

    // ========== 状态查询 ==========

    /**
     * @brief 是否售罄
     */
    [[nodiscard]] bool isOutOfStock() const noexcept { return m_uses >= m_maxUses; }

    /**
     * @brief 是否禁用（售罄且无法补货）
     */
    [[nodiscard]] bool isDisabled() const;

    /**
     * @brief 获取使用次数
     */
    [[nodiscard]] i32 getUses() const noexcept { return m_uses; }

    /**
     * @brief 获取最大使用次数
     */
    [[nodiscard]] i32 getMaxUses() const noexcept { return m_maxUses; }

    /**
     * @brief 获取剩余使用次数
     */
    [[nodiscard]] i32 getRemainingUses() const noexcept { return m_maxUses - m_uses; }

    /**
     * @brief 获取使用进度（0.0-1.0）
     */
    [[nodiscard]] f32 getProgress() const noexcept;

    // ========== 物品获取 ==========

    /**
     * @brief 获取第一买入物品
     */
    [[nodiscard]] const ItemStack& getBuyA() const noexcept { return m_buyA; }
    ItemStack& getBuyA() noexcept { return m_buyA; }

    /**
     * @brief 获取第二买入物品
     */
    [[nodiscard]] const std::optional<ItemStack>& getBuyB() const noexcept { return m_buyB; }
    std::optional<ItemStack>& getBuyB() noexcept { return m_buyB; }

    /**
     * @brief 获取卖出物品
     */
    [[nodiscard]] const ItemStack& getSell() const noexcept { return m_sell; }
    ItemStack& getSell() noexcept { return m_sell; }

    // ========== 价格调整 ==========

    /**
     * @brief 获取价格乘数
     */
    [[nodiscard]] f32 getPriceMultiplier() const noexcept { return m_priceMultiplier; }

    /**
     * @brief 获取特殊价格修正
     */
    [[nodiscard]] i32 getSpecialPrice() const noexcept { return m_specialPrice; }

    /**
     * @brief 设置特殊价格修正
     */
    void setSpecialPrice(i32 price) noexcept { m_specialPrice = price; }

    /**
     * @brief 获取需求修正
     */
    [[nodiscard]] i32 getDemand() const noexcept { return m_demand; }

    /**
     * @brief 设置需求修正
     */
    void setDemand(i32 demand) noexcept { m_demand = demand; }

    /**
     * @brief 应用需求调整
     * @param demandBonus 需求加成
     */
    void applyDemand(i32 demandBonus);

    /**
     * @brief 获取调整后的买入价格
     */
    [[nodiscard]] i32 getAdjustedBuyPrice() const noexcept;

    // ========== 经验 ==========

    /**
     * @brief 获取交易给予的经验
     */
    [[nodiscard]] i32 getXp() const noexcept { return m_xp; }

    // ========== 补货 ==========

    /**
     * @brief 获取今日补货次数
     */
    [[nodiscard]] i32 getRestocksToday() const noexcept { return m_restocksToday; }

    /**
     * @brief 重置每日补货计数
     */
    void resetDailyRestock() noexcept { m_restocksToday = 0; }

    /**
     * @brief 是否应该奖励经验球
     */
    [[nodiscard]] bool shouldRewardExp() const noexcept { return m_rewardExp; }

    // ========== 序列化 ==========

    /**
     * @brief 序列化到NBT
     */
    void serialize(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从NBT反序列化
     */
    static MerchantOffer deserialize(const nbt::tags::compound_tag& tag);

private:
    /// 第一买入物品
    ItemStack m_buyA;

    /// 第二买入物品（可选）
    std::optional<ItemStack> m_buyB;

    /// 卖出物品
    ItemStack m_sell;

    /// 已使用次数
    i32 m_uses = 0;

    /// 最大使用次数
    i32 m_maxUses = 12;

    /// 交易给予的经验
    i32 m_xp = 0;

    /// 价格乘数
    f32 m_priceMultiplier = 1.0f;

    /// 特殊价格修正（来自流言）
    i32 m_specialPrice = 0;

    /// 需求修正（动态调整）
    i32 m_demand = 0;

    /// 今日补货次数
    i32 m_restocksToday = 0;

    /// 上次补货的游戏时间
    i64 m_lastRestockTime = 0;

    /// 是否奖励经验球（默认true）
    bool m_rewardExp = true;
};

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
