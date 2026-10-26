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
#include <optional>
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
    [[nodiscard]] NodePtr getRoot() const noexcept
    {
        return m_root;
    }

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
    RequirementPredicate<S> m_requirement = [](const S&) {
        return true;
    };
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
    RequirementPredicate<S> m_requirement = [](const S&) {
        return true;
    };
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

    // 候选选择（对齐 brigadier 的 CommandDispatcher.parseNodes）：
    //  - 成功：整条输入吃完、末端节点可执行，取剩余最短者；
    //  - 失败：取「已读字符数 - 错误游标」最小者，用于报最贴切的错误。
    // Cubium 多了「转发节点」一类末端（/execute 的 run → command）：它自己不执行，
    // 命令在祖先节点上。这类候选在输入吃完时同样可执行，所以按同一个口径参与比较。
    struct Candidate {
        ParseResults<S> results;
        NodePtr leaf;
    };

    std::function<ParseResults<S>(StringReader&, NodePtr, std::unique_ptr<CommandContext<S>>)> parseRecursive;
    parseRecursive = [&](StringReader& currentReader,
                         NodePtr currentNode,
                         std::unique_ptr<CommandContext<S>> currentContext) -> ParseResults<S> {
        currentReader.skipWhitespace();
        currentContext->setCurrentNode(currentNode);
        const i32 start = currentReader.getCursor();
        if (currentReader.atEnd()) {
            // brigadier 把这种结果标成「成功、但不构成派发」（isCommand = 带命令或重定向）。
            // `list extra`：list 带命令，整段读完的同时还留着 "extra"，若一律按成功返回，上层
            // 会把 list 当成末端执行并忽略剩余文本。这里只让「可派发」的末端保持成功，
            // 其它节点交还给上层按失败候选比较。
            if (currentNode->hasCommand() || currentNode->hasRedirect() || currentNode->getType() == NodeType::Root) {
                return ParseResults<S>(std::move(currentContext), currentReader.getRemaining());
            }
            return ParseResults<S>(
                CommandException(
                    CommandErrorType::DispatcherUnknownArgument, "Unknown argument", currentReader.getCursor())
                    .withInput(currentContext->getInput()),
                currentReader.getCursor());
        }

        std::optional<Candidate> bestSuccess;
        std::optional<Candidate> bestFailure;
        i32 bestFailureRank = 0;

        // 末端可执行的命令：节点自己有命令就用它，否则沿转发链找最近的祖先。
        auto commandOf = [](NodePtr leaf) -> std::shared_ptr<CommandNode<S>> {
            while (leaf && !leaf->hasCommand() && leaf->getForwardParent()) {
                leaf = leaf->getForwardParent();
            }
            return leaf && leaf->hasCommand() ? leaf : nullptr;
        };

        auto consider = [&](ParseResults<S>&& candidate, NodePtr leaf) {
            if (candidate.isSuccess()) {
                auto* ctx = candidate.getContext();
                const auto tail = ctx ? ctx->getCurrentNode() : nullptr;
                const bool tailHasCommand = tail && commandOf(tail);
                // 还剩文本、末端节点本身又不是可执行节点（命令挂在祖先上）：这一跳没有解释掉
                // 剩余文本，不能算成功候选（`say <message>` 的 message 参数读走右引号后还留着
                // 文本，命令在 say 上，此时应继续交给 say 的其它候选比较）。
                // 注意末端的 tail 是转发节点时，剩余文本本就交给链尾解释（/execute run）。
                if (!candidate.getRemaining().empty() && (!tail || !tail->hasCommand())) {
                    return;
                }
                if (!bestSuccess || candidate.getRemaining().size() < bestSuccess->results.getRemaining().size()) {
                    bestSuccess = Candidate{std::move(candidate), std::move(leaf)};
                }
                return;
            }

            // 失败候选按「错误位置 - 已读字符数」比较：错误位置越靠后（这一跳读得越深），
            // 描述越贴合；与起点远近无关。
            const i32 consumed = currentReader.getCursor() - start;
            const i32 rank = candidate.getErrorCursor() - consumed;
            if (!bestFailure || rank > bestFailureRank) {
                bestFailureRank = rank;
                bestFailure = Candidate{std::move(candidate), nullptr};
            }
        };

        bool permissionDenied = false;

        auto tryChild = [&](const std::string& childName) {
            auto child = currentNode->getChild(childName);
            if (!child) {
                return;
            }
            if (!child->canUse(currentContext->getSource())) {
                // 输入精确匹配了该字面量但权限不足：与 brigadier 一致，直接返回 PermissionDenied，
                // 不再继续尝试其它候选（否则其它字面量的 Expected literal 会盖掉这个更贴切的错误）。
                // 本轮已有一处同样拒绝过时不再重复（`tp` 重定向到 `teleport`，两者会是同一个错误）。
                if (child->getType() == NodeType::Literal) {
                    StringReader probe = currentReader;
                    if (probe.readUnquotedString() == childName) {
                        const i32 deniedCursor = probe.getCursor();
                        if (!permissionDenied) {
                            permissionDenied = true;
                            consider(ParseResults<S>(CommandException(CommandErrorType::PermissionDenied,
                                                         "commands.permission.denied",
                                                         deniedCursor)
                                                         .withInput(currentContext->getInput()),
                                         deniedCursor),
                                nullptr);
                        }
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
                        // 重定向目标同样受权限约束（/tp 只是 /teleport 的别名，原版在派发时
                        // 按目标节点的 requirement 过滤）。
                        if (!redirectTarget->canUse(childContext->getSource())) {
                            redirectStack.erase(redirectTarget.get());
                            if (!permissionDenied) {
                                permissionDenied = true;
                                const i32 deniedCursor = childReader.getCursor();
                                consider(ParseResults<S>(CommandException(CommandErrorType::PermissionDenied,
                                                             "commands.permission.denied",
                                                             deniedCursor)
                                                             .withInput(currentContext->getInput()),
                                             deniedCursor),
                                    nullptr);
                            }
                            return;
                        }
                        consider(parseRecursive(childReader, redirectTarget, std::move(childContext)), child);
                        redirectStack.erase(redirectTarget.get());
                    } else {
                        consider(parseRecursive(childReader, child, std::move(childContext)), child);
                    }
                } else {
                    consider(parseRecursive(childReader, child, std::move(childContext)), child);
                }
            }
            catch (const CommandException& e) {
                consider(ParseResults<S>(e.withInput(currentContext->getInput()), e.cursor()), nullptr);
            }
        };

        for (const auto& childName : currentNode->getLiterals()) {
            tryChild(childName);
        }
        for (const auto& childName : currentNode->getArguments()) {
            tryChild(childName);
        }

        if (permissionDenied) {
            // 本轮有节点因权限被拒：与 brigadier 一致直接返回该错误，不再落到更模糊的候选。
            return std::move(bestFailure->results);
        }

        if (bestSuccess) {
            return std::move(bestSuccess->results);
        }

        // 没有可接受的成功候选、本节点却有命令时（`list extra` 之上层层回退的场景）：
        // 只有输入已读完或本节点可重定向才算成功，否则交给下面的失败候选。
        if (currentNode->hasCommand() && (currentReader.atEnd() || currentNode->hasRedirect())) {
            return ParseResults<S>(std::move(currentContext), currentReader.getRemaining());
        }

        if (bestFailure) {
            return std::move(bestFailure->results);
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
    {
        auto ex = parse.getException();
    }
    if (const auto exception = parse.getException(); exception.has_value()) {
        return Error(ErrorCode::InvalidArgument, exception->message());
    }

    auto* context = parse.getContext();
    if (!context) {
        return Error(ErrorCode::Unknown, "No command context");
    }

    // 「待解释」的中间结果（/execute 的 run 把游标交还给 dispatcher 继续派发 command）：
    // 末端节点自己不执行，命令由链上的转发祖先执行。
    if (context->isPendingHandOff()) {
        auto command = context->getCurrentNode();
        while (command && !command->hasCommand() && command->getForwardParent()) {
            command = command->getForwardParent();
        }
        if (command && command->hasCommand()) {
            try {
                i32 result = command->getCommand()(*context);
                return CommandResult::success(result);
            }
            catch (const CommandException& e) {
                return Error(ErrorCode::Unknown, e.message());
            }
        }
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
