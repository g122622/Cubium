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

#include "SetInstrumentFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/instrument/InstrumentTags.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/random/Random.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace loot {

SetInstrumentFunction::SetInstrumentFunction(ResourceLocation instrumentTagId)
    : m_instrumentTagId(std::move(instrumentTagId))
{}

ItemStack SetInstrumentFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }
    // 成员来自乐器标签注册表（内置占位 + 数据包 tags/instrument 加载结果）
    const auto& registry = item::instrument::InstrumentTags::instance();
    const item::instrument::InstrumentTag* tag = registry.getTag(m_instrumentTagId);
    if (tag == nullptr || tag->size() == 0) {
        return stack;
    }
    const auto& members = tag->members();
    const i32 index = context.getRandom().nextInt(static_cast<i32>(members.size()));
    stack.setInstrument(members[static_cast<usize>(index)].toString());
    return stack;
}

std::unique_ptr<LootFunction> SetInstrumentFunction::clone() const noexcept
{
    auto func = std::make_unique<SetInstrumentFunction>(m_instrumentTagId);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

const std::vector<ResourceLocation>* SetInstrumentFunction::getInstrumentTag(const ResourceLocation& tagId)
{
    const item::instrument::InstrumentTag* tag = item::instrument::InstrumentTags::instance().getTag(tagId);
    return tag == nullptr ? nullptr : &tag->members();
}

} // namespace loot
} // namespace mc