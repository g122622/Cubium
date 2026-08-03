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

#include "TemplateConfig.hpp"
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::tpl::core {

/**
 * @brief 模板错误类型枚举
 */
enum class TemplateErrorType : u8 {
    // 词法错误
    LexerError = 1,
    UnexpectedCharacter,
    UnterminatedString,
    InvalidEscapeSequence,

    // 语法错误
    ParserError = 10,
    UnexpectedToken,
    UnexpectedEndOfInput,
    InvalidTagName,
    InvalidAttributeName,
    MissingClosingTag,
    MismatchedClosingTag,

    // 语义错误
    SemanticError = 20,
    UnknownTag,
    UnknownAttribute,
    DuplicateId,
    InvalidBindingPath,
    InvalidCallbackName,
    InlineScriptNotAllowed,
    InlineExpressionNotAllowed,
    DynamicTagNameNotAllowed,

    // 编译错误
    CompileError = 30,
    WidgetNotFound,
    EventNotFound,
    BindingFailed,
    InvalidTemplate,

    // 运行时错误
    RuntimeError = 40,
    InstantiationFailed,
    UpdateFailed,
    InvalidOperation
};

/**
 * @brief 模板错误信息
 */
struct TemplateErrorInfo {
    TemplateErrorType type;
    std::string message;
    SourceLocation location;
    std::string sourcePath;
    std::string context; ///< 错误上下文（周围代码）

    TemplateErrorInfo(TemplateErrorType type_,
        std::string message_,
        SourceLocation location_ = SourceLocation(),
        std::string sourcePath_ = "")
        : type(type_)
        , message(std::move(message_))
        , location(location_)
        , sourcePath(std::move(sourcePath_))
    {}

    /**
     * @brief 获取错误类型名称
     */
    [[nodiscard]] const char* typeName() const
    {
        switch (type) {
            case TemplateErrorType::LexerError:
            case TemplateErrorType::UnexpectedCharacter:
            case TemplateErrorType::UnterminatedString:
            case TemplateErrorType::InvalidEscapeSequence:
                return "LexerError";

            case TemplateErrorType::ParserError:
            case TemplateErrorType::UnexpectedToken:
            case TemplateErrorType::UnexpectedEndOfInput:
            case TemplateErrorType::InvalidTagName:
            case TemplateErrorType::InvalidAttributeName:
            case TemplateErrorType::MissingClosingTag:
            case TemplateErrorType::MismatchedClosingTag:
                return "ParserError";

            case TemplateErrorType::SemanticError:
            case TemplateErrorType::UnknownTag:
            case TemplateErrorType::UnknownAttribute:
            case TemplateErrorType::DuplicateId:
            case TemplateErrorType::InvalidBindingPath:
            case TemplateErrorType::InvalidCallbackName:
            case TemplateErrorType::InlineScriptNotAllowed:
            case TemplateErrorType::InlineExpressionNotAllowed:
            case TemplateErrorType::DynamicTagNameNotAllowed:
                return "SemanticError";

            case TemplateErrorType::CompileError:
            case TemplateErrorType::WidgetNotFound:
            case TemplateErrorType::EventNotFound:
            case TemplateErrorType::BindingFailed:
            case TemplateErrorType::InvalidTemplate:
                return "CompileError";

            case TemplateErrorType::RuntimeError:
            case TemplateErrorType::InstantiationFailed:
            case TemplateErrorType::UpdateFailed:
            case TemplateErrorType::InvalidOperation:
                return "RuntimeError";

            default:
                return "UnknownError";
        }
    }

    /**
     * @brief 格式化为完整错误消息
     */
    [[nodiscard]] std::string format() const
    {
        std::string result;
        result += typeName();
        result += ": ";
        result += message;

        if (location.isValid()) {
            result += " at ";
            result += location.toString();
        }

        if (!sourcePath.empty()) {
            result += " in ";
            result += sourcePath;
        }

        return result;
    }
};

/**
 * @brief 模板错误异常
 *
 * 模板解析、编译、运行时抛出的异常类型
 */
class TemplateError : public std::runtime_error {
public:
    explicit TemplateError(TemplateErrorInfo info)
        : std::runtime_error(info.format())
        , m_info(std::move(info))
    {}

    TemplateError(TemplateErrorType type,
        const std::string& message,
        SourceLocation location = SourceLocation(),
        const std::string& sourcePath = "")
        : TemplateError(TemplateErrorInfo(type, message, location, sourcePath))
    {}

    /**
     * @brief 获取错误信息
     */
    [[nodiscard]] const TemplateErrorInfo& info() const { return m_info; }

    /**
     * @brief 获取错误类型
     */
    [[nodiscard]] TemplateErrorType type() const { return m_info.type; }

    /**
     * @brief 获取错误位置
     */
    [[nodiscard]] const SourceLocation& location() const { return m_info.location; }

    /**
     * @brief 添加上下文信息
     */
    void setContext(const std::string& context) { m_info.context = context; }

    // ========== 工厂方法 ==========

    /**
     * @brief 创建词法错误
     */
    static TemplateError lexer(const std::string& message, SourceLocation loc = SourceLocation())
    {
        return TemplateError(TemplateErrorType::LexerError, message, loc);
    }

    /**
     * @brief 创建语法错误
     */
    static TemplateError parser(const std::string& message, SourceLocation loc = SourceLocation())
    {
        return TemplateError(TemplateErrorType::ParserError, message, loc);
    }

    /**
     * @brief 创建语义错误
     */
    static TemplateError semantic(const std::string& message, SourceLocation loc = SourceLocation())
    {
        return TemplateError(TemplateErrorType::SemanticError, message, loc);
    }

    /**
     * @brief 创建编译错误
     */
    static TemplateError compile(const std::string& message, SourceLocation loc = SourceLocation())
    {
        return TemplateError(TemplateErrorType::CompileError, message, loc);
    }

    /**
     * @brief 创建运行时错误
     */
    static TemplateError runtime(const std::string& message)
    {
        return TemplateError(TemplateErrorType::RuntimeError, message);
    }

    /**
     * @brief 创建"未知标签"错误
     */
    static TemplateError unknownTag(const std::string& tagName, SourceLocation loc = SourceLocation())
    {
        return TemplateError(TemplateErrorType::UnknownTag, "Unknown tag: <" + tagName + ">", loc);
    }

    /**
     * @brief 创建"无效绑定路径"错误
     */
    static TemplateError invalidBindingPath(const std::string& path, SourceLocation loc = SourceLocation())
    {
        return TemplateError(TemplateErrorType::InvalidBindingPath, "Invalid binding path: '" + path + "'", loc);
    }

    /**
     * @brief 创建"内联脚本不允许"错误
     */
    static TemplateError inlineScriptNotAllowed(SourceLocation loc = SourceLocation())
    {
        return TemplateError(TemplateErrorType::InlineScriptNotAllowed,
            "Inline scripts/expressions are not allowed in Kagero templates (strict mode)",
            loc);
    }

private:
    TemplateErrorInfo m_info;
};

/**
 * @brief 模板错误收集器
 *
 * 用于收集多个错误（不立即抛出）
 */
class TemplateErrorCollector {
public:
    /**
     * @brief 添加错误
     */
    void addError(TemplateErrorInfo error) { m_errors.push_back(std::move(error)); }

    /**
     * @brief 添加错误（简化版）
     */
    void addError(TemplateErrorType type, const std::string& message, SourceLocation location = SourceLocation())
    {
        m_errors.emplace_back(type, message, location);
    }

    /**
     * @brief 检查是否有错误
     */
    [[nodiscard]] bool hasErrors() const { return !m_errors.empty(); }

    /**
     * @brief 获取错误数量
     */
    [[nodiscard]] Size errorCount() const { return m_errors.size(); }

    /**
     * @brief 获取所有错误
     */
    [[nodiscard]] const std::vector<TemplateErrorInfo>& errors() const { return m_errors; }

    /**
     * @brief 获取第一个错误
     */
    [[nodiscard]] const TemplateErrorInfo* firstError() const { return m_errors.empty() ? nullptr : &m_errors.front(); }

    /**
     * @brief 清除所有错误
     */
    void clear() { m_errors.clear(); }

    /**
     * @brief 如果有错误则抛出第一个错误
     */
    void throwIfErrors() const
    {
        if (hasErrors()) {
            throw TemplateError(m_errors.front());
        }
    }

    /**
     * @brief 合并另一个收集器的错误
     */
    void merge(const TemplateErrorCollector& other)
    {
        for (const auto& error : other.m_errors) {
            m_errors.push_back(error);
        }
    }

private:
    std::vector<TemplateErrorInfo> m_errors;
};

} // namespace mc::client::ui::kagero::tpl::core
