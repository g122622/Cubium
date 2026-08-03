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
#include "MerchantOffer.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace nbt {
namespace tags {
struct compound_tag;
}
} // namespace nbt

// 前向声明
class Player;
class Entity;
class ItemStack;

namespace world {
namespace village {
namespace trade {

/**
 * @brief 交易列表
 *
 * 管理商人的所有交易优惠
 */
class MerchantOffers {
public:
    MerchantOffers() = default;

    /**
     * @brief 添加交易
     */
    void addOffer(std::unique_ptr<MerchantOffer> offer);

    /**
     * @brief 移除交易
     */
    void removeOffer(size_t index);

    /**
     * @brief 获取交易
     */
    [[nodiscard]] MerchantOffer* getOffer(size_t index);
    [[nodiscard]] const MerchantOffer* getOffer(size_t index) const;

    /**
     * @brief 获取交易数量
     */
    [[nodiscard]] size_t size() const noexcept { return m_offers.size(); }

    /**
     * @brief 是否为空
     */
    [[nodiscard]] bool empty() const noexcept { return m_offers.empty(); }

    /**
     * @brief 根据支付物品查找匹配的交易
     * @param buyA 第一个支付物品
     * @param buyB 第二个支付物品（可选）
     * @param hint 选中的交易索引提示（-1表示无提示）
     * @return 匹配的交易指针，未找到返回nullptr
     */
    [[nodiscard]] MerchantOffer* getOfferFor(const ItemStack& buyA, const ItemStack& buyB, i32 hint = -1);

    /**
     * @brief 补货所有交易
     */
    void restockAll();

    /**
     * @brief 更新所有交易的需求值
     *
     * MC原版逻辑：对每个交易调用 updateDemand()，
     * 根据交易使用情况调整需求值和价格。
     */
    void updateDemandAll();

    /**
     * @brief 检查是否有交易需要补货
     * @return 如果有任何交易被使用过（uses > 0）则返回true
     */
    [[nodiscard]] bool needsRestockAny() const;

    /**
     * @brief 重置所有交易的每日补货计数
     */
    void resetDailyRestockAll();

    /**
     * @brief 更新价格（基于声誉）
     */
    void updatePrices(f32 modifier);

    // 序列化
    void serialize(nbt::tags::compound_tag& tag) const;
    static MerchantOffers deserialize(const nbt::tags::compound_tag& tag);

private:
    std::vector<std::unique_ptr<MerchantOffer>> m_offers;
};

/**
 * @brief 商人接口
 *
 * 所有可交易实体的接口（村民、流浪商人等）
 */
class IMerchant {
public:
    virtual ~IMerchant() = default;

    /**
     * @brief 获取交易列表
     */
    [[nodiscard]] virtual MerchantOffers& getOffers() = 0;
    [[nodiscard]] virtual const MerchantOffers& getOffers() const = 0;

    /**
     * @brief 设置交易列表（移动语义）
     */
    virtual void setOffers(MerchantOffers offers) = 0;

    /**
     * @brief 覆盖交易列表（用于客户端同步）
     */
    virtual void overrideOffers(MerchantOffers offers) = 0;

    /**
     * @brief 是否有交易
     */
    [[nodiscard]] virtual bool hasOffers() const = 0;

    /**
     * @brief 获取当前交易对象
     */
    [[nodiscard]] virtual Player* getTradingPlayer() const = 0;

    /**
     * @brief 开始交易
     */
    virtual void startTrading(Player* player) = 0;

    /**
     * @brief 结束交易
     */
    virtual void stopTrading() = 0;

    /**
     * @brief 是否正在交易
     */
    [[nodiscard]] virtual bool isTrading() const = 0;

    /**
     * @brief 获取经验值
     */
    [[nodiscard]] virtual i32 getExperience() const = 0;

    /**
     * @brief 设置经验值
     */
    virtual void setExperience(i32 exp) = 0;

    /**
     * @brief 增加经验值
     */
    virtual void addExperience(i32 amount) = 0;

    /**
     * @brief 补货
     */
    virtual void restock() = 0;

    /**
     * @brief 通知交易执行
     * @param offer 已执行的交易
     *
     * 当交易成功执行时调用，用于增加使用次数、播放音效、奖励经验等。
     */
    virtual void notifyTrade(MerchantOffer& offer) = 0;

    /**
     * @brief 通知交易更新（支付槽物品变化时）
     * @param resultStack 结果槽物品
     *
     * 当交易输入变化时调用，用于播放确认/否定音效。
     */
    virtual void notifyTradeUpdated(const ItemStack& resultStack) = 0;

    /**
     * @brief 获取村民经验值（用于交易界面显示）
     */
    [[nodiscard]] virtual i32 getVillagerXp() const = 0;

    /**
     * @brief 覆盖经验值（用于客户端同步）
     */
    virtual void overrideXp(i32 xp) = 0;

    /**
     * @brief 是否显示经验进度条
     */
    [[nodiscard]] virtual bool showProgressBar() const = 0;

    /**
     * @brief 是否可以补货
     */
    [[nodiscard]] virtual bool canRestock() const = 0;

    /**
     * @brief 是否在客户端
     */
    [[nodiscard]] virtual bool isClientSide() const = 0;

    /**
     * @brief 检查商人是否仍可被指定玩家交易
     * @param player 玩家
     * @return 如果可以交易返回true
     */
    [[nodiscard]] virtual bool stillValid(const Player& player) const = 0;

    /**
     * @brief 获取商人对应的实体（用于播放音效等）
     * @return 实体指针，如果不适用返回nullptr
     */
    [[nodiscard]] virtual Entity* asEntity() = 0;
    [[nodiscard]] virtual const Entity* asEntity() const = 0;
};

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
