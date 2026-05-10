#include "Parser.hpp"
#include <algorithm>

namespace mc::client::ui::kagero::tpl::parser {

Parser::Parser(const std::vector<Token>& tokens, const TemplateConfig& config)
    : m_tokens(tokens)
    , m_config(config)
    , m_current(0) {
}

Parser::Parser(const Lexer& lexer, const TemplateConfig& config)
    : m_tokens(lexer.tokens())
    , m_config(config)
    , m_current(0) {
}

std::unique_ptr<DocumentNode> Parser::parse() {
    m_errors.clear();
    m_current = 0;
    m_seenIds.clear();  // 清空已收集的ID集合

    auto document = std::make_unique<DocumentNode>();
    document->range.start = currentLocation();

    // 跳过前导空白
    skipWhitespaceAndNewlines();

    // 解析根元素
    while (!isAtEnd()) {
        if (check(TokenType::OpenTag)) {
            auto element = parseElement();
            if (element) {
                document->children.push_back(std::move(element));
            }
        } else if (check(TokenType::OpenComment) || check(TokenType::Comment)) {
            // 跳过注释
            auto comment = parseComment();
            if (comment) {
                document->children.push_back(std::move(comment));
            }
        } else if (check(TokenType::Text)) {
            // 跳过文本
            consume();
        } else {
            // 意外的Token
            addError(TemplateErrorType::UnexpectedToken,
                     "Expected element or comment, got " + std::string(tokenTypeName(current().type)));
            synchronize();
        }

        skipWhitespaceAndNewlines();
    }

    document->range.end = SourceLocation(m_tokens.back().location.line,
                                          m_tokens.back().location.column + 1,
                                          m_tokens.back().location.offset + 1);

    return document;
}

const Token& Parser::current() const {
    if (m_current < m_tokens.size()) {
        return m_tokens[m_current];
    }
    static const Token eofToken(TokenType::EndOfFile);
    return eofToken;
}

const Token& Parser::peek() const {
    if (m_current + 1 < m_tokens.size()) {
        return m_tokens[m_current + 1];
    }
    static const Token eofToken(TokenType::EndOfFile);
    return eofToken;
}

Token Parser::consume() {
    Token token = current();
    if (!isAtEnd()) {
        ++m_current;
    }
    return token;
}

bool Parser::check(TokenType type) const {
    return current().type == type;
}

bool Parser::check(TokenType type, const std::string& value) const {
    return current().is(type, value);
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        consume();
        return true;
    }
    return false;
}

bool Parser::match(TokenType type, const std::string& value) {
    if (check(type, value)) {
        consume();
        return true;
    }
    return false;
}

bool Parser::expect(TokenType type) {
    if (check(type)) {
        consume();
        return true;
    }
    addError(TemplateErrorType::UnexpectedToken,
             "Expected " + std::string(tokenTypeName(type)) +
             ", got " + std::string(tokenTypeName(current().type)),
             current());
    return false;
}

bool Parser::expect(TokenType type, const std::string& value) {
    if (check(type, value)) {
        consume();
        return true;
    }
    addError(TemplateErrorType::UnexpectedToken,
             "Expected '" + value + "', got '" + current().value + "'",
             current());
    return false;
}

Token Parser::expectIdentifier(const std::string& context) {
    if (!check(TokenType::Identifier)) {
        std::string msg = "Expected identifier";
        if (!context.empty()) {
            msg += " " + context;
        }
        addError(TemplateErrorType::UnexpectedToken, msg, current());
        return Token(TokenType::Error);
    }
    return consume();
}

Token Parser::expectStringLiteral(const std::string& context) {
    if (!check(TokenType::StringLiteral)) {
        std::string msg = "Expected string literal";
        if (!context.empty()) {
            msg += " " + context;
        }
        addError(TemplateErrorType::UnexpectedToken, msg, current());
        return Token(TokenType::Error);
    }
    return consume();
}

void Parser::synchronize() {
    // 跳过Token直到找到一个有效的同步点
    while (!isAtEnd()) {
        switch (current().type) {
            case TokenType::OpenTag:
            case TokenType::OpenCloseTag:
            case TokenType::EndOfFile:
                return;
            default:
                consume();
                break;
        }
    }
}

void Parser::skipWhitespaceAndNewlines() {
    while (check(TokenType::Whitespace) || check(TokenType::Newline)) {
        consume();
    }
}

std::unique_ptr<DocumentNode> Parser::parseDocument() {
    return parse(); // 主解析方法
}

std::unique_ptr<ElementNode> Parser::parseElement() {
    SourceLocation startLoc = currentLocation();

    // 开始标签
    if (!expect(TokenType::OpenTag)) {
        return nullptr;
    }

    // 标签名
    Token nameToken = expectIdentifier("for tag name");
    if (nameToken.type == TokenType::Error) {
        return nullptr;
    }

    std::string tagName = nameToken.value;

    // 验证标签名
    if (!isValidTagName(tagName)) {
        if (m_config.strictMode) {
            addError(TemplateErrorType::UnknownTag,
                     "Unknown tag: <" + tagName + ">", nameToken);
            // 非严格模式下继续解析
        }
    }

    // 创建元素节点
    NodeType nodeType = ast::getNodeTypeFromTagName(tagName);
    auto element = std::make_unique<ElementNode>(nodeType);
    element->tagName = tagName;
    element->range.start = startLoc;

    // 解析属性
    parseAttributes(*element);

    // 分类属性和提取指令（在自关闭标签检查之前）
    element->categorizeAttributes();
    element->loop = extractLoopInfo(*element);
    element->condition = extractConditionInfo(*element);

    // 自关闭标签
    if (match(TokenType::SelfCloseTag)) {
        element->range.end = currentLocation();
        validateElement(*element);
        return element;
    }

    // 结束标签
    if (!expect(TokenType::CloseTag)) {
        return nullptr;
    }

    // 解析内容
    parseContent(*element);

    // 结束标签
    if (!parseEndTag(tagName)) {
        // 尝试恢复：不返回nullptr，继续返回部分解析的元素
    }

    element->range.end = currentLocation();

    // 验证元素
    validateElement(*element);

    return element;
}

std::string Parser::parseStartTag(bool isOpenTag) {
    (void)isOpenTag; // 未使用

    if (!expect(TokenType::OpenTag)) {
        return "";
    }

    Token nameToken = expectIdentifier("for tag name");
    if (nameToken.type == TokenType::Error) {
        return "";
    }

    return nameToken.value;
}

void Parser::parseAttributes(ElementNode& element) {
    skipWhitespaceAndNewlines();

    while (!isAtEnd() &&
           !check(TokenType::CloseTag) &&
           !check(TokenType::SelfCloseTag)) {

        Attribute attr = parseAttribute();
        if (!attr.name.empty()) {
            // 检查重复属性
            if (element.hasAttribute(attr.name)) {
                // 警告但不报错，后定义的覆盖前面的
            }
            element.addAttribute(attr);

            // 特殊处理id属性
            if (attr.name == "id" && !attr.rawValue.empty()) {
                element.id = attr.rawValue;
            }
        }

        skipWhitespaceAndNewlines();
    }
}

Attribute Parser::parseAttribute() {
    SourceLocation attrLoc = currentLocation();

    // 属性名（可能带 bind:, on:, for:, if: 前缀）
    Token nameToken = parseAttributeName();
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

    static const std::string BIND_PREFIX = "bind:";
    static const std::string ON_PREFIX = "on:";
    static const std::string FOR_PREFIX = "for:";
    static const std::string IF_PREFIX = "if:";

    if (attrName.size() > BIND_PREFIX.size() &&
        attrName.substr(0, BIND_PREFIX.size()) == BIND_PREFIX) {
        isBinding = true;
        baseName = attrName.substr(BIND_PREFIX.size());
    } else if (attrName.size() > ON_PREFIX.size() &&
               attrName.substr(0, ON_PREFIX.size()) == ON_PREFIX) {
        isEvent = true;
        baseName = attrName.substr(ON_PREFIX.size());
    } else if (attrName.size() > FOR_PREFIX.size() &&
               attrName.substr(0, FOR_PREFIX.size()) == FOR_PREFIX) {
        isLoop = true;
        baseName = attrName.substr(FOR_PREFIX.size());
    } else if (attrName.size() > IF_PREFIX.size() &&
               attrName.substr(0, IF_PREFIX.size()) == IF_PREFIX) {
        isCondition = true;
        baseName = attrName.substr(IF_PREFIX.size());
    }

    // 期望等号
    if (!expect(TokenType::Equals)) {
        return Attribute();
    }

    // 属性值
    Token valueToken = parseAttributeValue();
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

Token Parser::parseAttributeName() {
    // 属性名可以是 identifier 或 identifier:identifier (bind:xxx, on:xxx)
    Token firstPart = expectIdentifier();
    if (firstPart.type == TokenType::Error) {
        return firstPart;
    }

    std::string name = firstPart.value;

    // 检查冒号
    if (check(TokenType::Colon)) {
        consume(); // 消费冒号

        Token secondPart = expectIdentifier();
        if (secondPart.type == TokenType::Error) {
            return secondPart;
        }

        name += ":" + secondPart.value;
    }

    Token result(TokenType::Identifier, name, firstPart.location);
    return result;
}

Token Parser::parseAttributeValue() {
    skipWhitespaceAndNewlines();

    // 字符串字面量
    if (check(TokenType::StringLiteral)) {
        return consume();
    }

    // 数字字面量（非标准但允许）
    if (check(TokenType::NumberLiteral)) {
        Token numToken = consume();
        // 转换为字符串
        return Token(TokenType::StringLiteral, numToken.value, numToken.location);
    }

    // 标识符（无引号属性值，非标准但允许）
    if (check(TokenType::Identifier)) {
        Token idToken = consume();
        return Token(TokenType::StringLiteral, idToken.value, idToken.location);
    }

    addError(TemplateErrorType::UnexpectedToken,
             "Expected attribute value (string, number, or identifier)",
             current());
    return Token(TokenType::Error);
}

void Parser::parseContent(ElementNode& parent) {
    while (!isAtEnd() && !check(TokenType::OpenCloseTag)) {
        skipWhitespaceAndNewlines();

        if (isAtEnd() || check(TokenType::OpenCloseTag)) {
            break;
        }

        if (check(TokenType::OpenTag)) {
            // 子元素
            auto child = parseElement();
            if (child) {
                parent.children.push_back(std::move(child));
            }
        } else if (check(TokenType::OpenComment) || check(TokenType::Comment)) {
            // 注释
            auto comment = parseComment();
            if (comment) {
                parent.children.push_back(std::move(comment));
            }
        } else if (check(TokenType::Text)) {
            // 文本
            auto text = parseText();
            if (text && !text->isWhitespace) {
                parent.children.push_back(std::move(text));
            }
        } else {
            // 意外的Token，尝试恢复
            addError(TemplateErrorType::UnexpectedToken,
                     "Unexpected token in element content: " + std::string(tokenTypeName(current().type)),
                     current());
            consume();
        }

        skipWhitespaceAndNewlines();
    }
}

std::unique_ptr<TextNode> Parser::parseText() {
    if (!check(TokenType::Text)) {
        return nullptr;
    }

    Token textToken = consume();
    auto node = std::make_unique<TextNode>();
    node->text = textToken.value;
    node->range = SourceRange(textToken.location, textToken.location);

    // 检查是否全是空白
    node->isWhitespace = std::all_of(textToken.value.begin(), textToken.value.end(),
                                     [](char c) { return Lexer::isWhitespace(c) || c == '\n' || c == '\r'; });

    return node;
}

std::unique_ptr<CommentNode> Parser::parseComment() {
    // 跳过注释开始标记
    if (check(TokenType::OpenComment)) {
        consume();
    }

    // 获取注释内容
    std::string content;
    if (check(TokenType::Comment)) {
        Token commentToken = consume();
        content = commentToken.value;
    }

    // 跳过注释结束标记
    if (check(TokenType::CloseComment)) {
        consume();
    }

    auto node = std::make_unique<CommentNode>();
    node->text = content;
    return node;
}

bool Parser::parseEndTag(const std::string& expectedTagName) {
    if (!check(TokenType::OpenCloseTag)) {
        addError(TemplateErrorType::MissingClosingTag,
                 "Expected closing tag </" + expectedTagName + ">",
                 current());
        return false;
    }

    consume(); // 消费 </

    Token nameToken = expectIdentifier("for closing tag name");
    if (nameToken.type == TokenType::Error) {
        return false;
    }

    if (nameToken.value != expectedTagName) {
        addError(TemplateErrorType::MismatchedClosingTag,
                 "Mismatched closing tag: expected </" + expectedTagName +
                 "> but got </" + nameToken.value + ">",
                 nameToken);
        // 尝试恢复
    }

    if (!expect(TokenType::CloseTag)) {
        return false;
    }

    return true;
}

void Parser::validateElement(ElementNode& element) {
    // 验证ID唯一性（如果有）
    if (!element.id.empty()) {
        // 检查ID是否已经被使用
        if (m_seenIds.find(element.id) != m_seenIds.end()) {
            // ID重复，添加错误
            addError(TemplateErrorType::DuplicateId,
                     "Duplicate ID: '" + element.id + "'. Each ID must be unique within the document.",
                     element.range.start);
        } else {
            // ID唯一，添加到已收集的ID集合
            m_seenIds.insert(element.id);
        }
    }

    // 验证所有属性
    for (const auto& [name, attr] : element.attributes) {
        validateAttribute(attr, element);
    }
}

void Parser::validateAttribute(const Attribute& attr, const ElementNode& element) {
    (void)element; // 暂时未使用

    // 验证属性名
    if (!ast::isValidAttributeName(attr.name)) {
        addError(TemplateErrorType::InvalidAttributeName,
                 "Invalid attribute name: " + attr.name,
                 attr.location);
        return;
    }

    // 验证绑定路径
    if (attr.isBinding()) {
        if (m_config.validateBindingPaths && attr.binding.has_value()) {
            validateBindingPath(attr.binding->path, attr.location);
        }
    }

    // 验证回调名称
    if (attr.isEvent()) {
        if (m_config.validateCallbackNames) {
            validateCallbackName(attr.callbackName, attr.location);
        }
    }

    // 检查内联表达式
    if (m_config.strictMode && !isInlineExpressionAllowed()) {
        // 检查绑定值中是否有表达式
        if (attr.isBinding() && !attr.rawValue.empty()) {
            // 检测常见的表达式模式
            static const std::string forbiddenPatterns[] = {
                "{{", "}}", "{%", "%}", "${", "+", "-", "*", "/", "==",
                "!=", "<=", ">=", "&&", "||", "?:", "=>"
            };

            for (const auto& pattern : forbiddenPatterns) {
                if (attr.rawValue.find(pattern) != std::string::npos) {
                    addError(TemplateErrorType::InlineExpressionNotAllowed,
                             std::string("Inline expressions are not allowed in strict mode. ") +
                             "Found pattern '" + pattern + "' in binding: " + attr.rawValue,
                             attr.location);
                    break;
                }
            }
        }
    }
}

void Parser::validateBindingPath(const std::string& path, const SourceLocation& loc) {
    if (!ast::isValidBindingPath(path)) {
        addError(TemplateErrorType::InvalidBindingPath,
                 "Invalid binding path: '" + path + "'",
                 loc);
    }
}

void Parser::validateCallbackName(const std::string& name, const SourceLocation& loc) {
    if (!ast::isValidCallbackName(name)) {
        addError(TemplateErrorType::InvalidCallbackName,
                 "Invalid callback name: '" + name + "'. Must be a valid identifier.",
                 loc);
    }
}

bool Parser::isInlineExpressionAllowed() const {
    if (m_config.strictMode) {
        return false;
    }
    return m_config.allowInlineExpression;
}

std::optional<LoopInfo> Parser::extractLoopInfo(const ElementNode& element) {
    // 查找 for:xxx 属性
    // 支持两种语法：
    // 1. for:item="item in collection" - 简单循环
    // 2. for:(item, index)="(item, index) in collection" - 带索引循环

    std::string forAttrName;
    std::string forAttrValue;
    SourceLocation forAttrLoc;

    // 查找任何以 "for:" 开头的属性
    for (const auto& [name, attr] : element.attributes) {
        if (name.size() > 4) {
            if (name[0] == 'f' && name[1] == 'o' && name[2] == 'r' && name[3] == ':') {
                forAttrName = name;
                forAttrValue = attr.rawValue;
                forAttrLoc = attr.location;
                break;
            }
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
    const std::string IN_KEYWORD = " in ";
    size_t inPos = expr.find(IN_KEYWORD);
    if (inPos == std::string::npos) {
        // 尝试小写的 " in "
        std::string lowerExpr = expr;
        std::transform(lowerExpr.begin(), lowerExpr.end(), lowerExpr.begin(),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        inPos = lowerExpr.find(" in ");
        if (inPos == std::string::npos) {
            addError(TemplateErrorType::SemanticError,
                     "Invalid loop syntax: expected 'item in collection' or '(item, index) in collection'",
                     forAttrLoc);
            return std::nullopt;
        }
    }

    std::string varsPart = expr.substr(0, inPos);
    std::string collectionPart = expr.substr(inPos + 4); // " in " 长度为4

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
    if (info.itemVarName.empty() && forAttrName.size() > 4) {
        info.itemVarName = forAttrName.substr(4);
    }

    // 如果仍然没有变量名，使用默认值
    if (info.itemVarName.empty()) {
        info.itemVarName = "item";
    }

    return info;
}

std::optional<ConditionInfo> Parser::extractConditionInfo(const ElementNode& element) {
    // 查找 if:xxx 属性
    // 支持语法：
    // 1. if:condition="booleanPath" - 条件为真时显示
    // 2. if:condition="!booleanPath" - 条件为假时显示（取反）

    std::string ifAttrValue;
    SourceLocation ifAttrLoc;

    // 查找任何以 "if:" 开头的属性
    for (const auto& [name, attr] : element.attributes) {
        // Check for "if:" prefix
        if (name.size() > 3) {
            if (name[0] == 'i' && name[1] == 'f' && name[2] == ':') {
                ifAttrValue = attr.rawValue;
                ifAttrLoc = attr.location;
                break;
            }
        }
    }

    if (ifAttrValue.empty()) {
        // 兼容旧的 bind:visible 属性
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

void Parser::addError(TemplateErrorType type, const std::string& message,
                      const SourceLocation& loc) {
    m_errors.emplace_back(type, message, loc);
}

void Parser::addError(TemplateErrorType type, const std::string& message, const Token& token) {
    m_errors.emplace_back(type, message, token.location);
}

bool Parser::isAtEnd() const {
    return m_current >= m_tokens.size() || current().type == TokenType::EndOfFile;
}

SourceLocation Parser::currentLocation() const {
    return current().location;
}

bool Parser::isValidTagName(const std::string& name) const {
    return ast::isValidWidgetTag(name);
}

} // namespace mc::client::ui::kagero::tpl::parser
