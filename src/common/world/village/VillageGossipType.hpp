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

#include "../../core/Types.hpp"
#include <string>

namespace mc {
namespace world {
namespace village {

/**
 * @brief 村庄流言类型
 *
 * 流言影响村民对玩家的态度，进而影响交易价格。
 * 正面流言降低价格，负面流言提高价格。
 *
 * 参考 MC 1.16.5 GossipType
 */
enum class VillageGossipType : u8 {
    /// 重大负面 - 攻击村民 (-100 声誉)
    MajorNegative = 0,

    /// 次要负面 - 小型伤害行为 (-20 声誉)
    MinorNegative,

    /// 交易 - 与村民交易 (+2 声誉)
    Trading,

    /// 次要正面 - 小型帮助行为 (+20 声誉)
    MinorPositive,

    /// 重大正面 - 治愈僵尸村民 (+100 声誉)
    MajorPositive
};

/**
 * @brief 流言类型工具类
 */
class GossipTypeHelper {
public:
    /**
     * @brief 获取流言类型名称
     */
    [[nodiscard]] static const char* getName(VillageGossipType type);

    /**
     * @brief 获取流言对声誉的影响值
     * @param type 流言类型
     * @return 声誉变化值（正数为正面影响）
     */
    [[nodiscard]] static i32 getReputationImpact(VillageGossipType type);

    /**
     * @brief 获取流言的最大值
     * @param type 流言类型
     * @return 该类型流言的最大累积值
     */
    [[nodiscard]] static i32 getMaxValue(VillageGossipType type);

    /**
     * @brief 获取流言的衰减间隔（游戏tick）
     * @param type 流言类型
     * @return 衰减间隔（tick）
     */
    [[nodiscard]] static i64 getDecayInterval(VillageGossipType type);

    /**
     * @brief 获取流言的衰减率
     * @param type 流言类型
     * @return 每次衰减保留的比例（0.0-1.0）
     */
    [[nodiscard]] static f32 getDecayRate(VillageGossipType type);

    /**
     * @brief 是否为负面流言
     */
    [[nodiscard]] static bool isNegative(VillageGossipType type);

    /**
     * @brief 是否为正面流言
     */
    [[nodiscard]] static bool isPositive(VillageGossipType type);
};

} // namespace village
} // namespace world
} // namespace mc
