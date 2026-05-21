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

#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/RandomRanges.hpp"

namespace mc {
namespace loot {

/**
 * @brief 时运条件
 *
 * 用于检测工具上的时运附魔等级。
 * 参考: net.minecraft.loot.conditions.TableBonus
 *
 * 时运附魔增加矿石掉落数量的概率。
 * 等级1-3，每个等级增加额外掉落的概率。
 */
class FortuneCondition : public LootCondition {
public:
    /**
     * @brief 构造时运条件
     * @param minLevel 最小时运等级（默认0，表示无时运也可满足）
     */
    explicit FortuneCondition(i32 minLevel = 0);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "fortune"; }

    /**
     * @brief 获取时运等级
     *
     * 从掉落上下文中获取时运附魔等级。
     *
     * @param context 掉落上下文
     * @return 时运等级（0-3）
     */
    [[nodiscard]] static i32 getFortuneLevel(LootContext& context);

    /**
     * @brief 计算时运加成后的掉落数量
     *
     * 参考 MC 1.16.5: Fortune对矿石的影响：
     * - Fortune I: 33%概率掉落+1
     * - Fortune II: 25%概率掉落+1, 25%概率掉落+2
     * - Fortune III: 20%概率掉落+1, 20%概率掉落+2, 20%概率掉落+3
     *
     * @param baseCount 基础掉落数量
     * @param fortuneLevel 时运等级（0-3）
     * @param random 随机数生成器
     * @return 加成后的掉落数量
     */
    [[nodiscard]] static i32 applyFortuneBonus(i32 baseCount, i32 fortuneLevel, math::Random& random);

private:
    i32 m_minLevel;
};

} // namespace loot
} // namespace mc
