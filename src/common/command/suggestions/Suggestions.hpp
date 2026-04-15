#pragma once

#include "common/core/Types.hpp"
#include <algorithm>
#include <cctype>
#include <future>
#include <string>
#include <utility>
#include <vector>

namespace mc::command {

template<typename S>
class CommandContext;

/**
 * @brief 建议文本
 *
 * 表示一个自动补全建议
 */
class Suggestion {
public:
    Suggestion(i32 start, String text)
        : m_start(start)
        , m_text(std::move(text)) {}

    Suggestion(i32 start, String text, String tooltip)
        : m_start(start)
        , m_text(std::move(text))
        , m_tooltip(std::move(tooltip)) {}

    [[nodiscard]] i32 getStart() const noexcept { return m_start; }
    [[nodiscard]] const String& getText() const noexcept { return m_text; }
    [[nodiscard]] const String& getTooltip() const noexcept { return m_tooltip; }
    [[nodiscard]] bool hasTooltip() const noexcept { return !m_tooltip.empty(); }

    /**
     * @brief 应用建议到输入字符串
     * @param input 原始输入
     * @return 应用建议后的完整字符串
     */
    [[nodiscard]] String apply(StringView input) const {
        String result;
        if (m_start > 0) {
            result += input.substr(0, static_cast<size_t>(m_start));
        }
        result += m_text;
        return result;
    }

    bool operator<(const Suggestion& other) const {
        return m_text < other.m_text;
    }

    bool operator==(const Suggestion& other) const {
        return m_start == other.m_start && m_text == other.m_text;
    }

private:
    i32 m_start;
    String m_text;
    String m_tooltip;
};

/**
 * @brief 建议结果
 *
 * 包含一组自动补全建议
 */
class Suggestions {
public:
    Suggestions() = default;
    explicit Suggestions(std::vector<Suggestion> suggestions)
        : m_suggestions(std::move(suggestions)) {
        sort();
    }

    [[nodiscard]] bool isEmpty() const noexcept { return m_suggestions.empty(); }
    [[nodiscard]] size_t size() const noexcept { return m_suggestions.size(); }
    [[nodiscard]] const std::vector<Suggestion>& getList() const noexcept { return m_suggestions; }

    /**
     * @brief 合并两组建议
     */
    static Suggestions merge(const Suggestions& a, const Suggestions& b) {
        std::vector<Suggestion> merged = a.m_suggestions;
        merged.insert(merged.end(), b.m_suggestions.begin(), b.m_suggestions.end());
        return Suggestions(std::move(merged));
    }

    /**
     * @brief 创建空建议
     */
    static Suggestions empty() {
        return Suggestions();
    }

private:
    void sort() {
        std::sort(m_suggestions.begin(), m_suggestions.end());
    }

    std::vector<Suggestion> m_suggestions;
};

/**
 * @brief 建议构建器
 *
 * 用于增量构建建议列表
 */
class SuggestionsBuilder {
public:
    explicit SuggestionsBuilder(StringView input, i32 start = 0)
        : SuggestionsBuilder(input, start, static_cast<i32>(input.size())) {}

    /**
     * @brief 构建指定范围的建议构建器
     * @param input 原始输入
     * @param start 建议起始位置
     * @param end 建议结束位置（开区间）
     */
    SuggestionsBuilder(StringView input, i32 start, i32 end)
        : m_input(input)
        , m_start(start)
        , m_remaining(input.substr(
              static_cast<size_t>(start),
              static_cast<size_t>(std::max<i32>(0, end - start)))) {}

    /**
     * @brief 添加建议
     */
    SuggestionsBuilder& suggest(const String& text) {
        m_suggestions.emplace_back(m_start, text);
        return *this;
    }

    /**
     * @brief 添加带工具提示的建议
     */
    SuggestionsBuilder& suggest(const String& text, const String& tooltip) {
        m_suggestions.emplace_back(m_start, text, tooltip);
        return *this;
    }

    /**
     * @brief 添加建议（指定起始位置）
     */
    SuggestionsBuilder& suggest(i32 start, const String& text) {
        m_suggestions.emplace_back(start, text);
        return *this;
    }

    /**
     * @brief 添加候选词列表
     */
    template<typename Container>
    SuggestionsBuilder& suggestAll(const Container& candidates) {
        for (const auto& candidate : candidates) {
            if (startsWith(m_remaining, candidate)) {
                suggest(String(candidate));
            }
        }
        return *this;
    }

    /**
     * @brief 构建最终建议
     */
    [[nodiscard]] Suggestions build() const {
        return Suggestions(m_suggestions);
    }

    /**
     * @brief 构建一个已就绪的异步结果
     */
    [[nodiscard]] std::future<Suggestions> buildFuture() const {
        std::promise<Suggestions> promise;
        promise.set_value(build());
        return promise.get_future();
    }

    [[nodiscard]] StringView getInput() const noexcept { return m_input; }
    [[nodiscard]] StringView getRemaining() const noexcept { return m_remaining; }
    [[nodiscard]] i32 getStart() const noexcept { return m_start; }

    /**
     * @brief 创建子构建器
     */
    [[nodiscard]] SuggestionsBuilder createOffset(i32 offset) const {
        return SuggestionsBuilder(m_input, m_start + offset);
    }

private:
    static bool startsWith(StringView str, StringView prefix) {
        if (prefix.length() > str.length()) return false;
        for (size_t i = 0; i < prefix.length(); ++i) {
            const unsigned char left = static_cast<unsigned char>(str[i]);
            const unsigned char right = static_cast<unsigned char>(prefix[i]);
            if (std::tolower(left) != std::tolower(right)) {
                return false;
            }
        }
        return true;
    }

    StringView m_input;
    i32 m_start;
    StringView m_remaining;
    std::vector<Suggestion> m_suggestions;
};

/**
 * @brief 建议提供者接口
 */
template<typename S>
class ISuggestionProvider {
public:
    virtual ~ISuggestionProvider() = default;

    /**
     * @brief 提供自动补全建议
     * @param context 命令上下文
     * @param builder 建议构建器
     * @return 建议结果
     */
    virtual std::future<Suggestions> getSuggestions(
        CommandContext<S>& context,
        SuggestionsBuilder& builder
    ) = 0;
};

// ========== 预定义建议提供者 ==========

/**
 * @brief 候选词建议提供者
 */
template<typename S>
class CandidateSuggestionProvider : public ISuggestionProvider<S> {
public:
    explicit CandidateSuggestionProvider(std::vector<String> candidates)
        : m_candidates(std::move(candidates)) {}

    std::future<Suggestions> getSuggestions(
        CommandContext<S>& /*context*/,
        SuggestionsBuilder& builder
    ) override {
        builder.suggestAll(m_candidates);
        return builder.buildFuture();
    }

private:
    std::vector<String> m_candidates;
};

} // namespace mc::command
