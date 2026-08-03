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

#include "Merchant.hpp"
#include "MerchantOffer.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace village {
namespace trade {

/**
 * @brief 流浪商人交易配方表
 *
 * 管理流浪商人的所有交易配方。
 * 流浪商人交易分为普通交易和稀有交易两个层级，
 * 每次刷新时随机从交易池中选取一部分交易。
 */
class WanderingTraderTrades {
public:
    /**
     * @brief 交易配方工厂函数类型
     *
     * 根据随机种子生成一个交易优惠。
     */
    using TradeFactory = std::function<std::unique_ptr<MerchantOffer>(u64 seed)>;

    /**
     * @brief 初始化所有交易配方
     *
     * 在游戏启动时调用，注册所有流浪商人交易。
     */
    static void initialize();

    /**
     * @brief 生成普通交易列表
     * @param seed 随机种子
     * @param count 要生成的交易数量（MC 默认从普通池选取）
     * @return 交易列表
     */
    [[nodiscard]] static std::unique_ptr<MerchantOffers> generateNormalOffers(u64 seed, i32 count = -1);

    /**
     * @brief 生成稀有交易列表
     * @param seed 随机种子
     * @param count 要生成的交易数量（MC 默认从稀有池选取）
     * @return 交易列表
     */
    [[nodiscard]] static std::unique_ptr<MerchantOffers> generateRareOffers(u64 seed, i32 count = -1);

    /**
     * @brief 生成完整的交易列表
     * @param seed 随机种子
     * @return 包含普通交易和稀有交易的完整列表
     */
    [[nodiscard]] static std::unique_ptr<MerchantOffers> generateOffers(u64 seed = 0);

    /**
     * @brief 获取普通交易数量
     */
    [[nodiscard]] static size_t getNormalTradeCount() { return s_normalTrades.size(); }

    /**
     * @brief 获取稀有交易数量
     */
    [[nodiscard]] static size_t getRareTradeCount() { return s_rareTrades.size(); }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized() { return s_initialized; }

private:
    // 普通交易列表（等级1）
    static std::vector<TradeFactory> s_normalTrades;
    // 稀有交易列表（等级2）
    static std::vector<TradeFactory> s_rareTrades;
    // 是否已初始化
    static bool s_initialized;

    // ========== 交易工厂辅助方法 ==========

    /**
     * @brief 创建物品换绿宝石交易（玩家出售物品获得绿宝石）
     * @param sellItem 玩家出售的物品
     * @param sellCount 出售数量
     * @param emeraldCount 获得的绿宝石数量
     * @param maxUses 最大使用次数
     * @param xp 交易经验
     * @return 交易工厂
     */
    static TradeFactory _sellForEmeralds(
        const char* sellItem, i32 sellCount, i32 emeraldCount, i32 maxUses = 12, i32 xp = 1);

    /**
     * @brief 创建绿宝石换物品交易（玩家用绿宝石购买物品）
     * @param emeraldCount 绿宝石数量
     * @param buyItem 购买的物品
     * @param buyCount 购买数量
     * @param maxUses 最大使用次数
     * @param xp 交易经验
     * @return 交易工厂
     */
    static TradeFactory _buyForEmeralds(
        i32 emeraldCount, const char* buyItem, i32 buyCount, i32 maxUses = 12, i32 xp = 1);

    /**
     * @brief 注册普通交易
     */
    static void _registerNormalTrade(TradeFactory factory);

    /**
     * @brief 注册稀有交易
     */
    static void _registerRareTrade(TradeFactory factory);

    // ========== 交易注册方法 ==========

    static void _registerOceanTrades();
    static void _registerPlantTrades();
    static void _registerFlowerTrades();
    static void _registerSeedTrades();
    static void _registerSaplingTrades();
    static void _registerDyeTrades();
    static void _registerCoralTrades();
    static void _registerRareItemTrades();
};

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
