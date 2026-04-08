#pragma once

#include "common/core/Types.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "ArgumentType.hpp"
#include <memory>
#include <functional>
#include <vector>

namespace mc {

// 前向声明
class Entity;
class Player;
class ServerPlayer;

namespace command {

// ========== 实体选择器类型 ==========

/**
 * @brief 实体选择器类型
 */
enum class EntitySelectorType {
    SinglePlayer,   // @p - 最近的单个玩家
    AllPlayers,     // @a - 所有玩家
    AllEntities,    // @e - 所有实体
    RandomPlayer,   // @r - 随机玩家
    Self            // @s - 自己（执行命令的实体）
};

/**
 * @brief 实体选择器
 *
 * 封装实体选择逻辑，支持：
 * - 选择器类型 (@p, @a, @e, @r, @s)
 * - 玩家名称
 * - UUID
 * - 选择器参数 (type=, limit=, sort=, distance=, etc.)
 *
 * 参考 MC 的 EntitySelector 类
 */
class EntitySelector {
public:
    EntitySelector() = default;

    explicit EntitySelector(EntitySelectorType type)
        : m_type(type) {}

    // ========== 选择器属性 ==========

    [[nodiscard]] EntitySelectorType type() const noexcept { return m_type; }
    [[nodiscard]] i32 limit() const noexcept { return m_limit; }
    [[nodiscard]] bool isSelf() const noexcept { return m_isSelf; }
    [[nodiscard]] bool includesNonPlayers() const noexcept { return m_includesNonPlayers; }
    [[nodiscard]] bool isSingle() const noexcept { return m_single; }

    // ========== 设置属性 ==========

    void setLimit(i32 limit) { m_limit = limit; }
    void setSelf(bool self) { m_isSelf = self; }
    void setIncludesNonPlayers(bool includes) { m_includesNonPlayers = includes; }
    void setSingle(bool single) { m_single = single; }
    void setUsername(const String& username) { m_username = username; }

    // ========== 选择器创建工厂 ==========

    static EntitySelector self() {
        EntitySelector selector(EntitySelectorType::Self);
        selector.m_isSelf = true;
        selector.m_single = true;
        return selector;
    }

    static EntitySelector nearestPlayer() {
        EntitySelector selector(EntitySelectorType::SinglePlayer);
        selector.m_single = true;
        selector.m_limit = 1;
        return selector;
    }

    static EntitySelector allPlayers() {
        EntitySelector selector(EntitySelectorType::AllPlayers);
        selector.m_single = false;
        return selector;
    }

    static EntitySelector allEntities() {
        EntitySelector selector(EntitySelectorType::AllEntities);
        selector.m_single = false;
        selector.m_includesNonPlayers = true;
        return selector;
    }

    static EntitySelector randomPlayer() {
        EntitySelector selector(EntitySelectorType::RandomPlayer);
        selector.m_single = true;
        selector.m_limit = 1;
        return selector;
    }

    static EntitySelector byUsername(const String& username) {
        EntitySelector selector;
        selector.m_username = username;
        selector.m_single = true;
        return selector;
    }

private:
    EntitySelectorType m_type = EntitySelectorType::SinglePlayer;
    i32 m_limit = INT32_MAX;
    bool m_isSelf = false;
    bool m_includesNonPlayers = false;
    bool m_single = true;
    String m_username;
};

/**
 * @brief 实体参数类型
 *
 * 解析实体选择器和玩家名称：
 * - @p - 最近玩家
 * - @a - 所有玩家
 * - @e - 所有实体
 * - @r - 随机玩家
 * - @s - 自己
 * - 玩家名称
 * - UUID
 *
 * 参考 MC 的 EntityArgument 类
 */
class EntityArgumentType : public ArgumentType<EntitySelector> {
public:
    /**
     * @brief 参数模式
     */
    enum class Mode {
        SingleEntity,    // 单个实体
        MultipleEntities, // 多个实体
        SinglePlayer,    // 单个玩家
        MultiplePlayers  // 多个玩家
    };

    explicit EntityArgumentType(Mode mode = Mode::SinglePlayer)
        : m_mode(mode) {}

    [[nodiscard]] EntitySelector parse(StringReader& reader) override {
        i32 start = reader.getCursor();

        // 检查是否是选择器
        if (reader.canRead() && reader.peek() == '@') {
            return parseSelector(reader, start);
        }

        // 解析玩家名称
        String name = reader.readString();
        return EntitySelector::byUsername(name);
    }

    [[nodiscard]] String getTypeName() const override {
        switch (m_mode) {
            case Mode::SingleEntity: return "entity";
            case Mode::MultipleEntities: return "entities";
            case Mode::SinglePlayer: return "player";
            case Mode::MultiplePlayers: return "players";
        }
        return "entity";
    }

    [[nodiscard]] std::vector<String> getExamples() const override {
        return {"Player", "0123", "@p", "@e[type=foo]", "@e[type=!cow]"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<EntityArgumentType> entity() {
        return std::make_shared<EntityArgumentType>(Mode::SingleEntity);
    }

    static std::shared_ptr<EntityArgumentType> entities() {
        return std::make_shared<EntityArgumentType>(Mode::MultipleEntities);
    }

    static std::shared_ptr<EntityArgumentType> player() {
        return std::make_shared<EntityArgumentType>(Mode::SinglePlayer);
    }

    static std::shared_ptr<EntityArgumentType> players() {
        return std::make_shared<EntityArgumentType>(Mode::MultiplePlayers);
    }

    // ========== 模式检查 ==========

    [[nodiscard]] bool isSingle() const noexcept {
        return m_mode == Mode::SingleEntity || m_mode == Mode::SinglePlayer;
    }

    [[nodiscard]] bool isPlayersOnly() const noexcept {
        return m_mode == Mode::SinglePlayer || m_mode == Mode::MultiplePlayers;
    }

    [[nodiscard]] Mode mode() const noexcept { return m_mode; }

private:
    Mode m_mode;

    /**
     * @brief 解析选择器 @p, @a, @e, @r, @s
     */
    EntitySelector parseSelector(StringReader& reader, i32 start) {
        reader.skip(); // 跳过 @

        if (!reader.canRead()) {
            throw CommandException(
                CommandErrorType::EntitySelectorInvalid,
                "Missing selector type",
                start
            );
        }

        char typeChar = reader.read();
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
                    start
                );
        }

        // 检查是否有选择器参数 [...]
        if (reader.canRead() && reader.peek() == '[') {
            parseSelectorArguments(reader, selector);
        }

        // 验证选择器模式
        validateSelector(selector, start);

        return selector;
    }

    /**
     * @brief 解析选择器参数 [type=..., limit=...]
     */
    void parseSelectorArguments(StringReader& reader, EntitySelector& selector) {
        reader.skip(); // 跳过 [

        while (reader.canRead() && reader.peek() != ']') {
            reader.skipWhitespace();

            if (!reader.canRead()) break;

            String paramName = readSelectorArgumentToken(reader);

            reader.skipWhitespace();

            // 期望 =
            if (!reader.canRead() || reader.peek() != '=') {
                throw CommandException(
                    CommandErrorType::EntitySelectorInvalid,
                    "Expected '=' after selector argument name",
                    reader.getCursor()
                );
            }
            reader.skip(); // 跳过 =
            reader.skipWhitespace();

            // 读取参数值
            String paramValue = reader.canRead() && reader.peek() == StringReader::SYNTAX_QUOTE
                ? reader.readString()
                : readSelectorArgumentToken(reader);

            // 应用参数
            applySelectorArgument(selector, paramName, paramValue);

            reader.skipWhitespace();

            // 检查是否有逗号分隔
            if (reader.canRead() && reader.peek() == ',') {
                reader.skip();
            }
        }

        if (!reader.canRead() || reader.peek() != ']') {
            throw CommandException(
                CommandErrorType::EntitySelectorInvalid,
                "Expected ']' to close selector arguments",
                reader.getCursor()
            );
        }

        reader.skip();
    }

    /**
     * @brief 应用选择器参数
     */
    void applySelectorArgument(EntitySelector& selector, const String& name, const String& value) {
        if (name == "limit" || name == "c") {
            i32 limit = std::stoi(value);
            selector.setLimit(limit);
            selector.setSingle(limit == 1);
        }
        // 其他参数可以后续扩展：type, sort, distance, x, y, z, dx, dy, dz, scores, tag, team, name, nbt, etc.
    }

    /**
     * @brief 验证选择器是否符合参数模式
     */
    void validateSelector(const EntitySelector& selector, i32 start) {
        // 检查是否允许非玩家实体
        if (isPlayersOnly() && selector.includesNonPlayers() && !selector.isSelf()) {
            throw CommandException(
                CommandErrorType::EntitySelectorNotAllowed,
                "Only players can be selected here",
                start
            );
        }

        // 检查是否要求单个实体
        if (isSingle() && !selector.isSingle() && selector.limit() > 1) {
            if (isPlayersOnly()) {
                throw CommandException(
                    CommandErrorType::PlayerTooMany,
                    "Only one player is allowed, but provided multiple",
                    start
                );
            } else {
                throw CommandException(
                    CommandErrorType::EntityTooMany,
                    "Only one entity is allowed, but provided multiple",
                    start
                );
            }
        }
    }

    [[nodiscard]] static String readSelectorArgumentToken(StringReader& reader) {
        i32 start = reader.getCursor();
        while (reader.canRead()) {
            char c = reader.peek();
            if (StringReader::isWhitespace(c) || c == '=' || c == ',' || c == ']') {
                break;
            }
            reader.skip();
        }

        if (reader.getCursor() == start) {
            throw CommandException(
                CommandErrorType::EntitySelectorInvalid,
                "Expected selector argument token",
                start
            );
        }

        const size_t startIndex = static_cast<size_t>(start);
        const size_t endIndex = static_cast<size_t>(reader.getCursor());
        return String(reader.getString().substr(startIndex, endIndex - startIndex));
    }
};

} // namespace command
} // namespace mc
