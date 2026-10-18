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
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/random/Random.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace mc {
namespace loot {

namespace {

std::unordered_map<ResourceLocation, std::vector<ResourceLocation>>& _instrumentTags()
{
    static std::unordered_map<ResourceLocation, std::vector<ResourceLocation>> tags = {
        // 原版 data/minecraft/tags/instrument/regular_goat_horns.json
        {ResourceLocation("minecraft:regular_goat_horns"),
            {ResourceLocation("minecraft:ponder_goat_horn"),
                ResourceLocation("minecraft:sing_goat_horn"),
                ResourceLocation("minecraft:seek_goat_horn"),
                ResourceLocation("minecraft:feel_goat_horn"),
                ResourceLocation("minecraft:admire_goat_horn"),
                ResourceLocation("minecraft:call_goat_horn"),
                ResourceLocation("minecraft:yearn_goat_horn"),
                ResourceLocation("minecraft:dream_goat_horn")}},
        // data/minecraft/tags/instrument/screaming_goat_horns.json
        {ResourceLocation("minecraft:screaming_goat_horns"),
            {ResourceLocation("minecraft:seek_goat_horn"), ResourceLocation("minecraft:feel_goat_horn")}},
    };
    return tags;
}

} // namespace

SetInstrumentFunction::SetInstrumentFunction(ResourceLocation instrumentTagId)
    : m_instrumentTagId(std::move(instrumentTagId))
{}

ItemStack SetInstrumentFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }
    const auto* tag = getInstrumentTag(m_instrumentTagId);
    if (tag == nullptr || tag->empty()) {
        return stack;
    }
    const i32 index = context.getRandom().nextInt(static_cast<i32>(tag->size()));
    stack.setInstrument((*tag)[static_cast<usize>(index)].toString());
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

void SetInstrumentFunction::clearInstruments()
{
    _instrumentTags().clear();
}

void SetInstrumentFunction::registerInstrument(const ResourceLocation& tagId, const ResourceLocation& instrumentId)
{
    auto& members = _instrumentTags()[tagId];
    if (std::find(members.begin(), members.end(), instrumentId) == members.end()) {
        members.push_back(instrumentId);
    }
}

const std::vector<ResourceLocation>* SetInstrumentFunction::getInstrumentTag(const ResourceLocation& tagId)
{
    const auto& tags = _instrumentTags();
    auto it = tags.find(tagId);
    return it == tags.end() ? nullptr : &it->second;
}

} // namespace loot
} // namespace mc
