#pragma once

#include "common/core/Types.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/command/CommandResult.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <set>

namespace mc::command {

// 前向声明
template<typename S>
class CommandContext;

template<typename S>
class CommandNode;

template<typename S>
class CommandDispatcher;

// ========== 类型别名 ==========

/// 命令执行回调
template<typename S>
using CommandCallback = std::function<i32(CommandContext<S>&)>;

/// 权限检查谓词
template<typename S>
using RequirementPredicate = std::function<bool(const S&)>;

// ========== 命令节点类型 ==========

/**
 * @brief 命令节点类型
 */
enum class NodeType {
    Root = 0,       // 根节点
    Literal = 1,    // 字面量节点（如 "gamemode"）
    Argument = 2,   // 参数节点（如 <mode>）
};

/**
 * @brief 命令节点重定向模式
 */
enum class RedirectModifier {
    None,           // 无重定向
    Single,         // 单一重定向
    Fork            // 分叉重定向（执行多个）
};

/**
 * @brief 命令节点基类
 *
 * 构建命令树的基本单元，支持：
 * - 子节点添加
 * - 权限检查
 * - 命令执行
 * - 重定向
 *
 * 参考 MC 的 CommandNode 设计
 */
template<typename S>
class CommandNode {
public:
    virtual ~CommandNode() = default;

    // ========== 节点属性 ==========

    [[nodiscard]] virtual NodeType getType() const noexcept = 0;
    [[nodiscard]] virtual String getName() const noexcept = 0;
    [[nodiscard]] virtual String getTypeName() const { return getName(); }
    [[nodiscard]] virtual std::vector<String> getExamples() const { return {}; }
    [[nodiscard]] virtual nlohmann::json getMetadata() const { return nlohmann::json::object(); }
    virtual void parse(StringReader& reader, CommandContext<S>& context) const = 0;

    [[nodiscard]] const String& getUsageText() const { return m_usageText; }
    void setUsageText(const String& text) { m_usageText = text; }

    // ========== 命令 ==========

    [[nodiscard]] bool hasCommand() const noexcept { return m_command != nullptr; }
    [[nodiscard]] const CommandCallback<S>& getCommand() const noexcept { return m_command; }
    void setCommand(CommandCallback<S> command) { m_command = std::move(command); }

    /**
     * @brief 检查是否携带自定义建议提供器
     */
    [[nodiscard]] bool hasCustomSuggestions() const noexcept {
        return static_cast<bool>(m_customSuggestions);
    }

    /**
     * @brief 获取自定义建议提供器
     */
    [[nodiscard]] const std::shared_ptr<ISuggestionProvider<S>>& getCustomSuggestions() const noexcept {
        return m_customSuggestions;
    }

    /**
     * @brief 设置自定义建议提供器
     */
    void setCustomSuggestions(std::shared_ptr<ISuggestionProvider<S>> provider) {
        m_customSuggestions = std::move(provider);
    }

    // ========== 权限 ==========

    [[nodiscard]] const RequirementPredicate<S>& getRequirement() const noexcept {
        return m_requirement;
    }
    void setRequirement(RequirementPredicate<S> requirement) {
        m_requirement = std::move(requirement);
    }

    [[nodiscard]] bool canUse(const S& source) const {
        return m_requirement(source);
    }

    // ========== 子节点 ==========

    void addChild(std::shared_ptr<CommandNode<S>> node) {
        m_children[node->getName()] = node;
        if (node->getType() == NodeType::Literal) {
            m_literals.insert(node->getName());
        } else {
            m_arguments.insert(node->getName());
        }
    }

    [[nodiscard]] const std::unordered_map<String, std::shared_ptr<CommandNode<S>>>&
    getChildren() const noexcept { return m_children; }

    [[nodiscard]] std::shared_ptr<CommandNode<S>> getChild(const String& name) const {
        auto it = m_children.find(name);
        return it != m_children.end() ? it->second : nullptr;
    }

    [[nodiscard]] const std::set<String>& getLiterals() const noexcept { return m_literals; }
    [[nodiscard]] const std::set<String>& getArguments() const noexcept { return m_arguments; }

    // ========== 重定向 ==========

    [[nodiscard]] std::shared_ptr<CommandNode<S>> getRedirect() const noexcept { return m_redirect; }
    [[nodiscard]] bool hasRedirect() const noexcept { return static_cast<bool>(m_redirect); }
    [[nodiscard]] RedirectModifier getRedirectModifier() const noexcept { return m_redirectModifier; }

    /**
     * @brief 设置重定向目标
     */
    void setRedirect(std::shared_ptr<CommandNode<S>> target) {
        setRedirect(std::move(target), RedirectModifier::Single);
    }

    /**
     * @brief 设置重定向目标和模式
     */
    void setRedirect(std::shared_ptr<CommandNode<S>> target, RedirectModifier modifier) {
        m_redirect = std::move(target);
        m_redirectModifier = modifier;
    }

    [[nodiscard]] bool isFork() const noexcept {
        return m_redirectModifier == RedirectModifier::Fork;
    }

    // ========== 比较 ==========

    virtual bool equals(const CommandNode<S>& other) const {
        // 简化比较：只比较重定向和类型
        return m_redirect == other.m_redirect &&
               m_redirectModifier == other.m_redirectModifier &&
               this->getType() == other.getType();
    }

    [[nodiscard]] virtual size_t hashCode() const {
        size_t hash = 0;
        // 简单的哈希组合
        return hash;
    }

protected:
    CommandNode() = default;

    CommandCallback<S> m_command;
    RequirementPredicate<S> m_requirement = [](const S&) { return true; };
    std::shared_ptr<ISuggestionProvider<S>> m_customSuggestions;
    std::unordered_map<String, std::shared_ptr<CommandNode<S>>> m_children;
    std::set<String> m_literals;
    std::set<String> m_arguments;
    std::shared_ptr<CommandNode<S>> m_redirect;
    RedirectModifier m_redirectModifier = RedirectModifier::None;
    String m_usageText;
};

/**
 * @brief 根命令节点
 */
template<typename S>
class RootCommandNode : public CommandNode<S> {
public:
    RootCommandNode() = default;

    [[nodiscard]] NodeType getType() const noexcept override { return NodeType::Root; }
    [[nodiscard]] String getName() const noexcept override { return ""; }
    void parse(StringReader& /*reader*/, CommandContext<S>& /*context*/) const override {}
};

/**
 * @brief 字面量命令节点
 *
 * 表示固定的命令字，如 "gamemode"、"tp"
 */
template<typename S>
class LiteralCommandNode : public CommandNode<S> {
public:
    explicit LiteralCommandNode(const String& literal)
        : m_literal(literal) {}

    [[nodiscard]] NodeType getType() const noexcept override { return NodeType::Literal; }
    [[nodiscard]] String getName() const noexcept override { return m_literal; }
    [[nodiscard]] const String& getLiteral() const noexcept { return m_literal; }

    void parse(StringReader& reader, CommandContext<S>& /*context*/) const override {
        const i32 start = reader.getCursor();
        const String literal = reader.readUnquotedString();
        if (literal != m_literal) {
            reader.setCursor(start);
            throw CommandException(
                CommandErrorType::DispatcherExpectedLiteral,
                "Expected literal '" + m_literal + "'",
                start
            );
        }
    }

    bool equals(const CommandNode<S>& other) const override {
        if (!CommandNode<S>::equals(other)) return false;
        if (other.getType() != NodeType::Literal) return false;
        return m_literal == static_cast<const LiteralCommandNode<S>&>(other).m_literal;
    }

private:
    String m_literal;
};

/**
 * @brief 参数命令节点
 *
 * 表示可变的命令参数，如 <player>、<pos>
 */
template<typename S, typename T>
class ArgumentCommandNode : public CommandNode<S> {
public:
    using Parser = std::function<T(StringView, i32&, CommandException&)>;

    /**
     * @brief 使用解析函数构造
     */
    ArgumentCommandNode(const String& name, Parser parser)
        : m_name(name)
        , m_parser(std::move(parser)) {}

    /**
     * @brief 使用 ArgumentType 构造
     *
     * 更方便的构造方式，从 ArgumentType 派生类创建。
     * 例如：ArgumentCommandNode<ServerCommandSource, i32>("value", IntegerArgumentType::integer())
     */
    ArgumentCommandNode(const String& name, std::shared_ptr<ArgumentType<T>> argumentType)
        : m_name(name)
        , m_parser([this](StringView input, i32& cursor, CommandException& error) -> T {
            StringReader reader(input);
            reader.setCursor(cursor);
            try {
                T result = m_argumentType->parse(reader);
                cursor = reader.getCursor();
                return result;
            } catch (const CommandException& e) {
                error = e;
                cursor = -1;
                return T{};
            }
        })
        , m_argumentType(std::move(argumentType)) {}

    [[nodiscard]] NodeType getType() const noexcept override { return NodeType::Argument; }
    [[nodiscard]] String getName() const noexcept override { return m_name; }
    [[nodiscard]] String getTypeName() const override {
        return m_argumentType ? m_argumentType->getTypeName() : "argument";
    }

    [[nodiscard]] nlohmann::json getMetadata() const override {
        return m_argumentType ? m_argumentType->serializeMetadata() : nlohmann::json::object();
    }

    [[nodiscard]] std::vector<String> getExamples() const override {
        return m_argumentType ? m_argumentType->getExamples() : std::vector<String>{};
    }

    void parse(StringReader& reader, CommandContext<S>& context) const override {
        const i32 start = reader.getCursor();
        i32 cursor = start;
        T result = parse(reader.getString(), cursor);
        reader.setCursor(cursor);
        context.setArgument(m_name, result, start);
    }

    /**
     * @brief 解析参数值
     * @param input 输入字符串
     * @param cursor 当前位置（会被更新）
     * @return 解析结果，失败时抛出异常
     */
    [[nodiscard]] T parse(StringView input, i32& cursor) const {
        CommandException error(CommandErrorType::Unknown, "Parse error");
        T result = m_parser(input, cursor, error);
        if (cursor < 0) {
            throw error;
        }
        return result;
    }

    /**
     * @brief 获取底层参数类型
     */
    [[nodiscard]] std::shared_ptr<ArgumentType<T>> getArgumentType() const noexcept {
        return m_argumentType;
    }

    bool equals(const CommandNode<S>& other) const override {
        if (!CommandNode<S>::equals(other)) return false;
        if (other.getType() != NodeType::Argument) return false;
        return m_name == static_cast<const ArgumentCommandNode<S, T>&>(other).m_name;
    }

private:
    String m_name;
    Parser m_parser;
    std::shared_ptr<ArgumentType<T>> m_argumentType;
};

} // namespace mc::command
