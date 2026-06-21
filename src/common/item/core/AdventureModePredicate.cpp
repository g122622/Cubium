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

#include "common/item/core/AdventureModePredicate.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"

namespace mc {

AdventureModePredicate::AdventureModePredicate(std::vector<std::string> predicates)
    : m_predicates(std::move(predicates))
{}

bool AdventureModePredicate::test(const BlockState& state) const
{
    if (m_predicates.empty()) {
        return false;
    }

    for (const auto& predicate : m_predicates) {
        if (matchesPredicate(predicate, state)) {
            return true;
        }
    }
    return false;
}

bool AdventureModePredicate::test(IWorld& /*world*/, const BlockState& state) const
{
    // 当前实现不需要世界上下文，直接委托给纯方块状态版本
    // 未来可扩展：检查方块实体的 NBT 匹配等
    return test(state);
}

bool AdventureModePredicate::matchesPredicate(const std::string& predicate, const BlockState& state) const
{
    if (predicate.empty()) {
        return false;
    }

    if (predicate[0] == '#') {
        // 标签引用格式: "#minecraft:logs"
        std::string tagId = predicate.substr(1);
        ResourceLocation tagLocation(tagId);

        auto* tag = BlockTags::getTag(tagLocation);
        if (tag != nullptr) {
            return tag->contains(state);
        }
        return false;
    }

    // 精确方块ID匹配: "minecraft:stone"
    ResourceLocation blockLocation(predicate);
    const auto& stateLocation = state.blockLocation();
    return stateLocation == blockLocation;
}

bool AdventureModePredicate::operator==(const AdventureModePredicate& other) const
{
    return m_predicates == other.m_predicates;
}

} // namespace mc
