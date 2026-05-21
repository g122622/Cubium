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

#include "ExplorationMapFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace loot {

ExplorationMapFunction::ExplorationMapFunction(Destination destination)
    : m_destination(destination)
{}

ItemStack ExplorationMapFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // TODO: 实现探险地图生成
    // 参考: net.minecraft.loot.functions.ExplorationMap
    // 需要地图系统和世界探索追踪支持

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> ExplorationMapFunction::clone() const
{
    auto func = std::make_unique<ExplorationMapFunction>(m_destination);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
