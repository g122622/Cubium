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

#pragma once

#include "common/command/CommandContext.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/CommandResult.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <cctype>
#include <cstddef>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc::command {

// ========== 前向声明 ==========

template <typename S>
class LiteralArgumentBuilder;

template <typename S, typename T>
class RequiredArgumentBuilder;

// ========== 分发器 ==========

/**
 * @brief 命令分发器
 *
 * 核心命令解析和执行引擎。
 *
 * 功能：
 * - 注册命令节点
 * - 解析命令字符串
 * - 执行命令
 * - 提供自动补全建议
 *
 * 使用示例：
 * @code
 * CommandDispatcher<CommandSource> dispatcher;
 *
 * // 注册命令
 * auto node = std::make_shared<LiteralCommandNode<CommandSource>>("gamemode");
 * node->setCommand([](CommandContext<CommandSource>&) { return 1; });
 * dispatcher.registerCommand(node);
 *
 * // 执行命令
 * auto result = dispatcher.execute("gamemode", source);
 * @endcode
 */
template <typename S>
class CommandDispatcher {
public:
    using NodePtr = std::shared_ptr<CommandNode<S>>;
    using RootNodePtr = std::shared_ptr<RootCommandNode<S>>;

    CommandDispatcher()
        : m_root(std::make_shared<RootCommandNode<S>>())
    {}

    // ========== 命令注册 ==========

    /**
     * @brief 注册命令节点
     * @param node 命令节点
     * @return 注册的命令节点
     */
    NodePtr registerCommand(std::shared_ptr<LiteralCommandNode<S>> node)
    {
        m_root->addChild(node);
        return node;
    }

    // ========== 命令解析 ==========

    /**
     * @brief 解析命令字符串
     * @param input 命令字符串（可带或不带 / 前缀）
     * @param source 命令源
     * @return 解析结果
     */
    [[nodiscard]] ParseResults<S> parse(std::string_view input, S& source) const
    {
        StringReader reader(input);

        // 跳过命令前缀
        if (reader.canRead() && reader.peek() == '/') {
            reader.skip();
        }

        auto context = std::make_unique<CommandContext<S>>(source, input, m_root);

        return parseNodes(reader, m_root, std::move(context));
    }

    /**
     * @brief 解析命令节点
     */
    [[nodiscard]] ParseResults<S> parseNodes(
        StringReader& reader, NodePtr node, std::unique_ptr<CommandContext<S>> context) const;

    // ========== 命令执行 ==========

    /**
     * @brief 执行命令
     * @param input 命令字符串
     * @param source 命令源
     * @return 执行结果
     */
    Result<CommandResult> execute(std::string_view input, S& source)
    {
        ParseResults<S> parseResult = parse(input, source);
        return execute(parseResult);
    }

    /**
     * @brief 获取命令建议
     * @param input 命令输入
     * @param source 命令源
     * @return 异步建议结果
     */
    [[nodiscard]] std::future<Suggestions> getSuggestions(std::string_view input, S& source) const;

    /**
     * @brief 执行已解析的命令
     * @param parse 解析结果
     * @return 执行结果
     */
    Result<CommandResult> execute(ParseResults<S>& parse);

    // ========== 命令查询 ==========

    /**
     * @brief 获取根节点
     */
    [[nodiscard]] NodePtr getRoot() const noexcept { return m_root; }

    /**
     * @brief 获取命令路径
     * @param node 命令节点
     * @return 路径字符串列表
     */
    [[nodiscard]] std::vector<std::string> getPath(NodePtr node) const;

    /**
     * @brief 查找歧义命令
     */
    void findAmbiguities(std::function<void(NodePtr, NodePtr, const std::set<std::string>&)> callback) const;

private:
    /**
     * @brief 执行命令节点
     */
    i32 _executeCommand(const CommandContext<S>& context, NodePtr node, StringReader& reader);

    RootNodePtr m_root;
};

// ========== 构建器工具函数 ==========

/**
 * @brief 创建字面量构建器
 */
template <typename S>
LiteralArgumentBuilder<S> literal(const std::string& name);

/**
 * @brief 创建参数构建器
 */
template <typename S, typename T>
RequiredArgumentBuilder<S, T> argument(const std::string& name, std::shared_ptr<ArgumentCommandNode<S, T>> type);

// ========== 字面量构建器 ==========

/**
 * @brief 字面量构建器
 */
template <typename S>
class LiteralArgumentBuilder {
public:
    using NodePtr = std::shared_ptr<CommandNode<S>>;

    explicit LiteralArgumentBuilder(const std::string& literal)
        : m_literal(literal)
    {}

    // ========== 命令执行 ==========

    LiteralArgumentBuilder& executes(CommandCallback<S> command)
    {
        m_command = std::move(command);
        return *this;
    }

    // ========== 权限 ==========

    LiteralArgumentBuilder& withRequirement(RequirementPredicate<S> requirement)
    {
        m_requirement = std::move(requirement);
        return *this;
    }

    // ========== 子节点 ==========

    LiteralArgumentBuilder& then(NodePtr node)
    {
        m_children.push_back(node);
        return *this;
    }

    template <typename Builder>
    LiteralArgumentBuilder& then(Builder&& builder)
    {
        m_children.push_back(builder.build());
        return *this;
    }

    // ========== 重定向 ==========

    LiteralArgumentBuilder& redirectsTo(NodePtr target)
    {
        m_redirect = target;
        return *this;
    }

    // ========== 构建 ==========

    [[nodiscard]] NodePtr build() const
    {
        auto node = std::make_shared<LiteralCommandNode<S>>(m_literal);

        if (m_command) {
            node->setCommand(m_command);
        }
        node->setRequirement(m_requirement);

        for (const auto& child : m_children) {
            node->addChild(child);
        }

        if (m_redirect) {
            node->setRedirect(m_redirect);
        }

        return node;
    }

private:
    std::string m_literal;
    CommandCallback<S> m_command;
    RequirementPredicate<S> m_requirement = [](const S&) { return true; };
    std::vector<NodePtr> m_children;
    NodePtr m_redirect;
};

// ========== 参数构建器 ==========

/**
 * @brief 参数构建器
 */
template <typename S, typename T>
class RequiredArgumentBuilder {
public:
    using NodePtr = std::shared_ptr<CommandNode<S>>;

    RequiredArgumentBuilder(const std::string& name, std::shared_ptr<ArgumentCommandNode<S, T>> type)
        : m_name(name)
        , m_type(type)
    {}

    // ========== 命令执行 ==========

    RequiredArgumentBuilder& executes(CommandCallback<S> command)
    {
        m_command = std::move(command);
        return *this;
    }

    // ========== 权限 ==========

    RequiredArgumentBuilder& withRequirement(RequirementPredicate<S> requirement)
    {
        m_requirement = std::move(requirement);
        return *this;
    }

    // ========== 子节点 ==========

    RequiredArgumentBuilder& then(NodePtr node)
    {
        m_children.push_back(node);
        return *this;
    }

    template <typename Builder>
    RequiredArgumentBuilder& then(Builder&& builder)
    {
        m_children.push_back(builder.build());
        return *this;
    }

    // ========== 构建 ==========

    [[nodiscard]] NodePtr build() const
    {
        if (m_command) {
            m_type->setCommand(m_command);
        }
        m_type->setRequirement(m_requirement);
        m_type->setCustomSuggestions(m_customSuggestions);

        for (const auto& child : m_children) {
            m_type->addChild(child);
        }

        return m_type;
    }

    /**
     * @brief 设置自定义建议提供器
     * @param provider 建议提供器实例，通常由 `CandidateSuggestionProvider` 或自定义实现提供
     * @return 当前构建器
     */
    RequiredArgumentBuilder& suggests(std::shared_ptr<ISuggestionProvider<S>> provider)
    {
        m_customSuggestions = std::move(provider);
        return *this;
    }

private:
    std::string m_name;
    std::shared_ptr<ArgumentCommandNode<S, T>> m_type;
    CommandCallback<S> m_command;
    RequirementPredicate<S> m_requirement = [](const S&) { return true; };
    std::shared_ptr<ISuggestionProvider<S>> m_customSuggestions;
    std::vector<NodePtr> m_children;
};

// ========== 模板实现 ==========

template <typename S>
LiteralArgumentBuilder<S> literal(const std::string& name)
{
    return LiteralArgumentBuilder<S>(name);
}

template <typename S, typename T>
RequiredArgumentBuilder<S, T> argument(const std::string& name, std::shared_ptr<ArgumentCommandNode<S, T>> type)
{
    return RequiredArgumentBuilder<S, T>(name, type);
}

// ========== CommandDispatcher 模板实现 ==========

template <typename S>
ParseResults<S> CommandDispatcher<S>::parseNodes(
    StringReader& reader, NodePtr node, std::unique_ptr<CommandContext<S>> context) const
{
    std::unordered_set<const CommandNode<S>*> redirectStack;

    std::function<ParseResults<S>(StringReader&, NodePtr, std::unique_ptr<CommandContext<S>>)> parseRecursive;
    parseRecursive = [&](StringReader& currentReader,
                         NodePtr currentNode,
                         std::unique_ptr<CommandContext<S>> currentContext) -> ParseResults<S> {
        currentReader.skipWhitespace();
        currentContext->setCurrentNode(currentNode);

        if (!currentReader.canRead()) {
            return ParseResults<S>(std::move(currentContext), currentReader.getRemaining());
        }

        ParseResults<S> bestSuccess;
        bool hasBestSuccess = false;
        ParseResults<S> bestFailure;
        bool hasBestFailure = false;

        auto considerResult = [&](ParseResults<S>&& candidate) {
            if (candidate.isSuccess()) {
                const auto* candidateContext = candidate.getContext();
                const auto candidateNode = candidateContext ? candidateContext->getCurrentNode() : nullptr;
                if (candidate.getRemaining().empty() && (!candidateNode || !candidateNode->hasCommand())) {
                    return;
                }
                if (!hasBestSuccess || candidate.getRemaining().size() < bestSuccess.getRemaining().size()) {
                    bestSuccess = std::move(candidate);
                    hasBestSuccess = true;
                }
                return;
            }

            if (!hasBestFailure || candidate.getErrorCursor() > bestFailure.getErrorCursor()) {
                bestFailure = std::move(candidate);
                hasBestFailure = true;
            }
        };

        auto tryChild = [&](const std::string& childName) {
            auto child = currentNode->getChild(childName);
            if (!child) {
                return;
            }
            if (!child->canUse(currentContext->getSource())) {
                // 权限不足但输入精确匹配该字面量节点时，记录友好失败候选。
                // 否则该节点被静默跳过，bestFailure 会落到其它字面量在当前 token 上抛出的
                // 误导性 Expected literal 'X'（如 /tp 无权限时报 Expected literal 'help'）。
                // 仅 Literal + 精确匹配才触发；参数节点或不匹配的输入仍走 Unknown command。
                if (child->getType() == NodeType::Literal) {
                    StringReader probe = currentReader; // 拷贝，不推进原 reader
                    std::string token = probe.readUnquotedString();
                    if (token == childName) {
                        const i32 deniedCursor = probe.getCursor();
                        CommandException denied(
                            CommandErrorType::PermissionDenied, "commands.permission.denied", deniedCursor);
                        considerResult(ParseResults<S>(denied.withInput(currentContext->getInput()), deniedCursor));
                    }
                }
                return;
            }

            StringReader childReader = currentReader;
            auto childContext =
                std::make_unique<CommandContext<S>>(currentContext->copyFor(currentContext->getRootNode()));

            try {
                child->parse(childReader, *childContext);
                childContext->setCurrentNode(child);

                if (child->hasRedirect()) {
                    auto redirectTarget = child->getRedirect();
                    if (redirectTarget && redirectStack.insert(redirectTarget.get()).second) {
                        considerResult(parseRecursive(childReader, redirectTarget, std::move(childContext)));
                        redirectStack.erase(redirectTarget.get());
                    } else {
                        considerResult(parseRecursive(childReader, child, std::move(childContext)));
                    }
                } else {
                    considerResult(parseRecursive(childReader, child, std::move(childContext)));
                }
            }
            catch (const CommandException& e) {
                considerResult(ParseResults<S>(e.withInput(currentContext->getInput()), e.cursor()));
            }
        };

        for (const auto& childName : currentNode->getLiterals()) {
            tryChild(childName);
        }

        for (const auto& childName : currentNode->getArguments()) {
            tryChild(childName);
        }

        if (hasBestSuccess) {
            return std::move(bestSuccess);
        }

        if (currentNode->hasCommand()) {
            return ParseResults<S>(std::move(currentContext), currentReader.getRemaining());
        }

        if (hasBestFailure) {
            return std::move(bestFailure);
        }

        return ParseResults<S>(
            CommandException(currentNode->getType() == NodeType::Root ? CommandErrorType::DispatcherUnknownCommand
                                                                      : CommandErrorType::DispatcherUnknownArgument,
                currentNode->getType() == NodeType::Root ? "Unknown command" : "Unknown argument",
                currentReader.getCursor())
                .withInput(currentContext->getInput()),
            currentReader.getCursor());
    };

    return parseRecursive(reader, node, std::move(context));
}

template <typename S>
Result<CommandResult> CommandDispatcher<S>::execute(ParseResults<S>& parse)
{
    if (const auto exception = parse.getException(); exception.has_value()) {
        return Error(ErrorCode::InvalidArgument, exception->message());
    }

    auto* context = parse.getContext();
    if (!context) {
        return Error(ErrorCode::Unknown, "No command context");
    }

    auto node = context->getCurrentNode();
    if (!node || !node->hasCommand()) {
        return Error(ErrorCode::Unknown, "No command to execute");
    }

    try {
        i32 result = node->getCommand()(*context);
        return CommandResult::success(result);
    }
    catch (const CommandException& e) {
        return Error(ErrorCode::Unknown, e.message());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::Unknown, e.what());
    }
}

template <typename S>
std::future<Suggestions> CommandDispatcher<S>::getSuggestions(std::string_view input, S& source) const
{
    StringReader reader(input);
    if (reader.canRead() && reader.peek() == '/') {
        reader.skip();
    }

    auto context = std::make_unique<CommandContext<S>>(source, input, m_root);
    NodePtr currentNode = m_root;

    auto startsWithIgnoreCase = [](std::string_view text, std::string_view prefix) {
        if (prefix.size() > text.size()) {
            return false;
        }

        for (size_t index = 0; index < prefix.size(); ++index) {
            const unsigned char left = static_cast<unsigned char>(text[index]);
            const unsigned char right = static_cast<unsigned char>(prefix[index]);
            if (std::tolower(left) != std::tolower(right)) {
                return false;
            }
        }

        return true;
    };

    auto collectSuggestions = [&](NodePtr suggestionNode, CommandContext<S>& suggestionContext, i32 start, i32 end) {
        Suggestions merged = Suggestions::empty();

        if (!suggestionNode) {
            return merged;
        }

        if (suggestionNode->hasRedirect()) {
            suggestionNode = suggestionNode->getRedirect();
        }

        SuggestionsBuilder literalBuilder(input, start, end);
        literalBuilder.suggestAll(suggestionNode->getLiterals());
        merged = Suggestions::merge(merged, literalBuilder.build());

        for (const auto& argumentName : suggestionNode->getArguments()) {
            auto child = suggestionNode->getChild(argumentName);
            if (!child || !child->canUse(suggestionContext.getSource())) {
                continue;
            }

            SuggestionsBuilder argumentBuilder(input, start, end);
            if (const auto& provider = child->getCustomSuggestions(); provider) {
                merged = Suggestions::merge(merged, provider->getSuggestions(suggestionContext, argumentBuilder).get());
            } else {
                argumentBuilder.suggestAll(child->getExamples());
                merged = Suggestions::merge(merged, argumentBuilder.build());
            }
        }

        return merged;
    };

    while (true) {
        reader.skipWhitespace();
        context->setCurrentNode(currentNode);

        if (!reader.canRead()) {
            std::promise<Suggestions> promise;
            promise.set_value(collectSuggestions(currentNode, *context, reader.getCursor(), reader.getCursor()));
            return promise.get_future();
        }

        const i32 tokenStart = reader.getCursor();
        StringReader tokenReader = reader;
        std::string token = tokenReader.readUnquotedString();
        const i32 tokenEnd = tokenReader.getCursor();

        NodePtr matchedNode;
        StringReader matchedReader = reader;
        std::unique_ptr<CommandContext<S>> matchedContext;
        bool matched = false;
        bool hasLiteralPrefix = false;

        for (const auto& childName : currentNode->getLiterals()) {
            auto child = currentNode->getChild(childName);
            if (!child || !child->canUse(context->getSource())) {
                continue;
            }

            if (startsWithIgnoreCase(childName, token)) {
                hasLiteralPrefix = true;
            }

            if (childName != token) {
                continue;
            }

            matchedReader = tokenReader;
            matchedContext = std::make_unique<CommandContext<S>>(context->copyFor(context->getRootNode()));
            matchedContext->setCurrentNode(child);
            matchedNode = child->hasRedirect() ? child->getRedirect() : child;
            matched = true;
            break;
        }

        if (!matched && hasLiteralPrefix && !token.empty()) {
            std::promise<Suggestions> promise;
            promise.set_value(collectSuggestions(currentNode, *context, tokenStart, tokenEnd));
            return promise.get_future();
        }

        if (!matched) {
            for (const auto& childName : currentNode->getArguments()) {
                auto child = currentNode->getChild(childName);
                if (!child || !child->canUse(context->getSource())) {
                    continue;
                }

                StringReader childReader = reader;
                auto childContext = std::make_unique<CommandContext<S>>(context->copyFor(context->getRootNode()));

                try {
                    child->parse(childReader, *childContext);
                    childContext->setCurrentNode(child);
                    matchedReader = childReader;
                    matchedContext = std::move(childContext);
                    matchedNode = child->hasRedirect() ? child->getRedirect() : child;
                    matched = true;
                    break;
                }
                catch (const CommandException&) {
                }
            }
        }

        if (!matched) {
            std::promise<Suggestions> promise;
            promise.set_value(collectSuggestions(currentNode, *context, tokenStart, tokenEnd));
            return promise.get_future();
        }

        reader = matchedReader;
        context = std::move(matchedContext);
        if (matchedNode) {
            currentNode = matchedNode;
        }
    }
}

template <typename S>
std::vector<std::string> CommandDispatcher<S>::getPath(NodePtr node) const
{
    if (!node) {
        return {};
    }

    if (node == m_root) {
        return {};
    }

    std::vector<std::string> currentPath;
    std::vector<std::string> result;

    std::function<bool(NodePtr)> collectPath = [&](NodePtr current) -> bool {
        if (!current) {
            return false;
        }

        if (current == node) {
            result = currentPath;
            return true;
        }

        for (const auto& childName : current->getLiterals()) {
            auto child = current->getChild(childName);
            if (!child) {
                continue;
            }

            currentPath.push_back(child->getName());
            if (collectPath(child)) {
                return true;
            }
            currentPath.pop_back();
        }

        for (const auto& childName : current->getArguments()) {
            auto child = current->getChild(childName);
            if (!child) {
                continue;
            }

            currentPath.push_back(child->getName());
            if (collectPath(child)) {
                return true;
            }
            currentPath.pop_back();
        }

        return false;
    };

    if (collectPath(m_root)) {
        return result;
    }

    return {};
}

template <typename S>
void CommandDispatcher<S>::findAmbiguities(
    std::function<void(NodePtr, NodePtr, const std::set<std::string>&)> callback) const
{
    if (!callback) {
        return;
    }

    auto toLower = [](const std::string& value) {
        std::string result = value;
        for (char& character : result) {
            if (character >= 'A' && character <= 'Z') {
                character = static_cast<char>(character - 'A' + 'a');
            }
        }
        return result;
    };

    auto collectCandidates = [&](NodePtr child) {
        std::set<std::string> candidates;
        if (!child) {
            return candidates;
        }

        if (child->getType() == NodeType::Literal) {
            candidates.insert(toLower(child->getName()));
            return candidates;
        }

        for (const auto& example : child->getExamples()) {
            candidates.insert(toLower(example));
        }

        return candidates;
    };

    std::function<void(NodePtr)> visit;
    visit = [&](NodePtr current) {
        if (!current) {
            return;
        }

        std::vector<NodePtr> children;
        children.reserve(current->getLiterals().size() + current->getArguments().size());

        for (const auto& childName : current->getLiterals()) {
            auto child = current->getChild(childName);
            if (child) {
                children.push_back(child);
            }
        }

        for (const auto& childName : current->getArguments()) {
            auto child = current->getChild(childName);
            if (child) {
                children.push_back(child);
            }
        }

        for (size_t leftIndex = 0; leftIndex < children.size(); ++leftIndex) {
            for (size_t rightIndex = leftIndex + 1; rightIndex < children.size(); ++rightIndex) {
                const auto leftCandidates = collectCandidates(children[leftIndex]);
                const auto rightCandidates = collectCandidates(children[rightIndex]);

                std::set<std::string> ambiguous;
                for (const auto& candidate : leftCandidates) {
                    if (rightCandidates.find(candidate) != rightCandidates.end()) {
                        ambiguous.insert(candidate);
                    }
                }

                if (!ambiguous.empty()) {
                    callback(children[leftIndex], children[rightIndex], ambiguous);
                }
            }
        }

        for (const auto& child : children) {
            visit(child);
        }
    };

    visit(m_root);
}

} // namespace mc::command
