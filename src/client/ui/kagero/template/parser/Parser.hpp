#pragma once

#include "../core/TemplateConfig.hpp"
#include "../core/TemplateError.hpp"
#include "Ast.hpp"
#include "Lexer.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_set>

namespace mc::client::ui::kagero::tpl::parser {

// 引入core命名空间的类型
using core::SourceLocation;
using core::TemplateConfig;
using core::TemplateError;
using core::TemplateErrorCollector;
using core::TemplateErrorInfo;
using core::TemplateErrorType;

// 引入ast命名空间的类型
using ast::Attribute;
using ast::CommentNode;
using ast::ConditionInfo;
using ast::DocumentNode;
using ast::ElementNode;
using ast::LoopInfo;
using ast::NodeType;
using ast::TextNode;

/**
 * @brief 语法分析器
 *
 * 将Token流解析为AST（抽象语法树）。
 * 使用递归下降解析算法。
 *
 * 支持的语法：
 * - XML样式标签: <tag>, </tag>, <tag/>
 * - 静态属性: attr="value"
 * - 绑定属性: bind:attr="path"
 * - 事件属性: on:event="callback"
 * - 循环指令: bind:items="collection"
 * - 条件指令: bind:visible="booleanPath"
 *
 * 使用示例：
 * @code
 * Lexer lexer(source);
 * lexer.tokenize();
 *
 * Parser parser(lexer.tokens());
 * auto ast = parser.parse();
 *
 * if (parser.hasErrors()) {
 *     // 处理错误
 * }
 * @endcode
 */
class Parser {
public:
    /**
     * @brief 构造函数
     * @param tokens Token列表
     * @param config 解析配置
     */
    explicit Parser(const std::vector<Token>& tokens, const TemplateConfig& config = TemplateConfig::defaults());

    /**
     * @brief 从Lexer构造
     * @param lexer 词法分析器
     * @param config 解析配置
     */
    explicit Parser(const Lexer& lexer, const TemplateConfig& config = TemplateConfig::defaults());

    /**
     * @brief 解析模板
     *
     * 将Token流解析为AST文档节点
     *
     * @return 文档节点，如果解析失败返回nullptr
     */
    [[nodiscard]] std::unique_ptr<DocumentNode> parse();

    /**
     * @brief 检查是否有错误
     */
    [[nodiscard]] bool hasErrors() const { return !m_errors.empty(); }

    /**
     * @brief 获取错误列表
     */
    [[nodiscard]] const std::vector<TemplateErrorInfo>& errors() const { return m_errors; }

    /**
     * @brief 获取第一个错误
     */
    [[nodiscard]] const TemplateErrorInfo* firstError() const { return m_errors.empty() ? nullptr : &m_errors.front(); }

    /**
     * @brief 获取当前Token
     */
    [[nodiscard]] const Token& current() const;

    /**
     * @brief 获取下一个Token（不前进）
     */
    [[nodiscard]] const Token& peek() const;

    /**
     * @brief 消费当前Token并前进
     */
    Token consume();

    /**
     * @brief 检查当前Token类型
     */
    [[nodiscard]] bool check(TokenType type) const;

    /**
     * @brief 检查当前Token类型和值
     */
    [[nodiscard]] bool check(TokenType type, const std::string& value) const;

    /**
     * @brief 匹配并消费Token
     *
     * 如果当前Token匹配，消费它并返回true
     */
    bool match(TokenType type);

    /**
     * @brief 匹配并消费Token（带值）
     */
    bool match(TokenType type, const std::string& value);

    /**
     * @brief 期望特定Token
     *
     * 如果当前Token不匹配，添加错误
     */
    bool expect(TokenType type);

    /**
     * @brief 期望特定Token（带值）
     */
    bool expect(TokenType type, const std::string& value);

    /**
     * @brief 期望标识符
     */
    [[nodiscard]] Token expectIdentifier(const std::string& context = "");

    /**
     * @brief 期望字符串字面量
     */
    [[nodiscard]] Token expectStringLiteral(const std::string& context = "");

    /**
     * @brief 跳过空白和换行Token
     */
    void skipWhitespaceAndNewlines();

    /**
     * @brief 同步到下一个有效位置（错误恢复）
     */
    void synchronize();

private:
    // ========== 解析方法 ==========

    /**
     * @brief 解析文档
     */
    [[nodiscard]] std::unique_ptr<DocumentNode> parseDocument();

    /**
     * @brief 解析元素
     */
    [[nodiscard]] std::unique_ptr<ElementNode> parseElement();

    /**
     * @brief 解析开始标签
     * @param isOpenTag 是否是开放标签 (<...>)
     * @return 标签名，如果解析失败返回空字符串
     */
    [[nodiscard]] std::string parseStartTag(bool isOpenTag);

    /**
     * @brief 解析属性列表
     * @param element 目标元素节点
     */
    void parseAttributes(ElementNode& element);

    /**
     * @brief 解析单个属性
     * @return 属性对象
     */
    [[nodiscard]] Attribute parseAttribute();

    /**
     * @brief 解析属性名（可能包含 bind: 或 on: 前缀）
     * @return 属性名Token
     */
    [[nodiscard]] Token parseAttributeName();

    /**
     * @brief 解析属性值
     * @return 属性值Token
     */
    [[nodiscard]] Token parseAttributeValue();

    /**
     * @brief 解析元素内容（子节点）
     * @param parent 父元素
     */
    void parseContent(ElementNode& parent);

    /**
     * @brief 解析文本内容
     */
    [[nodiscard]] std::unique_ptr<TextNode> parseText();

    /**
     * @brief 解析注释
     */
    [[nodiscard]] std::unique_ptr<CommentNode> parseComment();

    /**
     * @brief 解析结束标签
     * @param expectedTagName 期望的标签名
     * @return 是否匹配
     */
    bool parseEndTag(const std::string& expectedTagName);

    // ========== 语义验证 ==========

    /**
     * @brief 验证元素
     */
    void validateElement(ElementNode& element);

    /**
     * @brief 验证属性
     */
    void validateAttribute(const Attribute& attr, const ElementNode& element);

    /**
     * @brief 验证绑定路径
     */
    void validateBindingPath(const std::string& path, const SourceLocation& loc);

    /**
     * @brief 验证回调名称
     */
    void validateCallbackName(const std::string& name, const SourceLocation& loc);

    /**
     * @brief 检查是否允许的内联表达式
     */
    [[nodiscard]] bool isInlineExpressionAllowed() const;

    /**
     * @brief 提取循环指令信息
     *
     * 从bind:items属性中提取循环信息
     */
    [[nodiscard]] std::optional<LoopInfo> extractLoopInfo(const ElementNode& element);

    /**
     * * @brief 提取条件指令信息
     *
     * 从bind:visible属性中提取条件信息
     */
    [[nodiscard]] std::optional<ConditionInfo> extractConditionInfo(const ElementNode& element);

    // ========== 辅助方法 ==========

    /**
     * @brief 添加错误
     */
    void addError(TemplateErrorType type, const std::string& message, const SourceLocation& loc = SourceLocation());

    /**
     * @brief 添加错误（带上下文）
     */
    void addError(TemplateErrorType type, const std::string& message, const Token& token);

    /**
     * @brief 检查是否到达文件末尾
     */
    [[nodiscard]] bool isAtEnd() const;

    /**
     * @brief 获取当前位置
     */
    [[nodiscard]] SourceLocation currentLocation() const;

    /**
     * @brief 检查标签名是否有效
     */
    [[nodiscard]] bool isValidTagName(const std::string& name) const;

private:
    const std::vector<Token>& m_tokens;
    TemplateConfig m_config;
    size_t m_current = 0;
    std::vector<TemplateErrorInfo> m_errors;

    /**
     * @brief 已收集的ID集合（用于检测重复ID）
     *
     * 在parse()过程中收集所有元素的ID，用于validateElement()中的唯一性检查
     */
    std::unordered_set<std::string> m_seenIds;
};

} // namespace mc::client::ui::kagero::tpl::parser
