#pragma once

#include "common/core/Types.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "ArgumentType.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace mc {
class Entity;
class Player;
class ServerPlayer;

namespace command {

/**
 * @brief 实体选择器类型。
 */
enum class EntitySelectorType {
    SinglePlayer,
    AllPlayers,
    AllEntities,
    RandomPlayer,
    Self
};

/**
 * @brief 实体选择器。
 *
 * 封装命令层需要的最小选择结果描述，当前主要服务于玩家选择。
 */
class EntitySelector {
public:
    EntitySelector() = default;

    explicit EntitySelector(EntitySelectorType type)
        : m_type(type) {}

    /**
     * @brief 获取选择器类型。
     */
    [[nodiscard]] EntitySelectorType type() const noexcept { return m_type; }

    /**
     * @brief 获取结果数量上限。
     */
    [[nodiscard]] i32 limit() const noexcept { return m_limit; }

    /**
     * @brief 判断选择器是否明确表示自己。
     */
    [[nodiscard]] bool isSelf() const noexcept { return m_isSelf; }

    /**
     * @brief 判断是否允许选择非玩家实体。
     */
    [[nodiscard]] bool includesNonPlayers() const noexcept { return m_includesNonPlayers; }

    /**
     * @brief 判断选择器是否应当只解析出单个结果。
     */
    [[nodiscard]] bool isSingle() const noexcept { return m_single; }

    /**
     * @brief 获取显式指定的用户名。
     *
     * @return 用户名；若为空则表示并未按用户名指定。
     */
    [[nodiscard]] const String& username() const noexcept { return m_username; }

    /**
     * @brief 判断是否按用户名精确选择。
     */
    [[nodiscard]] bool hasUsername() const noexcept { return !m_username.empty(); }

    /**
     * @brief 设置数量上限。
     */
    void setLimit(i32 limit) { m_limit = limit; }

    /**
     * @brief 设置是否为自身选择器。
     */
    void setSelf(bool self) { m_isSelf = self; }

    /**
     * @brief 设置是否包含非玩家实体。
     */
    void setIncludesNonPlayers(bool includes) { m_includesNonPlayers = includes; }

    /**
     * @brief 设置是否只允许单结果。
     */
    void setSingle(bool single) { m_single = single; }

    /**
     * @brief 设置按用户名选择。
     */
    void setUsername(const String& username) { m_username = username; }

    /**
     * @brief 创建 @s 选择器。
     */
    [[nodiscard]] static EntitySelector self() {
        EntitySelector selector(EntitySelectorType::Self);
        selector.m_isSelf = true;
        selector.m_single = true;
        return selector;
    }

    /**
     * @brief 创建 @p 选择器。
     */
    [[nodiscard]] static EntitySelector nearestPlayer() {
        EntitySelector selector(EntitySelectorType::SinglePlayer);
        selector.m_single = true;
        selector.m_limit = 1;
        return selector;
    }

    /**
     * @brief 创建 @a 选择器。
     */
    [[nodiscard]] static EntitySelector allPlayers() {
        EntitySelector selector(EntitySelectorType::AllPlayers);
        selector.m_single = false;
        return selector;
    }

    /**
     * @brief 创建 @e 选择器。
     */
    [[nodiscard]] static EntitySelector allEntities() {
        EntitySelector selector(EntitySelectorType::AllEntities);
        selector.m_single = false;
        selector.m_includesNonPlayers = true;
        return selector;
    }

    /**
     * @brief 创建 @r 选择器。
     */
    [[nodiscard]] static EntitySelector randomPlayer() {
        EntitySelector selector(EntitySelectorType::RandomPlayer);
        selector.m_single = true;
        selector.m_limit = 1;
        return selector;
    }

    /**
     * @brief 创建按用户名精确匹配的选择器。
     */
    [[nodiscard]] static EntitySelector byUsername(const String& username) {
        EntitySelector selector(EntitySelectorType::SinglePlayer);
        selector.m_username = username;
        selector.m_single = true;
        selector.m_limit = 1;
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
 * @brief 实体参数类型。
 *
 * 当前支持玩家名和基础选择器语法，后续再逐步扩展完整的 Java 版 selector 能力。
 */
class EntityArgumentType : public ArgumentType<EntitySelector> {
public:
    /**
     * @brief 参数模式。
     */
    enum class Mode {
        SingleEntity,
        MultipleEntities,
        SinglePlayer,
        MultiplePlayers
    };

    explicit EntityArgumentType(Mode mode = Mode::SinglePlayer)
        : m_mode(mode) {}

    /**
     * @brief 解析实体选择器。
     *
     * @param reader 命令读取器。
     * @return 解析出的选择器。
     */
    [[nodiscard]] EntitySelector parse(StringReader& reader) override {
        const i32 start = reader.getCursor();

        if (reader.canRead() && reader.peek() == '@') {
            return parseSelector(reader, start);
        }

        const String name = reader.readString();
        return EntitySelector::byUsername(name);
    }

    /**
     * @brief 获取参数类型名。
     */
    [[nodiscard]] String getTypeName() const override {
        switch (m_mode) {
            case Mode::SingleEntity:
                return "entity";
            case Mode::MultipleEntities:
                return "entities";
            case Mode::SinglePlayer:
                return "player";
            case Mode::MultiplePlayers:
                return "players";
        }

        return "entity";
    }

    /**
     * @brief 获取示例输入。
     */
    [[nodiscard]] std::vector<String> getExamples() const override {
        return {"Player", "0123", "@p", "@a", "@s"};
    }

    /**
     * @brief 创建单实体参数类型。
     */
    [[nodiscard]] static std::shared_ptr<EntityArgumentType> entity() {
        return std::make_shared<EntityArgumentType>(Mode::SingleEntity);
    }

    /**
     * @brief 创建多实体参数类型。
     */
    [[nodiscard]] static std::shared_ptr<EntityArgumentType> entities() {
        return std::make_shared<EntityArgumentType>(Mode::MultipleEntities);
    }

    /**
     * @brief 创建单玩家参数类型。
     */
    [[nodiscard]] static std::shared_ptr<EntityArgumentType> player() {
        return std::make_shared<EntityArgumentType>(Mode::SinglePlayer);
    }

    /**
     * @brief 创建多玩家参数类型。
     */
    [[nodiscard]] static std::shared_ptr<EntityArgumentType> players() {
        return std::make_shared<EntityArgumentType>(Mode::MultiplePlayers);
    }

    /**
     * @brief 判断该参数类型是否只允许单结果。
     */
    [[nodiscard]] bool isSingle() const noexcept {
        return m_mode == Mode::SingleEntity || m_mode == Mode::SinglePlayer;
    }

    /**
     * @brief 判断该参数类型是否只允许玩家。
     */
    [[nodiscard]] bool isPlayersOnly() const noexcept {
        return m_mode == Mode::SinglePlayer || m_mode == Mode::MultiplePlayers;
    }

    /**
     * @brief 获取参数模式。
     */
    [[nodiscard]] Mode mode() const noexcept { return m_mode; }

private:
    /**
     * @brief 解析选择器语法。
     */
    [[nodiscard]] EntitySelector parseSelector(StringReader& reader, i32 start) {
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

    /**
     * @brief 解析选择器参数列表。
     */
    void parseSelectorArguments(StringReader& reader, EntitySelector& selector) {
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
            const String paramValue =
                reader.canRead() && reader.peek() == StringReader::SYNTAX_QUOTE
                    ? reader.readString()
                    : readSelectorArgumentToken(reader);

            applySelectorArgument(selector, paramName, paramValue);
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

    /**
     * @brief 应用单个选择器参数。
     *
     * @note 当前只支持 `limit` 和 `name`，后续可继续扩展。
     */
    void applySelectorArgument(EntitySelector& selector, const String& name, const String& value) {
        if (name == "limit" || name == "c") {
            const i32 limit = std::stoi(value);
            selector.setLimit(limit);
            selector.setSingle(limit == 1);
            return;
        }

        if (name == "name") {
            selector.setUsername(value);
            selector.setSingle(true);
        }
    }

    /**
     * @brief 校验解析结果是否与参数模式兼容。
     */
    void validateSelector(const EntitySelector& selector, i32 start) {
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

    /**
     * @brief 读取选择器参数 token。
     */
    [[nodiscard]] static String readSelectorArgumentToken(StringReader& reader) {
        const i32 start = reader.getCursor();
        while (reader.canRead()) {
            const char ch = reader.peek();
            if (StringReader::isWhitespace(ch) || ch == '=' || ch == ',' || ch == ']') {
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

private:
    Mode m_mode;
};

} // namespace command
} // namespace mc
