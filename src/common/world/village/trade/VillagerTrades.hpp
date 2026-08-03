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
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {
namespace world {
namespace village {
namespace trade {

/**
 * @brief 交易配方工厂函数类型
 *
 * 根据需求和随机种子生成一个交易优惠。
 * 使用函数而不是静态配方，以支持动态价格调整。
 */
using TradeFactory = std::function<std::unique_ptr<MerchantOffer>(i32 demand, u64 seed)>;

/**
 * @brief 村民交易配方表
 *
 * 管理所有村民职业的交易配方。
 * 每个职业有5个等级（新手1-5级大师），每个等级有多个交易选项。
 */
class VillagerTrades {
public:
    /**
     * @brief 初始化所有交易配方
     *
     * 在游戏启动时调用，注册所有职业的交易配方。
     */
    static void initialize();

    /**
     * @brief 获取指定职业和等级的交易列表
     * @param profession 职业
     * @param type 村民类型（影响某些外观相关交易）
     * @param level 等级（1-5）
     * @param demand 需求修正
     * @param seed 随机种子
     * @return 交易列表
     */
    [[nodiscard]] static std::unique_ptr<MerchantOffers> generateOffers(
        entity::VillagerProfession profession, entity::VillagerType type, i32 level, i32 demand = 0, u64 seed = 0);

    /**
     * @brief 检查职业是否有交易
     */
    [[nodiscard]] static bool hasTrades(entity::VillagerProfession profession);

    /**
     * @brief 获取职业的交易等级数量
     */
    [[nodiscard]] static i32 getTradeLevelCount(entity::VillagerProfession profession);

private:
    // 交易配方存储：[职业][等级] -> 交易工厂列表
    using LevelTrades = std::unordered_map<i32, std::vector<TradeFactory>>;
    static std::unordered_map<entity::VillagerProfession, LevelTrades> s_trades;

    // 是否已初始化
    static bool s_initialized;

    // ========== 职业交易注册 ==========

    static void _registerArmorerTrades();
    static void _registerButcherTrades();
    static void _registerCartographerTrades();
    static void _registerClericTrades();
    static void _registerFarmerTrades();
    static void _registerFishermanTrades();
    static void _registerFletcherTrades();
    static void _registerLeatherworkerTrades();
    static void _registerLibrarianTrades();
    static void _registerMasonTrades();
    static void _registerShepherdTrades();
    static void _registerToolsmithTrades();
    static void _registerWeaponsmithTrades();
    static void _registerNitwitTrades();

    // ========== 交易工厂辅助方法 ==========

    /**
     * @brief 创建简单物品交易（单物品买入）
     * @param buyItem 买入物品
     * @param buyCount 买入数量
     * @param sellItem 卖出物品
     * @param sellCount 卖出数量
     * @param maxUses 最大使用次数
     * @param xp 交易经验
     * @param priceMultiplier 价格乘数
     */
    static TradeFactory _simpleTrade(const char* buyItem,
        i32 buyCount,
        const char* sellItem,
        i32 sellCount,
        i32 maxUses = 12,
        i32 xp = 2,
        f32 priceMultiplier = 0.05f);

    /**
     * @brief 创建双物品交易
     * @param buyItemA 第一买入物品
     * @param buyCountA 第一买入数量
     * @param buyItemB 第二买入物品
     * @param buyCountB 第二买入数量
     * @param sellItem 卖出物品
     * @param sellCount 卖出数量
     * @param maxUses 最大使用次数
     * @param xp 交易经验
     * @param priceMultiplier 价格乘数
     */
    static TradeFactory _twoItemTrade(const char* buyItemA,
        i32 buyCountA,
        const char* buyItemB,
        i32 buyCountB,
        const char* sellItem,
        i32 sellCount,
        i32 maxUses = 12,
        i32 xp = 2,
        f32 priceMultiplier = 0.05f);

    /**
     * @brief 创建带需求调整的交易
     * @param baseBuyCount 基础买入数量
     * @param buyItem 买入物品
     * @param sellItem 卖出物品
     * @param sellCount 卖出数量
     * @param maxUses 最大使用次数
     * @param xp 交易经验
     * @param priceMultiplier 价格乘数
     */
    static TradeFactory _demandTrade(i32 baseBuyCount,
        const char* buyItem,
        const char* sellItem,
        i32 sellCount,
        i32 maxUses = 12,
        i32 xp = 2,
        f32 priceMultiplier = 0.05f);

    /**
     * @brief 注册交易配方
     * @param profession 职业
     * @param level 等级（1-5）
     * @param factory 交易工厂
     */
    static void _registerTrade(entity::VillagerProfession profession, i32 level, TradeFactory factory);
};

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
