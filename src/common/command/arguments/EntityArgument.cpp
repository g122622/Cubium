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
#include "common/command/arguments/NbtPathArgumentType.hpp"
#include "common/command/errors/CommandErrors.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/nbt/Nbt.hpp"

#include "common/util/NumberParsing.hpp"
#include <cctype>
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

Result<EntitySelector, CommandError> EntityArgumentTypeBase::parse(StringReader& reader)
{
    const i32 start = reader.getCursor();

    if (reader.canRead() && reader.peek() == '@') {
        return _parseSelector(reader, start);
    }

    MC_TRY_DECLARE(name, reader.readString());
    return EntitySelector::byUsername(name);
}

Result<EntitySelector, CommandError> EntityArgumentTypeBase::_parseSelector(StringReader& reader, i32 start)
{
    reader.skip();

    if (reader.atEnd()) {
        return CommandError(CommandErrorType::EntitySelectorInvalid, "Missing selector type", start);
    }

    const char typeChar = reader.peek();
    reader.skip();
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
            return CommandError(
                CommandErrorType::EntitySelectorInvalid, "Unknown selector type: @" + std::string(1, typeChar), start);
    }

    if (reader.canRead() && reader.peek() == '[') {
        MC_TRY(_parseSelectorArguments(reader, selector));
    }

    MC_TRY(_validateSelector(selector, start));
    return selector;
}

Result<void, CommandError> EntityArgumentTypeBase::_parseSelectorArguments(
    StringReader& reader, EntitySelector& selector)
{
    reader.skip();

    while (reader.canRead() && reader.peek() != ']') {
        reader.skipWhitespace();
        if (!reader.canRead()) {
            break;
        }

        MC_TRY_DECLARE(paramName, _readSelectorArgumentToken(reader));
        reader.skipWhitespace();
        if (!reader.canRead() || reader.peek() != '=') {
            return CommandError(CommandErrorType::EntitySelectorInvalid,
                "Expected '=' after selector argument name",
                reader.getCursor());
        }

        reader.skip();
        reader.skipWhitespace();

        const i32 cursor = reader.getCursor();
        std::string paramValue;
        if (reader.canRead() && reader.peek() == StringReader::SYNTAX_QUOTE) {
            MC_TRY_ASSIGN(paramValue, reader.readString());
        } else {
            MC_TRY_ASSIGN(paramValue, _readSelectorArgumentToken(reader));
        }

        MC_TRY(_applySelectorArgument(selector, paramName, paramValue, cursor));
        reader.skipWhitespace();
        if (reader.canRead() && reader.peek() == ',') {
            reader.skip();
        }
    }

    if (!reader.canRead() || reader.peek() != ']') {
        return CommandError(
            CommandErrorType::EntitySelectorInvalid, "Expected ']' to close selector arguments", reader.getCursor());
    }

    reader.skip();
    return Result<void, CommandError>::ok();
}

Result<f32, CommandError> EntityArgumentTypeBase::_parseCoordinate(
    const std::string& value, const std::string& argumentName, i32 cursor)
{
    // 坐标来自玩家输入：非法值必须报命令错误，而不是让 std::stof 抛异常逃逸
    auto parsed = util::parseFloat32(value);
    if (parsed.failed()) {
        return CommandError(CommandErrorType::EntitySelectorInvalid,
            "Invalid coordinate for '" + argumentName + "': '" + value + "'",
            cursor);
    }
    return parsed.value();
}

Result<void, CommandError> EntityArgumentTypeBase::_applySelectorArgument(
    EntitySelector& selector, const std::string& name, const std::string& value, i32 cursor)
{
    // limit / c - 结果数量限制
    if (name == "limit" || name == "c") {
        auto parsedLimit = util::parseInt32(value);
        if (parsedLimit.failed()) {
            return CommandError(
                CommandErrorType::EntitySelectorInvalid, "Invalid limit '" + value + "' (expected an integer)", cursor);
        }
        const i32 limit = parsedLimit.value();
        if (limit < 1) {
            return CommandError(CommandErrorType::EntitySelectorInvalid, "Limit must be at least 1", cursor);
        }
        selector.setLimit(limit);
        selector.setSingle(limit == 1);
        return Result<void, CommandError>::ok();
    }

    // name - 实体名称（支持取反）
    if (name == "name") {
        if (selector.hasUsername() || selector.hasUsernameNegated()) {
            return CommandError(CommandErrorType::EntitySelectorInvalid, "Name already set", cursor);
        }
        StringReader valueReader(value);
        if (_shouldInvertValue(valueReader)) {
            selector.setUsernameNegated(valueReader.getRemaining());
        } else {
            selector.setUsername(value);
        }
        return Result<void, CommandError>::ok();
    }

    // distance - 距离范围
    if (name == "distance") {
        StringReader valueReader(value);
        MC_TRY_DECLARE(range, _parseFloatRange(valueReader));
        selector.distance() = range;
        selector.setCurrentWorldOnly(true);
        return Result<void, CommandError>::ok();
    }

    // level - 等级范围（仅玩家）
    if (name == "level") {
        StringReader valueReader(value);
        MC_TRY_DECLARE(range, _parseIntRange(valueReader));
        selector.level() = range;
        selector.setIncludesNonPlayers(false);
        return Result<void, CommandError>::ok();
    }

    // x, y, z - 坐标偏移
    if (name == "x") {
        MC_TRY_DECLARE(parsedX, _parseCoordinate(value, "x", cursor));
        selector.setX(parsedX);
        selector.setCurrentWorldOnly(true);
        return Result<void, CommandError>::ok();
    }
    if (name == "y") {
        MC_TRY_DECLARE(parsedY, _parseCoordinate(value, "y", cursor));
        selector.setY(parsedY);
        selector.setCurrentWorldOnly(true);
        return Result<void, CommandError>::ok();
    }
    if (name == "z") {
        MC_TRY_DECLARE(parsedZ, _parseCoordinate(value, "z", cursor));
        selector.setZ(parsedZ);
        selector.setCurrentWorldOnly(true);
        return Result<void, CommandError>::ok();
    }

    // dx, dy, dz - 体积尺寸
    if (name == "dx") {
        MC_TRY_DECLARE(parsedDx, _parseCoordinate(value, "dx", cursor));
        selector.setDx(parsedDx);
        selector.setCurrentWorldOnly(true);
        return Result<void, CommandError>::ok();
    }
    if (name == "dy") {
        MC_TRY_DECLARE(parsedDy, _parseCoordinate(value, "dy", cursor));
        selector.setDy(parsedDy);
        selector.setCurrentWorldOnly(true);
        return Result<void, CommandError>::ok();
    }
    if (name == "dz") {
        MC_TRY_DECLARE(parsedDz, _parseCoordinate(value, "dz", cursor));
        selector.setDz(parsedDz);
        selector.setCurrentWorldOnly(true);
        return Result<void, CommandError>::ok();
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
            return CommandError(CommandErrorType::EntitySelectorInvalid, "Unknown sort type: " + value, cursor);
        }
        return Result<void, CommandError>::ok();
    }

    // type - 实体类型（支持取反和标签）
    if (name == "type") {
        if (selector.hasEntityType()) {
            return CommandError(CommandErrorType::EntitySelectorInvalid, "Type already set", cursor);
        }
        StringReader valueReader(value);
        bool negated = _shouldInvertValue(valueReader);
        std::string typeStr = valueReader.getRemaining();

        // 实体类型必须是已注册的类型（`#tag` 形式除外）：未知类型在原版是解析错误，
        // 而不是「空选择器」——后者会静默匹配不到任何实体或退化成命令源自身。
        // 注册表为空（单元测试等尚未初始化实体的环境）时跳过校验。
        if (!typeStr.empty() && typeStr.front() != '#') {
            auto& registry = entity::EntityRegistry::instance();
            if (registry.size() > 0 && registry.getType(ResourceLocation::parse(typeStr).toString()) == nullptr) {
                return CommandError(CommandErrorType::DispatcherUnknownArgument, "command.unknown.argument", cursor);
            }
        }

        // 如果是 minecraft:player 且未取反，则限制为仅玩家
        if (!negated && (typeStr == "minecraft:player" || typeStr == "player")) {
            selector.setIncludesNonPlayers(false);
        }
        selector.setEntityType(typeStr, negated);
        return Result<void, CommandError>::ok();
    }

    // tag - 实体标签（支持取反，可多次使用）
    if (name == "tag") {
        StringReader valueReader(value);
        bool negated = _shouldInvertValue(valueReader);
        selector.addTag(valueReader.getRemaining(), negated);
        return Result<void, CommandError>::ok();
    }

    // gamemode - 游戏模式（支持取反，仅玩家）
    if (name == "gamemode" || name == "m") {
        if (selector.hasGameMode()) {
            return CommandError(CommandErrorType::EntitySelectorInvalid, "Gamemode already set", cursor);
        }
        StringReader valueReader(value);
        bool negated = _shouldInvertValue(valueReader);
        selector.setGameMode(valueReader.getRemaining(), negated);
        selector.setIncludesNonPlayers(false);
        return Result<void, CommandError>::ok();
    }

    // team - 队伍（支持取反）
    if (name == "team") {
        if (selector.hasTeam()) {
            return CommandError(CommandErrorType::EntitySelectorInvalid, "Team already set", cursor);
        }
        StringReader valueReader(value);
        bool negated = _shouldInvertValue(valueReader);
        selector.setTeam(valueReader.getRemaining(), negated);
        return Result<void, CommandError>::ok();
    }

    // x_rotation - 俯仰角范围（pitch，-90 到 90 度）
    if (name == "x_rotation") {
        StringReader valueReader(value);
        MC_TRY_DECLARE(range, _parseFloatRange(valueReader));
        selector.xRotation() = range;
        return Result<void, CommandError>::ok();
    }

    // y_rotation - 偏航角范围（yaw，-180 到 180 度）
    if (name == "y_rotation") {
        StringReader valueReader(value);
        MC_TRY_DECLARE(range, _parseFloatRange(valueReader));
        selector.yRotation() = range;
        return Result<void, CommandError>::ok();
    }

    // nbt - NBT 数据（支持取反）
    // 格式: nbt={...} 或 nbt=!{...}
    if (name == "nbt") {
        StringReader valueReader(value);
        const bool negated = _shouldInvertValue(valueReader);
        valueReader.skipWhitespace();

        // 解析 NBT 标签（与 /data 的 nbt_compound_tag 参数共用一套 SNBT 读取）
        {
            auto tagResult = NbtPathArgumentType::readNbtValue(valueReader);
            if (tagResult.failed()) {
                return CommandError(CommandErrorType::EntitySelectorInvalid, "Invalid NBT format", cursor);
            }
            auto tag = std::move(tagResult).value();
            if (tag != nullptr && tag->id() == nbt::TagId::Compound) {
                EntitySelector::NbtCondition condition;
                condition.nbt =
                    std::shared_ptr<nbt::tags::compound_tag>(dynamic_cast<nbt::tags::compound_tag*>(tag.release()));
                condition.negated = negated;
                selector.setNbtCondition(condition);
            }
        }
        return Result<void, CommandError>::ok();
    }

    // scores - 记分板分数
    // 格式: scores={objective1=1..5,objective2=10..}
    if (name == "scores") {
        StringReader valueReader(value);
        valueReader.skipWhitespace();

        if (!valueReader.canRead() || valueReader.peek() != '{') {
            return CommandError(CommandErrorType::EntitySelectorInvalid, "Expected '{' for scores", cursor);
        }
        valueReader.skip(); // skip '{'
        valueReader.skipWhitespace();

        while (valueReader.canRead() && valueReader.peek() != '}') {
            valueReader.skipWhitespace();

            // 读取目标名称（遇到 = 时停止）
            const std::string objectiveName = _readScoresKey(valueReader);
            if (objectiveName.empty()) {
                return CommandError(CommandErrorType::EntitySelectorInvalid, "Expected objective name", cursor);
            }

            valueReader.skipWhitespace();
            if (!valueReader.canRead() || valueReader.peek() != '=') {
                return CommandError(
                    CommandErrorType::EntitySelectorInvalid, "Expected '=' after objective name", cursor);
            }
            valueReader.skip(); // skip '='
            valueReader.skipWhitespace();

            // 读取分数范围
            MC_TRY_DECLARE(range, _parseIntRange(valueReader));
            selector.addScoreCondition(objectiveName, range);

            valueReader.skipWhitespace();
            if (valueReader.canRead() && valueReader.peek() == ',') {
                valueReader.skip();
                valueReader.skipWhitespace();
            }
        }

        if (!valueReader.canRead() || valueReader.peek() != '}') {
            return CommandError(CommandErrorType::EntitySelectorInvalid, "Expected '}' to close scores", cursor);
        }
        valueReader.skip(); // skip '}'

        selector.setIncludesNonPlayers(false); // 记分板只对玩家有效
        return Result<void, CommandError>::ok();
    }

    // advancements - 进度
    // 格式: advancements={adv_id=true} 或 advancements={adv_id={criteria1=true,criteria2=false}}
    if (name == "advancements") {
        StringReader valueReader(value);
        valueReader.skipWhitespace();

        if (!valueReader.canRead() || valueReader.peek() != '{') {
            return CommandError(CommandErrorType::EntitySelectorInvalid, "Expected '{' for advancements", cursor);
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
                return CommandError(
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
                        return CommandError(CommandErrorType::EntitySelectorInvalid, "Expected criteria name", cursor);
                    }

                    valueReader.skipWhitespace();
                    if (!valueReader.canRead() || valueReader.peek() != '=') {
                        return CommandError(
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
                            return CommandError(
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
                    return CommandError(
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
                        return CommandError(CommandErrorType::EntitySelectorInvalid, "Expected true or false", cursor);
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
            return CommandError(CommandErrorType::EntitySelectorInvalid, "Expected '}' to close advancements", cursor);
        }
        valueReader.skip(); // skip '}'

        selector.setIncludesNonPlayers(false); // 进度只对玩家有效
        return Result<void, CommandError>::ok();
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
        return Result<void, CommandError>::ok();
    }

    // 未知参数
    return CommandError(CommandErrorType::EntitySelectorInvalid, "Unknown selector argument: " + name, cursor);
}

Result<void, CommandError> EntityArgumentTypeBase::_validateSelector(const EntitySelector& selector, i32 start)
{
    if (isPlayersOnly() && selector.includesNonPlayers() && !selector.isSelf()) {
        return CommandError(CommandErrorType::EntitySelectorNotAllowed, "Only players can be selected here", start);
    }

    if (isSingle() && !selector.isSingle() && selector.limit() > 1) {
        return CommandError(isPlayersOnly() ? CommandErrorType::PlayerTooMany : CommandErrorType::EntityTooMany,
            isPlayersOnly() ? "Only one player is allowed, but provided multiple"
                            : "Only one entity is allowed, but provided multiple",
            start);
    }

    return Result<void, CommandError>::ok();
}

Result<std::string, CommandError> EntityArgumentTypeBase::_readSelectorArgumentToken(StringReader& reader)
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
        return CommandError(CommandErrorType::EntitySelectorInvalid, "Expected selector argument token", start);
    }

    const size_t startIndex = static_cast<size_t>(start);
    const size_t endIndex = static_cast<size_t>(reader.getCursor());
    return std::string(reader.getString().substr(startIndex, endIndex - startIndex));
}

std::string EntityArgumentTypeBase::_readScoresKey(StringReader& reader)
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

std::string EntityArgumentTypeBase::_readAdvancementKey(StringReader& reader)
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

std::string EntityArgumentTypeBase::_readCriteriaKey(StringReader& reader)
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

bool EntityArgumentTypeBase::_shouldInvertValue(StringReader& reader)
{
    if (reader.canRead() && reader.peek() == '!') {
        reader.skip();
        return true;
    }
    return false;
}

Result<FloatRange, CommandError> EntityArgumentTypeBase::_parseFloatRange(StringReader& reader)
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
        MC_TRY_ASSIGN(maxValue, reader.readFloat());
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
            return CommandError(CommandErrorType::FloatExpected, "Expected float", start);
        }

        minValue = static_cast<f32>(negative ? -(intPart + fracPart) : (intPart + fracPart));
        hasMin = true;

        // 检查是否有 ".." 表示范围
        if (reader.canRead(2) && reader.peek() == '.' && reader.peek(1) == '.') {
            reader.skip(); // skip first '.'
            reader.skip(); // skip second '.'
            if (reader.canRead() && reader.peek() != ']' && reader.peek() != ',' && reader.peek() != '}') {
                MC_TRY_ASSIGN(maxValue, reader.readFloat());
                hasMax = true;
            }
        } else {
            // 没有 ".."，这是精确值，min = max
            maxValue = minValue;
            hasMax = true;
        }
    }

    if (hasMin) {
        range.setMin(minValue);
    }
    if (hasMax) {
        range.setMax(maxValue);
    }

    return range;
}

Result<IntRange, CommandError> EntityArgumentTypeBase::_parseIntRange(StringReader& reader)
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
            MC_TRY_ASSIGN(maxValue, reader.readInt());
        }
    } else {
        // 读取最小值
        MC_TRY_ASSIGN(minValue, reader.readInt());
        hasMin = true;

        // 检查是否有 ".." 表示范围
        if (reader.canRead(2) && reader.peek() == '.' && reader.peek(1) == '.') {
            reader.skip(); // skip first '.'
            reader.skip(); // skip second '.'
            if (reader.canRead() && reader.peek() != ']' && reader.peek() != ',' && reader.peek() != '}') {
                MC_TRY_ASSIGN(maxValue, reader.readInt());
                hasMax = true;
            }
        } else {
            // 没有 ".."，这是精确值，min = max
            maxValue = minValue;
            hasMax = true;
        }
    }

    if (hasMin) {
        range.setMin(minValue);
    }
    if (hasMax) {
        range.setMax(maxValue);
    }

    return range;
}

namespace {

/// 把解析结果转成实体 NBT 条件；顶层不是复合标签时按原版报解析错误。
Result<EntitySelector::NbtCondition, CommandError> _compoundCondition(
    const std::unique_ptr<nbt::tags::tag>& tag, i32 start)
{
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return CommandError(CommandErrorType::InvalidNbtPath, "Expected compound tag for entity NBT", start);
    }
    EntitySelector::NbtCondition condition;
    condition.nbt =
        std::shared_ptr<nbt::tags::compound_tag>(static_cast<nbt::tags::compound_tag*>(tag->copy().release()));
    return condition;
}

} // namespace

Result<EntitySelectorWithNbt, CommandError> EntityArgumentWithNbtType::parse(StringReader& reader)
{
    const i32 start = reader.getCursor();

    // 原版 EntitySelectorParser.parse：`{...}` 开头且不带 @ 时，选择器取默认（@e、当前维度），
    // 后面紧跟的复合标签整体作为 NBT 条件。
    if (reader.canRead() && reader.peek() == '{') {
        auto nbtValue = NbtPathArgumentType::readNbtValue(reader);
        if (nbtValue.failed()) {
            reader.setCursor(start);
            return CommandError(CommandErrorType::InvalidNbtPath, "Invalid NBT format", start);
        }

        const auto tag = std::move(nbtValue).value();
        EntitySelectorWithNbt result;
        result.selector = EntitySelector::allEntities();
        result.selector.setCurrentWorldOnly(true);
        MC_TRY_ASSIGN(result.nbt, _compoundCondition(tag, start));
        return result;
    }

    EntitySelectorWithNbt result;
    // 选择器语法与校验（@p/@a/@e/@r/@s、[参数]）与普通实体参数完全共用。
    MC_TRY_ASSIGN(result.selector, m_selector.parse(reader));

    // 原版允许在选择器后直接跟复合标签：`@e[type=pig]{NoAI:1b}`。
    reader.skipWhitespace();
    if (reader.canRead() && reader.peek() == '{') {
        auto nbtValue = NbtPathArgumentType::readNbtValue(reader);
        if (nbtValue.failed()) {
            reader.setCursor(start);
            return CommandError(CommandErrorType::InvalidNbtPath, "Invalid NBT format", start);
        }
        const auto tag = std::move(nbtValue).value();
        MC_TRY_ASSIGN(result.nbt, _compoundCondition(tag, start));
    }
    return result;
}

} // namespace command
} // namespace mc
