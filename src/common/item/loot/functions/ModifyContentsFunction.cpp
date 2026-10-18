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

#include "ModifyContentsFunction.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

ModifyContentsFunction::ModifyContentsFunction(std::string component, std::unique_ptr<LootFunction> modifier)
    : m_component(std::move(component))
    , m_modifier(std::move(modifier))
{}

ItemStack ModifyContentsFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty() || m_modifier == nullptr) {
        return stack;
    }
    // 原版仅支持 ContainerComponentManipulators（项目已建模的容器类组件）
    if (m_component != "minecraft:container" && m_component != "container") {
        return stack;
    }
    const auto* contents = stack.getContainerContents();
    if (contents == nullptr) {
        return stack;
    }

    std::vector<item::component::ItemContainerContentsPayload::Slot> kept;
    kept.reserve(contents->slots.size());
    for (const auto& slot : contents->slots) {
        // 槽位物品按值拷贝（ItemStack 深拷贝，非 unique_ptr 指针复制）
        if (slot.item == nullptr || slot.item->isEmpty() || !m_modifier->testConditions(context)) {
            kept.push_back(item::component::ItemContainerContentsPayload::Slot{
                slot.index, slot.item != nullptr ? std::make_unique<ItemStack>(*slot.item) : nullptr});
            continue;
        }
        auto modified = m_modifier->apply(*slot.item, context);
        // 修饰函数丢弃物品时该槽位变空（不保留空槽）
        if (!modified.isEmpty()) {
            kept.push_back(item::component::ItemContainerContentsPayload::Slot{
                slot.index, std::make_unique<ItemStack>(std::move(modified))});
        }
    }
    if (kept.empty()) {
        stack.clearContainerContents();
    } else {
        item::component::ItemContainerContentsPayload updated;
        updated.slots = std::move(kept);
        stack.setContainerContents(std::move(updated));
    }
    return stack;
}

std::unique_ptr<LootFunction> ModifyContentsFunction::clone() const noexcept
{
    auto func =
        std::make_unique<ModifyContentsFunction>(m_component, m_modifier != nullptr ? m_modifier->clone() : nullptr);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
