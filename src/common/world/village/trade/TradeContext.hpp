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

#include <cstddef>
#include <optional>

namespace mc {

class IWorld;

namespace entity {
class VillagerEntity;
enum class VillagerType : u8;
} // namespace entity

namespace world {
namespace village {
namespace trade {

/**
 * @brief 交易生成上下文
 *
 * vanilla 的交易生成签名是 `ItemListing.getOffer(ServerLevel, Entity trader, RandomSource)`，
 * 本结构把这三种信息来源收敛为一处：
 * - world：首选的随机源（与实体 tick 共享的世界随机数流），同时提供卖家位置、种子与
 *   结构查询能力；为 nullptr 时（单元测试等离线场景）退回 traderSeed；
 * - trader：卖家村民，读取其村民类型（决定地图/船只/盔甲的分支）与位置（寻宝地图搜索起点）。
 *
 * 显式传入而不再用默认参数，避免"值从哪来"的歧义。
 */
struct TradeContext {
    /// 世界（可为 nullptr，表示离线生成）
    IWorld* world = nullptr;

    /// 卖家村民（可为 nullptr，表示无村民类型的离线生成）
    entity::VillagerEntity* trader = nullptr;

    /// 离线随机种子（world 为 nullptr 时作为随机数流来源）
    u64 traderSeed = 0;

    /// 交易生成序号（同一实体的不同生成批次使用不同值，避免同种子重复）
    std::size_t sequence = 0;
};

/**
 * @brief 读取交易上下文中的村民类型
 * @param ctx 交易上下文
 * @return 村民类型；trader 为空或不是村民时返回 std::nullopt
 */
[[nodiscard]] std::optional<entity::VillagerType> tradeVillagerType(const TradeContext& ctx);

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc