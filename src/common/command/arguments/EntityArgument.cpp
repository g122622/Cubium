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

#include "common/util/math/MathUtils.hpp"
#include <algorithm>
#include <cmath>
#include <random>

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
    // 参考 MC 1.16.5 MinMaxBoundsWrapped.test() 的角度处理逻辑
    // min 为空时默认 0，max 为空时默认 359（规范后为 -1）
    const f32 min = m_min.has_value() ? math::wrapDegrees(m_min.value()) : 0.0f;
    const f32 max = m_max.has_value() ? math::wrapDegrees(m_max.value()) : -1.0f;

    // 核心角度范围测试逻辑（参考 MC 1.16.5 EntitySelector.createRotationPredicate）
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
        return parseSelector(reader, start);
    }

    const std::string name = reader.readString();
    return EntitySelector::byUsername(name);
}

EntitySelector EntityArgumentType::parseSelector(StringReader& reader, i32 start)
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
        parseSelectorArguments(reader, selector);
    }

    validateSelector(selector, start);
    return selector;
}

void EntityArgumentType::parseSelectorArguments(StringReader& reader, EntitySelector& selector)
{
    reader.skip();

    while (reader.canRead() && reader.peek() != ']') {
        reader.skipWhitespace();
        if (!reader.canRead()) {
            break;
        }

        const std::string paramName = readSelectorArgumentToken(reader);
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
            : readSelectorArgumentToken(reader);

        applySelectorArgument(selector, paramName, paramValue, cursor);
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

void EntityArgumentType::applySelectorArgument(
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
        if (shouldInvertValue(valueReader)) {
            selector.setUsernameNegated(valueReader.getRemaining());
        } else {
            selector.setUsername(value);
        }
        return;
    }

    // distance - 距离范围
    if (name == "distance") {
        StringReader valueReader(value);
        FloatRange range = parseFloatRange(valueReader);
        selector.distance() = range;
        selector.setCurrentWorldOnly(true);
        return;
    }

    // level - 等级范围（仅玩家）
    if (name == "level") {
        StringReader valueReader(value);
        IntRange range = parseIntRange(valueReader);
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
        bool negated = shouldInvertValue(valueReader);
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
        bool negated = shouldInvertValue(valueReader);
        selector.addTag(valueReader.getRemaining(), negated);
        return;
    }

    // gamemode - 游戏模式（支持取反，仅玩家）
    if (name == "gamemode" || name == "m") {
        if (selector.hasGameMode()) {
            throw CommandException(CommandErrorType::EntitySelectorInvalid, "Gamemode already set", cursor);
        }
        StringReader valueReader(value);
        bool negated = shouldInvertValue(valueReader);
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
        bool negated = shouldInvertValue(valueReader);
        selector.setTeam(valueReader.getRemaining(), negated);
        return;
    }

    // x_rotation - 俯仰角范围（pitch，-90 到 90 度）
    if (name == "x_rotation") {
        StringReader valueReader(value);
        FloatRange range = parseFloatRange(valueReader);
        selector.xRotation() = range;
        return;
    }

    // y_rotation - 偏航角范围（yaw，-180 到 180 度）
    if (name == "y_rotation") {
        StringReader valueReader(value);
        FloatRange range = parseFloatRange(valueReader);
        selector.yRotation() = range;
        return;
    }

    // nbt - NBT 数据（支持取反）
    if (name == "nbt") {
        // TODO: NBT 解析需要 NBT 系统
        return;
    }

    // scores - 记分板分数
    if (name == "scores") {
        // TODO: 需要记分板系统
        return;
    }

    // advancements - 进度
    if (name == "advancements") {
        // TODO: 需要进度系统
        return;
    }

    // predicate - 战利品表谓词
    if (name == "predicate") {
        // TODO: 需要谓词系统
        return;
    }

    // 未知参数
    throw CommandException(CommandErrorType::EntitySelectorInvalid, "Unknown selector argument: " + name, cursor);
}

void EntityArgumentType::validateSelector(const EntitySelector& selector, i32 start)
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

std::string EntityArgumentType::readSelectorArgumentToken(StringReader& reader)
{
    const i32 start = reader.getCursor();
    while (reader.canRead()) {
        const char ch = reader.peek();
        // 注意：'!' 是取反前缀，不是分隔符，不应在此处中断
        if (StringReader::isWhitespace(ch) || ch == '=' || ch == ',' || ch == ']') {
            break;
        }
        reader.skip();
    }

    if (reader.getCursor() == start) {
        throw CommandException(CommandErrorType::EntitySelectorInvalid, "Expected selector argument token", start);
    }

    const size_t startIndex = static_cast<size_t>(start);
    const size_t endIndex = static_cast<size_t>(reader.getCursor());
    return std::string(reader.getString().substr(startIndex, endIndex - startIndex));
}

bool EntityArgumentType::shouldInvertValue(StringReader& reader)
{
    if (reader.canRead() && reader.peek() == '!') {
        reader.skip();
        return true;
    }
    return false;
}

FloatRange EntityArgumentType::parseFloatRange(StringReader& reader)
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
            if (reader.canRead() && reader.peek() != ']' && reader.peek() != ',') {
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

IntRange EntityArgumentType::parseIntRange(StringReader& reader)
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
        maxValue = reader.readInt();
    } else {
        // 读取最小值
        minValue = reader.readInt();
        hasMin = true;

        // 检查是否有 ".." 表示范围
        if (reader.canRead(2) && reader.peek() == '.' && reader.peek(1) == '.') {
            reader.skip(); // skip first '.'
            reader.skip(); // skip second '.'
            if (reader.canRead() && reader.peek() != ']' && reader.peek() != ',') {
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
