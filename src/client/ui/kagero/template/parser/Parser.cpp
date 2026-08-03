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

#include "Parser.hpp"
#include "client/ui/kagero/template/parser/Ast.hpp"
#include "client/ui/kagero/template/parser/Lexer.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::tpl::parser {

namespace {

// 属性前缀常量
constexpr std::string_view BIND_PREFIX = "bind:";
constexpr std::string_view ON_PREFIX = "on:";
constexpr std::string_view FOR_PREFIX = "for:";
constexpr std::string_view IF_PREFIX = "if:";

} // namespace

Parser::Parser(const std::vector<Token>& tokens, const TemplateConfig& config)
    : m_tokens(tokens)
    , m_config(config)
    , m_current(0)
{}

Parser::Parser(const Lexer& lexer, const TemplateConfig& config)
    : m_tokens(lexer.tokens())
    , m_config(config)
    , m_current(0)
{}

std::unique_ptr<DocumentNode> Parser::parse()
{
    m_errors.clear();
    m_current = 0;
    m_seenIds.clear();

    auto document = std::make_unique<DocumentNode>();
    document->range.start = _currentLocation();

    // 跳过前导空白
    _skipWhitespaceAndNewlines();

    // 解析根元素
    while (!_isAtEnd()) {
        if (_check(TokenType::OpenTag)) {
            auto element = _parseElement();
            if (element) {
                document->children.push_back(std::move(element));
            }
        } else if (_check(TokenType::OpenComment) || _check(TokenType::Comment)) {
            // 跳过注释
            auto comment = _parseComment();
            if (comment) {
                document->children.push_back(std::move(comment));
            }
        } else if (_check(TokenType::Text)) {
            // 跳过文本
            _consume();
        } else {
            // 意外的Token
            _addError(TemplateErrorType::UnexpectedToken,
                "Expected element or comment, got " + std::string(tokenTypeName(_current().type)),
                _currentLocation());
            _synchronize();
        }

        _skipWhitespaceAndNewlines();
    }

    document->range.end = SourceLocation(
        m_tokens.back().location.line, m_tokens.back().location.column + 1, m_tokens.back().location.offset + 1);

    return document;
}

const Token& Parser::_current() const
{
    if (m_current < m_tokens.size()) {
        return m_tokens[m_current];
    }
    static const Token eofToken(TokenType::EndOfFile);
    return eofToken;
}

const Token& Parser::_peek() const
{
    if (m_current + 1 < m_tokens.size()) {
        return m_tokens[m_current + 1];
    }
    static const Token eofToken(TokenType::EndOfFile);
    return eofToken;
}

Token Parser::_consume()
{
    Token token = _current();
    if (!_isAtEnd()) {
        ++m_current;
    }
    return token;
}

bool Parser::_check(TokenType type) const
{
    return _current().type == type;
}

bool Parser::_check(TokenType type, const std::string& value) const
{
    return _current().is(type, value);
}

bool Parser::_match(TokenType type)
{
    if (_check(type)) {
        _consume();
        return true;
    }
    return false;
}

bool Parser::_match(TokenType type, const std::string& value)
{
    if (_check(type, value)) {
        _consume();
        return true;
    }
    return false;
}

bool Parser::_expect(TokenType type)
{
    if (_check(type)) {
        _consume();
        return true;
    }
    _addError(TemplateErrorType::UnexpectedToken,
        "Expected " + std::string(tokenTypeName(type)) + ", got " + std::string(tokenTypeName(_current().type)),
        _current());
    return false;
}

bool Parser::_expect(TokenType type, const std::string& value)
{
    if (_check(type, value)) {
        _consume();
        return true;
    }
    _addError(
        TemplateErrorType::UnexpectedToken, "Expected '" + value + "', got '" + _current().value + "'", _current());
    return false;
}

Token Parser::_expectIdentifier(const std::string& context)
{
    if (!_check(TokenType::Identifier)) {
        std::string msg = "Expected identifier";
        if (!context.empty()) {
            msg += " " + context;
        }
        _addError(TemplateErrorType::UnexpectedToken, msg, _current());
        return Token(TokenType::Error);
    }
    return _consume();
}

Token Parser::_expectStringLiteral(const std::string& context)
{
    if (!_check(TokenType::StringLiteral)) {
        std::string msg = "Expected string literal";
        if (!context.empty()) {
            msg += " " + context;
        }
        _addError(TemplateErrorType::UnexpectedToken, msg, _current());
        return Token(TokenType::Error);
    }
    return _consume();
}

void Parser::_synchronize()
{
    // 跳过Token直到找到一个有效的同步点
    while (!_isAtEnd()) {
        switch (_current().type) {
            case TokenType::OpenTag:
            case TokenType::OpenCloseTag:
            case TokenType::EndOfFile:
                return;
            default:
                _consume();
                break;
        }
    }
}

void Parser::_skipWhitespaceAndNewlines()
{
    while (_check(TokenType::Whitespace) || _check(TokenType::Newline)) {
        _consume();
    }
}

std::unique_ptr<DocumentNode> Parser::_parseDocument()
{
    return parse();
}

std::unique_ptr<ElementNode> Parser::_parseElement()
{
    SourceLocation startLoc = _currentLocation();

    // 开始标签
    if (!_expect(TokenType::OpenTag)) {
        return nullptr;
    }

    // 标签名
    Token nameToken = _expectIdentifier("for tag name");
    if (nameToken.type == TokenType::Error) {
        return nullptr;
    }

    std::string tagName = nameToken.value;

    // 验证标签名
    if (!_isValidTagName(tagName)) {
        if (m_config.strictMode) {
            _addError(TemplateErrorType::UnknownTag, "Unknown tag: <" + tagName + ">", nameToken);
        }
    }

    // 创建元素节点
    NodeType nodeType = ast::getNodeTypeFromTagName(tagName);
    auto element = std::make_unique<ElementNode>(nodeType);
    element->tagName = tagName;
    element->range.start = startLoc;

    // 解析属性
    _parseAttributes(*element);

    // 分类属性和提取指令（在自关闭标签检查之前）
    element->categorizeAttributes();
    element->loop = _extractLoopInfo(*element);
    element->condition = _extractConditionInfo(*element);

    // 自关闭标签
    if (_match(TokenType::SelfCloseTag)) {
        element->range.end = _currentLocation();
        _validateElement(*element);
        return element;
    }

    // 结束标签
    if (!_expect(TokenType::CloseTag)) {
        return nullptr;
    }

    // 解析内容
    _parseContent(*element);

    // 结束标签
    if (!_parseEndTag(tagName)) {
        // 尝试恢复：不返回nullptr，继续返回部分解析的元素
    }

    element->range.end = _currentLocation();

    // 验证元素
    _validateElement(*element);

    return element;
}

// TODO: isOpenTag参数目前未使用，后续需要完善对开放标签与自关闭标签的区分处理
std::string Parser::_parseStartTag(bool isOpenTag)
{
    (void)isOpenTag;

    if (!_expect(TokenType::OpenTag)) {
        return "";
    }

    Token nameToken = _expectIdentifier("for tag name");
    if (nameToken.type == TokenType::Error) {
        return "";
    }

    return nameToken.value;
}

void Parser::_parseAttributes(ElementNode& element)
{
    _skipWhitespaceAndNewlines();

    while (!_isAtEnd() && !_check(TokenType::CloseTag) && !_check(TokenType::SelfCloseTag)) {

        Attribute attr = _parseAttribute();
        if (!attr.name.empty()) {
            // 检查重复属性，后定义的覆盖前面的
            element.addAttribute(attr);

            // 特殊处理id属性
            if (attr.name == "id" && !attr.rawValue.empty()) {
                element.id = attr.rawValue;
            }
        }

        _skipWhitespaceAndNewlines();
    }
}

Attribute Parser::_parseAttribute()
{
    SourceLocation attrLoc = _currentLocation();

    // 属性名（可能带 bind:, on:, for:, if: 前缀）
    Token nameToken = _parseAttributeName();
    if (nameToken.type == TokenType::Error || nameToken.value.empty()) {
        return Attribute();
    }

    std::string attrName = nameToken.value;

    // 检查是否是特殊前缀
    bool isBinding = false;
    bool isEvent = false;
    bool isLoop = false;
    bool isCondition = false;
    std::string baseName = attrName;

    if (attrName.starts_with(BIND_PREFIX)) {
        isBinding = true;
        baseName = attrName.substr(BIND_PREFIX.size());
    } else if (attrName.starts_with(ON_PREFIX)) {
        isEvent = true;
        baseName = attrName.substr(ON_PREFIX.size());
    } else if (attrName.starts_with(FOR_PREFIX)) {
        isLoop = true;
        baseName = attrName.substr(FOR_PREFIX.size());
    } else if (attrName.starts_with(IF_PREFIX)) {
        isCondition = true;
        baseName = attrName.substr(IF_PREFIX.size());
    }

    // 期望等号
    if (!_expect(TokenType::Equals)) {
        return Attribute();
    }

    // 属性值
    Token valueToken = _parseAttributeValue();
    if (valueToken.type == TokenType::Error) {
        return Attribute();
    }

    // 创建属性
    Attribute attr;
    attr.name = attrName;
    attr.rawValue = valueToken.value;
    attr.location = attrLoc;

    if (isBinding) {
        attr = Attribute::createBinding(attrName, valueToken.value, attrLoc);
    } else if (isEvent) {
        attr = Attribute::createEvent(attrName, valueToken.value, attrLoc);
    } else if (isLoop) {
        attr = Attribute::createStatic(attrName, valueToken.value, attrLoc);
        attr.type = ast::AttributeType::Loop;
    } else if (isCondition) {
        attr = Attribute::createStatic(attrName, valueToken.value, attrLoc);
        attr.type = ast::AttributeType::Condition;
    } else {
        attr = Attribute::createStatic(attrName, valueToken.value, attrLoc);
    }

    return attr;
}

Token Parser::_parseAttributeName()
{
    // 属性名可以是 identifier 或 identifier:identifier (bind:xxx, on:xxx)
    Token firstPart = _expectIdentifier("");
    if (firstPart.type == TokenType::Error) {
        return firstPart;
    }

    std::string name = firstPart.value;

    // 检查冒号
    if (_check(TokenType::Colon)) {
        _consume(); // 消费冒号

        Token secondPart = _expectIdentifier("");
        if (secondPart.type == TokenType::Error) {
            return secondPart;
        }

        name += ":" + secondPart.value;
    }

    Token result(TokenType::Identifier, name, firstPart.location);
    return result;
}

Token Parser::_parseAttributeValue()
{
    _skipWhitespaceAndNewlines();

    // 字符串字面量
    if (_check(TokenType::StringLiteral)) {
        return _consume();
    }

    // 数字字面量（非标准但允许）
    if (_check(TokenType::NumberLiteral)) {
        Token numToken = _consume();
        // 转换为字符串
        return Token(TokenType::StringLiteral, numToken.value, numToken.location);
    }

    // 标识符（无引号属性值，非标准但允许）
    if (_check(TokenType::Identifier)) {
        Token idToken = _consume();
        return Token(TokenType::StringLiteral, idToken.value, idToken.location);
    }

    _addError(
        TemplateErrorType::UnexpectedToken, "Expected attribute value (string, number, or identifier)", _current());
    return Token(TokenType::Error);
}

void Parser::_parseContent(ElementNode& parent)
{
    while (!_isAtEnd() && !_check(TokenType::OpenCloseTag)) {
        _skipWhitespaceAndNewlines();

        if (_isAtEnd() || _check(TokenType::OpenCloseTag)) {
            break;
        }

        if (_check(TokenType::OpenTag)) {
            // 子元素
            auto child = _parseElement();
            if (child) {
                parent.children.push_back(std::move(child));
            }
        } else if (_check(TokenType::OpenComment) || _check(TokenType::Comment)) {
            // 注释
            auto comment = _parseComment();
            if (comment) {
                parent.children.push_back(std::move(comment));
            }
        } else if (_check(TokenType::Text)) {
            // 文本
            auto text = _parseText();
            if (text && !text->isWhitespace) {
                parent.children.push_back(std::move(text));
            }
        } else {
            // 意外的Token，尝试恢复
            _addError(TemplateErrorType::UnexpectedToken,
                "Unexpected token in element content: " + std::string(tokenTypeName(_current().type)),
                _current());
            _consume();
        }

        _skipWhitespaceAndNewlines();
    }
}

std::unique_ptr<TextNode> Parser::_parseText()
{
    if (!_check(TokenType::Text)) {
        return nullptr;
    }

    Token textToken = _consume();
    auto node = std::make_unique<TextNode>();
    node->text = textToken.value;
    node->range = SourceRange(textToken.location, textToken.location);

    // 检查是否全是空白
    node->isWhitespace = std::all_of(textToken.value.begin(), textToken.value.end(), [](char c) {
        return Lexer::isWhitespace(c) || c == '\n' || c == '\r';
    });

    return node;
}

std::unique_ptr<CommentNode> Parser::_parseComment()
{
    // 跳过注释开始标记
    if (_check(TokenType::OpenComment)) {
        _consume();
    }

    // 获取注释内容
    std::string content;
    if (_check(TokenType::Comment)) {
        Token commentToken = _consume();
        content = commentToken.value;
    }

    // 跳过注释结束标记
    if (_check(TokenType::CloseComment)) {
        _consume();
    }

    auto node = std::make_unique<CommentNode>();
    node->text = content;
    return node;
}

bool Parser::_parseEndTag(const std::string& expectedTagName)
{
    if (!_check(TokenType::OpenCloseTag)) {
        _addError(TemplateErrorType::MissingClosingTag, "Expected closing tag </" + expectedTagName + ">", _current());
        return false;
    }

    _consume(); // 消费 </

    Token nameToken = _expectIdentifier("for closing tag name");
    if (nameToken.type == TokenType::Error) {
        return false;
    }

    if (nameToken.value != expectedTagName) {
        _addError(TemplateErrorType::MismatchedClosingTag,
            "Mismatched closing tag: expected </" + expectedTagName + "> but got </" + nameToken.value + ">",
            nameToken);
        // 尝试恢复
    }

    if (!_expect(TokenType::CloseTag)) {
        return false;
    }

    return true;
}

void Parser::_validateElement(ElementNode& element)
{
    // 验证ID唯一性（如果有）
    if (!element.id.empty()) {
        if (m_seenIds.contains(element.id)) {
            // ID重复，添加错误
            _addError(TemplateErrorType::DuplicateId,
                "Duplicate ID: '" + element.id + "'. Each ID must be unique within the document.",
                element.range.start);
        } else {
            // ID唯一，添加到已收集的ID集合
            m_seenIds.insert(element.id);
        }
    }

    // 验证所有属性
    for (const auto& [name, attr] : element.attributes) {
        _validateAttribute(attr, element);
    }
}

// TODO: element参数暂时未使用，后续需要增加基于元素类型的属性验证逻辑
void Parser::_validateAttribute(const Attribute& attr, const ElementNode& element)
{
    (void)element;

    // 验证属性名
    if (!ast::isValidAttributeName(attr.name)) {
        _addError(TemplateErrorType::InvalidAttributeName, "Invalid attribute name: " + attr.name, attr.location);
        return;
    }

    // 验证绑定路径
    if (attr.isBinding()) {
        if (m_config.validateBindingPaths && attr.binding.has_value()) {
            _validateBindingPath(attr.binding->path, attr.location);
        }
    }

    // 验证回调名称
    if (attr.isEvent()) {
        if (m_config.validateCallbackNames) {
            _validateCallbackName(attr.callbackName, attr.location);
        }
    }

    // 检查内联表达式
    if (m_config.strictMode && !_isInlineExpressionAllowed()) {
        // 检查绑定值中是否有表达式
        if (attr.isBinding() && !attr.rawValue.empty()) {
            // 检测常见的表达式模式
            static const std::string forbiddenPatterns[] = {
                "{{", "}}", "{%", "%}", "${", "+", "-", "*", "/", "==", "!=", "<=", ">=", "&&", "||", "?:", "=>"};

            for (const auto& pattern : forbiddenPatterns) {
                if (attr.rawValue.find(pattern) != std::string::npos) {
                    _addError(TemplateErrorType::InlineExpressionNotAllowed,
                        std::string("Inline expressions are not allowed in strict mode. ") + "Found pattern '" +
                            pattern + "' in binding: " + attr.rawValue,
                        attr.location);
                    break;
                }
            }
        }
    }
}

void Parser::_validateBindingPath(const std::string& path, const SourceLocation& loc)
{
    if (!ast::isValidBindingPath(path)) {
        _addError(TemplateErrorType::InvalidBindingPath, "Invalid binding path: '" + path + "'", loc);
    }
}

void Parser::_validateCallbackName(const std::string& name, const SourceLocation& loc)
{
    if (!ast::isValidCallbackName(name)) {
        _addError(TemplateErrorType::InvalidCallbackName,
            "Invalid callback name: '" + name + "'. Must be a valid identifier.",
            loc);
    }
}

bool Parser::_isInlineExpressionAllowed() const
{
    if (m_config.strictMode) {
        return false;
    }
    return m_config.allowInlineExpression;
}

std::optional<LoopInfo> Parser::_extractLoopInfo(const ElementNode& element)
{
    // 查找 for:xxx 属性
    // 支持两种语法：
    // 1. for:item="item in collection" - 简单循环
    // 2. for:(item, index)="(item, index) in collection" - 带索引循环

    std::string forAttrName;
    std::string forAttrValue;
    SourceLocation forAttrLoc;

    // 查找任何以 "for:" 开头的属性
    for (const auto& [name, attr] : element.attributes) {
        if (name.starts_with(FOR_PREFIX)) {
            forAttrName = name;
            forAttrValue = attr.rawValue;
            forAttrLoc = attr.location;
            break;
        }
    }

    if (forAttrValue.empty()) {
        return std::nullopt;
    }

    LoopInfo info;
    info.location = forAttrLoc;

    // 解析循环表达式
    // 格式: "item in collection" 或 "(item, index) in collection"
    std::string expr = forAttrValue;

    // 查找 " in " 关键字
    static constexpr std::string_view IN_KEYWORD = " in ";
    size_t inPos = expr.find(IN_KEYWORD);
    if (inPos == std::string::npos) {
        _addError(TemplateErrorType::SemanticError,
            "Invalid loop syntax: expected 'item in collection' or '(item, index) in collection'",
            forAttrLoc);
        return std::nullopt;
    }

    std::string varsPart = expr.substr(0, inPos);
    std::string collectionPart = expr.substr(inPos + IN_KEYWORD.size());

    // 去除空白
    size_t varsStart = varsPart.find_first_not_of(" \t");
    size_t varsEnd = varsPart.find_last_not_of(" \t");
    if (varsStart != std::string::npos && varsEnd != std::string::npos) {
        varsPart = varsPart.substr(varsStart, varsEnd - varsStart + 1);
    }

    size_t collStart = collectionPart.find_first_not_of(" \t");
    size_t collEnd = collectionPart.find_last_not_of(" \t");
    if (collStart != std::string::npos && collEnd != std::string::npos) {
        collectionPart = collectionPart.substr(collStart, collEnd - collStart + 1);
    }

    info.collectionPath = collectionPart;

    // 解析变量部分
    // 检查是否是 "(item, index)" 格式
    if (!varsPart.empty() && varsPart[0] == '(' && varsPart.back() == ')') {
        // 解析 "(item, index)" 格式
        std::string inner = varsPart.substr(1, varsPart.size() - 2);
        size_t commaPos = inner.find(',');

        if (commaPos != std::string::npos) {
            std::string itemVar = inner.substr(0, commaPos);
            std::string indexVar = inner.substr(commaPos + 1);

            // 去除空白
            size_t itemStart = itemVar.find_first_not_of(" \t");
            size_t itemEnd = itemVar.find_last_not_of(" \t");
            if (itemStart != std::string::npos && itemEnd != std::string::npos) {
                itemVar = itemVar.substr(itemStart, itemEnd - itemStart + 1);
            }

            size_t indexStart = indexVar.find_first_not_of(" \t");
            size_t indexEnd = indexVar.find_last_not_of(" \t");
            if (indexStart != std::string::npos && indexEnd != std::string::npos) {
                indexVar = indexVar.substr(indexStart, indexEnd - indexStart + 1);
            }

            info.itemVarName = itemVar;
            info.indexVarName = indexVar;
            info.hasIndex = true;
        } else {
            // 单个变量在括号内: "(item)"
            size_t innerStart = inner.find_first_not_of(" \t");
            size_t innerEnd = inner.find_last_not_of(" \t");
            if (innerStart != std::string::npos && innerEnd != std::string::npos) {
                info.itemVarName = inner.substr(innerStart, innerEnd - innerStart + 1);
            }
        }
    } else {
        // 简单格式: "item"
        info.itemVarName = varsPart;
    }

    // 如果没有找到循环变量，使用属性名作为变量名
    // 例如 for:slot="..." 使用 "slot" 作为变量名
    if (info.itemVarName.empty() && forAttrName.size() > FOR_PREFIX.size()) {
        info.itemVarName = forAttrName.substr(FOR_PREFIX.size());
    }

    // 如果仍然没有变量名，使用默认值
    if (info.itemVarName.empty()) {
        info.itemVarName = "item";
    }

    return info;
}

std::optional<ConditionInfo> Parser::_extractConditionInfo(const ElementNode& element)
{
    // 查找 if:xxx 属性
    // 支持语法：
    // 1. if:condition="booleanPath" - 条件为真时显示
    // 2. if:condition="!booleanPath" - 条件为假时显示（取反）

    std::string ifAttrValue;
    SourceLocation ifAttrLoc;

    // 查找任何以 "if:" 开头的属性
    for (const auto& [name, attr] : element.attributes) {
        if (name.starts_with(IF_PREFIX)) {
            ifAttrValue = attr.rawValue;
            ifAttrLoc = attr.location;
            break;
        }
    }

    // TODO: bind:visible兼容逻辑，待if:指令全面替代后应移除此兼容分支
    if (ifAttrValue.empty()) {
        auto it = element.attributes.find("bind:visible");
        if (it == element.attributes.end()) {
            return std::nullopt;
        }

        const Attribute& attr = it->second;
        if (!attr.binding.has_value()) {
            return std::nullopt;
        }

        ifAttrValue = attr.binding->path;
        ifAttrLoc = attr.location;
    }

    ConditionInfo info;
    std::string path = ifAttrValue;

    // 检查是否有取反
    if (!path.empty() && path[0] == '!') {
        info.negate = true;
        path = path.substr(1);
        // 去除可能的空格
        size_t start = path.find_first_not_of(" \t");
        if (start != std::string::npos) {
            path = path.substr(start);
        }
    }

    info.booleanPath = path;
    info.location = ifAttrLoc;

    return info;
}

void Parser::_addError(TemplateErrorType type, const std::string& message, const SourceLocation& loc)
{
    m_errors.emplace_back(type, message, loc);
}

void Parser::_addError(TemplateErrorType type, const std::string& message, const Token& token)
{
    m_errors.emplace_back(type, message, token.location);
}

bool Parser::_isAtEnd() const
{
    return m_current >= m_tokens.size() || _current().type == TokenType::EndOfFile;
}

SourceLocation Parser::_currentLocation() const
{
    return _current().location;
}

bool Parser::_isValidTagName(const std::string& name) const
{
    return ast::isValidWidgetTag(name);
}

} // namespace mc::client::ui::kagero::tpl::parser
