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

#include "Lexer.hpp"
#include <cstddef>
#include <sstream>
#include <string>
#include <utility>

namespace mc::client::ui::kagero::tpl::parser {

// ========== TokenType名称 ==========

const char* tokenTypeName(TokenType type)
{
    switch (type) {
        case TokenType::OpenTag:
            return "OpenTag";
        case TokenType::CloseTag:
            return "CloseTag";
        case TokenType::OpenCloseTag:
            return "OpenCloseTag";
        case TokenType::SelfCloseTag:
            return "SelfCloseTag";
        case TokenType::OpenComment:
            return "OpenComment";
        case TokenType::CloseComment:
            return "CloseComment";
        case TokenType::Text:
            return "Text";
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::StringLiteral:
            return "StringLiteral";
        case TokenType::NumberLiteral:
            return "NumberLiteral";
        case TokenType::Equals:
            return "Equals";
        case TokenType::Colon:
            return "Colon";
        case TokenType::Whitespace:
            return "Whitespace";
        case TokenType::Newline:
            return "Newline";
        case TokenType::Comment:
            return "Comment";
        case TokenType::EndOfFile:
            return "EndOfFile";
        case TokenType::Error:
            return "Error";
        default:
            return "Unknown";
    }
}

// ========== Token ==========

std::string Token::format() const
{
    std::ostringstream oss;
    oss << tokenTypeName(type);
    if (!value.empty()) {
        oss << "(" << value << ")";
    }
    oss << " @ " << location.toString();
    return oss.str();
}

// ========== Lexer ==========

Lexer::Lexer(std::string source, std::string sourcePath)
    : m_source(std::move(source))
    , m_sourcePath(std::move(sourcePath))
    , m_location(1, 1, 0)
{}

bool Lexer::tokenize()
{
    m_tokens.clear();
    m_errors.clear();
    m_pos = 0;
    m_location = SourceLocation(1, 1, 0);
    m_currentIndex = 0;
    m_inTag = false;
    m_inAttribute = false;

    while (!_isAtEnd()) {
        Token token = _scanToken();
        if (token.type != TokenType::Error) {
            m_tokens.push_back(token);
        }

        if (token.type == TokenType::EndOfFile) {
            break;
        }
    }

    // 添加EOF Token
    if (m_tokens.empty() || m_tokens.back().type != TokenType::EndOfFile) {
        m_tokens.push_back(_makeToken(TokenType::EndOfFile, ""));
    }

    return !hasErrors();
}

const Token& Lexer::next()
{
    if (m_currentIndex < m_tokens.size()) {
        return m_tokens[m_currentIndex++];
    }
    static const Token eofToken(TokenType::EndOfFile);
    return eofToken;
}

const Token& Lexer::peek() const
{
    if (m_currentIndex + 1 < m_tokens.size()) {
        return m_tokens[m_currentIndex + 1];
    }
    static const Token eofToken(TokenType::EndOfFile);
    return eofToken;
}

const Token& Lexer::peek(size_t offset) const
{
    if (m_currentIndex + offset < m_tokens.size()) {
        return m_tokens[m_currentIndex + offset];
    }
    static const Token eofToken(TokenType::EndOfFile);
    return eofToken;
}

void Lexer::back()
{
    if (m_currentIndex > 0) {
        --m_currentIndex;
    }
}

void Lexer::skipWhitespace()
{
    while (hasNext() && (current().type == TokenType::Whitespace)) {
        next();
    }
}

void Lexer::skipWhitespaceAndNewlines()
{
    while (hasNext() && (current().type == TokenType::Whitespace || current().type == TokenType::Newline)) {
        next();
    }
}

bool Lexer::expect(TokenType type)
{
    if (!hasNext()) {
        _addError(TemplateErrorType::UnexpectedEndOfInput,
            "Expected " + std::string(tokenTypeName(type)) + " but reached end of file");
        return false;
    }

    if (current().type != type) {
        _addError(TemplateErrorType::UnexpectedToken,
            "Expected " + std::string(tokenTypeName(type)) + " but got " + std::string(tokenTypeName(current().type)));
        return false;
    }

    next();
    return true;
}

bool Lexer::expect(TokenType type, const std::string& value)
{
    if (!hasNext()) {
        _addError(TemplateErrorType::UnexpectedEndOfInput, "Expected " + value + " but reached end of file");
        return false;
    }

    if (!current().is(type, value)) {
        _addError(TemplateErrorType::UnexpectedToken, "Expected '" + value + "' but got '" + current().value + "'");
        return false;
    }

    next();
    return true;
}

std::string Lexer::getLineContent(size_t line) const
{
    if (line == 0) return "";

    size_t currentLine = 1;
    size_t lineStart = 0;

    // 找到目标行的起始位置
    for (size_t i = 0; i < m_source.size() && currentLine < line; ++i) {
        if (m_source[i] == '\n') {
            ++currentLine;
            lineStart = i + 1;
        }
    }

    // 找到行结束位置
    size_t lineEnd = m_source.find('\n', lineStart);
    if (lineEnd == std::string::npos) {
        lineEnd = m_source.size();
    }

    return m_source.substr(lineStart, lineEnd - lineStart);
}

std::string Lexer::getContext(const SourceLocation& loc, size_t contextLines) const
{
    std::ostringstream oss;

    size_t startLine = loc.line > contextLines ? loc.line - contextLines : 1;
    size_t endLine = loc.line + contextLines;

    for (size_t line = startLine; line <= endLine; ++line) {
        std::string content = getLineContent(line);
        oss << line << ": " << content << "\n";

        // 添加错误位置指示器
        if (line == loc.line) {
            oss << std::string(std::to_string(line).size() + 2, ' ');
            oss << std::string(loc.column - 1, ' ') << "^\n";
        }
    }

    return oss.str();
}

Token Lexer::_scanToken()
{
    _skipWhitespaceChars();

    if (_isAtEnd()) {
        return _makeToken(TokenType::EndOfFile, "");
    }

    char c = _currentChar();

    // 标签相关
    if (c == '<') {
        return _scanTagStart();
    }

    if (c == '>') {
        _advance();
        m_inTag = false;
        return _makeToken(TokenType::CloseTag, ">");
    }

    if (c == '/') {
        if (_peekChar() == '>') {
            _advance();
            _advance();
            m_inTag = false;
            return _makeToken(TokenType::SelfCloseTag, "/>");
        }
        // 否则是文本的一部分
        return _scanText();
    }

    // 属性相关
    if (m_inTag) {
        if (c == '=') {
            _advance();
            return _makeToken(TokenType::Equals, "=");
        }

        if (c == ':') {
            _advance();
            return _makeToken(TokenType::Colon, ":");
        }

        if (c == '"' || c == '\'') {
            return _scanStringLiteral();
        }

        if (isDigit(c) || (c == '-' && isDigit(_peekChar()))) {
            return _scanNumberLiteral();
        }

        if (isAlpha(c) || c == '_' || c == '$') {
            return _scanIdentifier();
        }

        // 未知字符在标签内
        _addError(TemplateErrorType::UnexpectedCharacter, "Unexpected character '" + std::string(1, c) + "' in tag");
        _advance();
        return _makeToken(TokenType::Error, std::string(1, c));
    }

    // 文本内容
    return _scanText();
}

Token Lexer::_scanTagStart()
{
    _advance(); // 跳过 '<'

    if (_currentChar() == '!') {
        _advance();
        if (_currentChar() == '-' && _peekChar() == '-') {
            // 注释 <!--
            _advance();
            _advance();
            return _scanComment();
        }
        _addError(TemplateErrorType::UnexpectedCharacter, "Expected '<!--' for comment");
        return _makeToken(TokenType::Error, "<!");
    }

    if (_currentChar() == '/') {
        _advance();
        m_inTag = true;
        return _makeToken(TokenType::OpenCloseTag, "</");
    }

    m_inTag = true;
    return _makeToken(TokenType::OpenTag, "<");
}

Token Lexer::_scanComment()
{
    // 已经扫描了 "<!--"
    size_t start = m_pos;
    size_t contentStart = m_pos;

    while (!_isAtEnd()) {
        // 检查注释结束
        if (_currentChar() == '-' && _peekChar() == '-' && _peekChar(2) == '>') {
            std::string content = m_source.substr(contentStart, m_pos - contentStart);

            _advance(); // '-'
            _advance(); // '-'
            _advance(); // '>'

            // 创建注释内容Token
            Token commentToken(TokenType::Comment, content);
            commentToken.location = m_location;
            m_tokens.push_back(commentToken);

            return _makeToken(TokenType::CloseComment, "-->");
        }
        _advance();
    }

    _addError(TemplateErrorType::UnterminatedString, "Unterminated comment");
    return _makeToken(TokenType::Error, m_source.substr(start));
}

Token Lexer::_scanIdentifier()
{
    size_t start = m_pos;
    SourceLocation startLoc = m_location;

    // 标识符可以包含: 字母、数字、下划线、连字符
    // 冒号（用于bind:, on:）作为独立的Colon token，由_scanToken处理
    // 第一个字符必须是字母或下划线
    while (!_isAtEnd() && isIdentifierChar(_currentChar())) {
        _advance();
    }

    std::string value = m_source.substr(start, m_pos - start);
    Token token(TokenType::Identifier, value);
    token.location = startLoc;
    return token;
}

Token Lexer::_scanStringLiteral()
{
    char quote = _currentChar();
    _advance(); // 跳过开始引号

    size_t start = m_pos;
    std::string value;

    while (!_isAtEnd() && _currentChar() != quote) {
        if (_currentChar() == '\\') {
            _advance(); // 跳过转义字符
            if (!_isAtEnd()) {
                char escaped = _currentChar();
                switch (escaped) {
                    case 'n':
                        value += '\n';
                        break;
                    case 't':
                        value += '\t';
                        break;
                    case 'r':
                        value += '\r';
                        break;
                    case '\\':
                        value += '\\';
                        break;
                    case '"':
                        value += '"';
                        break;
                    case '\'':
                        value += '\'';
                        break;
                    default:
                        value += escaped;
                        break;
                }
                _advance();
            }
        } else {
            value += _currentChar();
            _advance();
        }
    }

    if (_isAtEnd()) {
        _addError(TemplateErrorType::UnterminatedString, "Unterminated string literal");
        Token token(TokenType::Error, value);
        token.location = SourceLocation(m_location.line, m_location.column - value.size() - 1);
        return token;
    }

    _advance(); // 跳过结束引号

    Token token(TokenType::StringLiteral, value);
    token.location = SourceLocation(m_location.line, m_location.column - value.size() - 2);
    return token;
}

Token Lexer::_scanNumberLiteral()
{
    size_t start = m_pos;
    SourceLocation startLoc = m_location;

    // 负号
    if (_currentChar() == '-') {
        _advance();
    }

    // 整数部分
    while (!_isAtEnd() && isDigit(_currentChar())) {
        _advance();
    }

    // 小数部分
    if (_currentChar() == '.' && isDigit(_peekChar())) {
        _advance(); // 跳过'.'
        while (!_isAtEnd() && isDigit(_currentChar())) {
            _advance();
        }
    }

    std::string value = m_source.substr(start, m_pos - start);
    Token token(TokenType::NumberLiteral, value);
    token.location = startLoc;
    return token;
}

Token Lexer::_scanText()
{
    size_t start = m_pos;
    SourceLocation startLoc = m_location;
    std::string value;

    while (!_isAtEnd() && _currentChar() != '<') {
        value += _currentChar();
        _updatePosition(_currentChar());
        _advance();
    }

    // 移除末尾空白（保留前面的空白用于格式化）
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
        value.pop_back();
    }

    // 如果全是空白或换行，跳过
    bool onlyWhitespace = true;
    for (char c : value) {
        if (!isWhitespace(c) && c != '\n' && c != '\r') {
            onlyWhitespace = false;
            break;
        }
    }

    if (onlyWhitespace || value.empty()) {
        return _scanToken(); // 递归扫描下一个token
    }

    Token token(TokenType::Text, value);
    token.location = startLoc;
    return token;
}

void Lexer::_skipWhitespaceChars()
{
    while (!_isAtEnd() && isWhitespace(_currentChar())) {
        _updatePosition(_currentChar());
        _advance();
    }
}

char Lexer::_currentChar() const
{
    return _isAtEnd() ? '\0' : m_source[m_pos];
}

char Lexer::_peekChar() const
{
    return (m_pos + 1 < m_source.size()) ? m_source[m_pos + 1] : '\0';
}

char Lexer::_peekChar(size_t offset) const
{
    return (m_pos + offset < m_source.size()) ? m_source[m_pos + offset] : '\0';
}

void Lexer::_advance()
{
    if (!_isAtEnd()) {
        ++m_pos;
    }
}

void Lexer::_advance(size_t n)
{
    for (size_t i = 0; i < n && !_isAtEnd(); ++i) {
        ++m_pos;
    }
}

bool Lexer::_isAtEnd() const
{
    return m_pos >= m_source.size();
}

bool Lexer::isWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

bool Lexer::isNewline(char c)
{
    return c == '\n' || c == '\r';
}

bool Lexer::isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Lexer::isDigit(char c)
{
    return c >= '0' && c <= '9';
}

bool Lexer::isAlphaNumeric(char c)
{
    return isAlpha(c) || isDigit(c);
}

bool Lexer::isIdentifierChar(char c)
{
    // 冒号允许出现在标识符中部（如 bind:text, on:click），以使带前缀的属性名
    // 作为一个完整的 Identifier token 输出。注意 ':' 作为首字符时仍由 _scanToken
    // 单独产生 Colon token，此处仅在 _scanIdentifier 的连续扫描中生效。
    return isAlphaNumeric(c) || c == '_' || c == '-' || c == ':';
}

void Lexer::_addError(TemplateErrorType type, const std::string& message)
{
    m_errors.emplace_back(type, message, m_location, m_sourcePath);
}

void Lexer::_updatePosition(char c)
{
    if (c == '\n') {
        ++m_location.line;
        m_location.column = 1;
    } else {
        ++m_location.column;
    }
    ++m_location.offset;
}

Token Lexer::_makeToken(TokenType type, const std::string& value) const
{
    return Token(type, value, m_location);
}

} // namespace mc::client::ui::kagero::tpl::parser
