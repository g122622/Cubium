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

#include "TemplateCompiler.hpp"

#include "client/ui/kagero/template/parser/Ast.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <chrono>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::tpl::compiler {

// ========== CompiledTemplate实现 ==========

CompiledTemplate::CompiledTemplate() = default;

CompiledTemplate::~CompiledTemplate() = default;

CompiledTemplate::CompiledTemplate(CompiledTemplate&& other) noexcept
    : m_astRoot(std::move(other.m_astRoot))
    , m_bindingPlans(std::move(other.m_bindingPlans))
    , m_eventPlans(std::move(other.m_eventPlans))
    , m_loopPlans(std::move(other.m_loopPlans))
    , m_watchedPaths(std::move(other.m_watchedPaths))
    , m_registeredCallbacks(std::move(other.m_registeredCallbacks))
    , m_errors(std::move(other.m_errors))
    , m_sourcePath(std::move(other.m_sourcePath))
    , m_compileTime(other.m_compileTime)
{}

CompiledTemplate& CompiledTemplate::operator=(CompiledTemplate&& other) noexcept
{
    if (this != &other) {
        m_astRoot = std::move(other.m_astRoot);
        m_bindingPlans = std::move(other.m_bindingPlans);
        m_eventPlans = std::move(other.m_eventPlans);
        m_loopPlans = std::move(other.m_loopPlans);
        m_watchedPaths = std::move(other.m_watchedPaths);
        m_registeredCallbacks = std::move(other.m_registeredCallbacks);
        m_errors = std::move(other.m_errors);
        m_sourcePath = std::move(other.m_sourcePath);
        m_compileTime = other.m_compileTime;
    }
    return *this;
}

void CompiledTemplate::setAstRoot(std::unique_ptr<ast::DocumentNode> root)
{
    m_astRoot = std::move(root);
}

void CompiledTemplate::addBindingPlan(BindingPlan plan)
{
    m_bindingPlans.push_back(std::move(plan));
}

void CompiledTemplate::addEventPlan(EventPlan plan)
{
    m_eventPlans.push_back(std::move(plan));
}

void CompiledTemplate::addLoopPlan(LoopPlan plan)
{
    m_loopPlans.push_back(std::move(plan));
}

void CompiledTemplate::addWatchedPath(const std::string& path)
{
    m_watchedPaths.insert(path);
}

void CompiledTemplate::addRegisteredCallback(const std::string& name)
{
    m_registeredCallbacks.insert(name);
}

void CompiledTemplate::addError(TemplateErrorInfo error)
{
    m_errors.push_back(std::move(error));
}

std::string CompiledTemplate::debugDump() const
{
    std::ostringstream oss;

    oss << "=== Compiled Template ===\n";
    oss << "Source: " << m_sourcePath << "\n";
    oss << "Compile Time: " << m_compileTime << "ms\n";
    oss << "Valid: " << (isValid() ? "Yes" : "No") << "\n\n";

    oss << "--- Binding Plans (" << m_bindingPlans.size() << ") ---\n";
    for (const auto& plan : m_bindingPlans) {
        oss << "  " << plan.widgetPath << "." << plan.attributeName;
        oss << " <- " << plan.statePath;
        if (plan.isLoopBinding) {
            oss << " (loop: $" << plan.loopVarName << ")";
        }
        oss << "\n";
    }

    oss << "\n--- Event Plans (" << m_eventPlans.size() << ") ---\n";
    for (const auto& plan : m_eventPlans) {
        oss << "  " << plan.widgetPath << "." << plan.eventName;
        oss << " -> " << plan.callbackName << "\n";
    }

    oss << "\n--- Loop Plans (" << m_loopPlans.size() << ") ---\n";
    for (const auto& plan : m_loopPlans) {
        oss << "  " << plan.parentPath << " foreach $" << plan.itemVarName;
        oss << " in " << plan.collectionPath << "\n";
    }

    oss << "\n--- Watched Paths (" << m_watchedPaths.size() << ") ---\n";
    for (const auto& path : m_watchedPaths) {
        oss << "  " << path << "\n";
    }

    oss << "\n--- Registered Callbacks (" << m_registeredCallbacks.size() << ") ---\n";
    for (const auto& name : m_registeredCallbacks) {
        oss << "  " << name << "\n";
    }

    if (!m_errors.empty()) {
        oss << "\n--- Errors (" << m_errors.size() << ") ---\n";
        for (const auto& error : m_errors) {
            oss << "  " << error.format() << "\n";
        }
    }

    return oss.str();
}

// ========== TemplateCompiler实现 ==========

TemplateCompiler::TemplateCompiler()
    : m_config(TemplateConfig::defaults())
{}

TemplateCompiler::TemplateCompiler(const TemplateConfig& config)
    : m_config(config)
{}

std::unique_ptr<CompiledTemplate> TemplateCompiler::compile(const std::string& source, const std::string& sourcePath)
{

    m_lastErrors.clear();

    auto startTime = std::chrono::high_resolution_clock::now();

    auto result = std::make_unique<CompiledTemplate>();
    result->setSourcePath(sourcePath);

    // 1. 词法分析
    if (!_tokenize(source, sourcePath)) {
        result->setCompileTime(static_cast<u64>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
                .count()));
        for (const auto& error : m_lastErrors) {
            result->addError(error);
        }
        return result;
    }

    // 2. 语法分析
    if (!_parse(sourcePath)) {
        result->setCompileTime(static_cast<u64>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
                .count()));
        for (const auto& error : m_lastErrors) {
            result->addError(error);
        }
        return result;
    }

    // 使用_parse()生成的AST
    auto ast = std::move(m_parsedAst);

    // 3. 语义验证
    if (!_validate(ast.get())) {
        for (const auto& error : m_lastErrors) {
            result->addError(error);
        }
        result->setCompileTime(static_cast<u64>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
                .count()));
        return result;
    }

    // 4. 生成编译计划
    result->setAstRoot(std::move(ast));
    _generateBindingPlans(result->astRoot(), result.get());
    _generateEventPlans(result->astRoot(), result.get());
    _collectWatchedPaths(result->astRoot(), result.get());
    _collectCallbacks(result->astRoot(), result.get());

    u64 compileTime = static_cast<u64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
            .count());
    result->setCompileTime(compileTime);

    if (m_config.debugOutput) {
        // TODO: 调试输出功能待实现
    }

    return result;
}

std::unique_ptr<CompiledTemplate> TemplateCompiler::compileFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        auto result = std::make_unique<CompiledTemplate>();
        result->addError(TemplateErrorInfo(
            TemplateErrorType::CompileError, "Failed to open file: " + filePath, SourceLocation(), filePath));
        m_lastErrors = result->errors();
        return result;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    return compile(source, filePath);
}

std::unique_ptr<CompiledTemplate> TemplateCompiler::compileAst(
    std::unique_ptr<ast::DocumentNode> document, const std::string& sourcePath)
{

    auto startTime = std::chrono::high_resolution_clock::now();

    auto result = std::make_unique<CompiledTemplate>();
    result->setSourcePath(sourcePath);

    if (!document) {
        result->addError(
            TemplateErrorInfo(TemplateErrorType::CompileError, "Null AST document", SourceLocation(), sourcePath));
        m_lastErrors = result->errors();
        return result;
    }

    // 验证
    m_lastErrors.clear();
    if (!_validate(document.get())) {
        for (const auto& error : m_lastErrors) {
            result->addError(error);
        }
        result->setCompileTime(static_cast<u64>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
                .count()));
        return result;
    }

    // 生成编译计划
    result->setAstRoot(std::move(document));
    _generateBindingPlans(result->astRoot(), result.get());
    _generateEventPlans(result->astRoot(), result.get());
    _collectWatchedPaths(result->astRoot(), result.get());
    _collectCallbacks(result->astRoot(), result.get());

    result->setCompileTime(static_cast<u64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
            .count()));

    return result;
}

bool TemplateCompiler::_tokenize(const std::string& source, const std::string& sourcePath)
{
    Lexer lexer(source, sourcePath);

    if (!lexer.tokenize()) {
        m_lastErrors = lexer.errors();
        return false;
    }

    m_tokens = lexer.tokens();
    return true;
}

bool TemplateCompiler::_parse(const std::string& sourcePath)
{
    if (m_tokens.empty()) {
        m_lastErrors.push_back(
            TemplateErrorInfo(TemplateErrorType::ParserError, "No tokens to parse", SourceLocation(), sourcePath));
        return false;
    }

    Parser parser(m_tokens, m_config);
    m_parsedAst = parser.parse();

    if (!m_parsedAst || parser.hasErrors()) {
        for (const auto& error : parser.errors()) {
            m_lastErrors.push_back(error);
        }
        return false;
    }

    return true;
}

bool TemplateCompiler::_validate(ast::DocumentNode* document)
{
    if (!document) {
        m_lastErrors.push_back(
            TemplateErrorInfo(TemplateErrorType::InvalidTemplate, "Null document node", SourceLocation()));
        return false;
    }

    TemplateErrorCollector collector;

    // 验证文档根
    if (!_validateNode(document, collector)) {
        m_lastErrors = collector.errors();
        return false;
    }

    // 如果有错误，返回失败
    if (collector.hasErrors()) {
        m_lastErrors = collector.errors();
        return false;
    }

    return true;
}

bool TemplateCompiler::_validateNode(const ast::Node* node, TemplateErrorCollector& collector)
{
    if (!node) return true;

    // 根据节点类型验证
    switch (node->type) {
        case ast::NodeType::Document:
            // 验证文档的子节点
            for (const auto& child : node->children) {
                if (!_validateNode(child.get(), collector)) {
                    return false;
                }
            }
            break;

        case ast::NodeType::TextContent:
        case ast::NodeType::Comment:
            // 文本和注释节点无需验证
            break;

        default:
            // 元素节点
            if (auto* element = dynamic_cast<const ast::ElementNode*>(node)) {
                if (!_validateElement(element, collector)) {
                    return false;
                }
                // 验证子节点
                for (const auto& child : element->children) {
                    if (!_validateNode(child.get(), collector)) {
                        return false;
                    }
                }
            }
            break;
    }

    return true;
}

bool TemplateCompiler::_validateElement(const ast::ElementNode* element, TemplateErrorCollector& collector)
{
    MC_ASSERT_RELEASE(element != nullptr);

    // 1. 检查标签名白名单
    if (m_config.strictMode && !ast::isValidWidgetTag(element->tagName)) {
        collector.addError(
            TemplateErrorType::UnknownTag, "Unknown tag: <" + element->tagName + ">", element->range.start);
        // 非严格模式下继续验证其他内容
        if (m_config.strictMode) {
            return false;
        }
    }

    // 2. 验证属性
    for (const auto& [name, attr] : element->attributes) {
        if (!_validateAttribute(attr, element, collector)) {
            if (m_config.strictMode) {
                return false;
            }
        }
    }

    // 3. 检查内联脚本/表达式（严格模式）
    if (m_config.strictMode) {
        for (const auto& [name, attr] : element->attributes) {
            if (_containsInlineScript(attr.rawValue) || _containsForbiddenPattern(attr.rawValue)) {
                collector.addError(TemplateErrorType::InlineScriptNotAllowed,
                    "Inline scripts/expressions are not allowed in strict mode. " +
                        std::string("Found in attribute '") + name + "'",
                    attr.location);
                return false;
            }
        }
    }

    return true;
}

bool TemplateCompiler::_validateAttribute(
    const ast::Attribute& attr, const ast::ElementNode* element, TemplateErrorCollector& collector)
{
    // TODO: element参数暂未使用，后续属性验证可能需要元素上下文（如检查标签与属性的兼容性）

    // 验证属性名格式
    if (!ast::isValidAttributeName(attr.name)) {
        collector.addError(
            TemplateErrorType::InvalidAttributeName, "Invalid attribute name: '" + attr.name + "'", attr.location);
        return false;
    }

    // 验证绑定路径
    if (attr.isBinding() && m_config.validateBindingPaths) {
        if (attr.binding.has_value() && !attr.binding->path.empty()) {
            if (!ast::isValidBindingPath(attr.binding->path)) {
                collector.addError(TemplateErrorType::InvalidBindingPath,
                    "Invalid binding path: '" + attr.binding->path + "'",
                    attr.location);
                return false;
            }
        }
    }

    // 验证回调名称
    if (attr.isEvent() && m_config.validateCallbackNames) {
        if (!ast::isValidCallbackName(attr.callbackName)) {
            collector.addError(TemplateErrorType::InvalidCallbackName,
                "Invalid callback name: '" + attr.callbackName + "'",
                attr.location);
            return false;
        }
    }

    return true;
}

bool TemplateCompiler::_containsInlineScript(const std::string& value) const
{
    // 检查常见的脚本标记
    static const std::string scriptPatterns[] = {"<script", "</script>", "{{", "}}", "{%", "%}", "${", "<?php", "?>"};

    for (const auto& pattern : scriptPatterns) {
        if (value.find(pattern) != std::string::npos) {
            return true;
        }
    }

    return false;
}

bool TemplateCompiler::_containsForbiddenPattern(const std::string& value) const
{
    // 检查禁止的表达式模式
    static const std::string forbiddenPatterns[] = {
        "==", "!=", "<=", ">=", "&&", "||", "?:", "=>", "function(", "lambda", "eval(", "exec("};

    for (const auto& pattern : forbiddenPatterns) {
        if (value.find(pattern) != std::string::npos) {
            return true;
        }
    }

    return false;
}

void TemplateCompiler::_generateBindingPlans(ast::DocumentNode* document, CompiledTemplate* result)
{
    // 遍历AST，为每个绑定属性生成计划
    for (const auto& child : document->children) {
        std::string currentPath;
        _generateBindingPlansRecursive(child.get(), currentPath, result);
    }
}

void TemplateCompiler::_generateBindingPlansRecursive(
    const ast::Node* node, const std::string& parentPath, CompiledTemplate* result)
{
    if (!node) return;
    MC_ASSERT_RELEASE(result != nullptr);

    std::string currentPath = parentPath;

    // 处理元素节点
    if (auto* element = dynamic_cast<const ast::ElementNode*>(node)) {
        // 生成当前路径
        currentPath = _generateWidgetPath(element, parentPath);

        if (m_config.debugOutput) {
            // TODO: 调试输出功能待实现
        }

        // 处理绑定属性
        // TODO: 需要确保 categorizeAttributes() 已被调用
        // 当 bindingAttrs 为空但 attributes 不为空时，说明分类未执行，需要触发
        // const_cast 是因为遍历逻辑只需读语义信息，不修改逻辑结构
        if (element->bindingAttrs.empty() && !element->attributes.empty()) {
            const_cast<ast::ElementNode*>(element)->categorizeAttributes();
        }

        for (const auto& attr : element->bindingAttrs) {
            BindingPlan plan;
            plan.widgetPath = currentPath;
            plan.attributeName = attr.baseName();

            if (attr.binding.has_value()) {
                plan.statePath = attr.binding->path;
                plan.isLoopBinding = attr.binding->isLoopVariable;
                plan.loopVarName = attr.binding->loopVarName;
            }

            result->addBindingPlan(plan);
        }

        // 递归处理子节点
        for (const auto& child : element->children) {
            _generateBindingPlansRecursive(child.get(), currentPath, result);
        }
    }
}

void TemplateCompiler::_generateEventPlans(ast::DocumentNode* document, CompiledTemplate* result)
{
    // 遍历AST，为每个事件属性生成计划
    for (const auto& child : document->children) {
        std::string currentPath;
        _generateEventPlansRecursive(child.get(), currentPath, result);
    }
}

void TemplateCompiler::_generateEventPlansRecursive(
    const ast::Node* node, const std::string& parentPath, CompiledTemplate* result)
{
    if (!node) return;
    MC_ASSERT_RELEASE(result != nullptr);

    std::string currentPath = parentPath;

    // 处理元素节点
    if (auto* element = dynamic_cast<const ast::ElementNode*>(node)) {
        currentPath = _generateWidgetPath(element, parentPath);

        // TODO: 需要确保 categorizeAttributes() 已被调用
        // 当 eventAttrs 为空但 attributes 不为空时，说明分类未执行，需要触发
        // const_cast 是因为遍历逻辑只需读语义信息，不修改逻辑结构
        if (element->eventAttrs.empty() && !element->attributes.empty()) {
            const_cast<ast::ElementNode*>(element)->categorizeAttributes();
        }

        // 处理事件属性
        for (const auto& attr : element->eventAttrs) {
            EventPlan plan;
            plan.widgetPath = currentPath;
            plan.eventName = attr.baseName(); // 去除 "on:" 前缀
            plan.callbackName = attr.callbackName;

            result->addEventPlan(plan);
        }

        // 递归处理子节点
        for (const auto& child : element->children) {
            _generateEventPlansRecursive(child.get(), currentPath, result);
        }
    }
}

void TemplateCompiler::_collectWatchedPaths(ast::DocumentNode* document, CompiledTemplate* result)
{
    // TODO: document参数暂未使用，后续可直接从AST收集路径而不仅依赖BindingPlan
    (void)document;

    // 从绑定计划中收集状态路径
    for (const auto& plan : result->bindingPlans()) {
        if (!plan.statePath.empty() && !plan.isLoopBinding) {
            result->addWatchedPath(plan.statePath);
        }
    }

    // 从循环计划中收集集合路径
    for (const auto& plan : result->loopPlans()) {
        if (!plan.collectionPath.empty()) {
            result->addWatchedPath(plan.collectionPath);
        }
    }
}

void TemplateCompiler::_collectCallbacks(ast::DocumentNode* document, CompiledTemplate* result)
{
    // TODO: document参数暂未使用，后续可直接从AST收集回调而不仅依赖EventPlan
    (void)document;

    // 从事件计划中收集回调名称
    for (const auto& plan : result->eventPlans()) {
        if (!plan.callbackName.empty()) {
            result->addRegisteredCallback(plan.callbackName);
        }
    }
}

std::string TemplateCompiler::_generateWidgetPath(const ast::ElementNode* element, const std::string& parentPath)
{
    MC_ASSERT_RELEASE(element != nullptr);

    std::string path = parentPath;

    // 如果元素有ID，使用ID作为路径
    if (!element->id.empty()) {
        path = path.empty() ? element->id : path + "." + element->id;
    } else {
        // TODO: 当前使用标签名作为路径片段，多个同名标签可能产生歧义，后续应加上索引区分
        path = path.empty() ? element->tagName : path + "." + element->tagName;
    }

    return path;
}

void TemplateCompiler::_extractBindings(const ast::ElementNode* element,
    const std::string& widgetPath,
    std::vector<BindingPlan>& plans,
    std::vector<LoopPlan>& loopPlans)
{
    MC_ASSERT_RELEASE(element != nullptr);

    // 检查是否有循环指令 (bind:items)
    auto itemsIt = element->attributes.find("bind:items");
    if (itemsIt != element->attributes.end()) {
        LoopPlan loopPlan;
        loopPlan.parentPath = widgetPath;

        if (itemsIt->second.binding.has_value()) {
            loopPlan.collectionPath = itemsIt->second.binding->path;
        }

        // 推断循环变量名
        for (const auto& [name, attr] : element->attributes) {
            if (name.find("bind:") == 0 && attr.binding.has_value() && attr.binding->isLoopVariable) {
                loopPlan.itemVarName = attr.binding->loopVarName;
                break;
            }
        }

        if (loopPlan.itemVarName.empty()) {
            loopPlan.itemVarName = "item";
        }

        loopPlans.push_back(std::move(loopPlan));
    }

    // 提取普通绑定
    for (const auto& attr : element->bindingAttrs) {
        BindingPlan plan;
        plan.widgetPath = widgetPath;
        plan.attributeName = attr.baseName();

        if (attr.binding.has_value()) {
            plan.statePath = attr.binding->path;
            plan.isLoopBinding = attr.binding->isLoopVariable;
            plan.loopVarName = attr.binding->loopVarName;
        }

        plans.push_back(std::move(plan));
    }
}

} // namespace mc::client::ui::kagero::tpl::compiler
