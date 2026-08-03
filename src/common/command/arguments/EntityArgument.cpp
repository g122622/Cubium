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

#include "EntityArgument.hpp"

#include "common/command/StringReader.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <cstddef>
#include <exception>
#include <memory>
#include <sstream>
#include <string>

namespace mc {
namespace command {

// ========== FloatRange 角度测试实现 ==========

bool FloatRange::testAngle(f32 value) const noexcept
{
    // 如果范围无界，所有值都通过
    if (isUnbounded()) {
        return true;
    }

    // 将输入角度规范化到 [-180, 180) 范围
    const f32 normalizedValue = math::wrapDegrees(value);

    // 获取范围边界并规范化
    // min 为空时默认 0，max 为空时默认 359（规范后为 -1）
    const f32 min = m_min.has_value() ? math::wrapDegrees(m_min.value()) : 0.0f;
    const f32 max = m_max.has_value() ? math::wrapDegrees(m_max.value()) : -1.0f;

    // 核心角度范围测试逻辑
    // 如果 min > max，说明范围跨越了 -180/180 边界，需要使用 OR 逻辑
    // 例如 [170..-170] 表示从 170 度到 -170 度（跨越正北方向）
    if (min > max) {
        // 跨越边界：值在 [min, 180) 或 [-180, max] 范围内
        return normalizedValue >= min || normalizedValue <= max;
    } else {
        // 普通范围：值在 [min, max] 范围内
        return normalizedValue >= min && normalizedValue <= max;
    }
}

EntitySelector EntityArgumentType::parse(StringReader& reader)
{
    const i32 start = reader.getCursor();

    if (reader.canRead() && reader.peek() == '@') {
        return _parseSelector(reader, start);
    }

    const std::string name = reader.readString();
    return EntitySelector::byUsername(name);
}

EntitySelector EntityArgumentType::_parseSelector(StringReader& reader, i32 start)
{
    reader.skip();

    if (!reader.canRead()) {
        throw CommandException(CommandErrorType::EntitySelectorInvalid, "Missing selector type", start);
    }

    const char typeChar = reader.read();
    EntitySelector selector;

    switch (typeChar) {
        case 'p':
        case 'P':
            selector = EntitySelector::nearestPlayer();
            break;
        case 'a':
        case 'A':
            selector = EntitySelector::allPlayers();
            break;
        case 'e':
        case 'E':
            selector = EntitySelector::allEntities();
            break;
        case 'r':
        case 'R':
            selector = EntitySelector::randomPlayer();
            break;
        case 's':
        case 'S':
            selector = EntitySelector::self();
            break;
        default:
            reader.setCursor(start);
            throw CommandException(
                CommandErrorType::EntitySelectorInvalid, "Unknown selector type: @" + std::string(1, typeChar), start);
    }

    if (reader.canRead() && reader.peek() == '[') {
        _parseSelectorArguments(reader, selector);
    }

    _validateSelector(selector, start);
    return selector;
}

void EntityArgumentType::_parseSelectorArguments(StringReader& reader, EntitySelector& selector)
{
    reader.skip();

    while (reader.canRead() && reader.peek() != ']') {
        reader.skipWhitespace();
        if (!reader.canRead()) {
            break;
        }

        const std::string paramName = _readSelectorArgumentToken(reader);
        reader.skipWhitespace();
        if (!reader.canRead() || reader.peek() != '=') {
            throw CommandException(CommandErrorType::EntitySelectorInvalid,
                "Expected '=' after selector argument name",
                reader.getCursor());
        }

        reader.skip();
        reader.skipWhitespace();

        const i32 cursor = reader.getCursor();
        const std::string paramValue = reader.canRead() && reader.peek() == StringReader::SYNTAX_QUOTE
            ? reader.readString()
            : _readSelectorArgumentToken(reader);

        _applySelectorArgument(selector, paramName, paramValue, cursor);
        reader.skipWhitespace();
        if (reader.canRead() && reader.peek() == ',') {
            reader.skip();
        }
    }

    if (!reader.canRead() || reader.peek() != ']') {
        throw CommandException(
            CommandErrorType::EntitySelectorInvalid, "Expected ']' to close selector arguments", reader.getCursor());
    }

    reader.skip();
}

void EntityArgumentType::_applySelectorArgument(
    EntitySelector& selector, const std::string& name, const std::string& value, i32 cursor)
{
    // limit / c - 结果数量限制
    if (name == "limit" || name == "c") {
        const i32 limit = std::stoi(value);
        if (limit < 1) {
            throw CommandException(CommandErrorType::EntitySelectorInvalid, "Limit must be at least 1", cursor);
        }
        selector.setLimit(limit);
        selector.setSingle(limit == 1);
        return;
    }

    // name - 实体名称（支持取反）
    if (name == "name") {
        if (selector.hasUsername() || selector.hasUsernameNegated()) {
            throw CommandException(CommandErrorType::EntitySelectorInvalid, "Name already set", cursor);
        }
        StringReader valueReader(value);
        if (_shouldInvertValue(valueReader)) {
            selector.setUsernameNegated(valueReader.getRemaining());
        } else {
            selector.setUsername(value);
        }
        return;
    }

    // distance - 距离范围
    if (name == "distance") {
        StringReader valueReader(value);
        FloatRange range = _parseFloatRange(valueReader);
        selector.distance() = range;
        selector.setCurrentWorldOnly(true);
        return;
    }

    // level - 等级范围（仅玩家）
    if (name == "level") {
        StringReader valueReader(value);
        IntRange range = _parseIntRange(valueReader);
        selector.level() = range;
        selector.setIncludesNonPlayers(false);
        return;
    }

    // x, y, z - 坐标偏移
    if (name == "x") {
        selector.setX(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return;
    }
    if (name == "y") {
        selector.setY(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return;
    }
    if (name == "z") {
        selector.setZ(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return;
    }

    // dx, dy, dz - 体积尺寸
    if (name == "dx") {
        selector.setDx(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return;
    }
    if (name == "dy") {
        selector.setDy(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return;
    }
    if (name == "dz") {
        selector.setDz(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return;
    }

    // sort - 排序方式
    if (name == "sort") {
        if (value == "nearest") {
            selector.setSort(EntitySelectorSort::Nearest);
        } else if (value == "furthest") {
            selector.setSort(EntitySelectorSort::Furthest);
        } else if (value == "random") {
            selector.setSort(EntitySelectorSort::Random);
        } else if (value == "arbitrary") {
            selector.setSort(EntitySelectorSort::Arbitrary);
        } else {
            throw CommandException(CommandErrorType::EntitySelectorInvalid, "Unknown sort type: " + value, cursor);
        }
        return;
    }

    // type - 实体类型（支持取反和标签）
    if (name == "type") {
        if (selector.hasEntityType()) {
            throw CommandException(CommandErrorType::EntitySelectorInvalid, "Type already set", cursor);
        }
        StringReader valueReader(value);
        bool negated = _shouldInvertValue(valueReader);
        std::string typeStr = valueReader.getRemaining();

        // 如果是 minecraft:player 且未取反，则限制为仅玩家
        if (!negated && (typeStr == "minecraft:player" || typeStr == "player")) {
            selector.setIncludesNonPlayers(false);
        }
        selector.setEntityType(typeStr, negated);
        return;
    }

    // tag - 实体标签（支持取反，可多次使用）
    if (name == "tag") {
        StringReader valueReader(value);
        bool negated = _shouldInvertValue(valueReader);
        selector.addTag(valueReader.getRemaining(), negated);
        return;
    }

    // gamemode - 游戏模式（支持取反，仅玩家）
    if (name == "gamemode" || name == "m") {
        if (selector.hasGameMode()) {
            throw CommandException(CommandErrorType::EntitySelectorInvalid, "Gamemode already set", cursor);
        }
        StringReader valueReader(value);
        bool negated = _shouldInvertValue(valueReader);
        selector.setGameMode(valueReader.getRemaining(), negated);
        selector.setIncludesNonPlayers(false);
        return;
    }

    // team - 队伍（支持取反）
    if (name == "team") {
        if (selector.hasTeam()) {
            throw CommandException(CommandErrorType::EntitySelectorInvalid, "Team already set", cursor);
        }
        StringReader valueReader(value);
        bool negated = _shouldInvertValue(valueReader);
        selector.setTeam(valueReader.getRemaining(), negated);
        return;
    }

    // x_rotation - 俯仰角范围（pitch，-90 到 90 度）
    if (name == "x_rotation") {
        StringReader valueReader(value);
        FloatRange range = _parseFloatRange(valueReader);
        selector.xRotation() = range;
        return;
    }

    // y_rotation - 偏航角范围（yaw，-180 到 180 度）
    if (name == "y_rotation") {
        StringReader valueReader(value);
        FloatRange range = _parseFloatRange(valueReader);
        selector.yRotation() = range;
        return;
    }

    // nbt - NBT 数据（支持取反）
    // 格式: nbt={...} 或 nbt=!{...}
    if (name == "nbt") {
        StringReader valueReader(value);
        const bool negated = _shouldInvertValue(valueReader);
        valueReader.skipWhitespace();

        // 解析 NBT 标签（Mojangson 格式）
        try {
            // 使用 std::stringstream 桥接 StringReader 和 NBT 解析器
            std::istringstream nbtStream(valueReader.getRemaining());
            nbtStream >> nbt::contexts::mojangson;

            auto tag = nbt::tags::read(nbt::deduce_tag(nbtStream), nbtStream);
            if (tag != nullptr && tag->id() == nbt::TagId::Compound) {
                EntitySelector::NbtCondition condition;
                condition.nbt =
                    std::shared_ptr<nbt::tags::compound_tag>(dynamic_cast<nbt::tags::compound_tag*>(tag.release()));
                condition.negated = negated;
                selector.setNbtCondition(condition);
            }
        }
        catch (const std::exception&) {
            throw CommandException(CommandErrorType::EntitySelectorInvalid, "Invalid NBT format", cursor);
        }
        return;
    }

    // scores - 记分板分数
    // 格式: scores={objective1=1..5,objective2=10..}
    if (name == "scores") {
        StringReader valueReader(value);
        valueReader.skipWhitespace();

        if (!valueReader.canRead() || valueReader.peek() != '{') {
            throw CommandException(CommandErrorType::EntitySelectorInvalid, "Expected '{' for scores", cursor);
        }
        valueReader.skip(); // skip '{'
        valueReader.skipWhitespace();

        while (valueReader.canRead() && valueReader.peek() != '}') {
            valueReader.skipWhitespace();

            // 读取目标名称（遇到 = 时停止）
            const std::string objectiveName = _readScoresKey(valueReader);
            if (objectiveName.empty()) {
                throw CommandException(CommandErrorType::EntitySelectorInvalid, "Expected objective name", cursor);
            }

            valueReader.skipWhitespace();
            if (!valueReader.canRead() || valueReader.peek() != '=') {
                throw CommandException(
                    CommandErrorType::EntitySelectorInvalid, "Expected '=' after objective name", cursor);
            }
            valueReader.skip(); // skip '='
            valueReader.skipWhitespace();

            // 读取分数范围
            IntRange range = _parseIntRange(valueReader);
            selector.addScoreCondition(objectiveName, range);

            valueReader.skipWhitespace();
            if (valueReader.canRead() && valueReader.peek() == ',') {
                valueReader.skip();
                valueReader.skipWhitespace();
            }
        }

        if (!valueReader.canRead() || valueReader.peek() != '}') {
            throw CommandException(CommandErrorType::EntitySelectorInvalid, "Expected '}' to close scores", cursor);
        }
        valueReader.skip(); // skip '}'

        selector.setIncludesNonPlayers(false); // 记分板只对玩家有效
        return;
    }

    // advancements - 进度
    // 格式: advancements={adv_id=true} 或 advancements={adv_id={criteria1=true,criteria2=false}}
    if (name == "advancements") {
        StringReader valueReader(value);
        valueReader.skipWhitespace();

        if (!valueReader.canRead() || valueReader.peek() != '{') {
            throw CommandException(CommandErrorType::EntitySelectorInvalid, "Expected '{' for advancements", cursor);
        }
        valueReader.skip(); // skip '{'
        valueReader.skipWhitespace();

        while (valueReader.canRead() && valueReader.peek() != '}') {
            valueReader.skipWhitespace();

            // 读取进度 ID (ResourceLocation)
            // ResourceLocation 格式: namespace:path 或 path（默认 minecraft 命名空间）
            // 遇到 = 时停止
            const std::string advancementIdStr = _readAdvancementKey(valueReader);
            ResourceLocation advancementId = ResourceLocation::parse(advancementIdStr);

            valueReader.skipWhitespace();
            if (!valueReader.canRead() || valueReader.peek() != '=') {
                throw CommandException(
                    CommandErrorType::EntitySelectorInvalid, "Expected '=' after advancement id", cursor);
            }
            valueReader.skip(); // skip '='
            valueReader.skipWhitespace();

            EntitySelector::AdvancementCondition condition;

            // 判断是布尔值还是对象
            if (valueReader.canRead() && valueReader.peek() == '{') {
                // 对象格式：{criteria1=true,criteria2=false}
                valueReader.skip(); // skip '{'
                valueReader.skipWhitespace();

                while (valueReader.canRead() && valueReader.peek() != '}') {
                    valueReader.skipWhitespace();

                    // 读取准则名称（遇到 = 时停止）
                    const std::string criteriaName = _readCriteriaKey(valueReader);
                    if (criteriaName.empty()) {
                        throw CommandException(
                            CommandErrorType::EntitySelectorInvalid, "Expected criteria name", cursor);
                    }

                    valueReader.skipWhitespace();
                    if (!valueReader.canRead() || valueReader.peek() != '=') {
                        throw CommandException(
                            CommandErrorType::EntitySelectorInvalid, "Expected '=' after criteria name", cursor);
                    }
                    valueReader.skip(); // skip '='
                    valueReader.skipWhitespace();

                    // 读取布尔值（读取直到遇到 , 或 } 为止）
                    if (valueReader.canRead()) {
                        std::string boolValue = _readCriteriaKey(valueReader);
                        if (boolValue == "true" || boolValue == "TRUE" || boolValue == "True") {
                            condition.criteriaConditions[criteriaName] = true;
                        } else if (boolValue == "false" || boolValue == "FALSE" || boolValue == "False") {
                            condition.criteriaConditions[criteriaName] = false;
                        } else {
                            throw CommandException(
                                CommandErrorType::EntitySelectorInvalid, "Expected true or false", cursor);
                        }
                    }

                    valueReader.skipWhitespace();
                    if (valueReader.canRead() && valueReader.peek() == ',') {
                        valueReader.skip();
                        valueReader.skipWhitespace();
                    }
                }

                if (!valueReader.canRead() || valueReader.peek() != '}') {
                    throw CommandException(
                        CommandErrorType::EntitySelectorInvalid, "Expected '}' to close criteria", cursor);
                }
                valueReader.skip(); // skip '}'
            } else {
                // 布尔值格式：true/false
                // 读取直到遇到 , 或 } 为止
                if (valueReader.canRead()) {
                    std::string boolValue = _readCriteriaKey(valueReader); // 复用：读取直到 = 或 , 或 }
                    if (boolValue == "true" || boolValue == "TRUE" || boolValue == "True") {
                        condition.isComplete = true;
                    } else if (boolValue == "false" || boolValue == "FALSE" || boolValue == "False") {
                        condition.isComplete = false;
                    } else {
                        throw CommandException(
                            CommandErrorType::EntitySelectorInvalid, "Expected true or false", cursor);
                    }
                }
            }

            selector.addAdvancementCondition(advancementId, condition);

            valueReader.skipWhitespace();
            if (valueReader.canRead() && valueReader.peek() == ',') {
                valueReader.skip();
                valueReader.skipWhitespace();
            }
        }

        if (!valueReader.canRead() || valueReader.peek() != '}') {
            throw CommandException(
                CommandErrorType::EntitySelectorInvalid, "Expected '}' to close advancements", cursor);
        }
        valueReader.skip(); // skip '}'

        selector.setIncludesNonPlayers(false); // 进度只对玩家有效
        return;
    }

    // predicate - 战利品表谓词
    // 格式: predicate=namespace:predicate_name 或 predicate=!namespace:predicate_name
    if (name == "predicate") {
        StringReader valueReader(value);
        const bool negated = _shouldInvertValue(valueReader);
        valueReader.skipWhitespace();

        std::string predicateStr = valueReader.readUnquotedString();
        ResourceLocation predicateId = ResourceLocation::parse(predicateStr);
        EntitySelector::PredicateCondition condition;
        condition.predicate = predicateId;
        condition.negated = negated;
        selector.setPredicateCondition(condition);
        return;
    }

    // 未知参数
    throw CommandException(CommandErrorType::EntitySelectorInvalid, "Unknown selector argument: " + name, cursor);
}

void EntityArgumentType::_validateSelector(const EntitySelector& selector, i32 start)
{
    if (isPlayersOnly() && selector.includesNonPlayers() && !selector.isSelf()) {
        throw CommandException(CommandErrorType::EntitySelectorNotAllowed, "Only players can be selected here", start);
    }

    if (isSingle() && !selector.isSingle() && selector.limit() > 1) {
        throw CommandException(isPlayersOnly() ? CommandErrorType::PlayerTooMany : CommandErrorType::EntityTooMany,
            isPlayersOnly() ? "Only one player is allowed, but provided multiple"
                            : "Only one entity is allowed, but provided multiple",
            start);
    }
}

std::string EntityArgumentType::_readSelectorArgumentToken(StringReader& reader)
{
    const i32 start = reader.getCursor();

    // 检查是否以 { 开头，如果是则需要处理嵌套的大括号结构
    if (reader.canRead() && reader.peek() == '{') {
        // 读取整个嵌套的大括号结构（包括起始和结束的大括号）
        i32 braceDepth = 0;
        do {
            const char ch = reader.peek();
            if (ch == '{') {
                ++braceDepth;
            } else if (ch == '}') {
                --braceDepth;
            }
            reader.skip();
        } while (reader.canRead() && braceDepth > 0);
    } else {
        // 普通参数值：遇到空白、=、, 或 ] 时停止
        while (reader.canRead()) {
            const char ch = reader.peek();
            // 注意：'!' 是取反前缀，不是分隔符，不应在此处中断
            if (StringReader::isWhitespace(ch) || ch == '=' || ch == ',' || ch == ']') {
                break;
            }
            reader.skip();
        }
    }

    if (reader.getCursor() == start) {
        throw CommandException(CommandErrorType::EntitySelectorInvalid, "Expected selector argument token", start);
    }

    const size_t startIndex = static_cast<size_t>(start);
    const size_t endIndex = static_cast<size_t>(reader.getCursor());
    return std::string(reader.getString().substr(startIndex, endIndex - startIndex));
}

std::string EntityArgumentType::_readScoresKey(StringReader& reader)
{
    // 读取 scores 的目标名称，遇到 = 时停止
    // 目标名称格式：简单字符串（不含空格、等号、逗号、大括号）
    const i32 start = reader.getCursor();
    while (reader.canRead()) {
        const char ch = reader.peek();
        if (StringReader::isWhitespace(ch) || ch == '=' || ch == ',' || ch == '}' || ch == '{') {
            break;
        }
        reader.skip();
    }

    const size_t startIndex = static_cast<size_t>(start);
    const size_t endIndex = static_cast<size_t>(reader.getCursor());
    return std::string(reader.getString().substr(startIndex, endIndex - startIndex));
}

std::string EntityArgumentType::_readAdvancementKey(StringReader& reader)
{
    // 读取进度 ID (ResourceLocation)，遇到 = 时停止
    // ResourceLocation 格式：namespace:path 或 path（允许冒号）
    const i32 start = reader.getCursor();
    while (reader.canRead()) {
        const char ch = reader.peek();
        if (StringReader::isWhitespace(ch) || ch == '=' || ch == ',' || ch == '}' || ch == '{') {
            break;
        }
        reader.skip();
    }

    const size_t startIndex = static_cast<size_t>(start);
    const size_t endIndex = static_cast<size_t>(reader.getCursor());
    return std::string(reader.getString().substr(startIndex, endIndex - startIndex));
}

std::string EntityArgumentType::_readCriteriaKey(StringReader& reader)
{
    // 读取进度准则名称，遇到 = 时停止
    // 准则名称格式：简单字符串（允许冒号用于命名空间）
    const i32 start = reader.getCursor();
    while (reader.canRead()) {
        const char ch = reader.peek();
        if (StringReader::isWhitespace(ch) || ch == '=' || ch == ',' || ch == '}' || ch == '{') {
            break;
        }
        reader.skip();
    }

    const size_t startIndex = static_cast<size_t>(start);
    const size_t endIndex = static_cast<size_t>(reader.getCursor());
    return std::string(reader.getString().substr(startIndex, endIndex - startIndex));
}

bool EntityArgumentType::_shouldInvertValue(StringReader& reader)
{
    if (reader.canRead() && reader.peek() == '!') {
        reader.skip();
        return true;
    }
    return false;
}

FloatRange EntityArgumentType::_parseFloatRange(StringReader& reader)
{
    FloatRange range;

    // 解析格式: "value" 或 "min..max" 或 "min.." 或 "..max"
    // 注意：不能直接使用 readFloat()，因为它会把 "-" 和 ".." 误解析
    // 例如 "-45..45" 中 readFloat() 会把 "-45." 解析为 -45.0

    bool hasMin = false;
    bool hasMax = false;
    f32 minValue = 0.0f;
    f32 maxValue = 0.0f;

    // 检查是否以 ".." 开头（只有最大值）
    if (reader.canRead(2) && reader.peek() == '.' && reader.peek(1) == '.') {
        reader.skip(); // skip first '.'
        reader.skip(); // skip second '.'
        hasMax = true;
        maxValue = reader.readFloat();
    } else {
        // 读取最小值
        // 注意：需要手动处理负数和浮点数，因为 ".." 可能紧随其后
        i32 start = reader.getCursor();

        // 检查负号
        bool negative = false;
        if (reader.canRead() && reader.peek() == '-') {
            negative = true;
            reader.skip();
        }

        // 读取整数部分
        f64 intPart = 0.0;
        bool hasDigits = false;
        while (reader.canRead() && StringReader::isDigit(reader.peek())) {
            intPart = intPart * 10.0 + (reader.peek() - '0');
            reader.skip();
            hasDigits = true;
        }

        // 检查是否有小数部分，但要区分 ".." 和 "."
        f64 fracPart = 0.0;
        if (reader.canRead() && reader.peek() == '.') {
            // 检查下一个字符是数字还是 '.'
            if (reader.canRead(2) && StringReader::isDigit(reader.peek(1))) {
                // 这是小数点，读取小数部分
                reader.skip(); // skip '.'
                f64 decimalPlace = 0.1;
                while (reader.canRead() && StringReader::isDigit(reader.peek())) {
                    fracPart += (reader.peek() - '0') * decimalPlace;
                    decimalPlace *= 0.1;
                    reader.skip();
                    hasDigits = true;
                }
            }
            // 如果下一个字符不是数字，说明可能是 ".." 范围分隔符，不消费 '.'
        }

        if (!hasDigits) {
            throw CommandException(CommandErrorType::FloatExpected, "Expected float", start);
        }

        minValue = static_cast<f32>(negative ? -(intPart + fracPart) : (intPart + fracPart));
        hasMin = true;

        // 检查是否有 ".." 表示范围
        if (reader.canRead(2) && reader.peek() == '.' && reader.peek(1) == '.') {
            reader.skip(); // skip first '.'
            reader.skip(); // skip second '.'
            if (reader.canRead() && reader.peek() != ']' && reader.peek() != ',' && reader.peek() != '}') {
                maxValue = reader.readFloat();
                hasMax = true;
            }
        } else {
            // 没有 ".."，这是精确值，min = max
            maxValue = minValue;
            hasMax = true;
        }
    }

    if (hasMin) range.setMin(minValue);
    if (hasMax) range.setMax(maxValue);

    return range;
}

IntRange EntityArgumentType::_parseIntRange(StringReader& reader)
{
    IntRange range;

    // 解析格式: "value" 或 "min..max" 或 "min.." 或 "..max"
    bool hasMin = false;
    bool hasMax = false;
    i32 minValue = 0;
    i32 maxValue = 0;

    // 检查是否以 ".." 开头（只有最大值）
    if (reader.canRead(2) && reader.peek() == '.' && reader.peek(1) == '.') {
        reader.skip(); // skip first '.'
        reader.skip(); // skip second '.'
        hasMax = true;
        // 读取最大值，检查是否还有内容
        if (reader.canRead() && reader.peek() != ']' && reader.peek() != ',' && reader.peek() != '}') {
            maxValue = reader.readInt();
        }
    } else {
        // 读取最小值
        minValue = reader.readInt();
        hasMin = true;

        // 检查是否有 ".." 表示范围
        if (reader.canRead(2) && reader.peek() == '.' && reader.peek(1) == '.') {
            reader.skip(); // skip first '.'
            reader.skip(); // skip second '.'
            if (reader.canRead() && reader.peek() != ']' && reader.peek() != ',' && reader.peek() != '}') {
                maxValue = reader.readInt();
                hasMax = true;
            }
        } else {
            // 没有 ".."，这是精确值，min = max
            maxValue = minValue;
            hasMax = true;
        }
    }

    if (hasMin) range.setMin(minValue);
    if (hasMax) range.setMax(maxValue);

    return range;
}

} // namespace command
} // namespace mc
