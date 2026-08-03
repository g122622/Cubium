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
 * LIABILITY, IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "../../core/Item.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace item {

/**
 * @brief 不祥之瓶物品
 *
 * 可饮用的药水物品，饮用后给予不祥之兆效果。
 * 效果等级由物品的damage值（0-4）决定，对应I-V级不祥之兆。
 * 饮用后返还玻璃瓶。
 *
 * 获取方式：
 * - 从宝库战利品表中以一定概率获得
 * - 不同等级的不祥之瓶从不同的战利品表中获得
 *
 * 命名空间ID: minecraft:ominous_bottle
 */
class OminousBottleItem final : public Item {
public:
    /**
     * @brief 构造不祥之瓶
     * @param properties 物品属性
     */
    explicit OminousBottleItem(const ItemProperties& properties);

    /**
     * @brief 获取不祥之兆等级（I-V）
     * @param damage 物品damage值 (0-4)
     * @return 不祥之兆等级 (1-5)
     */
    [[nodiscard]] static i32 getBadOmenLevel(i32 damage);

    /// 不祥之瓶的最大damage值（对应V级不祥之兆）
    static constexpr i32 MAX_DAMAGE = 4;
};

} // namespace item
} // namespace mc
