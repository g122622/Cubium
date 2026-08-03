/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "common/item/loot/conditions/FishingOpenWaterCondition.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include <memory>

namespace mc {
namespace loot {

FishingOpenWaterCondition::FishingOpenWaterCondition(bool requireOpenWater)
    : m_requireOpenWater(requireOpenWater)
{}

bool FishingOpenWaterCondition::test(LootContext& context) const
{
    // 从上下文中获取开放水域状态
    bool* openWaterPtr = context.get<bool>(LootParams::IS_IN_OPEN_WATER);
    if (openWaterPtr == nullptr) {
        // 如果没有设置开放水域参数，默认返回 false（非开放水域）
        return !m_requireOpenWater;
    }

    // 检查是否满足开放水域条件
    return m_requireOpenWater == *openWaterPtr;
}

std::unique_ptr<LootCondition> FishingOpenWaterCondition::clone() const noexcept
{
    return std::make_unique<FishingOpenWaterCondition>(m_requireOpenWater);
}

} // namespace loot
} // namespace mc
