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

#include "common/item/core/AdventureModePredicate.hpp"

#include "common/advancement/trigger/conditions/NBTPredicate.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc {

AdventureModePredicate::AdventureModePredicate(std::vector<std::string> predicates)
    : m_predicates(std::move(predicates))
{}

void AdventureModePredicate::ensureParsed() const
{
    if (m_parsed) {
        return;
    }

    m_parsedPredicates.clear();
    m_parsedPredicates.reserve(m_predicates.size());

    for (const auto& predicate : m_predicates) {
        m_parsedPredicates.push_back(parsePredicate(predicate));
    }

    m_parsed = true;
}

AdventureModePredicate::ParsedPredicate AdventureModePredicate::parsePredicate(const std::string& predicate)
{
    ParsedPredicate result;

    if (predicate.empty()) {
        return result;
    }

    // 解析格式: block_part[properties]{nbt}
    // 其中 properties 和 nbt 部分都是可选的
    // 属性部分必须在 NBT 部分之前（MC 标准格式）

    // 第一步：查找 NBT 部分 '{' 的位置
    // NBT 部分从第一个 '{' 开始，Mojangson 解析器会处理嵌套的大括号
    const size_t nbtStart = predicate.find('{');

    // 第二步：查找属性部分 '[' 的位置
    // 只接受在 NBT 部分之前的 '['（如果有 NBT 部分）
    // 如果 '[' 出现在 '{' 之后，它是 NBT 内容的一部分，不是属性
    size_t bracketPos = std::string::npos;
    if (nbtStart != std::string::npos) {
        // 只在 '{' 之前查找 '['
        bracketPos = predicate.find('[', 0);
        if (bracketPos >= nbtStart) {
            // '[' 在 '{' 之后，不是有效的属性部分
            bracketPos = std::string::npos;
        }
    } else {
        bracketPos = predicate.find('[');
    }

    // 检查方括号是否闭合：未闭合的方括号视为无效格式
    size_t closeBracket = std::string::npos;
    if (bracketPos != std::string::npos) {
        closeBracket = predicate.find(']', bracketPos);
        if (closeBracket == std::string::npos) {
            // 未闭合的方括号，整个谓词条目无效
            return result;
        }
    }

    // 提取方块/标签部分
    // blockPart 的结束位置是 '[' 或 '{' 中先出现的那个
    size_t blockPartEnd = predicate.size();
    if (bracketPos != std::string::npos) {
        blockPartEnd = std::min(blockPartEnd, bracketPos);
    }
    if (nbtStart != std::string::npos) {
        blockPartEnd = std::min(blockPartEnd, nbtStart);
    }

    result.blockPart = predicate.substr(0, blockPartEnd);

    // 解析属性部分
    if (bracketPos != std::string::npos) {
        const std::string_view propsStr(predicate.data() + bracketPos + 1, closeBracket - bracketPos - 1);
        auto parsedProps = parseProperties(propsStr);
        if (!parsedProps.has_value()) {
            // 属性格式无效（如 "axis" 缺少等号），整个谓词条目无效
            result.blockPart.clear();
            return result;
        }
        result.properties = std::move(parsedProps);
    }

    // 解析 NBT 部分
    if (nbtStart != std::string::npos) {
        result.nbtTag = parseNbt(predicate, nbtStart);
        result.hasNbt = (result.nbtTag != nullptr);
    }

    return result;
}

std::shared_ptr<nbt::tags::compound_tag> AdventureModePredicate::parseNbt(const std::string& predicate, size_t nbtStart)
{
    // 提取从 '{' 到字符串末尾的 NBT 子串
    // Mojangson 解析器会自动处理嵌套的大括号，因此直接传递从 '{' 开始的子串
    const std::string nbtStr = predicate.substr(nbtStart);

    // 使用项目的 Mojangson 解析器解析 NBT 字符串
    auto tag = nbt::parseMojangson(nbtStr);
    if (tag == nullptr) {
        // NBT 解析失败，记录警告但不影响其他匹配
        return nullptr;
    }

    return std::shared_ptr<nbt::tags::compound_tag>(std::move(tag));
}

bool AdventureModePredicate::test(const BlockState& state) const
{
    if (m_predicates.empty()) {
        return false;
    }

    ensureParsed();

    for (const auto& predicate : m_parsedPredicates) {
        if (matchesParsedPredicate(predicate, state)) {
            return true;
        }
    }
    return false;
}

bool AdventureModePredicate::test(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    if (m_predicates.empty()) {
        return false;
    }

    ensureParsed();

    for (const auto& predicate : m_parsedPredicates) {
        if (matchesParsedPredicateWithNbt(predicate, world, pos, state)) {
            return true;
        }
    }
    return false;
}

bool AdventureModePredicate::test(IWorld& /*world*/, const BlockState& state) const
{
    // 无 BlockPos 时无法获取方块实体，退化为纯方块状态匹配
    // 注意：如果谓词包含 NBT 条件，此处无法检查方块实体 NBT，
    // 因此含 NBT 条件的谓词条目可能会错误地匹配（忽略 NBT 检查）。
    // 调用方应优先使用带 BlockPos 的重载版本以支持完整的 NBT 匹配。
    return test(state);
}

bool AdventureModePredicate::matchesParsedPredicate(const ParsedPredicate& predicate, const BlockState& state) const
{
    if (predicate.blockPart.empty()) {
        return false;
    }

    bool blockMatched = false;

    if (predicate.blockPart[0] == '#') {
        // 标签引用格式: "#minecraft:logs"
        const std::string tagId = predicate.blockPart.substr(1);
        const ResourceLocation tagLocation(tagId);

        auto* tag = BlockTags::getTag(tagLocation);
        if (tag != nullptr) {
            blockMatched = tag->contains(state);
        }
    } else {
        // 精确方块ID匹配: "minecraft:stone"
        const ResourceLocation blockLocation(predicate.blockPart);
        blockMatched = (state.blockLocation() == blockLocation);
    }

    // 如果方块/标签不匹配，直接返回 false
    if (!blockMatched) {
        return false;
    }

    // 如果没有属性条件，方块匹配即成功
    if (!predicate.properties.has_value() || predicate.properties->empty()) {
        return true;
    }

    // 检查属性条件
    return matchesProperties(state, *predicate.properties);
}

bool AdventureModePredicate::matchesParsedPredicateWithNbt(
    const ParsedPredicate& predicate, IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    // 先检查方块状态匹配（包括方块ID/标签和属性）
    if (!matchesParsedPredicate(predicate, state)) {
        return false;
    }

    // 如果没有 NBT 条件，方块状态匹配即成功
    if (!predicate.hasNbt || predicate.nbtTag == nullptr) {
        return true;
    }

    // 获取方块实体
    const BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr) {
        // 谓词要求 NBT 匹配，但该位置没有方块实体，匹配失败
        // 这与 MC Java 的行为一致：NbtPredicate 匹配时，null 标签返回 false
        return false;
    }

    // 将方块实体数据序列化为 NBT
    nbt::tags::compound_tag entityTag;
    blockEntity->saveToNBT(entityTag);

    // 使用 NBTPredicate::matchNBT 进行子集匹配
    // 期望标签中的所有字段必须在实际标签中存在且值相等（子集语义）
    return advancement::NBTPredicate::matchNBT(*predicate.nbtTag, entityTag);
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
