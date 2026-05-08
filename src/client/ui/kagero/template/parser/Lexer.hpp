#pragma once

#include "../core/TemplateConfig.hpp"
#include "../core/TemplateError.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace mc::client::ui::kagero::tpl::parser {

// 引入core命名空间的类型
using core::TemplateConfig;
using core::TemplateError;
using core::TemplateErrorType;
using core::TemplateErrorInfo;
using core::TemplateErrorCollector;
using core::SourceLocation;
using core::SourceRange;

/**
 * @brief Token类型枚举
 *
 * 定义模板语言中所有可能的token类型
 */
enum class TokenType : u8 {
    // 结构Token
    OpenTag,            ///< < 开始标签
    CloseTag,           ///< > 结束标签
    OpenCloseTag,       ///< </ 关闭标签开始
    SelfCloseTag,       ///< /> 自关闭标签
    OpenComment,        ///< <!-- 注释开始
    CloseComment,       ///< --> 注释结束

    // 文本和标识符
    Text,               ///< 文本内容
    Identifier,         ///< 标识符（标签名、属性名）
    StringLiteral,      ///< 字符串字面量（引号包围）
    NumberLiteral,      ///< 数字字面量

    // 属性相关
    Equals,             ///< = 等号
    Colon,              ///< : 冒号（用于bind:, on:）

    // 特殊
    Whitespace,         ///< 空白字符
    Newline,            ///< 换行符
    Comment,            ///< 注释内容
    EndOfFile,          ///< 文件结束

    // 错误
    Error               ///< 错误Token
};

/**
 * @brief 获取Token类型名称
 */
[[nodiscard]] const char* tokenTypeName(TokenType type);

/**
 * @brief Token结构
 */
struct Token {
    TokenType type;              ///< Token类型
    std::string value;                ///< Token值
    SourceLocation location;     ///< 源码位置

    Token(TokenType t = TokenType::Error, std::string v = "", SourceLocation loc = SourceLocation())
        : type(t), value(std::move(v)), location(loc) {}

    /**
     * @brief 检查是否是特定类型的Token
     */
    [[nodiscard]] bool is(TokenType t) const { return type == t; }

    /**
     * @brief 检查是否是特定类型和值的Token
     */
    [[nodiscard]] bool is(TokenType t, const std::string& v) const {
        return type == t && value == v;
    }

    /**
     * @brief 检查是否是标识符
     */
    [[nodiscard]] bool isIdentifier() const { return type == TokenType::Identifier; }

    /**
     * @brief 检查是否是文本
     */
    [[nodiscard]] bool isText() const { return type == TokenType::Text; }

    /**
     * @brief 检查是否是字符串字面量
     */
    [[nodiscard]] bool isStringLiteral() const { return type == TokenType::StringLiteral; }

    /**
     * @brief 检查是否是数字字面量
     */
    [[nodiscard]] bool isNumberLiteral() const { return type == TokenType::NumberLiteral; }

    /**
     * @brief 检查是否是关键字
     */
    [[nodiscard]] bool isKeyword(const std::string& keyword) const {
        return type == TokenType::Identifier && value == keyword;
    }

    /**
     * @brief 格式化为字符串（用于调试）
     */
    [[nodiscard]] std::string format() const;
};

/**
 * @brief 词法分析器
 *
 * 将模板源码转换为Token流。
 * 这是一个手写的零依赖词法分析器。
 *
 * 使用示例：
 * @code
 * Lexer lexer(templateSource);
 * lexer.tokenize();
 *
 * while (lexer.hasNext()) {
 *     Token token = lexer.next();
 *     // 处理token
 * }
 * @endcode
 */
class Lexer {
public:
    /**
     * @brief 构造函数
     * @param source 模板源码
     * @param sourcePath 源文件路径（用于错误报告）
     */
    explicit Lexer(std::string source, std::string sourcePath = "");

    /**
     * @brief 执行词法分析
     *
     * 将源码转换为Token流
     *
     * @return 是否成功（无错误）
     */
    bool tokenize();

    /**
     * @brief 获取所有Token
     */
    [[nodiscard]] const std::vector<Token>& tokens() const { return m_tokens; }

    /**
     * @brief 获取所有Token（可变）
     */
    [[nodiscard]] std::vector<Token>& tokens() { return m_tokens; }

    /**
     * @brief 获取错误信息
     */
    [[nodiscard]] const std::vector<TemplateErrorInfo>& errors() const { return m_errors; }

    /**
     * @brief 检查是否有错误
     */
    [[nodiscard]] bool hasErrors() const { return !m_errors.empty(); }

    /**
     * @brief 获取第一个错误
     */
    [[nodiscard]] const TemplateErrorInfo* firstError() const {
        return m_errors.empty() ? nullptr : &m_errors.front();
    }

    // ========== 迭代器接口 ==========

    /**
     * @brief 检查是否还有下一个Token
     */
    [[nodiscard]] bool hasNext() const {
        return m_currentIndex < m_tokens.size();
    }

    /**
     * @brief 获取当前Token
     */
    [[nodiscard]] const Token& current() const {
        static const Token errorToken(TokenType::Error);
        return m_currentIndex < m_tokens.size() ? m_tokens[m_currentIndex] : errorToken;
    }

    /**
     * @brief 获取下一个Token并前进
     */
    const Token& next();

    /**
     * @brief 预览下一个Token（不前进）
     */
    [[nodiscard]] const Token& peek() const;

    /**
     * @brief 预览下N个Token（不前进）
     */
    [[nodiscard]] const Token& peek(size_t offset) const;

    /**
     * @brief 回退到上一个Token
     */
    void back();

    /**
     * @brief 获取当前位置
     */
    [[nodiscard]] size_t position() const { return m_currentIndex; }

    /**
     * @brief 设置当前位置
     */
    void setPosition(size_t pos) { m_currentIndex = pos; }

    /**
     * @brief 跳过空白Token
     */
    void skipWhitespace();

    /**
     * * @brief 跳过空白和换行Token
     */
    void skipWhitespaceAndNewlines();

    /**
     * @brief 期望特定类型的Token
     *
     * 如果下一个Token不是期望的类型，添加错误
     *
     * @param type 期望的Token类型
     * @return 是否匹配
     */
    bool expect(TokenType type);

    /**
     * @brief 期望特定类型和值的Token
     */
    bool expect(TokenType type, const std::string& value);

    // ========== 源码访问 ==========

    /**
     * @brief 获取源码
     */
    [[nodiscard]] const std::string& source() const { return m_source; }

    /**
     * @brief 获取源文件路径
     */
    [[nodiscard]] const std::string& sourcePath() const { return m_sourcePath; }

    /**
     * @brief 获取指定位置的行内容（用于错误上下文）
     */
    [[nodiscard]] std::string getLineContent(size_t line) const;

    /**
     * @brief 获取指定位置的上下文（周围几行）
     */
    [[nodiscard]] std::string getContext(const SourceLocation& loc, size_t contextLines = 2) const;

    // ========== 静态工具方法 ==========

    /**
     * @brief 检查字符是否是空白字符
     */
    [[nodiscard]] static bool isWhitespace(char c);

    /**
     * @brief 检查字符是否是换行符
     */
    [[nodiscard]] static bool isNewline(char c);

    /**
     * @brief 检查字符是否是字母
     */
    [[nodiscard]] static bool isAlpha(char c);

    /**
     * @brief 检查字符是否是数字
     */
    [[nodiscard]] static bool isDigit(char c);

    /**
     * @brief 检查字符是否是字母或数字
     */
    [[nodiscard]] static bool isAlphaNumeric(char c);

    /**
     * @brief 检查字符是否是有效的标识符字符
     */
    [[nodiscard]] static bool isIdentifierChar(char c);

private:
    /**
     * @brief 扫描下一个Token
     * @return 扫描到的Token，如果是EOF返回EndOfFile
     */
    Token scanToken();

    /**
     * @brief 扫描标签开始（<）
     */
    Token scanTagStart();

    /**
     * @brief 扫描标签结束（> 或 />）
     */
    Token scanTagEnd();

    /**
     * @brief 扫描注释（<!-- ... -->）
     */
    Token scanComment();

    /**
     * @brief 扫描标识符
     */
    Token scanIdentifier();

    /**
     * @brief 扫描字符串字面量
     */
    Token scanStringLiteral();

    /**
     * @brief 扫描数字字面量
     */
    Token scanNumberLiteral();

    /**
     * @brief 扫描文本内容（标签之间的文本）
     */
    Token scanText();

    /**
     * @brief 跳过空白字符
     */
    void skipWhitespaceChars();

    // ========== 辅助方法 ==========

    /**
     * @brief 获取当前字符
     */
    [[nodiscard]] char currentChar() const;

    /**
     * @brief 获取下一个字符（不前进）
     */
    [[nodiscard]] char peekChar() const;

    /**
     * * @brief 获取下N个字符（不前进）
     */
    [[nodiscard]] char peekChar(size_t offset) const;

    /**
     * @brief 前进一个字符
     */
    void advance();

    /**
     * @brief 前进N个字符
     */
    void advance(size_t n);

    /**
     * @brief 检查是否到达文件末尾
     */
    [[nodiscard]] bool isAtEnd() const;

    /**
     * @brief 添加错误
     */
    void addError(TemplateErrorType type, const std::string& message);

    /**
     * @brief 更新位置
     */
    void updatePosition(char c);

    /**
     * @brief 创建Token
     */
    [[nodiscard]] Token makeToken(TokenType type, const std::string& value) const;

private:
    std::string m_source;                        ///< 源码
    std::string m_sourcePath;                    ///< 源文件路径

    size_t m_pos = 0;                       ///< 当前位置（字符偏移）
    SourceLocation m_location;              ///< 当前位置（行列）

    std::vector<Token> m_tokens;            ///< Token列表
    std::vector<TemplateErrorInfo> m_errors;///< 错误列表

    size_t m_currentIndex = 0;              ///< 当前Token索引（用于迭代）

    // 特殊状态
    bool m_inTag = false;                   ///< 是否在标签内部
    bool m_inAttribute = false;             ///< 是否在属性值内部
};

} // namespace mc::client::ui::kagero::tpl::parser
