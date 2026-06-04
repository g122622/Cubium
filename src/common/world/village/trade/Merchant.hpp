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
     * @brief 补货所有交易
     */
    void restockAll();

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
    [[nodiscard]] virtual MerchantOffers* getOffers() = 0;
    [[nodiscard]] virtual const MerchantOffers* getOffers() const = 0;

    /**
     * @brief 设置交易列表
     */
    virtual void setOffers(std::unique_ptr<MerchantOffers> offers) = 0;

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
};

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
