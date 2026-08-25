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

#include "UnbreakingEnchantment.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
namespace item {
namespace enchant {

// TODO 死代码：shouldConsumeDurability / shouldArmorConsumeDurability 从未被损耗链路调用。
// 实际生效的耐久损耗忽略逻辑在 EnchantmentHelper::shouldIgnoreDurabilityLoss（EnchantmentHelper.cpp:324），
// 由 ItemStack::attemptDamageItem 调用。此处两个方法用 random.nextFloat() >= chance 实现，
// 偏离 vanilla 1.21.11 的 nextInt(level+1)>0（工具）/ nextFloat()<0.6 门控+nextInt（盔甲）语义，
// 且与 EnchantmentHelper::shouldIgnoreDurabilityLoss 功能重复。保留仅因部分测试/文档可能引用，
// 勿接入损耗链路（会偏离 vanilla 随机序列与边界精度）。未来应统一到 EnchantmentHelper 删除此处。
bool UnbreakingEnchantment::shouldConsumeDurability(i32 level, math::Random& random)
{
    if (level <= 0) {
        return true;
    }
    // 有 level/(level+1) 的概率不消耗耐久
    // I: 50%, II: 67%, III: 75%
    f32 chance = static_cast<f32>(level) / static_cast<f32>(level + 1);
    return random.nextFloat() >= chance;
}

// TODO 死代码：见上 shouldConsumeDurability 注释，同理从未被调用，勿接入损耗链路。
bool UnbreakingEnchantment::shouldArmorConsumeDurability(i32 level, math::Random& random)
{
    if (level <= 0) {
        return true;
    }
    // 盔甲有 60% 概率忽略耐久保护
    // 所以实际保护概率 = 0.4 * (level / (level + 1))
    f32 chance = 0.4f * static_cast<f32>(level) / static_cast<f32>(level + 1);
    return random.nextFloat() >= chance;
}

} // namespace enchant
} // namespace item
} // namespace mc
