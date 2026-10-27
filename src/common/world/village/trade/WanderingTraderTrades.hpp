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
#include "common/world/village/trade/TradeContext.hpp"
#include "common/world/village/trade/VillagerTrades.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace village {
namespace trade {

class MerchantOffers;

/**
 * @brief 流浪商人交易表（vanilla VillagerTrades.WANDERING_TRADER_TRADES）
 *
 * 原版把流浪商人交易分成若干"池"，每个池有独立的抽取次数；每次刷新时对每个池
 * 独立抽取指定次数的随机交易。这与"从一个大列表里随机挑 N 条"不同：池之间
 * 互不影响、同一池内可以抽到同一条目两次。
 */
class WanderingTraderTrades {
public:
    /// 交易池数量（原版 WANDERING_TRADER_TRADES 的 Pair 个数）
    static constexpr size_t TRADE_POOL_COUNT = 2;

    /**
     * @brief 初始化所有交易池
     *
     * 在游戏启动时调用；内部幂等。
     */
    static void initialize();

    /**
     * @brief 生成流浪商人的完整交易列表
     * @param ctx 生成上下文（世界 / 商人实体 / 离线种子）
     * @return 交易列表
     */
    [[nodiscard]] static std::unique_ptr<MerchantOffers> generateOffers(const TradeContext& ctx);

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized()
    {
        return s_initialized;
    }

    /**
     * @brief 获取指定池的条目数量
     * @param pool 池下标
     * @return 条目数量（下标越界返回 0）
     */
    [[nodiscard]] static size_t getPoolSize(size_t pool);

private:
    /// 单个交易池（条目 + 抽取次数）
    struct Pool {
        std::vector<std::unique_ptr<TradeListing>> listings;
        i32 selections = 0;
    };

    /// 全部交易池
    static std::array<Pool, TRADE_POOL_COUNT> s_pools;

    /// 是否已初始化
    static bool s_initialized;

    // ========== 交易池注册 ==========

    /**
     * @brief 注册第一个池：收购物品 + 常见商品（抽取 2 次）
     */
    static void _registerFirstPool();

    /**
     * @brief 注册第二个池：稀有商品（抽取 2 次）
     */
    static void _registerSecondPool();
};

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
