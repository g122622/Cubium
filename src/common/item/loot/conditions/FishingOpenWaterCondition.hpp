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
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 钓鱼开放水域条件
 *
 * 检查钓鱼是否在开放水域进行。
 * 用于钓鱼掉落表中宝藏条目的条件判断。
 *
 * MC 1.16.5 中，宝藏只有在开放水域才能钓到。
 * 开放水域定义：浮标周围 5x4x5 区域（X-2到X+2，Y-1到Y+2，Z-2到Z+2）
 * - 水面上方层：必须是空气或睡莲
 * - 水层：必须是水源方块
 */
class FishingOpenWaterCondition : public LootCondition {
public:
    /**
     * @brief 构造开放水域条件
     * @param requireOpenWater 是否需要开放水域（默认 true）
     */
    explicit FishingOpenWaterCondition(bool requireOpenWater = true);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "fishing_hook_in_open_water"; }

    [[nodiscard]] bool requireOpenWater() const { return m_requireOpenWater; }

private:
    bool m_requireOpenWater;
};

} // namespace loot
} // namespace mc
