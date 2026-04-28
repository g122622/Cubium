#include "EntityArgument.hpp"

#include <algorithm>
#include <random>

namespace mc {
namespace command {

EntitySelector EntityArgumentType::parse(StringReader& reader) {
    const i32 start = reader.getCursor();

    if (reader.canRead() && reader.peek() == '@') {
        return parseSelector(reader, start);
    }

    const String name = reader.readString();
    return EntitySelector::byUsername(name);
}

EntitySelector EntityArgumentType::parseSelector(StringReader& reader, i32 start) {
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
                CommandErrorType::EntitySelectorInvalid,
                "Unknown selector type: @" + String(1, typeChar),
                start);
    }

    if (reader.canRead() && reader.peek() == '[') {
        parseSelectorArguments(reader, selector);
    }

    validateSelector(selector, start);
    return selector;
}

void EntityArgumentType::parseSelectorArguments(StringReader& reader, EntitySelector& selector) {
    reader.skip();

    while (reader.canRead() && reader.peek() != ']') {
        reader.skipWhitespace();
        if (!reader.canRead()) {
            break;
        }

        const String paramName = readSelectorArgumentToken(reader);
        reader.skipWhitespace();
        if (!reader.canRead() || reader.peek() != '=') {
            throw CommandException(
                CommandErrorType::EntitySelectorInvalid,
                "Expected '=' after selector argument name",
                reader.getCursor());
        }

        reader.skip();
        reader.skipWhitespace();

        const i32 cursor = reader.getCursor();
        const String paramValue =
            reader.canRead() && reader.peek() == StringReader::SYNTAX_QUOTE
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
            CommandErrorType::EntitySelectorInvalid,
            "Expected ']' to close selector arguments",
            reader.getCursor());
    }

    reader.skip();
}

void EntityArgumentType::applySelectorArgument(EntitySelector& selector, const String& name, const String& value, i32 cursor) {
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
        String typeStr = valueReader.getRemaining();

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

    // x_rotation - 俯仰角范围
    if (name == "x_rotation") {
        // TODO: 解析角度范围
        return;
    }

    // y_rotation - 偏航角范围
    if (name == "y_rotation") {
        // TODO: 解析角度范围
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

void EntityArgumentType::validateSelector(const EntitySelector& selector, i32 start) {
    if (isPlayersOnly() && selector.includesNonPlayers() && !selector.isSelf()) {
        throw CommandException(
            CommandErrorType::EntitySelectorNotAllowed,
            "Only players can be selected here",
            start);
    }

    if (isSingle() && !selector.isSingle() && selector.limit() > 1) {
        throw CommandException(
            isPlayersOnly() ? CommandErrorType::PlayerTooMany : CommandErrorType::EntityTooMany,
            isPlayersOnly()
                ? "Only one player is allowed, but provided multiple"
                : "Only one entity is allowed, but provided multiple",
            start);
    }
}

String EntityArgumentType::readSelectorArgumentToken(StringReader& reader) {
    const i32 start = reader.getCursor();
    while (reader.canRead()) {
        const char ch = reader.peek();
        if (StringReader::isWhitespace(ch) || ch == '=' || ch == ',' || ch == ']' || ch == '!') {
            break;
        }
        reader.skip();
    }

    if (reader.getCursor() == start) {
        throw CommandException(
            CommandErrorType::EntitySelectorInvalid,
            "Expected selector argument token",
            start);
    }

    const size_t startIndex = static_cast<size_t>(start);
    const size_t endIndex = static_cast<size_t>(reader.getCursor());
    return String(reader.getString().substr(startIndex, endIndex - startIndex));
}

bool EntityArgumentType::shouldInvertValue(StringReader& reader) {
    if (reader.canRead() && reader.peek() == '!') {
        reader.skip();
        return true;
    }
    return false;
}

FloatRange EntityArgumentType::parseFloatRange(StringReader& reader) {
    FloatRange range;

    // 解析格式: "value" 或 "min..max" 或 "min.." 或 "..max"
    bool hasMin = false;
    bool hasMax = false;
    f32 minValue = 0.0f;
    f32 maxValue = 0.0f;

    // 检查是否以 ".." 开头（只有最大值）
    if (reader.canRead() && reader.peek() == '.') {
        reader.skip(); // skip first '.'
        if (reader.canRead() && reader.peek() == '.') {
            reader.skip(); // skip second '.'
            hasMax = true;
            maxValue = reader.readFloat();
        }
    } else {
        // 读取最小值
        minValue = reader.readFloat();
        hasMin = true;

        // 检查是否有 ".." 表示范围
        if (reader.canRead() && reader.peek() == '.') {
            reader.skip(); // skip first '.'
            if (reader.canRead() && reader.peek() == '.') {
                reader.skip(); // skip second '.'
                if (reader.canRead() && reader.peek() != ']' && reader.peek() != ',') {
                    maxValue = reader.readFloat();
                    hasMax = true;
                }
            }
        }
    }

    if (hasMin) range.setMin(minValue);
    if (hasMax) range.setMax(maxValue);

    return range;
}

IntRange EntityArgumentType::parseIntRange(StringReader& reader) {
    IntRange range;

    // 解析格式: "value" 或 "min..max" 或 "min.." 或 "..max"
    bool hasMin = false;
    bool hasMax = false;
    i32 minValue = 0;
    i32 maxValue = 0;

    // 检查是否以 ".." 开头（只有最大值）
    if (reader.canRead() && reader.peek() == '.') {
        reader.skip(); // skip first '.'
        if (reader.canRead() && reader.peek() == '.') {
            reader.skip(); // skip second '.'
            hasMax = true;
            maxValue = reader.readInt();
        }
    } else {
        // 读取最小值
        minValue = reader.readInt();
        hasMin = true;

        // 检查是否有 ".." 表示范围
        if (reader.canRead() && reader.peek() == '.') {
            reader.skip(); // skip first '.'
            if (reader.canRead() && reader.peek() == '.') {
                reader.skip(); // skip second '.'
                if (reader.canRead() && reader.peek() != ']' && reader.peek() != ',') {
                    maxValue = reader.readInt();
                    hasMax = true;
                }
            }
        }
    }

    if (hasMin) range.setMin(minValue);
    if (hasMax) range.setMax(maxValue);

    return range;
}

} // namespace command
} // namespace mc
