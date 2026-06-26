/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software or
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
#include "common/util/property/IProperty.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
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
    // TODO: 当 NbtPredicate 实现后，可扩展用于检查方块实体的 NBT 匹配
    return test(state);
}

bool AdventureModePredicate::matchesPredicate(const std::string& predicate, const BlockState& state) const
{
    if (predicate.empty()) {
        return false;
    }

    // 解析属性部分：查找第一个 '['，支持 "minecraft:oak_log[axis=y]" 格式
    std::string blockPart;
    std::optional<std::vector<PropertyMatch>> properties;

    const size_t bracketPos = predicate.find('[');
    if (bracketPos != std::string::npos) {
        // 提取方块/标签部分（'[' 之前的内容）
        blockPart = predicate.substr(0, bracketPos);

        // 提取属性部分（'[' 和 ']' 之间的内容）
        const size_t closeBracket = predicate.find(']', bracketPos);
        if (closeBracket == std::string::npos) {
            // 没有闭合方括号，视为无效谓词
            return false;
        }

        // 解析属性键值对
        const std::string_view propsStr(predicate.data() + bracketPos + 1, closeBracket - bracketPos - 1);
        properties = parseProperties(propsStr);
        if (!properties.has_value()) {
            // 属性解析失败，视为不匹配
            return false;
        }
    } else {
        blockPart = predicate;
    }

    bool blockMatched = false;

    if (blockPart.empty()) {
        return false;
    }

    if (blockPart[0] == '#') {
        // 标签引用格式: "#minecraft:logs"
        const std::string tagId = blockPart.substr(1);
        const ResourceLocation tagLocation(tagId);

        auto* tag = BlockTags::getTag(tagLocation);
        if (tag != nullptr) {
            blockMatched = tag->contains(state);
        }
    } else {
        // 精确方块ID匹配: "minecraft:stone"
        const ResourceLocation blockLocation(blockPart);
        blockMatched = (state.blockLocation() == blockLocation);
    }

    // 如果方块/标签不匹配，直接返回 false
    if (!blockMatched) {
        return false;
    }

    // 如果没有属性条件，方块匹配即成功
    if (!properties.has_value() || properties->empty()) {
        return true;
    }

    // 检查属性条件
    return matchesProperties(state, *properties);
}

bool AdventureModePredicate::matchesProperties(
    const BlockState& state, const std::vector<PropertyMatch>& properties) const
{
    // 获取方块的状态容器
    const Block& block = state.getBlock();
    const auto& stateContainer = block.stateContainer();

    for (const auto& propMatch : properties) {
        // 在状态容器中查找属性
        const IProperty* property = stateContainer.getProperty(propMatch.name);
        if (property == nullptr) {
            // 方块没有此属性，匹配失败
            return false;
        }

        // 检查方块状态是否拥有此属性
        auto valueIndex = state.getValueIndex(*property);
        if (!valueIndex.has_value()) {
            return false;
        }

        // 解析属性值并比较
        auto parsedIndex = property->parseValue(propMatch.value);
        if (!parsedIndex.has_value()) {
            // 属性值无法解析（无效值），匹配失败
            return false;
        }

        if (*valueIndex != *parsedIndex) {
            // 属性值不匹配
            return false;
        }
    }

    return true;
}

std::optional<std::vector<AdventureModePredicate::PropertyMatch>> AdventureModePredicate::parseProperties(
    std::string_view propsStr)
{
    if (propsStr.empty()) {
        return std::vector<PropertyMatch>{};
    }

    std::vector<PropertyMatch> result;

    // 逐个解析 "key=value" 对，以逗号分隔
    size_t start = 0;
    while (start < propsStr.size()) {
        // 查找下一个逗号
        const size_t comma = propsStr.find(',', start);
        const size_t end = (comma == std::string_view::npos) ? propsStr.size() : comma;

        const std::string_view pair = propsStr.substr(start, end - start);

        // 查找等号
        const size_t eq = pair.find('=');
        if (eq == std::string_view::npos || eq == 0 || eq == pair.size() - 1) {
            // 无效格式：没有等号、等号在开头或末尾
            return std::nullopt;
        }

        const std::string name(pair.substr(0, eq));
        const std::string value(pair.substr(eq + 1));

        result.push_back(PropertyMatch{name, value});

        start = end + 1;
    }

    return result;
}

bool AdventureModePredicate::operator==(const AdventureModePredicate& other) const
{
    return m_predicates == other.m_predicates;
}

} // namespace mc
