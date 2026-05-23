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

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>

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

Result<EntitySelector> EntityArgumentType::parse(StringReader& reader)
{
    const i32 start = reader.getCursor();

    if (reader.canRead() && reader.peek() == '@') {
        return parseSelector(reader, start);
    }

    auto nameResult = reader.readString();
    if (nameResult.failed()) {
        return nameResult.error();
    }
    return EntitySelector::byUsername(nameResult.value());
}

Result<EntitySelector> EntityArgumentType::parseSelector(StringReader& reader, i32 start)
{
    reader.skip();

    if (!reader.canRead()) {
        reader.setCursor(start);
        return Error(ErrorCode::CommandSyntaxError, "Missing selector type");
    }

    auto charResult = reader.read();
    if (charResult.failed()) {
        reader.setCursor(start);
        return charResult.error();
    }
    const char typeChar = charResult.value();
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
            return Error(ErrorCode::CommandSyntaxError, "Unknown selector type: @" + std::string(1, typeChar));
    }

    if (reader.canRead() && reader.peek() == '[') {
        auto argResult = parseSelectorArguments(reader, selector);
        if (argResult.failed()) {
            reader.setCursor(start);
            return argResult.error();
        }
    }

    auto validationResult = validateSelector(selector, start);
    if (validationResult.failed()) {
        return validationResult.error();
    }
    return selector;
}

Result<void> EntityArgumentType::parseSelectorArguments(StringReader& reader, EntitySelector& selector)
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
            return Error(ErrorCode::CommandSyntaxError, "Expected '=' after selector argument name");
        }

        reader.skip();
        reader.skipWhitespace();

        const i32 cursor = reader.getCursor();
        std::string paramValue;
        if (reader.canRead() && reader.peek() == StringReader::SYNTAX_QUOTE) {
            auto strResult = reader.readString();
            if (strResult.failed()) {
                return strResult.error();
            }
            paramValue = strResult.value();
        } else {
            paramValue = readSelectorArgumentToken(reader);
        }

        auto applyResult = applySelectorArgument(selector, paramName, paramValue, cursor);
        if (applyResult.failed()) {
            return applyResult.error();
        }
        reader.skipWhitespace();
        if (reader.canRead() && reader.peek() == ',') {
            reader.skip();
        }
    }

    if (!reader.canRead() || reader.peek() != ']') {
        return Error(ErrorCode::CommandSyntaxError, "Expected ']' to close selector arguments");
    }

    reader.skip();
    return Result<void>::ok();
}

Result<void> EntityArgumentType::applySelectorArgument(
    EntitySelector& selector, const std::string& name, const std::string& value, i32 cursor)
{
    // limit / c - 结果数量限制
    if (name == "limit" || name == "c") {
        const i32 limit = std::stoi(value);
        if (limit < 1) {
            return Error(ErrorCode::CommandSyntaxError, "Limit must be at least 1");
        }
        selector.setLimit(limit);
        selector.setSingle(limit == 1);
        return Result<void>::ok();
    }

    // name - 实体名称（支持取反）
    if (name == "name") {
        if (selector.hasUsername() || selector.hasUsernameNegated()) {
            return Error(ErrorCode::CommandSyntaxError, "Name already set");
        }
        StringReader valueReader(value);
        if (shouldInvertValue(valueReader)) {
            selector.setUsernameNegated(valueReader.getRemaining());
        } else {
            selector.setUsername(value);
        }
        return Result<void>::ok();
    }

    // distance - 距离范围
    if (name == "distance") {
        StringReader valueReader(value);
        auto rangeResult = parseFloatRange(valueReader);
        if (rangeResult.failed()) {
            return rangeResult.error();
        }
        selector.distance() = rangeResult.value();
        selector.setCurrentWorldOnly(true);
        return Result<void>::ok();
    }

    // level - 等级范围（仅玩家）
    if (name == "level") {
        StringReader valueReader(value);
        auto rangeResult = parseIntRange(valueReader);
        if (rangeResult.failed()) {
            return rangeResult.error();
        }
        selector.level() = rangeResult.value();
        selector.setIncludesNonPlayers(false);
        return Result<void>::ok();
    }

    // x, y, z - 坐标偏移
    if (name == "x") {
        selector.setX(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return Result<void>::ok();
    }
    if (name == "y") {
        selector.setY(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return Result<void>::ok();
    }
    if (name == "z") {
        selector.setZ(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return Result<void>::ok();
    }

    // dx, dy, dz - 体积尺寸
    if (name == "dx") {
        selector.setDx(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return Result<void>::ok();
    }
    if (name == "dy") {
        selector.setDy(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return Result<void>::ok();
    }
    if (name == "dz") {
        selector.setDz(std::stof(value));
        selector.setCurrentWorldOnly(true);
        return Result<void>::ok();
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
            return Error(ErrorCode::CommandSyntaxError, "Unknown sort type: " + value);
        }
        return Result<void>::ok();
    }

    // type - 实体类型（支持取反和标签）
    if (name == "type") {
        if (selector.hasEntityType()) {
            return Error(ErrorCode::CommandSyntaxError, "Type already set");
        }
        StringReader valueReader(value);
        bool negated = shouldInvertValue(valueReader);
        std::string typeStr = valueReader.getRemaining();

        // 如果是 minecraft:player 且未取反，则限制为仅玩家
        if (!negated && (typeStr == "minecraft:player" || typeStr == "player")) {
            selector.setIncludesNonPlayers(false);
        }
        selector.setEntityType(typeStr, negated);
        return Result<void>::ok();
    }

    // tag - 实体标签（支持取反，可多次使用）
    if (name == "tag") {
        StringReader valueReader(value);
        bool negated = shouldInvertValue(valueReader);
        selector.addTag(valueReader.getRemaining(), negated);
        return Result<void>::ok();
    }

    // gamemode - 游戏模式（支持取反，仅玩家）
    if (name == "gamemode" || name == "m") {
        if (selector.hasGameMode()) {
            return Error(ErrorCode::CommandSyntaxError, "Gamemode already set");
        }
        StringReader valueReader(value);
        bool negated = shouldInvertValue(valueReader);
        selector.setGameMode(valueReader.getRemaining(), negated);
        selector.setIncludesNonPlayers(false);
        return Result<void>::ok();
    }

    // team - 队伍（支持取反）
    if (name == "team") {
        if (selector.hasTeam()) {
            return Error(ErrorCode::CommandSyntaxError, "Team already set");
        }
        StringReader valueReader(value);
        bool negated = shouldInvertValue(valueReader);
        selector.setTeam(valueReader.getRemaining(), negated);
        return Result<void>::ok();
    }

    // x_rotation - 俯仰角范围（pitch，-90 到 90 度）
    if (name == "x_rotation") {
        StringReader valueReader(value);
        auto rangeResult = parseFloatRange(valueReader);
        if (rangeResult.failed()) {
            return rangeResult.error();
        }
        selector.xRotation() = rangeResult.value();
        return Result<void>::ok();
    }

    // y_rotation - 偏航角范围（yaw，-180 到 180 度）
    if (name == "y_rotation") {
        StringReader valueReader(value);
        auto rangeResult = parseFloatRange(valueReader);
        if (rangeResult.failed()) {
            return rangeResult.error();
        }
        selector.yRotation() = rangeResult.value();
        return Result<void>::ok();
    }

    // nbt - NBT 数据（支持取反）
    // 格式: nbt={...} 或 nbt=!{...}
    // 参考 MC 1.16.5 EntityOptions.register("nbt", ...)
    if (name == "nbt") {
        StringReader valueReader(value);
        const bool negated = shouldInvertValue(valueReader);
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
            return Error(ErrorCode::CommandSyntaxError, "Invalid NBT format");
        }
        return Result<void>::ok();
    }

    // scores - 记分板分数
    // 格式: scores={objective1=1..5,objective2=10..}
    // 参考 MC 1.16.5 EntityOptions.register("scores", ...)
    if (name == "scores") {
        StringReader valueReader(value);
        valueReader.skipWhitespace();

        if (!valueReader.canRead() || valueReader.peek() != '{') {
            return Error(ErrorCode::CommandSyntaxError, "Expected '{' for scores");
        }
        valueReader.skip(); // skip '{'
        valueReader.skipWhitespace();

        while (valueReader.canRead() && valueReader.peek() != '}') {
            valueReader.skipWhitespace();

            // 读取目标名称（遇到 = 时停止）
            const std::string objectiveName = readScoresKey(valueReader);
            if (objectiveName.empty()) {
                return Error(ErrorCode::CommandSyntaxError, "Expected objective name");
            }

            valueReader.skipWhitespace();
            if (!valueReader.canRead() || valueReader.peek() != '=') {
                return Error(ErrorCode::CommandSyntaxError, "Expected '=' after objective name");
            }
            valueReader.skip(); // skip '='
            valueReader.skipWhitespace();

            // 读取分数范围
            auto rangeResult = parseIntRange(valueReader);
            if (rangeResult.failed()) {
                return rangeResult.error();
            }
            selector.addScoreCondition(objectiveName, rangeResult.value());

            valueReader.skipWhitespace();
            if (valueReader.canRead() && valueReader.peek() == ',') {
                valueReader.skip();
                valueReader.skipWhitespace();
            }
        }

        if (!valueReader.canRead() || valueReader.peek() != '}') {
            return Error(ErrorCode::CommandSyntaxError, "Expected '}' to close scores");
        }
        valueReader.skip(); // skip '}'

        selector.setIncludesNonPlayers(false); // 记分板只对玩家有效
        return Result<void>::ok();
    }

    // advancements - 进度
    // 格式: advancements={adv_id=true} 或 advancements={adv_id={criteria1=true,criteria2=false}}
    // 参考 MC 1.16.5 EntityOptions.register("advancements", ...)
    if (name == "advancements") {
        StringReader valueReader(value);
        valueReader.skipWhitespace();

        if (!valueReader.canRead() || valueReader.peek() != '{') {
            return Error(ErrorCode::CommandSyntaxError, "Expected '{' for advancements");
        }
        valueReader.skip(); // skip '{'
        valueReader.skipWhitespace();

        while (valueReader.canRead() && valueReader.peek() != '}') {
            valueReader.skipWhitespace();

            // 读取进度 ID (ResourceLocation)
            // ResourceLocation 格式: namespace:path 或 path（默认 minecraft 命名空间）
            // 遇到 = 时停止
            const std::string advancementIdStr = readAdvancementKey(valueReader);
            ResourceLocation advancementId = ResourceLocation::parse(advancementIdStr);

            valueReader.skipWhitespace();
            if (!valueReader.canRead() || valueReader.peek() != '=') {
                return Error(ErrorCode::CommandSyntaxError, "Expected '=' after advancement id");
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
                    const std::string criteriaName = readCriteriaKey(valueReader);
                    if (criteriaName.empty()) {
                        return Error(ErrorCode::CommandSyntaxError, "Expected criteria name");
                    }

                    valueReader.skipWhitespace();
                    if (!valueReader.canRead() || valueReader.peek() != '=') {
                        return Error(ErrorCode::CommandSyntaxError, "Expected '=' after criteria name");
                    }
                    valueReader.skip(); // skip '='
                    valueReader.skipWhitespace();

                    // 读取布尔值（读取直到遇到 , 或 } 为止）
                    if (valueReader.canRead()) {
                        std::string boolValue = readCriteriaKey(valueReader);
                        if (boolValue == "true" || boolValue == "TRUE" || boolValue == "True") {
                            condition.criteriaConditions[criteriaName] = true;
                        } else if (boolValue == "false" || boolValue == "FALSE" || boolValue == "False") {
                            condition.criteriaConditions[criteriaName] = false;
                        } else {
                            return Error(ErrorCode::CommandSyntaxError, "Expected true or false");
                        }
                    }

                    valueReader.skipWhitespace();
                    if (valueReader.canRead() && valueReader.peek() == ',') {
                        valueReader.skip();
                        valueReader.skipWhitespace();
                    }
                }

                if (!valueReader.canRead() || valueReader.peek() != '}') {
                    return Error(ErrorCode::CommandSyntaxError, "Expected '}' to close criteria");
                }
                valueReader.skip(); // skip '}'
            } else {
                // 布尔值格式：true/false
                // 读取直到遇到 , 或 } 为止
                if (valueReader.canRead()) {
                    std::string boolValue = readCriteriaKey(valueReader); // 复用：读取直到 = 或 , 或 }
                    if (boolValue == "true" || boolValue == "TRUE" || boolValue == "True") {
                        condition.isComplete = true;
                    } else if (boolValue == "false" || boolValue == "FALSE" || boolValue == "False") {
                        condition.isComplete = false;
                    } else {
                        return Error(ErrorCode::CommandSyntaxError, "Expected true or false");
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
            return Error(ErrorCode::CommandSyntaxError, "Expected '}' to close advancements");
        }
        valueReader.skip(); // skip '}'

        selector.setIncludesNonPlayers(false); // 进度只对玩家有效
        return Result<void>::ok();
    }

    // predicate - 战利品表谓词
    // 格式: predicate=namespace:predicate_name 或 predicate=!namespace:predicate_name
    // 参考 MC 1.16.5 EntityOptions.register("predicate", ...)
    if (name == "predicate") {
        StringReader valueReader(value);
        const bool negated = shouldInvertValue(valueReader);
        valueReader.skipWhitespace();

        std::string predicateStr = valueReader.readUnquotedString();
        ResourceLocation predicateId = ResourceLocation::parse(predicateStr);
        EntitySelector::PredicateCondition condition;
        condition.predicate = predicateId;
        condition.negated = negated;
        selector.setPredicateCondition(condition);
        return Result<void>::ok();
    }

    // 未知参数
    return Error(ErrorCode::CommandSyntaxError, "Unknown selector argument: " + name);
}

Result<void> EntityArgumentType::validateSelector(const EntitySelector& selector, i32 start)
{
    if (isPlayersOnly() && selector.includesNonPlayers() && !selector.isSelf()) {
        return Error(ErrorCode::CommandPermissionDenied, "Only players can be selected here");
    }

    if (isSingle() && !selector.isSingle() && selector.limit() > 1) {
        return Error(isPlayersOnly() ? ErrorCode::CommandInvalidArgument : ErrorCode::CommandInvalidArgument,
            isPlayersOnly() ? "Only one player is allowed, but provided multiple"
                            : "Only one entity is allowed, but provided multiple");
    }
    return Result<void>::ok();
}

std::string EntityArgumentType::readSelectorArgumentToken(StringReader& reader)
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

    // 注意：此函数不返回错误，调用者应检查 cursor 是否移动
    const size_t startIndex = static_cast<size_t>(start);
    const size_t endIndex = static_cast<size_t>(reader.getCursor());
    return std::string(reader.getString().substr(startIndex, endIndex - startIndex));
}

std::string EntityArgumentType::readScoresKey(StringReader& reader)
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

std::string EntityArgumentType::readAdvancementKey(StringReader& reader)
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

std::string EntityArgumentType::readCriteriaKey(StringReader& reader)
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

bool EntityArgumentType::shouldInvertValue(StringReader& reader)
{
    if (reader.canRead() && reader.peek() == '!') {
        reader.skip();
        return true;
    }
    return false;
}

Result<FloatRange> EntityArgumentType::parseFloatRange(StringReader& reader)
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
        auto floatResult = reader.readFloat();
        if (floatResult.failed()) {
            return floatResult.error();
        }
        maxValue = floatResult.value();
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
            reader.setCursor(start);
            return Error(ErrorCode::CommandSyntaxError, "Expected float");
        }

        minValue = static_cast<f32>(negative ? -(intPart + fracPart) : (intPart + fracPart));
        hasMin = true;

        // 检查是否有 ".." 表示范围
        if (reader.canRead(2) && reader.peek() == '.' && reader.peek(1) == '.') {
            reader.skip(); // skip first '.'
            reader.skip(); // skip second '.'
            if (reader.canRead() && reader.peek() != ']' && reader.peek() != ',' && reader.peek() != '}') {
                auto floatResult = reader.readFloat();
                if (floatResult.failed()) {
                    return floatResult.error();
                }
                maxValue = floatResult.value();
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

Result<IntRange> EntityArgumentType::parseIntRange(StringReader& reader)
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
            auto intResult = reader.readInt();
            if (intResult.failed()) {
                return intResult.error();
            }
            maxValue = intResult.value();
        }
    } else {
        // 读取最小值
        auto intResult = reader.readInt();
        if (intResult.failed()) {
            return intResult.error();
        }
        minValue = intResult.value();
        hasMin = true;

        // 检查是否有 ".." 表示范围
        if (reader.canRead(2) && reader.peek() == '.' && reader.peek(1) == '.') {
            reader.skip(); // skip first '.'
            reader.skip(); // skip second '.'
            if (reader.canRead() && reader.peek() != ']' && reader.peek() != ',' && reader.peek() != '}') {
                auto maxResult = reader.readInt();
                if (maxResult.failed()) {
                    return maxResult.error();
                }
                maxValue = maxResult.value();
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
